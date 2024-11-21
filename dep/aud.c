#include <LIBCD.H>
#include <stdio.h>

#define NULL 0

typedef struct {
    int start;
    int end;
} XA_TRACK;

#define CD_SECTOR_SIZE 2048
#define XA_SECTOR_OFFSET 4

static int  CurPos = -1;
static int  StartPos, EndPos;
static int gPlaying = 0;

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

    u_char param[4];

    param[0] = CdlModeRT|CdlModeSF;
    // Set the CD to XA mode
    int track = 2;
    
    CdControlB(CdlSetmode, param, NULL);
    CdControlF(CdlPause,0);
    
    filter.file = 1;
    filter.chan = channel;
    CdControlB(CdlSetfilter,(u_char *)&filter,0);

    CdlLOC  loc;
    CurPos = StartPos;
    printf("CD mode set to XA playback.\n");

    
    // Seek to the file's start position

    printf("check at xa file: %s and sector: %d \n",&xa_file.name,CdPosToInt(&xa_file.pos));
    printf("check 1: %i, cur: %i, start: %i \n", gPlaying,CurPos,StartPos);
    int status = CdStatus();
    printf("CD-ROM status after seek: %d\n", status);

    if (gPlaying == 0 && CurPos == StartPos){
            // Convert sector number to CD position in min/second/frame and set CdlLOC accordingly.
            CdIntToPos(StartPos, &loc);
            // Send CDROM read command
            CdControlF(CdlReadS, (u_char *)&loc);
            // Set playing flag
            gPlaying = 1;
            printf("playing.. \n");
        }

        if ((CurPos += XA_SECTOR_OFFSET) >= EndPos){
            gPlaying = 0;
        }
    CdControl(CdlSeekL, (unsigned char *)&xa_file.pos, NULL);

    
    // Start XA playback
   

    if ( gPlaying == 0 && CurPos >= EndPos ){
         // Stop XA playback
            // Stop CD playback
            CdControlF(CdlStop,0);
            // Optional
            // Reset parameters
            // param[0] = CdlModeSpeed;
            // Set CD mode
            // CdControlB(CdlSetmode, param, 0);
            // Switch to next channel and start play back
            channel = !channel;
            filter.chan = channel;
            // Set filter
            CdControlF(CdlSetfilter, (u_char *)&filter);
            CurPos = StartPos;

        }
}



void stop_playing(){
    
    

        
            // Stop XA playback
            // Stop CD playback
            CdControlF(CdlStop,0);
            // Optional
            // Reset parameters
            // param[0] = CdlModeSpeed;
            // Set CD mode
            // CdControlB(CdlSetmode, param, 0);
            // Switch to next channel and start play back
            channel = !channel;
            filter.chan = channel;
            // Set filter
            CdControlF(CdlSetfilter, (u_char *)&filter);
            CurPos = StartPos;
        
}