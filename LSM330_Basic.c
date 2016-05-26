/* Program for USB communication based on LUFA stack by Dean Camara.

MCU: Atmega32u2, 8 MHz

Pero, May 2016
*/

#include "VirtualSerial.h"
#include <util/delay.h>

/* SPI ports are first 4 bits of PORTB (PB0..3) */
#define SS 0
#define SCK 1
#define MOSI 2
#define MISO 3

static uint16_t room_temp = 0;

/* LUFA CDC Class driver interface configuration and state information.  */
USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = INTERFACE_ID_CDC_CCI,
				.DataINEndpoint           =
					{
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

// Standard file stream for the CDC interface 
static FILE USBSerialStream;

uint16_t read_sensor(void) {
/* MAX31855 Data Bits (32):
 * 31 : sign,
 * 30 - 18 : thermocouple temperature,
 * 17 : reserved(0),
 * 16 : 1 if fault,
 * 15 - 4 : cold junction temperature,
 * 3 : reserved(0),
 * 2 : 1 if thermocouple is shorted to Vcc,
 * 1 : 1 if thermocouple is shorted to ground,
 * 0 : 1 if thermocouple is open circuit */

    uint8_t sensor[4];  // MAX31855 delivers 32 bits while SPDR is only 8 bits long. That's why we'll read with the uint8_t array
    uint16_t temp;		// MAX31855 delivers delivers temperature in 14 bits
    int8_t i;
    /* SS = 0 */
    PORTB = (0<<SS);   // Enable Slave

    /* Read the 32 bits from MAX31855 */
    for(i=0;i<4;i++) {
        SPDR = 0x00;  //Writing to the register initiates data transmission 
        /* Wait for transmission to complete. SPIF is cleared by hardware when executing the
		   corresponding interrupt handling vector. Alternatively, the SPIF bit is cleared by first reading the
		   SPI Status Register with SPIF set, then accessing the SPI Data Register (SPDR). */
        while (!(SPSR & (1<<SPIF)));
        sensor[i] = SPDR;
    }

    /* Thermocouple temperature */
    if (sensor[0]&(1<<7)) {
        /* Negative temperature, clamp it to zero */
        temp = 0;
    } else {
        temp = (((uint16_t)sensor[0])<<6)+(sensor[1]>>2);  // convert 8 bit to 16 bit
    }

    /* Room temperature */
    if (sensor[2]&(1<<7)) {
        /* Negative temperature, clamp it to zero */
        room_temp = 0;
    } else {
        room_temp = (((uint16_t)sensor[2])<<4)+(sensor[3]>>4);
        room_temp = (room_temp >> 4); // Last four bits of 10bit temp data are 0.5, 0.25, 0.125 and 0.0625 of the celsius. I want only integer number
    }
	/* If tehre are any errors with thermocouple  */
    if (sensor[1]&0x01) {
        /* Fault */
        fprintf(&USBSerialStream,"Fault:%u\n",sensor[3]&0b00000111);
    }

    /* Disable slave */
    PORTB = (1<<SS);
    return (temp>>2);  // Last two bits of 14bit temp data are 0.5 and 0.25 of the celsius. I want only integer number
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
	/* Set !SS and SCK output, all others input */
	DDRB = (1<<SS)|(1<<SCK);
	//PORTB &= (1<<SCK);	/* Disable SCK */
	PORTB |= (1<<SS); /* Set !SS high (slave not enabled) */

	/* Hardware Initialization */
	USB_Init();
}

int main(void)
{
	char* HelloString = "Pero je opet jebac najveci! \n";
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
		if(count >=2000)
		{
			count=0;
			fprintf(&USBSerialStream, "%s", HelloString);
			_delay_ms(1);
		}


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

