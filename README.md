# embedded-ai-cam-sorting-controller
ESP32-CAM to perform on-device waste classification and control multi-axis servo sorting 

---

## Key Features

- On-device AI inference using Edge Impulse (FOMO model)
- Deterministic finite state machine (FSM) for system control
- FreeRTOS-based concurrent task execution
- Interrupt-driven event handling for real-time responsiveness
- Non-blocking stepper motor control using DRV8825 drivers
- UART-based logging for debugging and system visibility
- Modular ESP-IDF architecture with component-based design

---

## Tech Stack

### AI / Computer Vision
- Edge Impulse (FOMO object detection model)
- Roboflow (dataset collection and labeling)

---

### Embedded Software
- ESP-IDF (Espressif IoT Development Framework)
- FreeRTOS (task scheduling and concurrency)
- C / C++ (embedded systems programming)
- Xtensa toolchain (cross-compilation)

---

### Development Environment
- VS Code
- Linux CLI

---

### Hardware
- ESP32-CAM (AI-Thinker, ESP32-S + OV2640)
- DRV8825 Stepper Motor Drivers
- NEMA 17 Stepper Motors
- Buck Converter (power regulation)
- UART interface for flashing and debugging

---

## Embedded System Design: Finite State Machine (FSM)
The system is controlled using a deterministic FSM to manage transitions between sensing, inference, decision-making, and actuation.

### Development Environment
- VS Code
- Linux CLI

---

### Hardware
- ESP32-CAM (AI-Thinker, ESP32-S + OV2640)
- DRV8825 Stepper Motor Drivers
- NEMA 17 Stepper Motors
- Buck Converter (power regulation)
- UART interface for flashing and debugging

---

### FreeRTOS Task Architecture
The system is divided into independent tasks for scalability and responsiveness:

- Vision Task → image capture and AI inference
- Control Task → FSM state management
- Motor Task → stepper motor control

Inter-task communication is handled using queues and event groups.

---

### Interrupt-Driven Design
Hardware interrupts are used for:
- Object detection triggers
- Limit switch feedback (position calibration)
- Timer-based scheduling

---

## Build & Flash Process

Firmware is built using the ESP-IDF toolchain:
important** - make sure to run export.bat to turn ESP-IDF environment on for the terminal
```bash
idf.py build
idf.py flash
idf.py monitor