# Basic Serial Interface with Hardware

## Goal
I decided to implement this challenge by leveraging the **Bare-Metal STM32 drivers** I developed previously. This allows me to set up GPIOs and UART manually, demonstrating my understanding of direct memory manipulation. 

Since this task requires handling various command strings, I will also be developing a **custom command parser** to process user input efficiently on the MCU.

## 🛠 Hardware Setup
I am keeping the hardware setup simple to focus on the firmware logic.

* **MCU:** STM32 Nucleo-F446RE (ARM Cortex-M4)
* **Components:** 3 LEDs, 3 Tactile Switches, Resistors, Breadboard

## Development Plan

### Step 1: Circuit Assembly
* Physically build the circuit on the breadboard.
* Connect the LEDs and Switches to the Nucleo board using jumper wires.

### Step 2: System Architecture & Safety Logic
* **Role Definition:** The STM32 acts as the "muscle" to execute commands, while the PC acts as the brain/interface.
* **Robustness:** Even though the Python script will include input validation, I believe the **STM32 firmware must also have its own safeguards**. 
    * If the STM32 receives an invalid command, it should do nothing and return an error message. 
    * **Reasoning:** The Python script is just one possible high-level interface. If we later decide to switch to a different host API, the firmware itself must handle stay secure.

### Step 3: Implementation
* **Drivers:** Reuse my existing UART drivers to establish communication between the board and the PC.
* **Parsing:** Implement the string parsing logic in C to interpret commands.
* **GPIO Configuration:** * Consult the datasheet to find available pins.
    * I aim to group the 3 LEDs on the same Port (e.g., Port A) and the 3 Switches on another Port (e.g., Port C). This keeps the code clean and allows me to enable fewer GPIO clocks, which is slightly more efficient.