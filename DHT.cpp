/*------------------------------------------------------------------------------
	DHT22 CPP temperature & humidity sensor AM2302 (DHT22) driver for ESP32
	Jun 2017:	Ricardo Timmermann, new for DHT22

	This example code is in the Public Domain (or CC0 licensed, at your option.)
	Unless required by applicable law or agreed to in writing, this
	software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
	CONDITIONS OF ANY KIND, either express or implied.
	PLEASE KEEP THIS CODE IN LESS THAN 0XFF LINES. EACH LINE MAY CONTAIN ONE BUG !!!
---------------------------------------------------------------------------------*/

#include <driver/gpio.h>
#include <cstdint>
#include <esp_log.h>
#include <array>

#include "include/DHT.h"

static char TAG[] = "DHT";

/*-----------------------------------------------------------------------
;
;	Constructor & default settings
;
;------------------------------------------------------------------------*/

DHT::DHT(gpio_num_t gpioPin)
{
    DHTgpio_ = gpioPin;
	humidity_ = 0.;
	temperature_ = 0.;

}

// == error handler ===============================================

void DHT::ErrorHandler(int response)
{
	switch(response) {
		case DHT_TIMEOUT_ERROR :
			ESP_LOGE( TAG, "Sensor Timeout\n" );
			break;

		case DHT_CHECKSUM_ERROR:
			ESP_LOGE( TAG, "CheckSum error\n" );
			break;

		case DHT_OK:
			break;

		default :
			ESP_LOGE( TAG, "Unknown error\n" );
	}
}

/*-------------------------------------------------------------------------------
;
;	get next state
;
;	I don't like this logic. It needs some interrupt blocking / priority
;	to ensure it runs in realtime.
;
;--------------------------------------------------------------------------------*/

int DHT::GetSignalLevel( int usTimeOut, bool state )
{

	int uSec = 0;
	while( gpio_get_level(DHTgpio_)==state ) {

		if( uSec > usTimeOut )
			return -1;

		++uSec;
		ets_delay_us(1);		// uSec delay
	}

	return uSec;
}

/*----------------------------------------------------------------------------
;
;	read DHT22 sensor

	copy/paste from AM2302/DHT22 Docu:
	DATA: Hum = 16 bits, Temp = 16 Bits, check-sum = 8 Bits
	Example: MCU has received 40 bits data from AM2302 as
	0000 0010 1000 1100 0000 0001 0101 1111 1110 1110
	16 bits RH data + 16 bits T data + check sum

	1) we convert 16 bits RH data from binary system to decimal system, 0000 0010 1000 1100 → 652
	Binary system Decimal system: RH=652/10=65.2%RH
	2) we convert 16 bits T data from binary system to decimal system, 0000 0001 0101 1111 → 351
	Binary system Decimal system: T=351/10=35.1°C
	When highest bit of temperature is 1, it means the temperature is below 0 degree Celsius.
	Example: 1000 0000 0110 0101, T= minus 10.1°C: 16 bits T data
	3) Check Sum=0000 0010+1000 1100+0000 0001+0101 1111=1110 1110 Check-sum=the last 8 bits of Sum=11101110

	Signal & Timings:
	The interval of whole process must be beyond 2 seconds.

	To request data from DHT:
	1) Sent low pulse for > 1~10 ms (MILI SEC)
	2) Sent high pulse for > 20~40 us (Micros).
	3) When DHT detects the start signal, it will pull low the bus 80us as response signal,
	   then the DHT pulls up 80us for preparation to send data.
	4) When DHT is sending data to MCU, every bit's transmission begin with low-voltage-level that last 50us,
	   the following high-voltage-level signal's length decide the bit is "1" or "0".
		0: 26~28 us
		1: 70 us
;----------------------------------------------------------------------------*/


int DHT::ReadDHT()
{
    int uSec;
    std::array<std::uint8_t, MAXdhtData> dhtData{};
    std::uint8_t byteInx = 0;
    std::uint8_t bitInx = 7;


	// == Send start signal to DHT sensor ===========

	gpio_set_direction(DHTgpio_, GPIO_MODE_OUTPUT );

	// pull down for 3 ms for a smooth and nice wake up
	gpio_set_level(DHTgpio_, 0 );
	ets_delay_us( 3000 );

	// pull up for 25 us for a gentile asking for data
	gpio_set_level(DHTgpio_, 1 );
	ets_delay_us( 25 );

	gpio_set_direction(DHTgpio_, GPIO_MODE_INPUT );		// change to input mode

	// == DHT will keep the line low for 80 us and then high for 80us ====

	uSec = GetSignalLevel(85, false);
//	ESP_LOGI( TAG, "Response = %d", uSec );
	if( uSec<0 ) return DHT_TIMEOUT_ERROR;

	// -- 80us up ------------------------

	uSec = GetSignalLevel(85, true);
//	ESP_LOGI( TAG, "Response = %d", uSec );
	if( uSec<0 ) return DHT_TIMEOUT_ERROR;

	// == No errors, read the 40 data bits ================

	for( int k = 0; k < 40; k++ ) {

		// -- starts new data transmission with >50us low signal

		uSec = GetSignalLevel(56, false);
		if( uSec<0 ) return DHT_TIMEOUT_ERROR;

		// -- check to see if after >70us rx data is a 0 or a 1

		uSec = GetSignalLevel(75, true);
		if( uSec<0 ) return DHT_TIMEOUT_ERROR;

		// add the current read to the output data
		// since all dhtData array where set to 0 at the start,
		// only look for "1" (>28us us)

		if (uSec > 40) {
			dhtData[ byteInx ] |= (1u << bitInx);
			}

		// index to next byte

		if (bitInx == 0) { bitInx = 7; ++byteInx; }
		else bitInx--;
	}

	// == get humidity from Data[0] and Data[1] ==========================
    std::uint16_t humidity = dhtData[0];
	humidity *= 0x100;					// >> 8
	humidity += dhtData[1];
	humidity_ = static_cast<float>(humidity) / 10;						// get the decimal

	// == get temp from Data[2] and Data[3]

    std::uint16_t temperature = dhtData[2] & 0x7Fu;
	temperature *= 0x100;				// >> 8
	temperature += dhtData[3];
	temperature_ = static_cast<float>(temperature) / 10;

	if( dhtData[2] & 0x80u ) 			// negative temp, brrr it's freezing
		temperature_ *= -1;


	// == verify if checksum is ok ===========================================
	// Checksum is the sum of Data 8 bits masked out 0xFF
    return VerifyCrc(dhtData);
}

int DHT::VerifyCrc(std::array<std::uint8_t, 5> verifyData) {
    std::uint16_t data = verifyData[0] + verifyData[1] + verifyData[2] + verifyData[3];
    if (verifyData[4] == (data & 0xFFu)){
        return DHT_OK;
    }
    else{
        return DHT_CHECKSUM_ERROR;
    }
}
