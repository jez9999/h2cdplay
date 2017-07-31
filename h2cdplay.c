/*
 * Basic HOMM2 player
 */

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include "bass.h"

#define MAXTRACK 64
HANDLE evt[MAXTRACK];

char *trkpath = "./tracks";
int now_playing = 0;
int *t_savepos;     // "save position" flag
QWORD *t_position;  // saved position

int log_str(const char *format, ...) {
	char timebuf[100];
	va_list ap;
	struct tm *tmp;
	time_t t;

	t = time(NULL);
	tmp = localtime(&t);
	strftime(timebuf, sizeof(timebuf), "%H:%M:%S", tmp);
	printf("[%s] ", timebuf);

	va_start(ap, format);
	return vprintf(format, ap);
}

void create_events() {
	char evtnamebuf[1024];
	int i;
	for (i = 0; i < MAXTRACK; i++) {
		sprintf(evtnamebuf, "h2cd_play_track#%02d", i);
		evt[i] = CreateEvent(NULL, FALSE, FALSE, evtnamebuf);
	}
}

// Display error messages
void Error(const char *text) {
	log_str("Error(%d): %s\n", BASS_ErrorGetCode (), text);
	BASS_Free();
	exit(1);
}

// Save position, stop playback
void StopChan(DWORD chan) {
	// Stop the track that has been playing
	if (!BASS_ChannelIsActive(chan)) {
		return;
	}

	if (t_savepos[now_playing]) {
		t_position[now_playing] = BASS_ChannelGetPosition(chan, BASS_POS_BYTE);
		log_str("Saved position for track #%02d (%08ld bytes)\n", now_playing, t_position[now_playing]);
	}

	// Fadeout and stop
	log_str("Stopping %02d playback...\n", now_playing);
	BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, 0.0);
	BASS_ChannelStop(chan);
	now_playing = 0;
}

int main(int argc, char **argv) {
	DWORD chan = 0;
	int i;
	int trknum = 0;
	float volume = 1.0;
	char trkbuf[1024];

	printf("HOMM2 CD music player - Jez patch v1.0\n");

	// Check the correct BASS was loaded
	if (HIWORD(BASS_GetVersion()) != BASSVERSION) {
		printf("An incorrect version of BASS was loaded");
		return 1;
	}

	// Setup output - default device
	if (!BASS_Init(-1, 44100, 0, 0, NULL)) {
		Error("Can't initialize device");
	}
	create_events();

	// Initialize t_savepos / t_position, set to zero
	t_savepos = calloc(MAXTRACK, sizeof (t_savepos[0]));
	t_position = calloc(MAXTRACK, sizeof (t_position[0]));

	// Reset all pending events:
	for (i = 0; i < MAXTRACK; i++) {
		ResetEvent(evt[i]);
	}

	log_str("Will play tracks from '%s'\n", trkpath);
	log_str("Waiting for events...\n");
	while (1) {
		DWORD dwWaitResult = WaitForMultipleObjects(MAXTRACK, evt, FALSE, INFINITE);

		log_str("Event fired: %ld\n", dwWaitResult);
		trknum = dwWaitResult;

		// Process stop/terminate events
		if (trknum == 0) {
			StopChan(chan);
			continue;
		}

		if (trknum == 63) {
			log_str("Terminating\n");
			BASS_Free();
			return 0;
		}

		// Process playback events
		if (trknum >= 1 && trknum <= 49) {
			if (trknum == now_playing) {
				log_str("%02d is requested, and it's now playing\n", trknum);
				continue;
			}

			// Preload next chan
			DWORD next = 0;

			sprintf(trkbuf, "%s/track%02d.mp3", trkpath, trknum);
			next = BASS_StreamCreateFile(FALSE, trkbuf, 0, 0, BASS_STREAM_AUTOFREE);
			if (!next) {
				log_str("Error while opening track #%d\n", trknum);
				continue;
			}

			// okay, it's not a jingle. do full-fledged processing (stop, restore, etc)
			StopChan(chan);
			chan = next;
			now_playing = trknum;

			// Set looping (that's okay, we checked for length)
			BASS_ChannelFlags(chan, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);

			// Set volume
			BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume);

			// Seek to start
			if (t_savepos[now_playing] && t_position[now_playing] > 0) {
				log_str("Playing track %s, volume = %.2f (saved position = %08ld)\n", trkbuf, volume, t_position[now_playing]);
				if (!BASS_ChannelSetPosition(chan, t_position[now_playing], BASS_POS_BYTE)) {
					log_str("Seek failed");
				}
				else {
					// we're starting from the middle of track, do fade-in
					BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume);
				}
			}
			else {
				log_str("Playing track %s, volume = %.2f\n", trkbuf, volume);
			}

			BASS_ChannelPlay(chan, FALSE);
			continue;
		}

		// Process volume events
		if (trknum >= 50 && trknum <= 60) {
			int vol_req = trknum - 50;
			int tvar = 11 - vol_req;
			if (tvar > 10) {
				tvar = 0;
			}
			volume = 0.1 * tvar;
			log_str("Volume requested: %d, final: %.2f\n", vol_req, volume);

			if (BASS_ChannelIsActive(chan)) {
				BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume);
			}
			continue;
		}

		if (trknum == 61 && !t_savepos[now_playing]) {
			log_str("Setting 'save position' flag for track %d\n", now_playing);
			t_savepos[now_playing] = 1;
			continue;
		}
	}
}
