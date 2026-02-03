# 2026-02-02-22:00 Status Update & Debugging

I have successfully reused my previous drivers and implemented a simple switch/case logic to handle the 8 commands for the LEDs. This meets the requirements to "set the state of each LED output individually" and "set the state of all LED outputs simultaneously."

However, I noticed two issues during testing.

First, there is a concurrency bug. If I send commands back-to-back (e.g., "78"), the first character '7' is read and begins execution. Since the UART interrupt is faster than my main processing loop, '8' arrives and updates the data variable while '7' is still being processed. The problem is that once the logic for '7' finishes, my code unconditionally resets the data variable to 0. This effectively wipes out the '8' before it can be interpreted.

Second, I encountered a hardware noise issue similar to my previous cat feeder project. Whenever I flash the board or reboot, I see "garbage characters" on the console. Initially, the welcome message was also printing four times. I added a software delay loop right after `hardware_setup()`, which successfully eliminated the repeated welcome messages, but the garbage characters remain. I highly suspect this is a physical hardware behavior: when the STM32 resets, the TX pin is briefly floating or transitioning, which the receiver misinterprets as random data.

I will now focus on fixing the first bug (the "78" command issue), as that is a clear software logic error involving the buffer reset timing.

# 2026-02-02-19:00 Hardware Setup: Wiring and Verification

Even though I previously jumped straight into complex circuits involving the A4988 driver and NEMA 17 motors for my Cat Feeder project, I realized I had very little hands-on experience wiring basic LEDs on a breadboard from scratch. I want to make sure the fundamentals are solid for this challenge.

### 1. Pin Selection & Datasheet Check
I referenced **Table 11 (Alternate Function)** in the STM32F446RE datasheet to select the GPIO pins.

* **Avoided:** `PA2` (USART2_TX) and `PA3` (USART2_RX) to preserve the serial connection for debugging.
* **Selected:** `PA5`, `PA6`, and `PA7`.

**Why PA5?**
I specifically chose `PA5` because it is hardwired to the onboard **Green LED (LD2)**. This serves as a perfect hardware debugging proxy:
* **Scenario A:** If the onboard LD2 lights up but my breadboard LED doesn't, I know the code is working and the issue is likely my external wiring or a bad component.
* **Scenario B:** If neither lights up, the issue is likely in my firmware logic or clock configuration.

### 2. Circuit Safety (Current Limiting)
Before connecting the GPIOs, I needed to ensure I wouldn't burn anything. The STM32 GPIO outputs **3.3V**.

According to standard specs, LED Forward Voltages ($V_f$) are:
* **Red:** ~1.8V - 2.1V
* **Green/Blue:** ~3.0V

Without a resistor, the voltage difference ($3.3V - V_f$) would drop across the wire. Since the wire's resistance is near zero, Ohm's Law ($I = V/R$) dictates that the current would spike massively, potentially damaging the STM32 pin or blowing the LED. I added current-limiting resistors in series to prevent this.

### 3. Verification (The Smoke Test)
I attempted to verify the polarity using my multimeter. However, the probes were a bit too thick for the breadboard holes, and the Diode Mode voltage didn't seem sufficient to light up the Blue LED fully.

To be 100% sure, I performed a manual "live test":
1.  I disconnected the GPIO wires.
2.  I used a jumper wire connected to the **STM32 3.3V rail** to manually touch the input of each resistor.
3.  **Result:** All three LEDs (Red, Green, Blue) lit up correctly.

Now that the hardware is verified, I can safely connect them to `PA5`, `PA6`, and `PA7` and start writing the driver.