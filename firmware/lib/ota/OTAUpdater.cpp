#include "OTAUpdater.hh"
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include <cstring>
#include <ctype.h>

namespace lib::ota
{
    static constexpr size_t  MAX_URL_LEN          = 256;          // Firmware URL after "{version}" expansion
    static constexpr char    VERSION_PLACEHOLDER[] = "{version}";
    static constexpr uint8_t MAX_VERSION_PARTS    = 4;            // MAJOR.MINOR.PATCH.BUILD

    // -----------------------------
    // Version helpers
    // -----------------------------

    // Parses "[v]MAJOR[.MINOR[.PATCH[.BUILD]]]" into numeric parts (missing = 0).
    // Fails on empty or non-numeric components, suffixes ("-rc1") or overflow.
    static bool parse_version(const char *str, uint32_t (&parts)[MAX_VERSION_PARTS])
    {
        memset(parts, 0, sizeof(parts));

        if (*str == 'v' || *str == 'V')
            str++;

        for (uint8_t n = 0; n < MAX_VERSION_PARTS; n++)
        {
            if (!isdigit((unsigned char)*str))
                return false;

            uint32_t value = 0;
            while (isdigit((unsigned char)*str))
            {
                value = value * 10 + (uint32_t)(*str++ - '0');
                if (value > 0xFFFF)
                    return false;
            }
            parts[n] = value;

            if (*str == '\0')
                return true;
            if (*str++ != '.')
                return false;
        }

        return false;   // more than MAX_VERSION_PARTS components
    }

    // Accepts only tokens made of [0-9A-Za-z.+_-]: rejects HTML error pages,
    // multi-line files and anything unsafe to splice into the firmware URL.
    static bool is_valid_version_token(const char *str)
    {
        if (*str == '\0')
            return false;

        for (; *str; str++)
        {
            const char c = *str;
            if (!isalnum((unsigned char)c) && c != '.' && c != '-' && c != '_' && c != '+')
                return false;
        }
        return true;
    }

    // Expands the first "{version}" in tmpl.  A template without placeholder
    // (e.g. ".../releases/latest/download/firmware.bin") is copied as is.
    static bool build_firmware_url(const char *tmpl, const char *version, char *out, size_t out_len)
    {
        const char *mark = strstr(tmpl, VERSION_PLACEHOLDER);
        int n;

        if (mark == nullptr)
            n = snprintf(out, out_len, "%s", tmpl);
        else
            n = snprintf(out, out_len, "%.*s%s%s",
                         (int)(mark - tmpl), tmpl,
                         version,
                         mark + sizeof(VERSION_PLACEHOLDER) - 1);

        return n > 0 && (size_t)n < out_len;
    }

    // -----------------------------
    // HTTPS helpers
    // -----------------------------

    static void setup_tls(WiFiClientSecure &client, const char *root_ca)
    {
        if (root_ca != nullptr)
            client.setCACert(root_ca);
        else
            client.setInsecure();   // encrypted but server not authenticated (see README)

        client.setHandshakeTimeout((OTA_HTTP_TIMEOUT_MS + 999) / 1000);   // API takes seconds
    }

    // Issues a GET following redirects: GitHub answers release downloads with
    // 302 github.com -> objects/release-assets.githubusercontent.com.
    static int http_get(HTTPClient &http, WiFiClientSecure &client, const char *url)
    {
        http.setConnectTimeout(OTA_HTTP_TIMEOUT_MS);
        http.setTimeout(OTA_HTTP_TIMEOUT_MS);
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

        if (!http.begin(client, url))
        {
            MESH_LOG("[OTA] Invalid URL: %s\r\n", url);
            return HTTPC_ERROR_CONNECTION_REFUSED;
        }

        return http.GET();
    }

    // Streams total_len bytes into the inactive OTA slot.  The inactivity
    // timeout is re-armed on every received chunk, so it bounds stalls rather
    // than the total download time.
    static OTAResult write_stream(WiFiClient &stream, size_t total_len)
    {
        if (!Update.begin(total_len, U_FLASH))
        {
            MESH_LOG("[OTA] Update.begin(%u) failed: %s\r\n", (unsigned)total_len, Update.errorString());
            return OTAResult::k_flash_error;
        }

        uint8_t  buf[OTA_BUFFER_SIZE];
        size_t   written      = 0;
        uint8_t  next_pct     = 10;
        uint32_t last_data_ms = millis();

        while (written < total_len)
        {
            const size_t avail = stream.available();

            if (avail > 0)
            {
                size_t to_read = total_len - written;
                if (to_read > avail)       to_read = avail;
                if (to_read > sizeof(buf)) to_read = sizeof(buf);

                const int n = stream.read(buf, to_read);
                if (n > 0)
                {
                    if (Update.write(buf, (size_t)n) != (size_t)n)
                    {
                        MESH_LOG("[OTA] Update.write failed: %s\r\n", Update.errorString());
                        Update.abort();
                        return OTAResult::k_flash_error;
                    }

                    written     += (size_t)n;
                    last_data_ms = millis();

                    if (written * 100 >= (size_t)next_pct * total_len)
                    {
                        MESH_LOG("[OTA] %u%% (%u/%u bytes)\r\n", next_pct, (unsigned)written, (unsigned)total_len);
                        next_pct += 10;
                    }
                    continue;
                }
            }
            else if (!stream.connected())
            {
                MESH_LOG("[OTA] Connection closed at %u/%u bytes\r\n", (unsigned)written, (unsigned)total_len);
                break;
            }

            if (millis() - last_data_ms > OTA_DOWNLOAD_TIMEOUT_MS)
            {
                MESH_LOG("[OTA] No data for %lu ms, aborting\r\n", OTA_DOWNLOAD_TIMEOUT_MS);
                break;
            }

            delay(1);   // yield to other tasks while waiting for data
        }

        if (written != total_len)
        {
            Update.abort();
            return OTAResult::k_download_error;
        }

        // end() validates the image (header magic, checksum and the SHA-256
        // appended by esptool) and only then marks the new slot as bootable.
        if (!Update.end())
        {
            MESH_LOG("[OTA] Update.end failed: %s\r\n", Update.errorString());
            return OTAResult::k_flash_error;
        }

        return OTAResult::k_updated;
    }

    // -----------------------------
    // OTAUpdater
    // -----------------------------

    OTAUpdater::OTAUpdater(const char *ssid,
                           const char *password,
                           const char *version_url,
                           const char *firmware_url,
                           const char *current_version,
                           const char *root_ca)
        : _ssid(ssid),
          _password(password),
          _version_url(version_url),
          _firmware_url(firmware_url),
          _current_version(current_version),
          _root_ca(root_ca)
    {
    }

    bool OTAUpdater::connect_wifi(uint32_t timeout_ms)
    {
        if (WiFi.status() == WL_CONNECTED)
            return true;

        WiFi.persistent(false);   // do not rewrite the credentials to NVS on every boot
        WiFi.mode(WIFI_STA);
        WiFi.begin(_ssid, _password);

        const uint32_t start_ms = millis();
        while (WiFi.status() != WL_CONNECTED)
        {
            if (millis() - start_ms >= timeout_ms)
            {
                MESH_LOG("[OTA] WiFi \"%s\" not reachable after %lu ms\r\n", _ssid, (unsigned long)timeout_ms);
                return false;
            }
            delay(100);
        }

        MESH_LOG("[OTA] WiFi connected, IP %s, RSSI %d dBm\r\n",
                 WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    }

    void OTAUpdater::shutdown_wifi()
    {
        WiFi.disconnect(true);   // drop the association and disable STA
        WiFi.mode(WIFI_OFF);     // stop and de-initialise the driver (frees its heap for BLE)
    }

    bool OTAUpdater::is_newer_version(const char *remote, const char *local)
    {
        uint32_t r[MAX_VERSION_PARTS];
        uint32_t l[MAX_VERSION_PARTS];

        if (parse_version(remote, r) && parse_version(local, l))
        {
            for (uint8_t i = 0; i < MAX_VERSION_PARTS; i++)
            {
                if (r[i] != l[i])
                    return r[i] > l[i];
            }
            return false;   // same version
        }

        // Non-numeric scheme: any change is treated as an update.
        MESH_LOG("[OTA] Non-numeric version, comparing as strings\r\n");
        return strcmp(remote, local) != 0;
    }

    bool OTAUpdater::fetch_latest_version(char *out, size_t out_len)
    {
        WiFiClientSecure client;
        HTTPClient       http;
        setup_tls(client, _root_ca);

        const int code = http_get(http, client, _version_url);
        if (code != HTTP_CODE_OK)
        {
            MESH_LOG("[OTA] GET %s -> %d\r\n", _version_url, code);
            http.end();
            return false;
        }

        String body = http.getString();
        http.end();
        body.trim();   // version.txt usually ends with a newline

        if (body.length() == 0 || body.length() >= out_len || !is_valid_version_token(body.c_str()))
        {
            MESH_LOG("[OTA] Invalid version file content\r\n");
            return false;
        }

        memcpy(out, body.c_str(), body.length() + 1);
        return true;
    }

    OTAResult OTAUpdater::download_and_apply(const char *version)
    {
        char url[MAX_URL_LEN];
        if (!build_firmware_url(_firmware_url, version, url, sizeof(url)))
        {
            MESH_LOG("[OTA] Firmware URL too long\r\n");
            return OTAResult::k_download_error;
        }

        WiFiClientSecure client;
        HTTPClient       http;
        setup_tls(client, _root_ca);

        MESH_LOG("[OTA] Downloading %s\r\n", url);
        const int code = http_get(http, client, url);
        if (code != HTTP_CODE_OK)
        {
            MESH_LOG("[OTA] GET firmware -> %d\r\n", code);
            http.end();
            return OTAResult::k_download_error;
        }

        // Update.begin() needs the exact image size to check it against the OTA slot.
        const int content_len = http.getSize();
        WiFiClient *stream = http.getStreamPtr();
        if (content_len <= 0 || stream == nullptr)
        {
            MESH_LOG("[OTA] Missing Content-Length (%d), aborting\r\n", content_len);
            http.end();
            return OTAResult::k_download_error;
        }

        const OTAResult res = write_stream(*stream, (size_t)content_len);
        http.end();
        return res;
    }

    OTAResult OTAUpdater::check_and_update()
    {
        MESH_LOG("[OTA] Running firmware %s, checking for updates\r\n", _current_version);

        OTAResult res;
        char latest[OTA_MAX_VERSION_LEN];

        if (!connect_wifi())
        {
            res = OTAResult::k_no_wifi;
        }
        else if (!fetch_latest_version(latest, sizeof(latest)))
        {
            res = OTAResult::k_version_error;
        }
        else if (!is_newer_version(latest, _current_version))
        {
            MESH_LOG("[OTA] Up to date (remote %s)\r\n", latest);
            res = OTAResult::k_up_to_date;
        }
        else
        {
            MESH_LOG("[OTA] New firmware %s available\r\n", latest);
            res = download_and_apply(latest);
        }

        shutdown_wifi();

        if (res == OTAResult::k_updated)
        {
            MESH_LOG("[OTA] Update installed, rebooting\r\n");
            delay(500);   // let the log drain before the reset
            ESP.restart();
        }

        MESH_LOG("[OTA] No update installed (%d), WiFi off\r\n", (int)res);
        return res;
    }

} // namespace lib::ota
