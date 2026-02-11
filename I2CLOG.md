# 2026-02-10: BMP280 - Datasheet Reading & Driver Implementation

## Pin Selection & Alternate Functions
According to **Table 11 (Alternate Function)** in the STM32F446MC datasheet, AF 4 is the correct setting. 
* **I2C1_SCL:** Supported by PB6 and PB8.
* **I2C1_SDA:** Supported by PB7 and PB9.
* (PB10 and PB11 support I2C2).

I learned that unlike TIMERS—where features vary significantly (e.g., Basic TIM6 vs. Advanced TIM1)—the three I2C peripherals (I2C1, 2, 3) are essentially identical in functionality. I chose **PB8 and PB9** simply because they are physically consecutive pins on the CN10 header, which makes wiring cleaner.

## Driver Implementation
Honestly, setting up the I2C1 registers is just bit manipulation and pointer arithmetic, similar to the GPIO driver. However, while I found some conceptual similarities between I2C and UART, the implementation differences are significant.

### Physical Connection
I connected 4 pins: **VIN, GND, SCL, and SDA**.
* **VIN & GND:** Power the BMP280 sensor via the STM32.
* **SCL (Serial Clock):** Since the STM32 is the **Master**, it generates the clock. I had to calculate the correct values for the CR2 and CCR registers to achieve the standard **100kHz** frequency.
* **SDA (Serial Data):** This is where the data transfer happens. Similar to TX/RX in UART, writing to the Data Register (DR) shifts bytes out through this pin.

### Conceptual Model: UART vs. I2C
My understanding of the difference is:
* **UART (Asynchronous):** Feels like "two one-way streets." Even though it’s bidirectional, the TX and RX lines are independent. There is no shared clock line.
* **I2C (Synchronous):** Is truly bidirectional on a *single* data line (Half-Duplex). The Master must follow a strict protocol sequence to control the bus.

### The Protocol Sequence
The communication flow I implemented is:
1.  **START:** Master sets the START bit to claim the bus.
2.  **ADDRESS:** Master sends "Device Address + Read/Write bit" (0 for Write, 1 for Read).
3.  **ACK:** The target device (BMP280) responds with an ACK. 
    * *Note:* I assume this is handled automatically by the hardware. I enabled the ACK bit in the STM32 configuration, so the hardware waits for this handshake. I don't need to manually "force" the sensor to reply.
4.  **DATA:** Master sends the specific Register Address (e.g., `0xD0` for ID) and waits for another ACK.

Once the STM32 finishes "talking" (requesting the ID), the roles switch. The BMP280 takes over the SDA line to send data back.

### The "NACK" Trick
My goal is to read the Chip ID at register `0xD0` (1 byte). I followed the **"Master Receiver"** sequence from the reference manual. 

There is a specific nuance here:
* Immediately after the **ADDR** flag sets (indicating address matched), I **disable the ACK bit** in CR1.
* This tells the hardware to send a **NACK** after receiving the next byte.
* This NACK effectively tells the slave (BMP280) to "stop talking." If I sent an ACK instead, the slave might try to send the next register value, confusing the protocol.
* Finally, I set the STOP bit.

### TXE vs. BTF flags
I noticed I2C has two flags that map to UART concepts:
1.  **TXE (Transmit Empty):** The buffer is empty and ready for the next byte.
2.  **BTF (Byte Transfer Finished):** Comparable to **TC** in UART. It means the transfer is fully complete and the shift register is empty.

## The Issue: Hardware Reality
Despite the code logic seemingly correct, I could not read the ID. The program hangs infinitely at this line:

```c
while ( (I2C1->SR1 & (1u << 1)) == 0 ); // Wait for ADDR to become 1
```

**Diagnosis:**
This loop waits for the hardware to set the **ADDR** flag, which is dependent on receiving an **ACK** from the sensor.

* Since I did not solder the headers to the BMP280 module (relying on loose physical contact), the signal integrity is likely compromised.
* The STM32 sends the address `0x76`, but the BMP280 either doesn't receive it clearly or its response (ACK) isn't reaching the STM32.
* Because the STM32 never sees the ACK, ADDR never sets, and the software is stuck.

**Conclusion:** The code logic is valid, but the physical layer failed. Soldering is required for I2C communication.

# 2026-02-09: I2C Chips - Basic Understanding & Hardware Logic

Although I already knew from previous exposure to I2C that 4 pins are required (VCC, GND, SCL, SDA), and that pull-up resistors must be enabled in Open-Drain mode, I was curious about the exact physical hardware reasons behind these requirements.

### I2C vs. UART: Key Differences

First, I clarified the difference between UART (which I previously used for Serial Print) and I2C:

1.  **Speed & Clock:** I2C is generally faster than UART (though not as fast as SPI). Crucially, I2C is **Synchronous** (it uses a clock signal), whereas UART is **Asynchronous**. In UART, two devices must agree on a specific baud rate ahead of time; if they don't, the communication fails. I2C devices just follow the clock line (SCL) driven by the master.
2.  **Architecture:** UART is typically a one-to-one connection, often used for debugging or simple communication. I2C uses a specific protocol that allows multiple external devices (Sensors, EEPROMs, etc.) to be connected to a single central device (like my STM32).
3.  **Why BMP280 uses I2C:** Even though I use UART to print debug info to my PC, the BMP280 sensor itself is manufactured with an I2C interface. It physically provides 4 pins (VIN, GND, SCL, SDA), so I must conform to its protocol.

### The Physics: Push-Pull vs. Open-Drain

I spent some time understanding why we need **Open-Drain** and **Pull-up Resistors**, specifically regarding "short circuits."

**1. Why Unidirectional logic (LEDs/Motors) doesn't short:**
Previously, when I configured PA0 to output PWM for the A4988 motor driver, or GPIOs to drive LEDs, I used **Push-Pull** mode.
* **LEDs:** These are purely "loads" that consume energy.
* **A4988 (Step Pin):** This is a **Unidirectional** setup (STM32 talks, Driver listens). The A4988 input pin has **High Impedance** (Hi-Z), meaning it effectively looks like an open circuit to the STM32. It reads the voltage but draws almost no current. There is no risk of the A4988 trying to "push back" voltage to the STM32.

**2. The Danger of Bidirectional Logic (I2C):**
I2C is **Bidirectional**. The BMP280 sensor needs to send data (like IDs or temperature) back to the STM32. This means the sensor also has the ability to control the voltage on the wire.

* **The "Push-Pull" Disaster Scenario:**
    If I configured the STM32 and Sensor in Push-Pull mode, a conflict could occur.
    * Imagine STM32 tries to output **High (3.3V)** (Internal Top Switch closed).
    * At the same time, the Sensor tries to output **Low (GND)** (Internal Bottom Switch closed).
    * **Result:** A direct path forms from STM32 VCC (3.3V) -> Wire -> Sensor GND. Since there is effectively **zero resistance** in this loop, a massive current would flow, instantly frying the transistors in both chips.

**3. The Solution: Open-Drain + Pull-Up Resistor**
To prevent this, I2C uses **Open-Drain** mode.
* **Mechanism:** In this mode, the chips **cannot** physically output 3.3V (the internal "Top Switch" is removed or disabled). They can only do two things:
    1.  **Pull Low:** Connect the line to GND (Logic 0).
    2.  **Let Go (Float):** Disconnect from the line entirely.
* **The Role of the Resistor:** Since no one can output 3.3V, we use an external **Pull-Up Resistor** to pull the line to 3.3V when everyone is "letting go." This makes High (3.3V) the default **Idle State**.
* **Safety:** Now, if STM32 "lets go" (High) and the Sensor "pulls down" (Low), current flows from the 3.3V source, through the **Pull-Up Resistor**, and then to the Sensor's GND. The resistor limits the current to a safe level (e.g., < 1mA), preventing any damage.
* **Active Low Logic:** Because the chips can only pull down, **Logic 0** becomes the "Active" signal (e.g., an ACK signal is a Low pulse), and Logic 1 is just the idle state.

### Project Structure 

For the code implementation, I will not create a brand new project. Instead:
1.  I will keep the existing `main.c` (renaming its function to `main_led_demo`).
2.  I will create a new file (e.g., `main_i2c.c`) for the I2C logic.
3.  Since the compiler only looks for one `int main(void)` entry point, I can simply toggle which file contains the active `main` function depending on which demo I want to run.