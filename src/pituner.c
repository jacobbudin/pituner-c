
/*
 * Copyright (C) Jacob Budin
 */


#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <bass.h>
#include <wiringPi.h>
#include <lcd.h>
#include "parson.h"
#include "pituner.h"
#include "ptn_signal.h"
#include "ptn_pls.h"


#define PTN_DIAL_PIN1 4
#define PTN_DIAL_PIN2 5
#define PTN_TAG_SEPARATOR " - "
#define PTN_LCD_ROWS 2
#define PTN_LCD_COLS 16


HSTREAM ptn_chan;


static int ptn_debug_mode = 0;


struct ptn_station{
	char *name;
	char *url;
	struct ptn_station *next;
	struct ptn_station *prev;
};


struct ptn_stream{
	char *title;
	char *song;
	char *artist;
};


struct ptn_stream  ptn_current_stream;

struct ptn_station  *ptn_current_station;


char  *ptn_station_file = "etc/stations.json";


int ptn_display_fd = -1;


int ptn_d_dir = 0;
int ptn_p1_val = -1;
int ptn_p2_val = -1;


void
ptn_error(const char *error)
{
	fprintf(stderr, "Pituner: %s\n", error);
	exit(1);
}

void
ptn_debug(const char *format, ...)
{
	va_list args;
	const char *msg_prefix = "Pituner: ";
	char *msg;

	if (!ptn_debug_mode)
		return;

	va_start(args, format);

	msg = malloc((strlen(msg_prefix) + strlen(format) + 2) * sizeof(char));
	sprintf(msg, "%s%s\n", msg_prefix, format);

	vfprintf(stdout, msg, args);

	free(msg);
	va_end(args);
}


int
ptn_update_stream()
{
	const char *tags;
	char *title;
	int i = 0;

	tags = BASS_ChannelGetTags(ptn_chan, BASS_TAG_META);

	if (!tags) {
		if (ptn_current_stream.title && strstr(ptn_current_station->name, ptn_current_stream.title)) {
			return 0;
		}
		else {
			ptn_current_stream.title = strdup(ptn_current_station->name);
			ptn_current_stream.artist = ptn_current_stream.title;
			ptn_current_stream.song = "\0";
			return 1;
		}	
	}

	// stream already exit? check if changed
	if (ptn_current_stream.title) {
		if (strstr(tags, ptn_current_stream.title) && strstr(tags, ptn_current_stream.song))
			return 0;
		else
			free(ptn_current_stream.title);
	}

	ptn_current_stream.title = title = malloc((strlen(tags)+1)*sizeof(char));

	// check if abides by expected format
	if (strncmp(tags, "StreamTitle='", 13) != 0) {
		ptn_current_stream.title = "\0";
		return 1;
	}
	
	while (*tags) {
		if (strncmp(tags, "';StreamUrl=", 12) == 0) {
			*title = '\0';
			break;
		}

		if (i >= 13) {
			*title = *tags;
			title++;
		}

		i++;
		tags++;
	}

	// does it include an artist-song separator
	if (!(ptn_current_stream.song = strstr(ptn_current_stream.title, PTN_TAG_SEPARATOR))) {
		return 1;
	}

	*ptn_current_stream.song = '\0';

	ptn_current_stream.artist = ptn_current_stream.title;
	ptn_current_stream.song += strlen(PTN_TAG_SEPARATOR);

	return 1;
}


void
ptn_update_display()
{
	int x = 0;
	char *artist = ptn_current_stream.artist;
	char *song = ptn_current_stream.song;

	lcdClear(ptn_display_fd);
	lcdCursor(ptn_display_fd, 0);
	lcdCursorBlink(ptn_display_fd, 0);

	if (!ptn_current_stream.title)
		return;

	while (*artist) {
		lcdPosition(ptn_display_fd, x, 0);
		lcdPutchar(0, (unsigned char) *artist);
		x++;
		artist++;

		if (x == PTN_LCD_COLS)
			break;
	}

	x = 0;

	while (*song) {
		lcdPosition(ptn_display_fd, x, 1);
		lcdPutchar(0, (unsigned char) *song);
		x++;
		song++;

		if (x == PTN_LCD_COLS)
			break;
	}
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

	root_value = json_parse_file(ptn_station_file);

	if (json_value_get_type(root_value) != JSONArray)
		ptn_error("stations.json is not valid");

	stations = json_value_get_array(root_value);

	for (i = 0; i < json_array_get_count(stations); i++) {
		station = json_array_get_object(stations, i);
		s = malloc(sizeof(struct ptn_station));

		if (!s)
			ptn_error("malloc() failed");

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
	while (ptn_current_station->prev)
		ptn_current_station = ptn_current_station->prev;
}


void
ptn_free()
{
	struct ptn_station *s;
	struct ptn_station *s_next;

	if (ptn_current_station) {
		ptn_reset_station();

		s = ptn_current_station;

		while (s) {
			s_next = s->next;
			free(s);
			s = s_next;
		}
	}

	if (ptn_chan)
		BASS_StreamFree(ptn_chan);

	BASS_Free();
	lcdClear(ptn_display_fd);
}


int
ptn_check_keyboard()
{
	return 0;
}


void
ptn_reset_dial()
{
	ptn_p1_val = -1;
	ptn_p2_val = -1;
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

	if (p1_val == ptn_p1_val && p2_val == ptn_p2_val)
		return 0;

	if ((p1_val + p2_val) == 1) {
		ptn_d_dir = ((p1_val + ptn_p2_val) != 1) ? 1 : -1;
		return 0;
	}

	ptn_p1_val = p1_val;
	ptn_p2_val = p2_val;

	return ptn_d_dir;
}

void
ptn_load_station()
{
	char *stream_url;

	if (ptn_chan)
		BASS_StreamFree(ptn_chan);

	stream_url = ptn_get_stream_url(ptn_current_station->url);
	ptn_chan = BASS_StreamCreateURL(stream_url, 0, BASS_STREAM_BLOCK | BASS_STREAM_STATUS | BASS_STREAM_AUTOFREE, NULL, 0);

	if (ptn_chan) {
		ptn_debug("Loaded station: \"%s\" (%s)", ptn_current_station->name, ptn_current_station->url);
	}
	else {
		ptn_debug("Couldn't load station: \"%s\" (%s)", ptn_current_station->name, ptn_current_station->url);
		sleep(1);
		ptn_change_station(ptn_d_dir || 1);
	}

	if (ptn_update_stream())
		ptn_update_display();
}

void
ptn_play_station()
{
	int progress;
	int offset;

	if (!ptn_chan)
		return;

	while (1) {
		progress = (BASS_StreamGetFilePosition(ptn_chan, BASS_FILEPOS_BUFFER) * 100) / BASS_StreamGetFilePosition(ptn_chan, BASS_FILEPOS_END);

		if (progress > 75) {
			BASS_ChannelPlay(ptn_chan, FALSE);
			ptn_reset_dial();

			while (1) {
				// determine if stream data has been updated
				if (ptn_update_stream())
					ptn_update_display();

				// determine if station should be changed
				offset = ptn_check_dial() + ptn_check_keyboard();

				if (offset) {
					if (ptn_change_station(offset))
						return;
				}
			}
		}

		sleep(1);
	}
}

void
ptn_stop_station()
{
	if (ptn_chan)
		BASS_ChannelStop(ptn_chan);
}

int
ptn_change_station(int offset)
{
	struct ptn_station *s;
	int i;

	if (!offset)
		return 0;

	s = ptn_current_station;

	for (i = 0; i < abs(offset); i++) {
		if (offset > 0 && s->next)
			s = s->next;
		else if (offset < 0 && s->prev)
			s = s->prev;
	}

	if (ptn_current_station == s)
		return 0;

	ptn_current_station = s;
	ptn_load_station();

	return 1;
}


int
main(int argc, char* argv[])
{
	int c;
 
	while (1) {
		static struct option long_options[] = {
			{"debug", no_argument, 0, 'd'},
			{"help", no_argument, 0, 'h'},
			{"stations", required_argument, 0, 's'},
			{0}
		};

		int option_index = 0;
 
		c = getopt_long(argc, argv, "dhs:", long_options, &option_index);
 
		if (c == -1)
			break;
 
		switch (c) {
			case 'd':
				ptn_debug_mode = 1;
				break;
	 
			case 's':
				ptn_station_file = strdup(optarg);
				break;
	 
			case 'h':
				printf("\n");
				printf("Usage: pituner [options]\n");
				printf("\n");
				printf("  -d        Enabled debug mode\n");
				printf("  -h        Display this help\n");
				printf("  -s <file> Load station file\n");
				printf("\n");
				exit(0);
				break;
		 }
	 }
 
	// check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION)
		ptn_error("An incorrect version of BASS was loaded");

	// initialize default output device
	if (!BASS_Init(-1, 44100, 0, NULL, NULL))
		ptn_error("Can't initialize device");

	// must be superuser to use GPIO pins
	if (geteuid() != 0)
		ptn_error("Must have superuser privileges to launch");

	// initialize WiringPi
	int wp_error = wiringPiSetup();
	if (wp_error)
		ptn_error("Can't initialize WiringPi");

	// initialize LCD
	ptn_display_fd = lcdInit(PTN_LCD_ROWS, PTN_LCD_COLS, 4, 11, 10, 0, 1, 2, 3, 0, 0, 0, 0);
	if (ptn_display_fd == -1)
		ptn_error("Can't initialize LCD");

	// load stations
	ptn_read_config();

	// initialize GPIO pins for quadratic rotary encoder
	pinMode(PTN_DIAL_PIN1, INPUT);
	pullUpDnControl(PTN_DIAL_PIN1, PUD_UP);
	pinMode(PTN_DIAL_PIN2, INPUT);
	pullUpDnControl(PTN_DIAL_PIN2, PUD_UP);

	BASS_SetVolume(1);
	
	// enable playlist processing
	BASS_SetConfig(BASS_CONFIG_NET_PLAYLIST, 1);

	// minimize automatic pre-buffering, so we can do it (and display it) instead
	BASS_SetConfig(BASS_CONFIG_NET_PREBUF, 0); 

	// register SIGTERM signal
	struct sigaction action;
	action.sa_handler = ptn_end;
	sigemptyset(&action.sa_mask);
	action.sa_flags = 0;
	sigaction(SIGTERM, &action, NULL);
	sigaction(SIGINT, &action, NULL);

	ptn_load_station();

	while (ptn_current_station)
		ptn_play_station();

	return EXIT_SUCCESS;
}
