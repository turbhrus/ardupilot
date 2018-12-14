/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-

/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// Adapted from...
/***************************************************
  This is a library for the SHT31 Digital Humidity & Temp Sensor

  Designed specifically to work with the SHT31 Digital sensor from Adafruit
  ----> https://www.adafruit.com/products/2857

  These displays use I2C to communicate, 2 pins are required to
  interface
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  BSD license, all text above must be included in any redistribution
 ****************************************************/


/*
  backend driver for humidity from a Sensirion I2C SHT-31 sensor
 */
 
#include "Humidity_SHT31.h"

#define SHT31_DEFAULT_ADDR    0x44
#define SHT31_MEAS_HIGHREP_STRETCH 0x2C06
#define SHT31_MEAS_MEDREP_STRETCH  0x2C0D
#define SHT31_MEAS_LOWREP_STRETCH  0x2C10
#define SHT31_MEAS_HIGHREP         0x2400
#define SHT31_MEAS_MEDREP          0x240B
#define SHT31_MEAS_LOWREP          0x2416
#define SHT31_READSTATUS           0xF32D
#define SHT31_CLEARSTATUS          0x3041
#define SHT31_SOFTRESET            0x30A2
#define SHT31_HEATEREN             0x306D
#define SHT31_HEATERDIS            0x3066

#define SHT31_I2C_BUS        1  

extern const AP_HAL::HAL &hal;


/////////////////////////////////////////////////////////////////////
// probe and initialise the sensor
bool Humidity_SHT31::init(uint16_t update_period_ms)
{
    _dev = hal.i2c_mgr->get_device(SHT31_I2C_BUS, SHT31_DEFAULT_ADDR);

    // take i2c bus sempahore
    if (!_dev || !_dev->get_semaphore()->take(200)) {
        return false;
    }

    if (!_reset()){
    	return false;
    }
    hal.scheduler->delay(5);
    _measure();
    hal.scheduler->delay(20);
	_collect();
    _dev->get_semaphore()->give();

    if (_last_sample_time_ms == 0) {
        return false;
    }
	
	// time between measurements
	_update_period_ms = (update_period_ms > 25) ? update_period_ms : 25;

	// 1kHz timer
    hal.scheduler->register_timer_process(FUNCTOR_BIND_MEMBER(&Humidity_SHT31::_timer, void));

    return true;
}

/////////////////////////////////////////////////////////////////////
// start a measurement
void Humidity_SHT31::_measure()
{
	if (_send_sht31_cmd(SHT31_MEAS_HIGHREP)) {
		_measurement_started_ms = AP_HAL::millis();
		_measuring = true;
    }
}

/////////////////////////////////////////////////////////////////////
// read values from the sensor
bool Humidity_SHT31::_collect()
{
    uint8_t data[6];

    if (!_dev->transfer(nullptr, 0, data, sizeof(data))) {
        return false;
    }

    // check data integrity
    if (data[2] != crc8(data, 2) || data[5] != crc8(data+3, 2)){
    	return false;
    }

    // compute temperature according to datasheet (deg C)
    uint16_t ST = (data[0] << 8) | data[1];
    _temperature = 0.0026703288f * (float)ST - 45.0;

    // compute humidity according to datasheet (%)
    uint16_t SRH = (data[3] << 8) | data[4];
    _humidity = 0.0015259022f * (float)SRH;

	_last_sample_time_ms = AP_HAL::millis();
	_measuring = false;
	return true;
}

/////////////////////////////////////////////////////////////////////
// 1kHz timer
void Humidity_SHT31::_timer()
{
    if (!_dev->get_semaphore()->take_nonblocking()) {
        return;
    }

    if (!_measuring) {
    	if( (AP_HAL::millis() - _measurement_started_ms) >= _update_period_ms){
			// start a new measurement
			_measure();
    	}
        _dev->get_semaphore()->give();
        return;
    }

    if ((AP_HAL::millis() - _measurement_started_ms) > 20) {
		// get measurement results
    	if (_collect()){
			_dev->get_semaphore()->give();
			return;
    	}

    	if ((AP_HAL::millis() - _measurement_started_ms) > 2*_update_period_ms) {
    		// something went wrong - try reset
    		if (_reset()){
    			_measurement_started_ms = AP_HAL::millis() - _update_period_ms + 10;
    			_measuring = false;
    		}
    	}
    }
	_dev->get_semaphore()->give();
}

/////////////////////////////////////////////////////////////////////
// current relative humidity if available (%)
bool Humidity_SHT31::get_humidity(float &humidity)
{
    if ((AP_HAL::millis() - _last_sample_time_ms) <= _update_period_ms) {
		humidity = _humidity;
		return true;
    }
	return false;
}

/////////////////////////////////////////////////////////////////////
// current temperature if available (deg C)
bool Humidity_SHT31::get_temperature(float &temperature)
{
    if ((AP_HAL::millis() - _last_sample_time_ms) <= _update_period_ms) {
		temperature = _temperature;
		return true;
    }
	return false;
}

/////////////////////////////////////////////////////////////////////
// utility function to send a 2-byte command
bool Humidity_SHT31::_send_sht31_cmd(uint16_t cmd)
{
	uint8_t buf[2];

	buf[0] = cmd >> 8;
    buf[1] = cmd & 0xff;
    if (_dev->transfer(buf, 2, nullptr, 0)) {
        return true;
    }
	return false;
}

/////////////////////////////////////////////////////////////////////
// reset chip state
bool Humidity_SHT31::_reset(){

	if (_send_sht31_cmd(SHT31_SOFTRESET)) {
        return true;
    }
    return false;
}

/////////////////////////////////////////////////////////////////////
// CRC check
uint8_t Humidity_SHT31::crc8(const uint8_t *data, int len)
{
/*
 * CRC-8 formula from page 14 of SHT spec pdf
 *
 * Test data 0xBE, 0xEF should yield 0x92
 *
 * Initialization data 0xFF
 * Polynomial 0x31 (x8 + x5 +x4 +1)
 * Final XOR 0x00
 */

  const uint8_t POLYNOMIAL(0x31);
  uint8_t crc(0xFF);

  for ( int j = len; j; --j ) {
      crc ^= *data++;

      for ( int i = 8; i; --i ) {
	crc = ( crc & 0x80 )
	  ? (crc << 1) ^ POLYNOMIAL
	  : (crc << 1);
      }
  }
  return crc;
}



//uint16_t Adafruit_SHT31::readStatus(void) {
//  writeCommand(SHT31_READSTATUS);
//  Wire.requestFrom(_i2caddr, (uint8_t)3);
//  uint16_t stat = Wire.read();
//  stat <<= 8;
//  stat |= Wire.read();
//  //Serial.println(stat, HEX);
//  return stat;
//}
//
//
//void Adafruit_SHT31::heater(boolean h) {
//  if (h)
//    writeCommand(SHT31_HEATEREN);
//  else
//    writeCommand(SHT31_HEATERDIS);
//}
//
//
//boolean Adafruit_SHT31::readTempHum(void) {
//  uint8_t readbuffer[6];
//
//  writeCommand(SHT31_MEAS_HIGHREP);
//
//  delay(500);
//  Wire.requestFrom(_i2caddr, (uint8_t)6);
//  if (Wire.available() != 6)
//    return false;
//  for (uint8_t i=0; i<6; i++) {
//    readbuffer[i] = Wire.read();
//  //  Serial.print("0x"); Serial.println(readbuffer[i], HEX);
//  }
//  uint16_t ST, SRH;
//  ST = readbuffer[0];
//  ST <<= 8;
//  ST |= readbuffer[1];
//
//  if (readbuffer[2] != crc8(readbuffer, 2)) return false;
//
//  SRH = readbuffer[3];
//  SRH <<= 8;
//  SRH |= readbuffer[4];
//
//  if (readbuffer[5] != crc8(readbuffer+3, 2)) return false;
//
// // Serial.print("ST = "); Serial.println(ST);
//  double stemp = ST;
//  stemp *= 175;
//  stemp /= 0xffff;
//  stemp = -45 + stemp;
//  temp = stemp;
//
////  Serial.print("SRH = "); Serial.println(SRH);
//  double shum = SRH;
//  shum *= 100;
//  shum /= 0xFFFF;
//
//  humidity = shum;
//
//  return true;
//}
