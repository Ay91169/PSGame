#include <libsnd.h>
#include <libspu.h>
#include <libcd.h>

SpuCommonAttr spuSettings;
#define XA_SECTOR_OFFSET 4
// Number of XA files
#define XA_TRACKS 1

typedef struct {
    int start;
    int end;
} XA_TRACK;

XA_TRACK XATrack[XA_TRACKS];



//your audio here
static char * loadXA = "\\Sounds/o.xa;1";
CdlFILE XAPos = {0};
// Start and end position of XA data, in sectors
static int      StartPos, EndPos;
// Current pos in file
static int      CurPos = -1;
// Playback status : 0 not playing, 1 playing
static int gPlaying = 0;
// Current XA channel 
static char channel = 0;

CdlLOC  loc;
// This specifies the file and channel number to actually read data from.
CdlFILTER filter;


void init_audio_system(char *loadXA){
    SpuInit();
    spuSettings.mask = (SPU_COMMON_MVOLL | SPU_COMMON_MVOLR | SPU_COMMON_CDVOLL | SPU_COMMON_CDVOLR | SPU_COMMON_CDMIX);
    spuSettings.mvol.left  = 0x6000;
    spuSettings.mvol.right = 0x6000;
    spuSettings.cd.volume.left = 0x6000;
    spuSettings.cd.volume.right = 0x6000;
    SpuSetCommonAttr(&spuSettings);
    SpuSetTransferMode(SPU_TRANSFER_BY_DMA);
    // Init CD system
    CdInit();

    CdSearchFile( &XAPos, loadXA);
    XATrack[0].start = CdPosToInt(&XAPos.pos);
    XATrack[0].end = XATrack[0].start + (XAPos.size/StSECTOR_SIZE) - 1;    
    StartPos = XATrack[0].start;
    EndPos = XATrack[0].end;
    u_char param[4];
    // ORing the parameters we need to set ; drive speed,  ADPCM play, Subheader filter, sector size
    // If using CdlModeSpeed(Double speed), you need to load an XA file that has 8 channels.
    // In single speed, a 4 channels XA is to be used.
    param[0] = CdlModeSpeed|CdlModeRT|CdlModeSF|CdlModeSize1;
    // Issue primitive command to CD-ROM system (Blocking-type)
    // Set the parameters above
    CdControlB(CdlSetmode, param, 0);
    // Pause at current pos
    CdControlF(CdlPause,0);
    // Set filter 
    // This specifies the file and channel number to actually read data from.
    CdlFILTER filter;
    // Use file 1, channel 0
    filter.file = 1;
    filter.chan = channel;
    // Set filter
    CdControlF(CdlSetfilter, (u_char *)&filter);
    // Position of file on CD
    
    // Set CurPos to StartPos
    CurPos = StartPos;
}


void loadxa(){
     CdSearchFile( &XAPos, loadXA);
    XATrack[0].start = CdPosToInt(&XAPos.pos);
    XATrack[0].end = XATrack[0].start + (XAPos.size/StSECTOR_SIZE) - 1;    
    StartPos = XATrack[0].start;
    EndPos = XATrack[0].end;
    u_char param[4];
    // ORing the parameters we need to set ; drive speed,  ADPCM play, Subheader filter, sector size
    // If using CdlModeSpeed(Double speed), you need to load an XA file that has 8 channels.
    // In single speed, a 4 channels XA is to be used.
    param[0] = CdlModeSpeed|CdlModeRT|CdlModeSF|CdlModeSize1;
    // Issue primitive command to CD-ROM system (Blocking-type)
    // Set the parameters above
    CdControlB(CdlSetmode, param, 0);
    // Pause at current pos
    CdControlF(CdlPause,0);
    // Set filter 
    
    // Use file 1, channel 0
    filter.file = 1;
    filter.chan = channel;
    // Set filter
    CdControlF(CdlSetfilter, (u_char *)&filter);
    
    // Set CurPos to StartPos
    CurPos = StartPos;
}


void playxa(){
    // Begin XA file playback
        if (gPlaying == 0 && CurPos == StartPos){
            // Convert sector number to CD position in min/second/frame and set CdlLOC accordingly.
            CdIntToPos(StartPos, &loc);
            // Send CDROM read command
            CdControlF(CdlReadS, (u_char *)&loc);
            // Set playing flag
            gPlaying = 1;
        }
        // When endPos is reached, set playing flag to 0
        if ((CurPos += XA_SECTOR_OFFSET) >= EndPos){
            gPlaying = 0;
        }
        // If XA file end is reached, stop playback
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
