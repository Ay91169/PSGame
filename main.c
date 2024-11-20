#include <sys/types.h>
#include <libetc.h>
#include <libgte.h>
#include <libapi.h>
#include <libgpu.h>
#include <libgs.h>
#include <libpad.h>
#include <pad.h>
#include <stdio.h>
#include <abs.h>

//FUNCS
#include "dep/CDread.h"



// TMD models
#include "models/PT.c"



// Maximum number of objects
#define MAX_OBJECTS 3


// Screen resolution and dither mode
#define SCREEN_XRES		640
#define SCREEN_YRES 	480
#define DITHER			1

#define CENTERX			SCREEN_XRES/2
#define CENTERY			SCREEN_YRES/2


// Increasing this value (max is 14) reduces sorting errors in certain cases
#define OT_LENGTH	12

#define OT_ENTRIES	1<<OT_LENGTH
#define PACKETMAX	2048


GsOT		myOT[2];						// OT handlers
GsOT_TAG	myOT_TAG[2][OT_ENTRIES];		// OT tables
PACKET		myPacketArea[2][PACKETMAX*24];	// Packet buffers
int			myActiveBuff=0;					// Page index counter


// Camera coordinates
struct {
	int		x,y,z;
	int		pan,til,rol,panv;
    
	VECTOR	pos;
	SVECTOR rot;
	GsRVIEW2 view;
	GsCOORDINATE2 coord2;
} Camera = {0};

int i;



struct {
    int PadStatus;   // Digital button state
    unsigned char ljoy_h; // Left stick X-axis
    unsigned char ljoy_v; // Left stick Y-axis
    unsigned char rjoy_h; // Right stick X-axis
    unsigned char rjoy_v; // Right stick Y-axis
} PADTYPE;

// Dual Shock values
static unsigned char padbuff[2][34];
unsigned char Motor[2];
int portNo = 0x00;
int ctr;
static unsigned char align[6]={0,1,0xFF,0xFF,0xFF,0xFF};

unsigned char pad_buffer[34];
#define DEADZONE 5


#define DUALSHOCKMODE ((u_long *)0x80010000)
unsigned long *storedMode = DUALSHOCKMODE;


// Object handler
GsDOBJ2	Object[MAX_OBJECTS]={0};
int		ObjectCount=0;

// Lighting coordinates
GsF_LIGHT pslt[1];


// Prototypes
int main();

void CalculateCamera();
void PutObject(VECTOR pos, SVECTOR rot, GsDOBJ2 *obj);

int LinkModel(u_long *tmd, GsDOBJ2 *obj);
void LoadTexture(u_long *addr);

void init();
void PrepDisplay();
void Display();



//debug code

void log_pad_buffer(unsigned char *buffer, int size) {
    printf("Controller Buffer: ");
    for (int i = 0; i < size; i++) {
        printf("%02X ", buffer[i]);
    }
    printf("\n");
}




void handle_movement(unsigned char lx, unsigned char ly, const char* stick_name) {
    // Define the deadzone boundary values
    unsigned char deadzone_min = 0x50;
    unsigned char deadzone_max = 0xBC;

    // Movement speed factor based on distance from the center (0x80)
    int speed_factor;
    
    // Calculate the distance from center (0x80) for both X and Y
    int x_distance = lx - 0x80;
    int y_distance = ly - 0x80;

    // If within the deadzone, ignore movement
    if (lx >= deadzone_min && lx <= deadzone_max) {
        printf("%s X-Axis Deadzone\n", stick_name);
        x_distance = 0;
    } else {
        // Speed based on the distance from center
        speed_factor = (x_distance > 0) ? x_distance : -x_distance;
        printf("%s X-Axis Speed: %d\n", stick_name, speed_factor);  // Display the speed factor for debugging
    }

    if (ly >= deadzone_min && ly <= deadzone_max) {
        printf("%s Y-Axis Deadzone\n", stick_name);
        y_distance = 0;
    } else {
        // Speed based on the distance from center
        speed_factor = (y_distance > 0) ? y_distance : -y_distance;
        printf("%s Y-Axis Speed: %d\n", stick_name, speed_factor);  // Display the speed factor for debugging
    }

    // Horizontal movement: left or right based on the X-axis value
    if (x_distance < 0) {
        printf("%s Move Left with Speed %d\n", stick_name, speed_factor);
    } else if (x_distance > 0) {
        printf("%s Move Right with Speed %d\n", stick_name, speed_factor);
    }

    // Vertical movement: up or down based on the Y-axis value
    if (y_distance < 0) {
        printf("%s Move Up with Speed %d\n", stick_name, speed_factor);
    } else if (y_distance > 0) {
        printf("%s Move Down with Speed %d\n", stick_name, speed_factor);
    }
}

void handle_dualshock(unsigned char *pad_buffer) {
    // Left stick (X and Y axes)
    unsigned char left_x = pad_buffer[6];
    unsigned char left_y = pad_buffer[7];

    // Right stick (X and Y axes)
    unsigned char right_x = pad_buffer[4];
    unsigned char right_y = pad_buffer[5];

    printf("Processing Left Stick:\n");
    handle_movement(left_x, left_y, "Left Stick");

    printf("Processing Right Stick:\n");
    handle_movement(right_x, right_y, "Right Stick");
}

//debug code


void hbuts(){
    int speed= 3;

	
	handle_dualshock(pad_buffer);
	

	

	PADTYPE.PadStatus = PadRead(0);                             // Read pads input. id is unused, always 0.
	PADTYPE.ljoy_v = PadRead(0);
	PADTYPE.ljoy_h = PadRead(0);
	PADTYPE.rjoy_h = PadRead(0);
	PADTYPE.rjoy_v = PadRead(0);
                                                      // PadRead() returns a 32 bit value, where input from pad 1 is stored in the low 2 bytes and input from pad 2 is stored in the high 2 bytes. (https://matiaslavik.wordpress.com/2015/02/13/diving-into-psx-development/)
        // D-pad


		if (PADTYPE.PadStatus & 0xF000) {
   		 	FntPrint("Analog mode active\n");
		} else {
    		FntPrint("Digital mode active\n");
		}

		if (PADTYPE.PadStatus > 0) {
        printf("Raw PadStatus: 0x%08X\n", PADTYPE.PadStatus);
    	}     
        if(PADTYPE.PadStatus & PADLup)   {
            FntPrint("up \n"); Camera.panv += 4;Camera.pos.vz -= speed;
            } // 🡩           // To access pad 2, use ( pad >> 16 & PADLup)...
        if(PADTYPE.PadStatus & PADLdown) {FntPrint("down \n"); Camera.panv -= 4;Camera.pos.vz += speed; } // 🡫
        if(PADTYPE.PadStatus & PADLright){FntPrint("right \n"); Camera.rol += 4;Camera.pos.vx -= speed;} // 🡪
        if(PADTYPE.PadStatus & PADLleft) {FntPrint("left \n"); Camera.rol -= 4;
             Camera.pos.vx += speed;} // 🡨
        // Buttons
        if(PADTYPE.PadStatus & PADRup)   {FntPrint("tri \n");
		
		} // △
        if(PADTYPE.PadStatus & PADRdown) {FntPrint("X \n");
		;
		
		} // ╳
        if(PADTYPE.PadStatus & PADRright){FntPrint("O \n");
		
		Camera.z += (csin(Camera.pan)*1);
		Camera.x -= (ccos(Camera.pan)*1);
		} // ⭘
        if(PADTYPE.PadStatus & PADRleft) {FntPrint("Sqr \n");} // ⬜
        // Shoulder buttons
        if(PADTYPE.PadStatus & PADL1){FntPrint("L1 \n");} // L1
        if(PADTYPE.PadStatus & PADL2){FntPrint("L2 \n");} // L2
        if(PADTYPE.PadStatus & PADR1){FntPrint("R1 \n");} // R1
        if(PADTYPE.PadStatus & PADR2){FntPrint("R2 \n");} // R2
        // Start & Select
        if(PADTYPE.PadStatus & PADstart){FntPrint("Start \n");} // START
        if(PADTYPE.PadStatus & PADselect){FntPrint("sel \n");}  // SELECT
		if(PADTYPE.PadStatus & PADR3){FntPrint("R3 \n");}
		if(PADTYPE.PadStatus & PADL3){FntPrint("L3 \n");}   

	int leftStickX = PADTYPE.ljoy_h; // Left stick X-axis (0–255)
    int leftStickY = PADTYPE.ljoy_v; // Left stick Y-axis (0–255)
    int rightStickX = PADTYPE.rjoy_h; // Right stick X-axis (0–255)
    int rightStickY = PADTYPE.rjoy_v; // Right stick Y-axis (0–255)

	// Center is around 128, so we calculate offset
    int leftX = leftStickX - 128; // -128 to 127
    int leftY = leftStickY - 128; // -128 to 127
    int rightX = rightStickX - 128; // -128 to 127
    int rightY = rightStickY - 128; // -128 to 127

	// Log the analog stick values
    //printf("Left Stick: X=%d Y=%d\n", leftX, leftY);
    //printf("Right Stick: X=%d Y=%d\n", rightX, rightY);
	if (leftX > 10) { // Add deadzone for smoother control
        Camera.pos.vx += leftX / 10; // Adjust divisor for sensitivity
    }
    if (leftY > 10) {
        Camera.pos.vz += leftY / 10;
    }
    if (rightX > 10) {
        Camera.panv += rightX / 10;
    }
    if (rightY > 10) {
        Camera.rol += rightY / 10;
    }

}


// Main stuff
int main() {
	
	PadInitDirect(pad_buffer,NULL);
	PadSetAct(0,0,1);
	PadStartCom();
	
    
	int Accel;
	
	// Object coordinates
	VECTOR	plat_pos={0};
	SVECTOR	plat_rot={0};
	
	VECTOR	obj_pos={0};
	SVECTOR	obj_rot={0};
	
	VECTOR	bulb_pos={0};
	SVECTOR	bulb_rot={0};
	
	// Player coordinates
	struct {
		int x,xv;
		int y,yv;
		int z,zv;
		int pan,panv;
		int til,tilv;
	} Player = {0};
	
	
	// Init everything
	init();
	
	
	// Load the texture for our test model
	
	
	// Link the TMD models
	ObjectCount += LinkModel((u_long*)tmd_platform, &Object[0]);	// Platform
	
	
	Object[0].attribute |= GsDIV1;	// Set 2x2 sub-division for the platform to reduce clipping errors
	for(i=2; i<ObjectCount; i++) {
		Object[2].attribute = 0;		// Re-enable lighting for the test model
	}
	
	
	// Default camera/player position
	Player.x = ONE*-640;
	Player.y = ONE*510;
	Player.z = ONE*800;
	
	Player.pan = -660;
	Player.til = -245;
	
	
	// Object positions
	plat_pos.vy = 1024;
	bulb_pos.vz = -800;
	bulb_pos.vy = -400;
	obj_pos.vy = 400;
	
	
	while(1) {
		
		hbuts();
		
		log_pad_buffer(pad_buffer,34);	
		FntPrint("Left Stick xy-Axis: %02X, %02X,\n Right Stick: %02X,%02X \n", pad_buffer[6],pad_buffer[7],pad_buffer[5],pad_buffer[5]);

		// Prepare for rendering
		PrepDisplay();
		
		
		// Print banner and camera stats
		
		FntPrint(" CX:%d CY:%d CZ:%d\n", Camera.pos.vx, Camera.pos.vy, Camera.pos.vz);
		FntPrint(" CP:%d CT:%d CR:%d\n", Camera.rot.vy, Camera.rot.vx, Camera.rot.vz);
		
		
		// Calculate the camera and viewpoint matrix
		CalculateCamera();
		
		
		// Set the light source coordinates
		pslt[0].vx = -(bulb_pos.vx);
		pslt[0].vy = -(bulb_pos.vy);
		pslt[0].vz = -(bulb_pos.vz);
		
		pslt[0].r =	0xff;	pslt[0].g = 0xff;	pslt[0].b = 0xff;
		GsSetFlatLight(0, &pslt[0]);
		
		
		// Rotate the object
		//obj_rot.vx += 4;
		obj_rot.vy += 4;
		
		
		// Sort the platform and bulb objects
		PutObject(plat_pos, plat_rot, &Object[0]);
		PutObject(bulb_pos, bulb_rot, &Object[1]);
		
		// Sort our test object(s)
		for(i=2; i<ObjectCount; i++) {	// This for-loop is not needed but its here for TMDs with multiple models
			PutObject(obj_pos, obj_rot, &Object[i]);
		}
		
		
		// Display the new frame
		Display();
		
	}
	
}


void CalculateCamera() {
	
	// This function simply calculates the viewpoint matrix based on the camera coordinates...
	// It must be called on every frame before drawing any objects.
	
	VECTOR	vec;
	GsVIEW2 view;
	
	// Copy the camera (base) matrix for the viewpoint matrix
	view.view = Camera.coord2.coord;
	view.super = WORLD;
	
	// I really can't explain how this works but I found it in one of the ZIMEN examples
	RotMatrix(&Camera.rot, &view.view);
	ApplyMatrixLV(&view.view, &Camera.pos, &vec);
	TransMatrix(&view.view, &vec);
	
	// Set the viewpoint matrix to the GTE
	GsSetView2(&view);
	
}

void PutObject(VECTOR pos, SVECTOR rot, GsDOBJ2 *obj) {
	
	/*	This function draws (or sorts) a TMD model linked to a GsDOBJ2 structure... All
		matrix calculations are done automatically for simplified object placement.
		
		Parameters:
			pos 	- Object position.
			rot		- Object orientation.
			*obj	- Pointer to a GsDOBJ2 structure that is linked to a TMD model.
			
	*/
	
	MATRIX lmtx,omtx;
	GsCOORDINATE2 coord;
	
	// Copy the camera (base) matrix for the model
	coord = Camera.coord2;
	
	// Rotate and translate the matrix according to the specified coordinates
	RotMatrix(&rot, &omtx);
	TransMatrix(&omtx, &pos);
	CompMatrixLV(&Camera.coord2.coord, &omtx, &coord.coord);
	coord.flg = 0;
	
	// Apply coordinate matrix to the object
	obj->coord2 = &coord;
	
	// Calculate Local-World (for lighting) and Local-Screen (for projection) matrices and set both to the GTE
	GsGetLws(obj->coord2, &lmtx, &omtx);
	GsSetLightMatrix(&lmtx);
	GsSetLsMatrix(&omtx);
	
	// Sort the object!
	GsSortObject4(obj, &myOT[myActiveBuff], 14-OT_LENGTH, getScratchAddr(0));
	
}


int LinkModel(u_long *tmd, GsDOBJ2 *obj) {
	
	/*	This function prepares the specified TMD model for drawing and then
		links it to a GsDOBJ2 structure so it can be drawn using GsSortObject4().
		
		By default, light source calculation is disabled but can be re-enabled by
		simply setting the attribute variable in your GsDOBJ2 structure to 0.
		
		Parameters:
			*tmd - Pointer to a TMD model file loaded in memory.
			*obj - Pointer to an empty GsDOBJ2 structure.
	
		Returns:
			Number of objects found inside the TMD file.
			
	*/
	
	u_long *dop;
	int i,NumObj;
	
	// Copy pointer to TMD file so that the original pointer won't get destroyed
	dop = tmd;
	
	// Skip header and then remap the addresses inside the TMD file
	dop++; GsMapModelingData(dop);
	
	// Get object count
	dop++; NumObj = *dop;

	// Link object handler with the specified TMD
	dop++;
	for(i=0; i<NumObj; i++) {
		GsLinkObject4((u_long)dop, &obj[i], i);
		obj[i].attribute = (1<<6);	// Disables light source calculation
	}
	
	// Return the object count found inside the TMD
	return(NumObj);
	
}
void LoadTexture(u_long *addr) {
	
	// A simple TIM loader... Not much to explain
	
	RECT rect;
	GsIMAGE tim;
	
	// Get TIM information
	GsGetTimInfo((addr+1), &tim);
	
	// Load the texture image
	rect.x = tim.px;	rect.y = tim.py;
	rect.w = tim.pw;	rect.h = tim.ph;
	LoadImage(&rect, tim.pixel);
	DrawSync(0);
	
	// Load the CLUT (if present)
	if ((tim.pmode>>3) & 0x01) {
		rect.x = tim.cx;	rect.y = tim.cy;
		rect.w = tim.cw;	rect.h = tim.ch;
		LoadImage(&rect, tim.clut);
		DrawSync(0);
	}
	
}


void init() {
	
	SVECTOR VScale={0};
	
	// Initialize the GS
	ResetGraph(0);
	if (SCREEN_YRES <= 240) {
		GsInitGraph(SCREEN_XRES, SCREEN_YRES, GsOFSGPU|GsNONINTER, DITHER, 0);
		GsDefDispBuff(0, 0, 0, 256);
	} else {
		GsInitGraph(SCREEN_XRES, SCREEN_YRES, GsOFSGPU|GsINTER, DITHER, 0);	
		GsDefDispBuff(0, 0, 0, 0);
	}
	
	// Prepare the ordering tables
	myOT[0].length	=OT_LENGTH;
	myOT[1].length	=OT_LENGTH;
	myOT[0].org		=myOT_TAG[0];
	myOT[1].org		=myOT_TAG[1];
	
	GsClearOt(0, 0, &myOT[0]);
	GsClearOt(0, 0, &myOT[1]);
	
	
	// Initialize debug font stream
	FntLoad(960, 0);
	FntOpen(-CENTERX, -CENTERY, SCREEN_XRES, SCREEN_YRES, 0, 512);
	
	
	// Setup 3D and projection matrix
	GsInit3D();
	GsSetProjection(CENTERX);
	
	
	// Initialize coordinates for the camera (it will be used as a base for future matrix calculations)
	GsInitCoordinate2(WORLD, &Camera.coord2);
	
	
	// Set ambient color (for lighting)
	GsSetAmbient(ONE/4, ONE/4, ONE/4);
	
	// Set default lighting mode
	GsSetLightMode(0);
	
	
	// Initialize controller
	PadInit(0);
	
}

void PrepDisplay() {
	
	// Get active buffer ID and clear the OT to be processed for the next frame
	myActiveBuff = GsGetActiveBuff();
	GsSetWorkBase((PACKET*)myPacketArea[myActiveBuff]);
	GsClearOt(0, 0, &myOT[myActiveBuff]);
	
}
void Display() {
	
	// Flush the font stream
	FntFlush(-1);
	
	// Wait for VSync, switch buffers, and draw the new frame.
	VSync(0);
	GsSwapDispBuff();
	GsSortClear(0, 64, 0, &myOT[myActiveBuff]);
	GsDrawOt(&myOT[myActiveBuff]);
	
}