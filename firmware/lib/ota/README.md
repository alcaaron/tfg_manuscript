# lib/ota — Boot-time OTA updates from GitHub

`lib::ota::OTAUpdater` checks a GitHub repository for a newer firmware at
boot and installs it over HTTPS. It is called from `setup()` in
`src/main.cpp` **before** BLE and LoRa are started, and it always switches
WiFi off before returning, so WiFi never coexists with BLE/LoRa. If the
network is not reachable within `OTA_WIFI_TIMEOUT_MS`, the node simply
continues its normal boot.

Flow: join `WIFI_SSID` -> GET `OTA_VERSION_URL` -> compare it with
`A3M_VERSION` -> if newer, GET `OTA_FIRMWARE_URL` (following GitHub's
redirects) -> write it to the inactive OTA slot -> validate -> reboot.

## Prerequisites

- **Partition table with two app slots.** `custom.csv` defines
  `app0`/`app1` (`ota_0`/`ota_1`, 1920K each) plus `otadata`. After changing
  the partition table the node must be flashed once over USB
  (`pio run -t upload`). Later updates can then go over the air.
- **Public GitHub repository.** Private repositories need a token and
  extra HTTP headers, which this library does not handle.

## GitHub repository setup

1. In the repository root, on branch `main`, add `version.txt` containing
   **only** the version number, for example `1.0.0`. No spaces, comments
   or extra lines. A trailing newline is fine.
   `firmware/.gitignore` ignores `*.txt`, so keep the file outside
   `firmware/`, or force-add it with `git add -f`.
2. Set the URLs in `include/config.h`:
   ```c
   #define OTA_VERSION_URL  "https://raw.githubusercontent.com/<user>/<repo>/main/version.txt"
   #define OTA_FIRMWARE_URL "https://github.com/<user>/<repo>/releases/download/v{version}/firmware.bin"
   ```
   Use the **Raw** URL of `version.txt`, not the normal GitHub page.
   `{version}` is replaced by the content of `version.txt`, so the node
   downloads the release tagged `v<version>`. If that release does not
   exist yet, the download returns 404 and nothing is installed.
   A fixed URL without the placeholder, such as
   `.../releases/latest/download/firmware.bin`, also works. In that case
   publish the release **before** you update `version.txt`. Otherwise
   nodes would reinstall the previous binary on every boot.
3. Set `WIFI_SSID` and `WIFI_PASSWORD` in `.env` (copy `.env.example`; never commit it). Set `OTA_ENABLED 0` to skip the
   check entirely.

## Publishing a new version

1. Raise `A3M_VERSION` in `include/config.h`, for example `"1.0.1"`.
2. Build with `pio run`. The image is
   `.pio/build/esp32doit-devkit-v1/firmware.bin`. Check that the reported
   Flash usage stays below the 1920K slot.
3. Create a GitHub release with tag `v1.0.1` and attach that file with the
   exact name `firmware.bin`. In the web UI, go to Releases, then Draft a
   new release. With the GitHub CLI:
   ```bash
   gh release create v1.0.1 .pio/build/esp32doit-devkit-v1/firmware.bin \
       --title "Firmware v1.0.1" --notes "..."
   ```
4. Change `version.txt` to `1.0.1`, then commit and push. Nodes pick up the
   update on their next boot. `raw.githubusercontent.com` caches files for
   about 5 minutes.

## Version comparison

Versions are parsed as `[v]MAJOR[.MINOR[.PATCH[.BUILD]]]` and compared
numerically, so `1.0.10` is newer than `1.0.9`. An update is installed
only when the remote version is **strictly newer**, which prevents
downgrades and reinstall loops. If either string is not purely numeric
(for example `1.1.0-rc1`), the comparison falls back to plain inequality.
The remote string must be a single token of `[0-9A-Za-z.+_-]` shorter than
`OTA_MAX_VERSION_LEN`. Anything else, such as an HTML error page, is
rejected.

## Security notes

- By default (`OTA_ROOT_CA nullptr`) the connection is encrypted, but the
  server certificate is **not verified** (`setInsecure()`). An attacker
  who controls the network could serve a different image. To verify the
  server, set `OTA_ROOT_CA` to a PEM string with the root CAs of
  `raw.githubusercontent.com`, `github.com` and the asset host the release
  redirects to (`*.githubusercontent.com`). Several PEM blocks can be
  concatenated. To inspect the chains, run
  `openssl s_client -showcerts -connect <host>:443`.
- The Update API validates the image before switching slots (header
  magic, checksum and the SHA-256 that esptool appends). A truncated or
  corrupt download leaves the running firmware untouched.
