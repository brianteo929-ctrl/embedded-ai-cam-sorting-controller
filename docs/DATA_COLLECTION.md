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

The capture tool maintains `captures/metadata.csv` across all session folders.
Its required columns are:

```text
path,label,object_ids,session_id,split,reviewed,notes
session03/paper-017.jpg,paper,object017,session03,training,false,
```

It additionally records `sha256,width,height,captured_at_utc`. The example above
illustrates the required fields, not a complete row to append manually.

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

## Host capture utility

Use the existing Pillow environment, separate from the ESP-IDF environment:

```bat
.venv\Scripts\python tools\data\capture.py --help
.venv\Scripts\python tests\test_capture.py
```

The computer running this utility must reach the ESP32 AP. Phone verification
does not give the development computer a route to `192.168.4.1`. Prefer a second
network adapter if available, or explicitly connect the capture computer to the
ESP32 AP when ready; its internet connection may be interrupted. No tool changes
Windows networking automatically. No AP password is supplied to this script.

Default endpoint: `http://192.168.4.1/capture`. The tool follows up to two redirects
on the same host/scheme, including the firmware's port-81 image server. Override
with `--endpoint http://ADDRESS:81/capture` when appropriate. Credentials, queries
and fragments in URLs are rejected. HTTP status/content type, bounded body size,
JPEG start/end markers, actual decoding and expected 320x240 dimensions are
checked before saving. `--width`/`--height` are explicit expectations for a
deliberately different camera configuration, not resizing options.

Only the eight initial labels are allowed, checked against `waste_labels.def`:
paper, cardboard, plastic_bottle, food_scraps, battery, background, unknown and
multiple_items. IDs must be lowercase and path-safe. Background takes no
`--object-id`; a single item takes one; multiple_items takes repeated options for
at least two physical objects. Session and split are mandatory. The tool rejects
reuse of an object or session in another split within the same collection root;
it cannot verify whether the operator assigned truthful physical identities.

Images are retained byte-for-byte at
`captures/SESSION/LABEL.SHA256.jpg`. No crop, resize or augmentation is applied.
Exact duplicates are skipped across the entire collection. `--count` is a bounded
number of attempts (1-100), not a promise to manufacture distinct frames. Exit 2
means some attempts were duplicates; other errors stop without automatic retry.
The default interval is 3 seconds and network timeout is 5 seconds (max 30).
Use `--notes` for provenance/scene details, never credentials. Images start with
`reviewed=false` and require manual acceptance before dataset conversion.

`--output-dir` changes the collection root. Inside this repository it must remain
under ignored `captures/`; external folders are allowed. Use one root for all
splits so checks can see all object/session assignments. Separate roots cannot
detect leakage between each other.

CSV updates use a flushed/fsynced temporary file and atomic replacement while a
collection lock excludes other writers. Existing JPEGs are opened exclusively;
filename collisions never overwrite data. Keep Excel or other CSV editors closed
while capturing. Windows sharing/lock violations get at most three atomic-replace
attempts with short delays; persistent locks stop collection without replacing the
old CSV. On interruption, previously committed rows remain intact. A hard
crash may leave `.capture.lock`, a temporary CSV, or an unindexed JPEG. After
confirming no collector is running, inspect these files; preserve the committed
CSV and quarantine unindexed images outside active session folders until reviewed.
Remove only the stale lock after checking its PID. The next run detects partial
CSV records, checksum changes and unindexed images and stops rather than silently
relabeling or deleting them. Filesystem/power-loss guarantees still depend on the
underlying disk; retain backups of accepted data.

## Pilot, after HTTP/JPEG verification

No pilot photos have been collected yet. `captures/` was absent at the start of
this stage, so `session01` is the proposed first session; recheck before use.
Use **training** for every pilot image. The pilot is a workflow check, not an
independent validation or test set. Keep VMOT and all external wiring disconnected.

Run only the command for the scene currently placed on the plate:

| Scene | Label | Object IDs | Session / split |
|---|---|---|---|
| Empty actual plate | background | none | session01 / training |
| One clean sheet or scrap of paper | paper | paper01 | session01 / training |
| One ordinary eraser | unknown | eraser01 | session01 / training |
| That paper and eraser together | multiple_items | paper01;eraser01 | session01 / training |

```bat
.venv\Scripts\python tools\data\capture.py --label background --session session01 --split training --count 3 --interval 3
.venv\Scripts\python tools\data\capture.py --label paper --object-id paper01 --session session01 --split training --count 3 --interval 3
.venv\Scripts\python tools\data\capture.py --label unknown --object-id eraser01 --session session01 --split training --count 3 --interval 3
.venv\Scripts\python tools\data\capture.py --label multiple_items --object-id paper01 --object-id eraser01 --session session01 --split training --count 3 --interval 3
```

These are four separate, operator-assisted groups, not a batch to run before
changing the scene. Aim for 2-3 useful images per group; vary placement or normal
lighting between frames and allow the scene to settle. Duplicate skips reduce
the saved count. Keep notes about the photographer, plate, camera mounting and
lighting. Do not use hazardous examples for this pilot.

Before further collection, inspect all 8-12 resulting images and metadata:
decoding/dimensions, label spelling, exact IDs/split, duplicates, sharpness,
exposure and orientation. Confirm the full frame is retained and the item is in
the model's center crop: for 320x240 input this is the central 240x240 square
(horizontal pixels 40-279). Confirm the framing represents the intended plate.
Do not upload to Edge Impulse until the pilot passes this review.

## Bounded expansion, gated on pilot acceptance

For each of the five known classes, start with 6-8 physically distinct examples
and 10-20 varied frames each. Prefer eight objects split 4 training / 2 validation
/ 2 test, with separate sessions assigned before capture. Six objects with a
4/1/1 split provide especially weak per-class generalization evidence. For each
reject class, initially target about 100 useful images across independent
training/validation/test sessions (for example 60/20/20). Unknown objects and
constituent objects in paired scenes must stay in their assigned splits.

This is approximately 700-1,100 useful images, not hundreds of nearly identical
frames of one item. Use different orientations, positions and ordinary lighting.
An intact battery class still needs appropriate handling and original data;
damaged batteries and other hazardous objects are excluded from this stage.
Counts are starting targets only; validation and held-out error counts determine
sufficiency. Full collection and model training remain pending pilot acceptance.

## Acceptance before physical inference

After training, freeze confidence/margin thresholds using validation predictions;
report held-out per-class metrics, hazardous misroutes, false actuation on empty
and unknown scenes, and rejection coverage. Keep model validation and physical
motors disabled until actual camera-domain tests and the separate power/mechanical
checks pass. The one-item loading assumption must still be enforced physically.
