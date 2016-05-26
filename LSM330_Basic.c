/* Program for USB communication based on LUFA stack by Dean Camara.

MCU: Atmega32u2, 8 MHz

Pero, May 2016
*/

#include "VirtualSerial.h"
#include "LSM330.h"
#include <util/delay.h>

/* SPI ports are first 4 bits of PORTB (PB0..3) */
#define SS_A 0
#define SS_G 4
#define SCK 1
#define MOSI 2
#define MISO 3

// Standard file stream for the CDC interface 
static FILE USBSerialStream;

uint16_t LSM330_ReadRegister(void){
/* Reading a single register from LSM330. Data is clocked in 16 cycles.
Bits 15-8: Register content
Bits  7-2: Register address
Bit     1: M/_S, increment address in multiple readings/writings.
Bit     0: R/_W, read/write mode     
*/
	
	uint8_t dataIn;   // data from the SPI device
	uint8_t i;
	uint8_t address = CTRL1_REG1_A;	
	uint8_t mode = 0b10;   // bit 1: read/_write, bit 0: auto increment address in multiple readings/writings
	uint8_t dataOut = (uint16_t)((mode << 6) | address); //This goes on MOSI
	fprintf(&USBSerialStream, "Address: %u\n", dataOut);
	
	/* SS = 0 */
	PORTB = (0 << SS_A);   // Enable Slave
	
	SPDR = dataOut;			 // Writing to SPDR initiates data transmission
	while (!(SPSR & (1 << SPIF)));			// Wait till the data is written to SPDR, SPIF will be high
	
	for (i = 0; i < 3; i++){
		SPDR = 0x00;
		while (!(SPSR & (1 << SPIF)));			// Wait till the data is written to SPDR, SPIF will be high
		dataIn = SPDR;					        //  Read MISO
		fprintf(&USBSerialStream, "CTRL1 Register: %u\n", dataIn);
	}
		/* Disable slave */
	PORTB = (1 << SS_A); 
	return 0;
}


void SetupHardware(void)
{
	/* Disable watchdog if enabled by bootloader/fuses */
	MCUSR &= ~(1 << WDRF);
	wdt_disable();

	/* Disable clock division */
	clock_prescale_set(clock_div_1);	
	
	/****** SPI Setup *******/
	/* Enable SPI, Master, set clock rate fck/2 */
	SPCR = (1<<SPE) | (1<<MSTR);  // Last two bits in SPCR set the SPI clock. When 00 it's f_clk/4.
	//SPSR = (1<<SPI2X);  // SPI clock is 2 x f_clk/4 = f_clk/2.	
	DDRB = (1<<SS_A)|(1<<SS_G)|(1<<SCK);  /* Set !SS and SCK output, all others input */
	PORTB |= (1<<SS_A)|(1<<SS_G); /* Set !SS high (slave not enabled) */

	/* Hardware Initialization */
	USB_Init();
}

int main(void)
{
	uint16_t count = 0;
	SetupHardware();

	/* Create a regular character stream for the interface so that it can be used with the stdio.h functions */
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	sei();

	for (;;)
	{
		/*The following block of code demonstrates a simple print that is
		sent about once each 2 seconds. */
		
		count++;
		if(count >=1000)
		{
			count=0;
			//fprintf(&USBSerialStream, "Pero testira\n");
			LSM330_ReadRegister();
			
		}
		_delay_ms(1);


		//Needed for LUFA
		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
	}
}

/** Event handler for the library USB Connection event. */
void EVENT_USB_Device_Connect(void)
{
	;
}

/** Event handler for the library USB Disconnection event. */
void EVENT_USB_Device_Disconnect(void)
{
	;
}

/** Event handler for the library USB Configuration Changed event. */
void EVENT_USB_Device_ConfigurationChanged(void)
{
	CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);
	
}

/** Event handler for the library USB Control Request reception event. */
void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

