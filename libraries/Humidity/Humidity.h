/// -*- tab-width: 4; Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*-
#pragma once

#include "Humidity_SHT31.h"


class Humidity
{
public:
	Humidity(){}

    bool init();

    // get relative humidity if available (%)
    bool get_humidity(float &humidity);

    // get temperature if available (deg C)
    bool get_temperature(float &temperature);

private:
    float           _humidity;
    float           _temperature;

#if CONFIG_HAL_BOARD == HAL_BOARD_PX4 || CONFIG_HAL_BOARD == HAL_BOARD_VRBRAIN
    Humidity_SHT31    sensor;
#endif
};
