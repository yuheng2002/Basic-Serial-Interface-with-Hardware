# Basic Serial Interface with Hardware

## Goal
I implemented this challenge by leveraging **Bare-Metal STM32 drivers** that I developed from scratch. This approach allows me to manually configure GPIOs and UART peripherals, demonstrating understanding of direct memory manipulation without relying on HAL abstraction layers.

## 🛠 Hardware Setup
I kept the hardware setup minimal to prioritize firmware logic and signal integrity.

* **MCU:** STM32 Nucleo-F446RE (ARM Cortex-M4)
* **Components:** * 3 x LEDs (Red, Green, Blue)
    * 3 x Tactile Switches
    * Resistors & Breadboard wiring

## Development Plan

### Step 1: Circuit Assembly
* Build the physical circuit on the breadboard.

### Step 2: System Architecture & Logic
* **Role Definition:** The STM32 acts as the **Muscle**. It listens for commands and executes them immediately.
    * If the STM32 receives an invalid command, it should do nothing and return an error message.
* **Communication Protocol:** * **Mode:** Character-Based (Immediate Execution).
    * **Reasoning:** I chose a non-blocking, interrupt-driven approach. This ensures the system has low latency.

### Step 3: Implementation
* **Drivers:** Port existing custom UART/GPIO drivers to the new project.
* **Interrupt Handling:** Use `USART2_IRQHandler` to process real-time data.
* **Command Processing:** Implement Switch-Case logic loop to process commands (`1-8`) and handle Switch inputs.
* **GPIO Configuration:** * Consult the datasheet to find available pins.
    * I aim to group the 3 LEDs on the same Port (e.g., Port A) and the 3 Switches on another Port (e.g., Port C). This keeps the code clean and allows me to enable fewer GPIO clocks, which is slightly more efficient.