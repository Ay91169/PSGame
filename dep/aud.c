#include <LIBCD.H>
#include <stdio.h>
#include <libgpu.h>


#define NULL 0

typedef struct {
    int start;
    int end;
} XA_TRACK;

#define CD_SECTOR_SIZE 2048
#define XA_SECTOR_OFFSET 4

int  CurPos = -1;
int  StartPos, EndPos;
int gPlaying = 0;

// Current XA channel 
static char channel = 0;

XA_TRACK track; 
// Structure to hold XA file information
CdlFILE xa_file;
CdlFILTER filter;

int find_xa_file(char *filename) {
    if (!CdSearchFile(&xa_file, filename)) {
        printf("Error: XA file %s not found!\n", filename);
        return 0; // Failure
    }
    printf("XA file %s found at sector: %d\n", filename, CdPosToInt(&xa_file.pos));
    return 1; // Success
}


void play_xa_audio(char *filename) {

    track.start = CdPosToInt(&xa_file.pos);
    track.end = track.start + (xa_file.size/CD_SECTOR_SIZE) - 1;
    StartPos = track.start;
    EndPos = track.end;

    u_char param[1];
    param[0] = CdlModeSpeed|CdlModeRT|CdlModeSF|CdlModeSize1;
    CdControlB(CdlSetmode, param, NULL);
    printf("CD mode set to XA playback.\n");
    
    if (!CdControlB(CdlSetmode, param, NULL)) {
        printf("Error: Unable to set CD mode to XA.\n");
        return;
    }
    
    
    filter.file = 1;
    filter.chan = channel;
    if (!CdControlB(CdlSetfilter,(u_char *)&filter,NULL)) {
        printf("failed to set filter");
    }
    printf("CD filter set: File = %d, Channel = %d\n", filter.file, filter.chan);
    

        // Seek to start position
    CdlLOC loc;
    CdIntToPos(StartPos, &loc);
    CurPos = StartPos;
    if (!CdControlF(CdlSeekL, (u_char *)&loc)) {
        printf("Error: Unable to seek to XA file start position.\n");
        return;
    }

    // Start playback
    if (!CdControlF(CdlReadS, (u_char *)&loc)) {
        printf("Error: Unable to start XA playback.\n");
        return;
    }

    printf("CD status after playback start: %02X\n", CdStatus());

    gPlaying = 1;
    printf("Playing XA audio from sector %d to %d on channel %d.\n", StartPos, EndPos, channel);
}




void stop_playing(){
    
    
if (gPlaying) {
        CdControlF(CdlStop, NULL);
        gPlaying = 0;
        printf("XA playback stopped.\n");
    }
        
}

