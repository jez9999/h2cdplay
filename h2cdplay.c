/*
 * HOMM2 music player server
 */

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>
#include "bass.h"

#define MAXTRACK 64
HANDLE evt[MAXTRACK];

char *trkpath = "./tracks";
int now_playing = 0; // holds track number; 1-indexed (0 means no track playing)
const int track_data[] = {
	// Track data bitfield:
	// 1: reset this track to beginning when playing it
	// 2: loop this track
	// 4: reset this track to beginning when playing it IF restartcastlemusic is on
	1,  // track01: jingle music
	3,  // track02: battle music
	3,  // track03: battle music
	3,  // track04: battle music
	6,  // track05: castle music
	6,  // track06: castle music
	6,  // track07: castle music
	6,  // track08: castle music
	6,  // track09: castle music
	6,  // track10: castle music
	2,  // track11: overworld music
	2,  // track12: overworld music
	2,  // track13: overworld music
	2,  // track14: overworld music
	2,  // track15: overworld music
	2,  // track16: overworld music
	2,  // track17: overworld music
	2,  // track18: overworld music
	1,  // track19: game over music
	1,  // track20: new week music
	1,  // track21: new week music
	3,  // track22: campaign music
	1,  // track23: obelisk music
	3,  // track24: campaign music
	1,  // track25: jingle music
	1,  // track26: jingle music
	1,  // track27: jingle music
	2,  // track28: enemy move music
	1,  // track29: jingle music
	1,  // track30: jingle music
	1,  // track31: jingle music
	1,  // track32: jingle music
	1,  // track33: jingle music
	1,  // track34: jingle music
	1,  // track35: jingle music
	1,  // track36: jingle music
	1,  // track37: jingle music
	1,  // track38: jingle music
	1,  // track39: jingle music
	1,  // track40: jingle music
	1,  // track41: jingle music
	2,  // track42: main menu music
	1   // track43: credits music
};
QWORD *track_position;  // saved position

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
	log_str("Error(%d): %s\n", BASS_ErrorGetCode(), text);
	BASS_Free();
	exit(1);
}

// Save position, stop playback
void StopChan(DWORD chan) {
	if (!BASS_ChannelIsActive(chan)) {
		// No need to do anything; already not playing
		return;
	}

	// Save position of the track that has been playing
	if (now_playing > 0 && !(track_data[now_playing-1] & 1)) {
		track_position[now_playing] = BASS_ChannelGetPosition(chan, BASS_POS_BYTE);
		log_str("Saved position for track #%02d (%08ld bytes)\n", now_playing, track_position[now_playing]);
	}

	// Stop track
	log_str("Stopping %02d playback...\n", now_playing);
	BASS_ChannelStop(chan);
	now_playing = 0;
}

int main(int argc, char **argv) {
	DWORD chan = 0;
	int i;
	int trknum = 0;
	float volume = 1.0;
	char trkbuf[1024];
	int restartcastlemusic = 0;

	printf("HOMM2 music player - Jez patch v2.1\n");

	if (argc > 1) {
		if (strcmp(argv[1], "restartcastlemusic") == 0)
		{
			printf("\n");
			printf("Will restart the castle music each time instead of saving position.\n");
			restartcastlemusic = 1;
		}
		else if (strcmp(argv[1], "/?") == 0 || strcmp(argv[1], "-h") == 0)
		{
			printf("\n");
			printf("Usage: %s [options]\n", argv[0]);
			printf("Options:\n");
			printf("  restartcastlemusic   Restart the castle music each time instead of saving position\n");
			printf("  /? or -h:            Display this help message\n");
			return 0;
		}
	}

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

	// Initialize track_position, set to zero
	track_position = calloc(MAXTRACK, sizeof(track_position[0]));

	// Reset all pending events
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
				log_str("%02d is requested, and it's already playing\n", trknum);
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

			// Do track processing (stop, restore, etc.)
			StopChan(chan);
			chan = next;
			now_playing = trknum;

			// Set looping
			if (now_playing > 0 && track_data[now_playing-1] & 2) {
				BASS_ChannelFlags(chan, BASS_SAMPLE_LOOP, BASS_SAMPLE_LOOP);
			}

			// Set volume
			BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume);

			// Seek to start
			if (
				now_playing > 0 &&
				track_position[now_playing] > 0 &&
				!(
					(track_data[now_playing-1] & 1) ||
					((track_data[now_playing-1] & 4) && restartcastlemusic)
				)
			) {
				log_str("Playing track %s, volume = %.2f (saved position = %08ld)\n", trkbuf, volume, track_position[now_playing]);
				if (!BASS_ChannelSetPosition(chan, track_position[now_playing], BASS_POS_BYTE)) {
					log_str("Seek failed");
				}
				else {
					// We're starting from the middle of track, set volume
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
			volume = 0.1f * tvar;
			log_str("Volume requested: %d, final: %.2f\n", vol_req, volume);

			if (BASS_ChannelIsActive(chan)) {
				BASS_ChannelSetAttribute(chan, BASS_ATTRIB_VOL, volume);
			}
			continue;
		}
	}
}
