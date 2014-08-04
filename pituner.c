
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


#define PTN_DIAL_PIN1 4
#define PTN_DIAL_PIN2 5


HSTREAM chan;


struct ptn_display{
    char *line1;
    char *line2;
};


const char  *ptn_station_urls[] = {
    "http://prem1.di.fm:80/vocaltrance_hi?643945d6aa1da7d29705e61b",
    "http://app.musicone.fm/listen/mp3_160.pls"
};


int ptn_station_url_i = 0;


int ptn_display_fd = -1;


int ptn_p1_val = -1;
int ptn_p2_val = -1;


void
ptn_error(const char *error)
{
    printf("%s", error);
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
    if(!offset)
	return;

    int i;
    int j = ptn_station_url_i;

    if (offset > 0) {
	for (i = 1; i <= offset; i++) {
	    j++;
	    if (!ptn_station_urls[j])
		j = 0;
	}
    }
    else {
	for (i = 1; i <= offset; i++) {
	    j--;
	    if (!j)
		j = sizeof(j) - 1;
	}
    }

    ptn_station_url_i = j;
    printf("%s\n", ptn_station_urls[ptn_station_url_i]);
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

    // initialize GPIO pins for quadratic rotary encoder
    pinMode(PTN_DIAL_PIN1, INPUT);
    pullUpDnControl(PTN_DIAL_PIN1, PUD_UP);
    pinMode(PTN_DIAL_PIN2, INPUT);
    pullUpDnControl(PTN_DIAL_PIN2, PUD_UP);

    BASS_SetVolume(1);
    BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing
    BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 0); // minimize automatic pre-buffering, so we can do it (and display it) instead

    BASS_StreamFree(chan);
    chan = BASS_StreamCreateURL(ptn_station_urls[ptn_station_url_i], 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, NULL, 0);
    
    while (1) {
	int progress = (BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFER) * 100) / BASS_StreamGetFilePosition(chan, BASS_FILEPOS_END);

	if (progress > 75) {
	    BASS_ChannelPlay(chan, FALSE);
	    while (1) {
		struct ptn_display info = ptn_get_stream_info();
		ptn_update_display(&info);
		ptn_check_dial();
		sleep(5);
		ptn_change_station(3);
	    }
	}

	sleep(1);
    }

    BASS_StreamFree(chan);
    BASS_Free();

    return 0;
}
