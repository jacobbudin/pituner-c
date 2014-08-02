
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


HSTREAM chan;


struct ptn_display{
    char *line1;
    char *line2;
};


const char  *urls[] = {
    "http://prem1.di.fm:80/vocaltrance_hi?643945d6aa1da7d29705e61b"
};


char  proxy[] = "";


int ptn_display_fd = -1;


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


void
ptn_check_dial()
{
    int p1_val = digitalRead(4);
    int p2_val = digitalRead(5);
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
    if(wp_error){
	ptn_error("Can't initialize WiringPi");
    }

    // initialize LCD
    ptn_display_fd = lcdInit(2, 16, 8, 11, 10, 0, 1, 2, 3, 4, 5, 6, 7);
    if(ptn_display_fd == -1){
	ptn_error("Can't initialize LCD");
    }

    // initialize GPIO pins for quadratic rotary encoder
    pinMode(4, INPUT);
    pullUpDnControl(4, PUD_UP);
    pinMode(5, INPUT);
    pullUpDnControl(5, PUD_UP);

    BASS_SetVolume(1);
    BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing
    BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 0); // minimize automatic pre-buffering, so we can do it (and display it) instead
    BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, proxy); // setup proxy server location

    BASS_StreamFree(chan);
    chan = BASS_StreamCreateURL(urls[0], 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, NULL, 0);
    
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
