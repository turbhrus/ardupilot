/*
  ToneAlarm PX4 driver
*/
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

#include <AP_HAL/AP_HAL.h>

#if CONFIG_HAL_BOARD == HAL_BOARD_PX4
#include "AudioVario_PX4.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <drivers/drv_tone_alarm.h>
#include <stdio.h>
#include <errno.h>

extern const AP_HAL::HAL& hal;

const char AudioVario_PX4::tone_names[12][3] =
	{"C","C#","D","D#","E","F","F#","G","G#","A","A#","B"};

const AudioVario_PX4::alarm_params AudioVario_PX4::_alarms[] {
    { AUDIO_ALARM_NULL,              0, "",                             0 },
    { AUDIO_ALARM_WAYPOINT_REACHED, 20, "MFT250L4O3AL16BCDEFG>ABCDEFL4G", 900 },
    { AUDIO_ALARM_AIRSPACE_WARNING, 30, "MFT100L16>B<C>B<C>B<C>B",   1050 },
    { AUDIO_ALARM_TERRAIN_WARNING,  40, "MFT200L2A-G-A-G-A-G-",       500 },
};


bool AudioVario_PX4::init()
{
	active_alarm = false;

    // open the tone alarm device
    _audiovario_fd = open(TONEALARM0_DEVICE_PATH, O_WRONLY);
    if (_audiovario_fd == -1) {
        hal.console->printf("AudioVario_PX4: Unable to open " TONEALARM0_DEVICE_PATH);
        return false;
    }
    _cont_tone_playing = -1;
    return true;
}

// start a new beep
uint32_t AudioVario_PX4::beep(uint8_t note, uint16_t duration, bool pos){
	char tone_str[3], beep_str[53], tmp_str[9];
	static uint8_t aeolian[8] = {12,10,8,7,5,3,2,0};

	// clear anything currently playing
	play_string("stop");

	if(pos){
		// create beep command
		tone_name(note, tone_str);
		// The initial quick blip is to remove the latency associated with low tempos.
		sprintf( beep_str, "MFMLT255L64O%dN%sT%dL32O%dN%s", octave(note), tone_str, tempo(duration), octave(note), tone_str);

	}else{
		// create sink tune
		sprintf( beep_str, "MFMLT255L64");
		for( int i=0; i<8; ++i){
			tone_name(note+aeolian[i], tone_str);
			sprintf(tmp_str,"O%dN%s", octave(note+aeolian[i]), tone_str);
			strcat(beep_str, tmp_str);
		}
		strcat(beep_str, ".");
	}
	// play
//	uint32_t beep_start_time = hal.scheduler->millis();
	uint32_t beep_start_time = AP_HAL::millis();
	play_string( beep_str);

	return beep_start_time;
}

// map note to letter as a string: "C","C#",...,"B"
void AudioVario_PX4::tone_name(uint8_t note, char* str){
	strcpy( str, tone_names[(note-1)%12]);
}

// set audio tempo to control beep length
uint8_t AudioVario_PX4::tempo( uint16_t duration) {
	// calculation: 3/2*60/4*1000 = 22500
	return (uint8_t)((22500)/duration);
}

// octave: 0-6
uint8_t AudioVario_PX4::octave(uint8_t note){
	return (note-1)/12;
}

// play_tune
void AudioVario_PX4::play_tone(const uint8_t tone_index)
{
}

void AudioVario_PX4::play_string(const char *str) {
    write(_audiovario_fd, str, strlen(str) + 1);
}

void AudioVario_PX4::stop_cont_tone() {
    if(_cont_tone_playing == _tone_playing) {
        play_string("stop");
        _tone_playing = -1;
    }
    _cont_tone_playing = -1;
}

/******************************
 * Alarm related
 ******************************/

void AudioVario_PX4::trigger_alarm( uint8_t alarm_id){

	bool start_new_alarm = false;

	if( active_alarm == false){
		// free to start the alarm
		start_new_alarm = true;
	}else{
		if( _alarms[alarm_id].priority > _alarms[active_alarm_id].priority){
			// this one is more important
			start_new_alarm = true;
		}
	}
	if( start_new_alarm){
		active_alarm_id = alarm_id;
		active_alarm = true;

		// clear anything currently playing
		play_string("stop");

		// play
		play_string( _alarms[active_alarm_id].str);
		alarm_start_time_ms = AP_HAL::millis();
	}
}

bool AudioVario_PX4::alarm_sounding(){

	if( active_alarm){
		// check for timeout
		if( AP_HAL::millis() - alarm_start_time_ms > _alarms[active_alarm_id].duration_ms){
			active_alarm = false;
		}
	}
	return active_alarm;
}

//const ToneAlarm_PX4::Tone ToneAlarm_PX4::_tones[] {
//    #define AP_NOTIFY_PX4_TONE_QUIET_NEG_FEEDBACK 0
//    { "MFT200L4<<<B#A#2", false },
//    #define AP_NOTIFY_PX4_TONE_LOUD_NEG_FEEDBACK 1
//    { "MFT100L4>B#A#2P8B#A#2", false },
//    #define AP_NOTIFY_PX4_TONE_QUIET_NEU_FEEDBACK 2
//    { "MFT200L4<B#", false },
//    #define AP_NOTIFY_PX4_TONE_LOUD_NEU_FEEDBACK 3
//    { "MFT100L4>B#", false },
//    #define AP_NOTIFY_PX4_TONE_QUIET_POS_FEEDBACK 4
//    { "MFT200L4<A#B#", false },
//    #define AP_NOTIFY_PX4_TONE_LOUD_POS_FEEDBACK 5
//    { "MFT100L4>A#B#", false },
//    #define AP_NOTIFY_PX4_TONE_LOUD_READY_OR_FINISHED 6
//    { "MFT100L4>G#6A#6B#4", false },
//    #define AP_NOTIFY_PX4_TONE_QUIET_READY_OR_FINISHED 7
//    { "MFT200L4<G#6A#6B#4", false },
//    #define AP_NOTIFY_PX4_TONE_LOUD_ATTENTION_NEEDED 8
//    { "MFT100L4>B#B#B#B#", false },
//    #define AP_NOTIFY_PX4_TONE_QUIET_ARMING_WARNING 9
//    { "MNT75L1O2G", false },
//    #define AP_NOTIFY_PX4_TONE_LOUD_WP_COMPLETE 10
//    { "MFT200L8G>C3", false },
//    #define AP_NOTIFY_PX4_TONE_LOUD_LAND_WARNING_CTS 11
//    { "MBT200L2A-G-A-G-A-G-", true },
//    #define AP_NOTIFY_PX4_TONE_LOUD_LOST_COPTER_CTS 12
//    { "MBT200>B#1", true },
//    #define AP_NOTIFY_PX4_TONE_LOUD_BATTERY_ALERT_CTS 13
//    { "MBNT255>B#8B#8B#8B#8B#8B#8B#8B#8B#8B#8B#8B#8B#8B#8B#8B#8", true },
//};


#endif // CONFIG_HAL_BOARD == HAL_BOARD_PX4
