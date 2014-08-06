
/*
 * Copyright (C) Jacob Budin
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <bass.h>
#include <wiringPi.h>
#include <lcd.h>
#include "parson.h"


#define PTN_DIAL_PIN1 4
#define PTN_DIAL_PIN2 5


HSTREAM chan;


struct ptn_station{
    char *name;
    char *url;
    struct ptn_station *next;
    struct ptn_station *prev;
};


struct ptn_display{
    char *line1;
    char *line2;
};


struct ptn_station  *ptn_current_station;


int ptn_display_fd = -1;


int ptn_p1_val = -1;
int ptn_p2_val = -1;


void
ptn_error(const char *error)
{
    printf("%s\n", error);
    exit(1);
}


struct ptn_display
ptn_get_stream_info()
{
    struct ptn_display info = {"Hello", "World"};
    return info;
}


void
ptn_update_display(struct ptn_display *info)
{
    lcdPrintf(ptn_display_fd, "%s\n%s", info->line1, info->line2);
}


void
ptn_read_config()
{
    JSON_Value *root_value;
    JSON_Array *stations;
    JSON_Object *station;
    int i;
    struct ptn_station *s;
    struct ptn_station *s_prev = NULL;

    root_value = json_parse_file("stations.json");
    if (json_value_get_type(root_value) != JSONArray) {
	ptn_error("stations.json is not valid");
    }

    stations = json_value_get_array(root_value);
    for (i = 0; i < json_array_get_count(stations); i++) {
	station = json_array_get_object(stations, i);
	s = malloc(sizeof(struct ptn_station));

	if (!s_prev)
	    ptn_current_station = s;

	s->name = strdup(json_object_get_string(station, "name"));
	s->url = strdup(json_object_get_string(station, "url"));
	s->next = NULL;
	s->prev = s_prev;
	if (s_prev)
	    s_prev->next = s;

	s_prev = s;
    }

    json_value_free(root_value);
}


void
ptn_reset_station()
{
    while (ptn_current_station->prev) {
	ptn_current_station = ptn_current_station->prev;
    }
}


void
ptn_free()
{
    struct ptn_station *s;
    struct ptn_station *s_next;

    ptn_reset_station();

    s = ptn_current_station;

    while (s) {
	s_next = s->next;
	free(s);
	s = s_next;
    }
}


int
ptn_check_dial()
{
    int p1_val = digitalRead(PTN_DIAL_PIN1);
    int p2_val = digitalRead(PTN_DIAL_PIN2);

    if (ptn_p1_val == -1) {
	ptn_p1_val = p1_val;
	ptn_p2_val = p2_val;
	return 0;
    }

    if (p1_val == ptn_p1_val &&
	    p2_val == ptn_p2_val) {
	return 0;
    }

    int change;

    if (p1_val != ptn_p1_val) {
	change = ((p1_val + p2_val) == 1) ? 1 : -1;
	ptn_p1_val = p1_val;
    }
    else {
	change = ((p1_val + p2_val) != 1) ? 1 : -1;
	ptn_p2_val = p2_val;
    }

    return change;
}


void
ptn_change_station(int offset)
{
    struct ptn_station *s;
    int i;

    if (!offset)
	return;

    s = ptn_current_station;

    for (i = 0; i < abs(offset); i++) {
	if (offset > 0 && s->next)
	    s = s->next;
	else if (offset < 0 && s->prev)
	    s = s->prev;
    }

    if (ptn_current_station == s)
	return;
    
    ptn_current_station = s;
}


int
main(int argc, char* argv[])
{
    // check the correct BASS was loaded
    if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
	ptn_error("An incorrect version of BASS was loaded");
    }

    // initialize default output device
    if (!BASS_Init(-1, 44100, 0, NULL, NULL)) {
	ptn_error("Can't initialize device");
    }

    if (geteuid() != 0) {
	ptn_error("Must have superuser privileges to launch");
    }

    // initialize WiringPi
    int wp_error = wiringPiSetup();
    if (wp_error) {
	ptn_error("Can't initialize WiringPi");
    }

    // initialize LCD
    ptn_display_fd = lcdInit(2, 16, 8, 11, 10, 0, 1, 2, 3, 4, 5, 6, 7);
    if (ptn_display_fd == -1) {
	ptn_error("Can't initialize LCD");
    }

    // load stations
    ptn_read_config();

    // initialize GPIO pins for quadratic rotary encoder
    pinMode(PTN_DIAL_PIN1, INPUT);
    pullUpDnControl(PTN_DIAL_PIN1, PUD_UP);
    pinMode(PTN_DIAL_PIN2, INPUT);
    pullUpDnControl(PTN_DIAL_PIN2, PUD_UP);

    BASS_SetVolume(1);
    BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing
    BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 0); // minimize automatic pre-buffering, so we can do it (and display it) instead

    BASS_StreamFree(chan);
    ptn_change_station(0);
    chan = BASS_StreamCreateURL(ptn_current_station->url, 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, NULL, 0);
    
    while (1) {
	int progress = (BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFER) * 100) / BASS_StreamGetFilePosition(chan, BASS_FILEPOS_END);

	if (progress > 75) {
	    BASS_ChannelPlay(chan, FALSE);
	    while (1) {
		struct ptn_display info = ptn_get_stream_info();
		ptn_update_display(&info);
		ptn_check_dial();
		sleep(1);
	    }
	}

	sleep(1);
    }

    BASS_StreamFree(chan);
    BASS_Free();

    return 0;
}
