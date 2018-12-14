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

#ifndef __AUDIO_VARIO_H__
#define __AUDIO_VARIO_H__

#include <AP_Common/AP_Common.h>
#include <AP_Notify/Buzzer.h>
#include <AudioVario/AudioVario_PX4.h>

class AudioVario
{
private:

    // vertical speed saturation limits (cm/s)
    int16_t vertical_speed_lower, vertical_speed_upper;

    // vertical speed dead zone limits (cm/s)
    int16_t deadzone_lower, deadzone_upper;

    // pitch limits
    // notes are numbered 1-84, corresponding to pitches C0-B6
    const uint8_t min_note=10, zero_note=37, max_note=68;

    // limits on beep duration (ms)
    const uint16_t min_beep_duration=145, max_beep_duration=600;

    // vertical speed (energy compensated) in cm/s
    int16_t vertical_speed;

    // current pitch
    // notes are numbered 1-84, corresponding to pitches C0-B6
    uint8_t current_note;

    // length of beep+silence in ms
    uint16_t current_beep_duration;

    // timestamp when the current beep started (ms)
    uint32_t current_beep_start;

    // map vario to pitch and beep duration
	void vario_to_beep_params(int16_t vspeed);

public:

    /* Handy note: 1 cm/sec ~= 2ft/min */

	// constructor
	AudioVario() : vertical_speed_lower(-500), vertical_speed_upper(500),
				   deadzone_lower(-250), deadzone_upper(5),
				   vertical_speed(0),
				   current_beep_duration(0),
				   current_beep_start(0) {}

	// initialisation
    void init(void);

    // main audio update function, called at 50Hz
    void update(int16_t vspeed);

    // getter/setter functions
    void set_vario_limit(int16_t minv, int16_t maxv);
    int16_t get_vario_limit_lower(){return vertical_speed_lower;}
    int16_t get_vario_limit_upper(){return vertical_speed_upper;}
    void set_dead_zone(int16_t mind, int16_t maxd);
    int16_t get_dead_zone_lower(){return deadzone_lower;}
    int16_t get_dead_zone_upper(){return deadzone_upper;}


public: /// !!!!
#if CONFIG_HAL_BOARD == HAL_BOARD_PX4 || CONFIG_HAL_BOARD == HAL_BOARD_VRBRAIN
	AudioVario_PX4 vario;
#endif
};

#endif    // __AUDIO_VARIO_H__
