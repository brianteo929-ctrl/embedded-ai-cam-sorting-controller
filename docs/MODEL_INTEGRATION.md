# Model integration

The current workflow is in [AI_MODEL.md](AI_MODEL.md), with the architecture
choice in [MODEL_SELECTION.md](MODEL_SELECTION.md). No trained model is installed.

Three build choices share inference.h: deterministic simulation (default), FOMO
(inference_ei.cpp), and classification (inference_ei_classification.cpp). FOMO
metadata checks remain in place. Missing exports fail configuration; incompatible
image/output metadata fails compilation or initialization. Adapter tests use a
fake SDK only. Actual SDK/IDF compatibility awaits a trained export.

Frame path: QVGA JPEG -> bounded RGB888 decode -> centered crop / half-pixel
bilinear resize -> packed 0xRRGGBB signal -> SDK Image processing -> inference ->
output-specific decision -> existing FSM. JPEG bytes are never model pixels.
The SDK performs grayscale conversion/normalization; do not normalize twice.
Compare resize interpolation against Studio using known vectors before validation.

The single vision task owns input throughout inference and clears the adapter
callback pointer after return. Camera ownership remains mutex-protected; preview
serves an independent JPEG copy. Control deadlines and RMT pulses run separately.
Cancellation invalidates the cycle; late results release only their own request
and cannot actuate or clear a newer request. Recovery waits for inference to
finish. A permanently hung SDK requires reset; task deletion is not safe recovery.

Follow [BRING_UP.md](BRING_UP.md) with simulated motors and disconnected VMOT.
