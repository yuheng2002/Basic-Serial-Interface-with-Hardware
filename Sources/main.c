/*
 * I2C_demo_main.c
 *
 *  Created on: 2026/2/10
 *      Author: Yuheng
 */
#include <stdint.h>
#include "stm32f446xx.h"
#include "stm32f446xx_gpio_driver.h"

/*--- Macros ---*/
#define I2C1_CLK_EN()   ( SET_BIT(RCC->APB1ENR, 21) ) // APB1-> Bit 21 -> I2C1EN

#define BMP280_ADDR     ( 0x76 ) // In its User Manual
#define BMP280_ID       ( 0xD0 ) // In its ds001 datasheet -> WHO_AM_I
/*
 * I2C1 Initialization (Later I will refractor this into a separate driver.c file)
 */
void I2C1_Init(void){
	/* 1. Enable CLock for I2C1 */
	I2C1_CLK_EN();

	/* 2. Configure CR2->FREQ
	 * this needs to match the APB1 Bus Frequency (at default, HSL = 16MHz)
	 * Bits 5:0 FREQ[5:0]: Peripheral clock frequency
	 */
	I2C1->CR2 &= ~(0x3F) ; // clear bits 5:0 to 0
	I2C1->CR2 |= 16u;      // set to 16 (0 1 0 0 0 0)

	/*
	 * 3. Configure CCR (SCL -> 100kHz)
	 *
	 * (a) Bit 15 F/S: I2C master mode selection (0: Sm mode I2C / 1: Fm mode I2C)
	 * Here, I choose Sm mode (standard mode) -> 100kHz -> set to 0
	 * (b) Bit 14 DUTY: Fm mode duty cycle
	 * set to 0 <- it only works in Fm (fast mode)
	 * (c) Bits 13:12 Reserved, must be kept at reset value -> keep at 0
	 * (d) Bits 11:0 CCR[11:0]: Clock control register in Fm/Sm mode (Master mode)
	 * Formula: CCR = PCLK1 / (2 * Target_SCL) = 16M / 200k = 80
	 */
	I2C1->CCR &= ~(0xFFF); // clear bits 11:0
	I2C1->CCR &= ~(1u << 15); // Mode set to Standard (0)
	I2C1->CCR |= 80u; // CCR value is now 80

	/*
	 * 4. Configure TRISE (maximum Rise time)
	 * Formula: F_MHz + 1 = 16 + 1 = 17
	 */
	I2C1->TRISE = 17;

	/*
	 * 5. Enable Peripheral (turn it on)
	 *
	 * (a) ACK
	 * Bit 10 -> 1: Acknowledge returned after a byte is received (matched address or data)
	 * (b) PE
	 * Bit 0 -> 1: Peripheral enable
	 * In master mode, this bit must not be reset before the end of the communication.
	 */
	I2C1->CR1 |= (1u << 10); // Enable ACK
	I2C1->CR1 |= (1u << 0); // Enable PE

}

void I2C1_setup(void){
	/*
	 * I chose :
	 * PB8 -> I2C1_SCL (AF 4)
	 * PB9 -> I2C1_SDA (AF 4)
	 */

	/* Enable Port B Clock */
	GPIOB_PCLK_EN(); // Macro defined in gpio_driver.h

	/*
	 * ====================
	 * PB8 Configuration
	 * ====================
	 */
	GPIO_Handle_t I2C1_SCL;
	I2C1_SCL.pGPIOx = GPIOB;
	I2C1_SCL.GPIO_PinConfig.GPIO_PinNumber = 8;
	I2C1_SCL.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTN; // AF mode
	I2C1_SCL.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU; // Pull-up resistor enabled
	I2C1_SCL.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD; // Open Drain
	I2C1_SCL.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_VERY_HIGH; // Max speed
	I2C1_SCL.GPIO_PinConfig.GPIO_PinAltFunMode = GPIO_AF_4; // AF 4

	GPIO_Init(&I2C1_SCL);

	/*
	 * ====================
	 * PB9 Configuration
	 * ====================
	 */
	GPIO_Handle_t I2C1_SDA;
	I2C1_SDA.pGPIOx = GPIOB;
	I2C1_SDA.GPIO_PinConfig.GPIO_PinNumber = 9;
	I2C1_SDA.GPIO_PinConfig.GPIO_PinMode = GPIO_MODE_ALTN; // AF mode
	I2C1_SDA.GPIO_PinConfig.GPIO_PinPuPdControl = GPIO_PIN_PU; // Pull-up resistor enabled
	I2C1_SDA.GPIO_PinConfig.GPIO_PinOPType = GPIO_OP_TYPE_OD; // Open Drain
	I2C1_SDA.GPIO_PinConfig.GPIO_PinSpeed = GPIO_SPEED_VERY_HIGH; // Max speed
	I2C1_SDA.GPIO_PinConfig.GPIO_PinAltFunMode = GPIO_AF_4; // AF 4

	GPIO_Init(&I2C1_SDA);
}

uint8_t BMP280_ReadID(void){
	uint8_t data = 0;
	uint32_t READ_ADDR= 0;

	/*
	 * ------------------------------
	 * Master [WRITE] to Slave (Transmitter)
	 * ------------------------------
	 */

	/*
	 * ==============================
	 * 		 Step 1. START
	 * ==============================
	 * CR1 -> Bit 8 START: Start generation
	 * In Master Mode:
	 *     0: No Start generation
	 *     1: Repeated start generation
	 * In Slave mode:
	 *     0: No Start generation
	 *     1: Start generation when the bus is free
	 */
	I2C1->CR1 |= (1u << 8);
	// once START is set to 1, hardware will automatically generates square waves
	// I will need to "check" when hardware finishes its part

	// --- Status Register 1 ---
	// Bit 0 SB: Start bit (Master mode)
	// 1: Start condition generated.
	// – Set when a Start condition generated.
	while ( (I2C1->SR1 & (1u << 0)) == 0); // only proceed when Start condition is checked

	/*
	 * ==============================
	 * Step 2. Write Device Address
	 * ==============================
	 * Now write the device address (getting BMP280's attention)
	 *
	 * LSB -> 0 = Write / 1 = Read
	 *
	 	 I2C1->DR &= ~1u;
	 	 I2C1->DR |= (BMP280_ADDR << 1);

	 * Commented out because writing to DR register triggers the transmission immediately
	 * MUST use = instead of |= to avoid sending double packets
	 */
	I2C1->DR = (BMP280_ADDR << 1); // Bits 7:1 are the actual data (in this case, 0x76)

	/*
	 * ==============================
	 *     Step 3. ADDR
	 * ==============================
	 * SR1: Bit 1 ADDR: Address sent (master mode)/matched (slave mode)
	 * "This bit is cleared by software reading SR1 register followed reading SR2..."
	 * [Address sent (Master)]
	 *     0: No end of address transmission
	 *     1: End of address transmission
	 * – For 10-bit addressing, the bit is set after the ACK of the 2nd byte.
	 * – For 7-bit addressing, the bit is set after the ACK of the byte.
	 */
	while ( (I2C1->SR1 & (1u << 1)) == 0 ); // Wait for ADDR to become 1 (address matched)
	READ_ADDR = I2C1->SR1; // Read SR1->ADDR
	READ_ADDR = I2C1->SR2; // Immediately read SR2
	(void)READ_ADDR; // to get rid of compiler warning

	/*
	 * ==============================
	 * Step 4. Write Register Address
	 * ==============================
	 * Now write the Register Address (in this case, 0xD0 -> "id")
	 */
	I2C1->DR = 0xD0; // BMP280 now points to "id" register

	/*
	 * Wait for TEX
	 *
	 * Bit 7 TxE: Data register empty
	 *     0: Data register not empty
	 *     1: Data register empty
	 * "Cleared by software writing to the DR register or
	 * by hardware after a start or a stop condition"
	 *
	 * Basically, if it is 0, then the data register still has data from last load
	 * -> not ready to load again
	 * -> must wait for it to become empty (1)
	 */
	while ( (I2C1->SR1 & (1u << 7)) == 0 ); // Wait until it becomes 1

	/*
	 * Wait for BTF (similar to TC in UART)
	 *
	 * Bit 2 BTF: Byte transfer finished
	 *
	 * Think of an archer shooting an arrow:
     * - TXE is like the archer's hand. If the hand is empty (TXE=1), he can grab another arrow.
     * - BTF is like the arrow hitting the target.
     *
     *     0: Data byte transfer not done
     *     1: Data byte transfer succeeded
     *
     * We can ONLY move forward when this flag is set to 1 by hardware
	 */
	while ( (I2C1->SR1 & (1u<< 2)) == 0 ); // Wait

	/*
	 * NOTE:
	 * checking TXE is standard in data transfer policy (same in UART)
	 * But here, we MUST check BTF (like TC in UART)
	 * Because we will now change the Direction of Data Flow
	 * (Master sent the request, now it's time for Slave to talk)
	 */

	/*
	 * ------------------------------
	 * Master [READ] from Slave (Receiver)
	 * ------------------------------
	 */

	/*
	 * ==============================
	 * 		1. Repeated Start
	 * ==============================
	 * Same logic as when [Write]
	 */
	I2C1->CR1 |= (1u << 8);
	while ( (I2C1->SR1 & (1u << 0)) == 0 ); // Wait for StartBit

	/*
	 * ==============================
	 * 		2. Send Address Again
	 * ==============================
	 * only this time, Bit 0 is 1 to indicate [READ]
	 */
	I2C1->DR = ( (BMP280_ADDR << 1u) | 1u);

	/*
	 * ==============================
	 *     Step 3. ADDR
	 * ==============================
	 */
	while ( (I2C1->SR1 & (1u << 1)) == 0 ); // Wait for ADDR to become 1 (address matched)

	/*
	 * Step A:
	 * Disabling ACK immediately because here I only want to [Read 1 Byte]
	 *
	 * Logically, this equates to a NACK (Negative Acknowledgment)
	 *
	 * Scenario:
	 * STM32 stops checking in with BMP280 (by disabling ACK)
	 * BMP280 no longer sends data to STM32
	 * But I already claimed the byte I want, which contains BMP280's "id"
	 */
	I2C1->CR1 &= ~(1u << 10); // Disable ACK

	/* Step B: Clear ADDR */
	READ_ADDR = I2C1->SR1; // Read SR1->ADDR
	READ_ADDR = I2C1->SR2; // Immediately read SR2
	(void)READ_ADDR; // to get rid of compiler warning

	/*
	 * ==============================
	 *     Step C: STOP
	 * ==============================
	 * CR1 -> Bit 9 STOP: Stop generation
	 * In Master Mode:
	 * 0: No Stop generation.
	 * 1: Stop generation after the current byte transfer
	 * 	  or after the current Start condition is sent.
	 */
	I2C1->CR1 |= (1u << 9);

	/*
	 * ==============================
	 * Step 4. Wait for Data (RXNE)
	 * ==============================
	 * We check the RXNE (Read Data Register Not Empty) flag in the Status Register (SR).
	 * Bit 6 RxNE: Data register not empty (receivers)
	 * = 0: " Data register empty" -> No data received yet.
	 * = 1: "Data register not empty" -> Data has arrived and is ready to be read.
	 */
	while ( (I2C1->SR1 & (1u << 6)) == 0 ); // Wait until it becomes 1

	/*
	 * ==============================
	 * Step 5. now READ into Buffer
	 * ==============================
	 */
	data = I2C1->DR;

	return data;
}

int main(void){
	I2C1_setup(); // GPIOs
	I2C1_Init();  // I2C Config

	while (1){
		volatile uint8_t id = BMP280_ReadID();
		(void)id; // to get rid of compiler warning
	}
}
