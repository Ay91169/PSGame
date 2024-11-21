#include <LIBCD.H>
#include <stdio.h>

// Structure to hold XA file information
CdlFILE xa_file;

int find_xa_file(char *filename) {
    if (!CdSearchFile(&xa_file, filename)) {
        printf("Error: XA file %s not found!\n", filename);
        return 0; // Failure
    }
    printf("XA file %s found at sector: %d\n", filename, CdPosToInt(&xa_file.pos));
    return 1; // Success
}


void play_xa_audio(char *filename) {
    // Set the CD to XA mode
    int track = 1;
    unsigned char mode = CdlModeSpeed | CdlModeRT | CdlModeStream; // Speed, Realtime, XA mode
    CdControl(CdlSetmode, &mode, 0);
    printf("CD mode set to XA playback.\n");

    // Seek to the file's start position
    CdControl(CdlSeekL, (unsigned char *)&xa_file.pos, 0);

    printf("current mode: %d \n", mode);
    // Start XA playback
    if (CdPlay(mode, &track, 0)) { // Track 1, Sub-track 1 (adjust as needed)
        printf("Playing XA audio.\n");
    } else {
        printf("Failed to play XA audio.\n");
    }
}



