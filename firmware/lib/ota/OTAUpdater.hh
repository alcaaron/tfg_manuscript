#pragma once
#include <config.h>
#include <stdint.h>
#include <stddef.h>

namespace lib::ota
{
    /*!
     * \brief Outcome of an OTA check.
     *
     * \c k_updated is only produced internally: after a successful update the
     * node reboots and check_and_update() never returns to the caller.
     */
    enum class OTAResult : int8_t
    {
        k_updated        =  1, /*!< New image written and validated; node is rebooting. */
        k_up_to_date     =  0, /*!< Remote version is not newer than the running one. */
        k_no_wifi        = -1, /*!< WiFi association timed out. */
        k_version_error  = -2, /*!< version file could not be fetched or is malformed. */
        k_download_error = -3, /*!< Firmware download failed (HTTP error, size, timeout). */
        k_flash_error    = -4, /*!< Update API failed to write or validate the image. */
    };

    /*!
     * \brief Boot-time firmware updater that pulls new images from a GitHub
     *        release over HTTPS.
     *
     * Flow of check_and_update():
     *  1. Join the configured WiFi network (bounded by a timeout, never blocks
     *     the boot indefinitely).
     *  2. GET the version file (raw.githubusercontent.com) and compare it with
     *     the running version (semantic, numeric MAJOR.MINOR.PATCH[.BUILD]).
     *  3. If the remote version is newer, GET the firmware image (following the
     *     GitHub redirects to its asset CDN), stream it into the inactive OTA
     *     slot with the Update API and reboot.
     *  4. Switch WiFi off in every non-rebooting path so it never coexists with
     *     BLE/LoRa.
     *
     * Must be called before the BLE and LoRa tasks are started.
     */
    class OTAUpdater
    {
    public:
        /*!
         * \param [IN] ssid             WiFi network name.
         * \param [IN] password         WiFi passphrase.
         * \param [IN] version_url      URL of the plain-text version file.
         * \param [IN] firmware_url     URL of the firmware image; "{version}" is
         *                              replaced by the remote version string.
         * \param [IN] current_version  Version of the running firmware.
         * \param [IN] root_ca          PEM root CA(s) used to verify the servers,
         *                              or nullptr to skip TLS verification.
         *
         * All strings must outlive the object (string literals from config.h).
         */
        OTAUpdater(const char *ssid            = WIFI_SSID,
                   const char *password        = WIFI_PASSWORD,
                   const char *version_url     = OTA_VERSION_URL,
                   const char *firmware_url    = OTA_FIRMWARE_URL,
                   const char *current_version = A3M_VERSION,
                   const char *root_ca         = OTA_ROOT_CA);

        /*!
         * \brief Connect to WiFi in station mode, waiting at most \p timeout_ms.
         *
         * \retval true   Connected.
         * \retval false  Timed out (WiFi is left on; call shutdown_wifi()).
         */
        bool connect_wifi(uint32_t timeout_ms = OTA_WIFI_TIMEOUT_MS);

        /*!
         * \brief Run the whole OTA check: connect, compare versions, update.
         *
         * Reboots the node on a successful update.  Otherwise WiFi is turned off
         * before returning and the caller can continue the normal boot.
         *
         * \retval OTAResult  Reason why no update was installed.
         */
        OTAResult check_and_update();

        /*!
         * \brief Disconnect and fully de-initialise the WiFi driver, releasing
         *        its heap and radio time for BLE.
         */
        static void shutdown_wifi();

        /*!
         * \brief Decide whether \p remote should replace \p local.
         *
         * Both strings are parsed as "[v]MAJOR[.MINOR[.PATCH[.BUILD]]]"
         * (missing components count as 0) and compared numerically, so only
         * strictly newer versions are installed (no downgrades, "1.0.10" >
         * "1.0.9").  If either string is not purely numeric the comparison
         * falls back to plain inequality.
         *
         * \retval true   \p remote is newer (or differs, in fallback mode).
         */
        static bool is_newer_version(const char *remote, const char *local);

    private:
        const char *_ssid;
        const char *_password;
        const char *_version_url;
        const char *_firmware_url;
        const char *_current_version;
        const char *_root_ca;

        bool fetch_latest_version(char *out, size_t out_len);
        OTAResult download_and_apply(const char *version);
    };

} // namespace lib::ota
