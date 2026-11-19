# Dataset selection — 2026-11-19

No dataset is approved for final training yet. Five candidates were screened;
the first two are selected for a **download/audit queue**, not accepted as a
combined training set. Interactive Roboflow access was denied by the browser
permission gate. No raw images were downloaded, visually inspected, or merged.
Publicly indexed project metadata is the evidence below.

| Rank / candidate | License, version and published data | Decision |
|---|---|---|
| 1. [Material Identification / GARBAGE CLASSIFICATION 3](https://universe.roboflow.com/material-identification/garbage-classification-3/dataset/2) | CC BY 4.0; v2, 10,464 bounding-box images; train 7,324 / validation 2,098 / test 1,042; 416×416 stretch. Biodegradable, cardboard, glass, metal, paper, plastic. | Best documented broad material coverage. COCO download advertised. Broad plastic/metal/glass cannot be renamed bottle/aluminum-can without inspecting images. No special-waste class. |
| 2. [battery detection / Waste-battery-only](https://universe.roboflow.com/battery-detection-yuv0z/waste-battery-only/dataset/12) | CC BY 4.0; v12, 753 bounding-box images; 597 / 156 / **0**; 640×640 with white padding, no augmentation. Alkaline, Alkaline_jp, CR_Lithium, Lithium, Lithium_jp, LR_Alkaline, Manganese, Manganese_jp. | Small hazardous-object supplement; merge chemistry labels only to `battery`, never infer chemistry. Must create held-out object/session groups before training. COCO download advertised. |
| 3. [Waste segregation / waste classification](https://universe.roboflow.com/waste-segregation-cxcrv/waste-classification-e2jvj/dataset/1) | CC BY 4.0; v1, 6,152 source images → 14,810 generated; 12,988 / 1,210 / 612; 640×640 stretch, flips/rotation and partial grayscale. | 52 multi-label classes include background/empty/multiple, bottles/paper/cans and e-waste, but inconsistent spellings and no clear food coverage. Do not turn multi-label annotations into a single class automatically. |
| 4. [Waste Classification / waste-classification-4j3md](https://universe.roboflow.com/waste-classification-bweuv/waste-classification-4j3md) | v3 advertises 25,556 generated images from 15,046 originals. Eight labels: ewaste, food_waste, leaf_waste, metal_cans, paper_waste, plastic_bags, plastic_bottles, wood_waste. **License not verified on inspected page.** | Relevant visual categories, but excluded until redistribution/training permission is established. No download or use. |
| 5. [waste classification / WasteClassification01](https://universe.roboflow.com/waste-classification-s6ei0/wasteclassification01) | CC BY 4.0; v1, 2,218 originals, 5,325 generated; organic/inorganic classification. | Too broad; inorganic mixes recyclable and special waste. Not a defensible three-destination classifier. |

Per-class image/object counts, actual box quality, empty scenes, pose, lighting,
occlusion, imbalance, duplicates, video-frame similarity and split leakage are
**unknown for all five** until raw-image audit. Published split counts are not
proof of independent physical objects. Dataset titles do not establish a fixed
overhead-camera domain. No candidate supplies verified coverage of light bulbs
or aerosols; neither class may be claimed trained merely because firmware maps it.

CC BY 4.0 candidates can be considered together with attribution and a record of
transformations. Keep source author, URL, version, license and archive hash per
source; inspect upstream provenance before distributing derived data. A project's
declared license is not a visual or annotation-quality guarantee. No incompatible
or unknown-license dataset has been combined.

Download only these two candidates first: open their version links, choose
**Download → COCO JSON**, and save locally under ignored `datasets/`. Do not
copy generated API snippets or token-bearing URLs into source/history. The
auditor and explicit battery normalization are in `tools/data/`; see
[DATASET_AUDIT.md](DATASET_AUDIT.md). If neither passes, collect a small licensed
camera-domain dataset rather than widening this survey indefinitely.
