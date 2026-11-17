# Model integration

No trained model is supplied. Default `inference_mock.c` ignores the pixels and emits explicitly labeled deterministic test results. It measures neither accuracy nor model latency. Do not use it to classify real waste.

## Export contract

1. In Roboflow, define consistent object annotations and licensing/provenance. Avoid classifying visually indistinguishable material by appearance alone. Export images and bounding boxes in a format accepted by the current Edge Impulse uploader. Do not commit credentials or the dataset here.
2. Split by object/session/source **before** augmentation, so near-duplicate frames cannot leak across train/validation/test. Include empty scenes, difficult lighting, occlusion, multiple objects, and unknown waste. Preserve a held-out set.
3. Create an Edge Impulse camera object-detection impulse using FOMO. Start with a small input such as 96×96; the adapter supports dimensions from metadata up to 256×256, subject to measured RAM/latency. Use the same center-crop geometry as this firmware. RGB pixels are packed as `0xRRGGBB` floats; the exported SDK performs its own image processing/normalization, including grayscale if selected. Do not apply normalization twice.
4. Train, evaluate per-class precision/recall and confusion, tune confidence on validation data, and export the **C++ library** with the selected quantized model/EON option. Record project/model version, export options and checksum outside the generated source tree. FOMO returns detections, not just a whole-image class.
5. Create a local ESP-IDF component at `components/edge_impulse/` containing the export folders `edge-impulse-sdk/`, `model-parameters/`, and `tflite-model/`. Supply a `CMakeLists.txt` registering that export's generated model sources, the SDK's required inference sources and **Espressif** port, with `INCLUDE_DIRS "."`. Use the export's accompanying build files and the [official ESP32 integration example](https://docs.edgeimpulse.com/hardware/deployments/run-cpp-espressif-esp32); do not recursively compile every SDK platform port. Export layouts and supported IDF versions differ: this component cannot be finalized or build-verified until the actual export is available.
6. In `tools\idf.cmd menuconfig`, select Waste sorter → External Edge Impulse component. Set the three exact exported label strings (or extend the `labels` table in `main/controller.c` and test it). Keep physical motors disabled. `components/vision/inference_ei.cpp` calls `run_classifier`, reads bounding boxes and rejects output overflow. CMake fails clearly if the external component is missing; it never silently falls back to mock.
7. Run `python tools/test.py`, `tools\idf.cmd build`, and `tools\idf.cmd size`. Verify the final ELF/map, model tensor arena placement, internal heap/largest block, PSRAM, and stack high-water marks. IDF 6.0.2 compatibility must be checked against the actual export, not inferred from the adapter.
8. Compare firmware preprocessing and detections against desktop/Studio results using the **same held-out images**. Include a color chart to confirm red/blue order and a geometry grid for crop/resize alignment. Tune exposure/lighting and verify board orientation. Only then test real-time inference on camera frames, with motors still simulated.

The [Edge Impulse C++ deployment guide](https://docs.edgeimpulse.com/hardware/deployments/run-cpp) is the API reference. No model artifact, version, score, accuracy, timing, or tensor-arena size has been invented here.

## Image path and decision policy

QVGA JPEG → bounded RGB888 decode → centered aspect-ratio crop → half-pixel bilinear resize → SDK signal. The pinned camera/JPEG source produces RGB order for JPEG; no swap is applied. Keep training/inference resize behavior aligned and quantify interpolation differences against Studio. The 96×96 mock input occupies 27648 bytes plus the 230400-byte QVGA decode buffer. Actual model memory is additional and unknown.

Only one detected object, with one known label and sufficient finite confidence, can produce a destination. Zero objects, low confidence, unknown labels, multiple objects, conflicting categories, invalid floats/counts and model errors never produce a guessed bin. The default threshold is 80%, not a validated operating point. Overflow beyond 16 detections is an error, not truncation to a convenient single detection.

The real adapter is supplied as an integration boundary, not a validated trained backend. Generated SDK/model files are ignored by Git. Keep release artifacts in versioned storage with a checksum and reproducible import/build instructions when available.
