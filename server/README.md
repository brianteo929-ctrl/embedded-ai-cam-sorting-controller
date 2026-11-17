# Historical Node relay

`server.js` is an old image-upload/MJPEG prototype, independent of the production ESP-IDF controller. The current firmware serves images directly; it does not POST frames to this relay. Do not expose this unauthenticated, unbounded prototype to an untrusted network.

If needed for reference, `npm ci` restores its locked dependencies and `npm run start:legacy` starts it. The repository's `npm test` now runs native firmware regression; it is not a test of this relay. Dependency directories remain local and are excluded from source control.
