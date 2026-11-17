# Historical Arduino reference

This prototype supplied the camera pin mapping and AP/MJPEG behavior for the native ESP-IDF port. It is not the production controller and was not rebuilt in the current verification.

It enables repetitive motor motion and does not implement the new arming, homing-reference, timeout, or fault policy. Do not flash it for current bring-up. A former embedded AP password was removed; compiling it now requires a private `LEGACY_AP_PASSWORD` build definition. Use the root README and `tools/idf.cmd` for the supported firmware.
