# Camera-domain dataset needed before training

The next external input is a local folder of original, authorized photographs of
the sorting plate. The two audited public sources do not supply a defensible
training package with independent splits and genuine rejection examples. Do not
replace missing negatives with unannotated battery images, blank generated images,
or crops assumed to be empty. No additional public dataset audit is scheduled;
the requested two-candidate audit limit has been reached.

## Collect with the current firmware

1. Keep VMOT disconnected and motor simulation enabled. Use the mock-inference
   build; a trained model is not required for camera capture. Follow BRING_UP.md
   for logic power, board identification and flashing at the observed COM port.
2. Mount the camera over the actual plate. Fix its height, focus, plate background
   and expected item position. Record the setup and lighting in a session note.
3. Save original JPEGs from `http://192.168.4.1/capture` into ignored
   `captures/SESSION/`. Keep the full frame; firmware applies the center crop.
   A phone mounted in a comparable position can supply preliminary images if the
   board is unavailable, but those are not ESP32-CAM validation images.
4. Start with visual classes actually needed and distinguishable in this setup,
   covering each destination. For example: `paper`, `cardboard`, `plastic_bottle`,
   `food_scraps`, and `battery`. Use intact, harmless examples; do not handle
   damaged batteries, pressurized cans or broken lamps for this collection.
   Replicas must be identified as replicas and cannot establish real-item recall.
5. Capture `background` (empty plate), `unknown` (unmapped harmless objects), and
   `multiple_items` (two or more objects, including same-category pairs).
   A single known object with uncertain identity is not automatically `unknown`;
   resolve its label or exclude it. Reject classes do not prove exhaustive unknown
   or multiple-object recognition.
6. Include multiple distinct physical examples, views, placements, normal lighting
   changes and partial occlusion. Avoid collecting hundreds of nearly identical
   frames of one object. More frames do not create independent test examples.

## Record ownership and split identity

Maintain a CSV alongside the images with these columns:

```text
path,label,object_ids,session_id,split,reviewed,notes
session03/paper-017.jpg,paper,object017,session03,training,false,
```

Record the photographer/rights holder and permission for training and model
distribution. Do not assume downloaded product images are owned. Dataset images
stay outside Git. Collection filenames alone do not prove physical identity.

Assign whole physical objects and collection sessions to training, validation or
test before augmentation. Keep paired-object scenes in the same split as their
constituent objects. Reserve genuinely new objects/sessions for test and never
tune thresholds on that split. Every trained class needs examples in all three
sets; the tool's minimum coverage check is not a claim that one image is enough.
Sample sufficiency must be evaluated through per-class results and error counts.

Review every image, normalize labels against `components/fsm/waste_labels.def`,
and create the classification manifest described in DATASET_AUDIT.md. For
classification, `annotations` may be omitted; do not invent bounding boxes.
Archive the approved originals, record its SHA-256 and retain group/ownership
records. Run audit and prepare, then upload the three explicit splits to existing
Edge Impulse project 1168582. No project clone or API key is needed.

## Acceptance before physical inference

After training, freeze confidence/margin thresholds using validation predictions;
report held-out per-class metrics, hazardous misroutes, false actuation on empty
and unknown scenes, and rejection coverage. Keep model validation and physical
motors disabled until actual camera-domain tests and the separate power/mechanical
checks pass. The one-item loading assumption must still be enforced physically.
