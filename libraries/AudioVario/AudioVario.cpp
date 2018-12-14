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

#include <AP_Math/AP_Math.h>
#include <AudioVario/AudioVario.h>
#include <AP_HAL/AP_HAL.h>

extern const AP_HAL::HAL& hal;

// initialisation
void AudioVario::init()
{
	vario.init();
}

// main update function, called at 50Hz
void AudioVario::update(int16_t vspeed)
{
	bool update_audio = false;

	// don't sound vario if an alarm is playing
	if( vario.alarm_sounding()){
		return;
	}

	// Check for dead zone.
	if( vspeed < deadzone_lower || vspeed > deadzone_upper){

		// If vario has changed sufficiently we want to know about it immediately, even if it
		// means rudely interrupting the current beep.
		if( vspeed > 0){
			if( (vspeed > MAX(1.2*(float)vertical_speed, (float)(vertical_speed+50))) ||
				(vspeed < MIN(.83*(float)vertical_speed, (float)(vertical_speed-50)))){
				update_audio = true;
			}
		}
		// If old beep has finished then start the next one
		if( (AP_HAL::millis()-current_beep_start) > current_beep_duration){
			update_audio = true;
		}
	}

	// start the beep
	if(update_audio){
		vario_to_beep_params(vspeed);
		current_beep_start = vario.beep(current_note, current_beep_duration, vspeed>=0);
		vertical_speed = vspeed;
	}
}

// map vario to audio signal
void AudioVario::vario_to_beep_params(int16_t _vspeed)
{
	uint8_t note;
	int16_t vspeed;

	vspeed = constrain_int16(_vspeed, vertical_speed_lower, vertical_speed_upper);

	// map vario to pitch
	if(vspeed > 0){
		note = ((vspeed-deadzone_upper)*(max_note-zero_note)) /
					(vertical_speed_upper-deadzone_upper) + zero_note;
	}else{
		note = ((vspeed-vertical_speed_lower)*(zero_note-min_note)) /
					(deadzone_lower-vertical_speed_lower) + min_note;
	}
	current_note = constrain_uint8( note, min_note, max_note);

	// map vario to duration
	if(vspeed >= 0){
		current_beep_duration = min_beep_duration +
				(vertical_speed_upper - vspeed) * (max_beep_duration-min_beep_duration) /
				vertical_speed_upper;
	}else{
		current_beep_duration = 800;
	}
}

void AudioVario::set_vario_limit(int16_t minv, int16_t maxv)
{
	if(minv < deadzone_lower){
		vertical_speed_lower = minv;
	}
	if(maxv > deadzone_upper){
		vertical_speed_upper = maxv;
	}
}

void AudioVario::set_dead_zone(int16_t mind, int16_t maxd)
{
	if(mind <= 0 && mind > vertical_speed_lower){
		deadzone_lower = mind;
	}
	if(maxd >= 0 && maxd < vertical_speed_upper){
		deadzone_upper = maxd;
	}
}

