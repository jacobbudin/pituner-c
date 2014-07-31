
/*
 * Copyright (C) Jacob Budin
 */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <bass.h>


HSTREAM chan;


const char  *urls[1] = {
    "http://app.musicone.fm/listen/mp3_160.pls"
};


char  proxy[100] = "";


void
ptn_error(const char *error)
{
    printf("%s", error);
    exit(1);
}


void CALLBACK
ptn_status_proc(const void *buffer, DWORD length, void *user)
{
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

    BASS_SetVolume(1);
    BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1); // enable playlist processing
    BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 0); // minimize automatic pre-buffering, so we can do it (and display it) instead
    BASS_SetConfigPtr(BASS_CONFIG_NET_PROXY, proxy); // setup proxy server location

    BASS_StreamFree(chan);
    chan = BASS_StreamCreateURL(urls[0], 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, ptn_status_proc, 0);
    
    while (1) {
	int progress = (BASS_StreamGetFilePosition(chan, BASS_FILEPOS_BUFFER) * 100) / BASS_StreamGetFilePosition(chan, BASS_FILEPOS_END);

	if (progress > 75) {
	    BASS_ChannelPlay(chan, FALSE);
	    while (1) {
	    }
	}

	sleep(1);
    }

    BASS_StreamFree(chan);
    BASS_Free();

    return 0;
}
