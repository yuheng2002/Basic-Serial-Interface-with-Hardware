# 2026-02-03-2:00 Final Update

For this challenge, I used PA10, PA11, and PA12 as my three inputs. I double-checked Table 11 (Alternate Function) in the datasheet and confirmed these are technically "free" pins that aren't tied to system functions like MCO1, unlike PA8. This follows the KISS principle—there's no need to overcomplicate things by enabling extra clocks when I can just use the available GPIOs from the same Port A.

Implementation-wise, I had to account for the physical difference between driving an LED and reading a switch. Unlike an output which locks a voltage, an input pin can "float" if left unconnected, leading to random noise. To fix this, I enabled the internal pull-up resistors for all three pins. This ensures the default state is physically 3.3V (High). When the button is pressed, it connects to Ground (Low). This is standard "Active Low" logic, meaning the software sees a '0' as a press and a '1' as a release.

Initially, my code kept reporting "Switch Pressed" regardless of whether I touched the buttons or not. I first checked my software logic to ensure I didn't get the if (input == 0) condition backward. But I quickly realized that even if the logic was inverted, I should still see the status toggle when I pressed the button. Since the state never changed, I deduced this was likely a hardware issue, not software.

I grabbed my multimeter to isolate the problem. First, I removed the switches entirely. PA12 showed a steady 3.3V, confirming the pull-up was working, but PA10 and PA11 were reading 0V. I traced this back to a bad connection on the breadboard's side power rail—essentially a dead zone. I swapped the wires to a different grounding node, which restored the default 3.3V on all channels.

Even with the voltage rail fixed, putting the switches back in immediately dropped the signal to 0V again. I used the multimeter's **Continuity Mode** to check the switch pins and realized I had misunderstood the pinout. These rectangular switches have their pins internally connected along the longer side. Because of this, I had inadvertently wired both the GPIO and the Ground wire to the same "side" of the metal contact, creating a permanent short circuit.

The fix was simple: I re-wired the switches diagonally (connecting Top-Left to Bottom-Right) to force the current to pass through the actual switching mechanism. 

Finally, I refactored the code to align with the Layered Architecture principles. I extracted the repetitive hardware logic from the main switch/case superloop and moved it into generic helper functions. This makes the main loop much more readable. Also, if I want to add more LEDs or switches later, I can simply call the function instead of copy-pasting code blocks and manually editing pin numbers.

# 2026-02-02-22:25 Quick Update

A quick update before I move on to the Switches.

I have separated the switch/case implementation into a `Process_Command()` function and adjusted the logic flow in `main` to follow a "Read-Clear-Process" pattern:

```c
uint8_t temp = message; // 1. Read -> Copy Command Data
message = 0;            // 2. Clear -> Release buffer for ISR
Process_Command(temp);  // 3. Process -> Handle the command
```
With this logic, if I input "78", the system behaves exactly as the hardware dictates:

'7' is saved to temp, the buffer is cleared, and '7' is processed (turning LEDs ON).

Meanwhile, '8' arrives in the Data Register (DR) and triggers the interrupt.

The next loop processes '8' (turning LEDs OFF).

This confirms the expected hardware behavior: since the UART Data Register is 8-bit, it handles data character-by-character.

Logically, "78" should probably be treated as an invalid command rather than two sequential valid ones. However, fixing this in firmware would require switching from "Immediate Execution" to a "Buffered" approach (waiting for a newline character), which adds unnecessary complexity for this demo.

Instead, I will rely on the high-level user API (Python) to handle input validation and ensure only single characters are sent. The firmware will remain lightweight and responsive.

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