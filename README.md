# Basic Serial Interface with Hardware

## Goal
I implemented this challenge by partially re-using the **Bare-Metal STM32 drivers** that I previously developed from scratch. This approach allows me to manually configure GPIOs and UART peripherals, demonstrating understanding of direct memory manipulation without relying on HAL.

## Hardware Setup
I kept the hardware setup minimal to prioritize firmware logic and signal integrity.

* **MCU:** STM32 Nucleo-F446RE (ARM Cortex-M4)
* **Components:** * 3 x LEDs (Red, Green, Blue)
    * 3 x Tactile Switches
    * (220Ω) Resistors & Breadboard wiring

## Development Plan

### Step 1: Circuit Assembly
* Build the physical circuit on the breadboard.

### Step 2: System Architecture & Logic
* **Role Definition:** The STM32 acts as the **Muscle**. It listens for commands and executes them immediately.
    * If the STM32 receives an invalid command, it should do nothing and return an error message.
* **Communication Protocol:** 
    * **Mode:** Character-Based (Immediate Execution).
    * **Reasoning:** I chose a non-blocking, interrupt-driven approach. This ensures the system has low latency.

### Step 3: Implementation
* **Drivers:** Port existing custom UART/GPIO drivers to the new project.
* **Interrupt Handling:** Use `USART2_IRQHandler` to process real-time data.
* **Command Processing:** Implement Switch-Case logic loop to process commands (`1-8`) and handle Switch inputs.
* **GPIO Configuration:** Consult the datasheet to find available pins.
    * I aim to group the 3 LEDs (e.g. PA 5, 6, 7) and the 3 Switches on the same Port (e.g. PA 10, 11, 12). This keeps the code clean and allows me to enable fewer GPIO clocks, which is slightly more efficient.
<img width="960" height="720" alt="image" src="https://github.com/user-attachments/assets/619d68fd-0a29-4dd4-b1e2-5f0a2dc39197" />

<img width="1193" height="775" alt="image" src="https://github.com/user-attachments/assets/f13c13ed-3fcf-4f36-af39-35ecc877e10c" />

## User Manual / Commands
Connect via Serial Monitor in VSCode (Baud: 115200, 8-N-1):
| Key | Action |
| :--- | :--- |
| **1 / 2** | Red LED On / Off |
| **3 / 4** | Green LED On / Off |
| **5 / 6** | Blue LED On / Off |
| **7 / 8** | **All LEDs** On / Off |
| **a / b / c** | Read Switch A / B / C state |
| **d** | Read **All Switches** state |
