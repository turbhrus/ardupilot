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
#pragma once

/*
  backend driver for humidity from a Sensirion I2C SHT-31 sensor
 */

#include <AP_HAL/AP_HAL.h>
#include <AP_HAL/utility/OwnPtr.h>
#include <AP_HAL/I2CDevice.h>


class Humidity_SHT31
{
public:
    Humidity_SHT31() : _measuring(false),
					   _last_sample_time_ms(0),
					   _measurement_started_ms(0){}
    bool init(uint16_t update_period_ms);
    bool get_humidity(float &humidity);
    bool get_temperature(float &temperature);

private:
    void _measure();
    bool _collect();
    void _timer();
    bool _reset();
    bool _send_sht31_cmd(uint16_t cmd);
	uint8_t crc8(const uint8_t *data, int len);
	bool _measuring;
    float _temperature;
    float _humidity;
    uint32_t _last_sample_time_ms;
    uint32_t _measurement_started_ms;
	uint16_t _update_period_ms;
    AP_HAL::OwnPtr<AP_HAL::I2CDevice> _dev;
};
