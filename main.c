#include <sys/types.h>
#include <libetc.h>
#include <libgpu.h>
#include <libgte.h>
#include <libgs.h>
#include <stdio.h>
#include <stdlib.h>
#include <pad.h>
#include <libpad.h>
#include "dep/CDread.h"


//MODEL
#include "models/PT.c"


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

#define OTSIZE 4096
#define SCREEN_Z 512
#define CUBESIZE 196

#define SCREEN_XRES 320

#define CENTERX			SCREEN_XRES/2

#define MAX_OBJS 5


// Increasing this value (max is 14) reduces sorting errors in certain cases
#define OT_LENGTH	12

#define OT_ENTRIES	1<<OT_LENGTH
#define PACKETMAX	2048

GsOT		myOT[2];						// OT handlers
GsOT_TAG	myOT_TAG[2][OT_ENTRIES];		// OT tables
PACKET		myPacketArea[2][PACKETMAX*24];	// Packet buffers
int			myActiveBuff=0;					// Page index counter


// Object handler
GsDOBJ2	Object[MAX_OBJS]={0};
int		ObjectCount=0;

int pad= 0;

//array for cd data variables.
u_long* CDData[7];

struct {
	VECTOR position;
	SVECTOR rotation;
} grid;



typedef struct {
	int		x,xv;
	int		y,yv;
	int		z,zv;
	int		pan,panv;
	int		til,tilv;
	int		rol;
	
	VECTOR	pos;
	SVECTOR rot;
	SVECTOR	dvs;
	
	MATRIX	mat;
    GsRVIEW2 view;
	GsCOORDINATE2 coord2;
} CAMERA;
CAMERA		camera={0};

typedef struct DB {
    DRAWENV draw;
    DISPENV disp;
    u_long ot[OTSIZE];
    POLY_F4 s[6];
} DB;

static SVECTOR cube_vertices[] = {
    {-CUBESIZE / 2, -CUBESIZE / 2, -CUBESIZE / 2, 0}, {CUBESIZE / 2, -CUBESIZE / 2, -CUBESIZE / 2, 0},
    {CUBESIZE / 2, CUBESIZE / 2, -CUBESIZE / 2, 0},   {-CUBESIZE / 2, CUBESIZE / 2, -CUBESIZE / 2, 0},
    {-CUBESIZE / 2, -CUBESIZE / 2, CUBESIZE / 2, 0},  {CUBESIZE / 2, -CUBESIZE / 2, CUBESIZE / 2, 0},
    {CUBESIZE / 2, CUBESIZE / 2, CUBESIZE / 2, 0},    {-CUBESIZE / 2, CUBESIZE / 2, CUBESIZE / 2, 0},
};

static int cube_indices[] = {
    0, 1, 2, 3, 1, 5, 6, 2, 5, 4, 7, 6, 4, 0, 3, 7, 4, 5, 1, 0, 6, 7, 3, 2,
};







//DEBUGGING STUFF



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
	coord = camera.coord2;
	
	// Rotate and translate the matrix according to the specified coordinates
	RotMatrix(&rot, &omtx);
	TransMatrix(&omtx, &pos);
	CompMatrixLV(&camera.coord2.coord, &omtx, &coord.coord);
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

void PrepDisplay() {
	
	// Get active buffer ID and clear the OT to be processed for the next frame
	myActiveBuff = GsGetActiveBuff();
	GsSetWorkBase((PACKET*)myPacketArea[myActiveBuff]);
	GsClearOt(0, 0, &myOT[myActiveBuff]);
	
}



//DEBUGGING STUFF





static void init_cube(DB *db, CVECTOR *col) {
    size_t i;

    for (i = 0; i < ARRAY_SIZE(db->s); ++i) {
        SetPolyF4(&db->s[i]);
        setRGB0(&db->s[i], col[i].r, col[i].g, col[i].b);
    }
}

static void add_cube(u_long *ot, POLY_F4 *s, MATRIX *transform) {
    long p, otz, flg;
    int nclip;
    size_t i;

    

    for (i = 0; i < ARRAY_SIZE(cube_indices); i += 4, ++s) {
        nclip = RotAverageNclip4(&cube_vertices[cube_indices[i + 0]], &cube_vertices[cube_indices[i + 1]],
                                 &cube_vertices[cube_indices[i + 2]], &cube_vertices[cube_indices[i + 3]],
                                 (long *)&s->x0, (long *)&s->x1, (long *)&s->x3, (long *)&s->x2, &p, &otz, &flg);

        if (nclip <= 0) continue;

        if ((otz > 0) && (otz < OTSIZE)) AddPrim(&ot[otz], s);
    }
}


void ApplyCamera(CAMERA *cam) {
	
	// I really can't explain this, found it in one ofthe ZIMEN PsyQ examples
	
	VECTOR	vec;
	
	RotMatrix(&cam->rot, &cam->mat);
	
	ApplyMatrixLV(&cam->mat, &cam->pos, &vec);	
	
	TransMatrix(&cam->mat, &vec);
	SetRotMatrix(&cam->mat);
	SetTransMatrix(&cam->mat);
	
}


void hbuts(){
    int speed= 3;
	pad = PadRead(0);                             // Read pads input. id is unused, always 0.
                                                      // PadRead() returns a 32 bit value, where input from pad 1 is stored in the low 2 bytes and input from pad 2 is stored in the high 2 bytes. (https://matiaslavik.wordpress.com/2015/02/13/diving-into-psx-development/)
        // D-pad        
        if(pad & PADLup)   {
            FntPrint("up \n");
            camera.tilv -= 4;
             camera.pos.vx += speed;} // 🡩           // To access pad 2, use ( pad >> 16 & PADLup)...
        if(pad & PADLdown) {FntPrint("down \n"); camera.tilv += 4;camera.pos.vx -= speed;} // 🡫
        if(pad & PADLright){FntPrint("right \n"); camera.panv -= 4;camera.pos.vz += speed;} // 🡪
        if(pad & PADLleft) {FntPrint("left \n"); camera.panv += 4;camera.pos.vz -= speed;} // 🡨
        // Buttons
        if(pad & PADRup)   {FntPrint("tri \n");
		
		} // △
        if(pad & PADRdown) {FntPrint("X \n");
		;
		
		} // ╳
        if(pad & PADRright){FntPrint("O \n");
		
		camera.zv += (csin(camera.pan)*1);
		camera.xv -= (ccos(camera.pan)*1);
		} // ⭘
        if(pad & PADRleft) {FntPrint("Sqr \n");} // ⬜
        // Shoulder buttons
        if(pad & PADL1){FntPrint("L1 \n");} // L1
        if(pad & PADL2){FntPrint("L2 \n");} // L2
        if(pad & PADR1){FntPrint("R1 \n");} // R1
        if(pad & PADR2){FntPrint("R2 \n");} // R2
        // Start & Select
        if(pad & PADstart){FntPrint("Start \n");} // START
        if(pad & PADselect){FntPrint("sel \n");}                                             // SELECT
}


void INIT(){

    // Prepare the ordering tables
	//myOT[0].length	=OT_LENGTH;
	//myOT[1].length	=OT_LENGTH;
	//myOT[0].org		=myOT_TAG[0];
	//myOT[1].org		=myOT_TAG[1];

    //GsClearOt(0, 0, &myOT[0]);
	//GsClearOt(0, 0, &myOT[1]);

    // Setup 3D and projection matrix
	//GsInit3D();
	//GsSetProjection(CENTERX);

    //GsInitCoordinate2(WORLD, &camera.coord2);
	

    ReadcdInit();
    ResetGraph(0);
    InitGeom();
    SetGraphDebug(0);
    PadInit(0);
    
    opencd();  
    //cd_read_file("GRID.TMD", &CDData[0]);
	closecd();

}

int main(void) {

    

    DB db[2];
    DB *cdb;
    SVECTOR rotation = {0};
    VECTOR translation = {0, 0, (SCREEN_Z * 3) / 2, 0};
    MATRIX transform;
    CVECTOR col[6];
    size_t i;
    VECTOR	plat_pos={0};
	SVECTOR	plat_rot={0};
    VECTOR	obj_pos={0};
	SVECTOR	obj_rot={0};






    
    INIT();
    

    

    //ObjectCount += LinkModel((u_long*)tmd_platform, &Object[0]);	// Platform

    FntLoad(960, 256);
    SetDumpFnt(FntOpen(32, 32, 320, 64, 0, 512));

    SetGeomOffset(320, 240);
    SetGeomScreen(SCREEN_Z);

    SetDefDrawEnv(&db[0].draw, 0, 0, 640, 480);
    SetDefDrawEnv(&db[1].draw, 0, 0, 640, 480);
    SetDefDispEnv(&db[0].disp, 0, 0, 640, 480);
    SetDefDispEnv(&db[1].disp, 0, 0, 640, 480);

    srand(0);

    for (i = 0; i < ARRAY_SIZE(col); ++i) {
        col[i].r = rand();
        col[i].g = rand();
        col[i].b = rand();
    }

    init_cube(&db[0], col);
    init_cube(&db[1], col);

    SetDispMask(1);

    PutDrawEnv(&db[0].draw);
    PutDispEnv(&db[0].disp);

	

    camera.pos.vx = -(camera.x/ONE);
		camera.pos.vy = -(camera.y/ONE);
		camera.pos.vz = -(camera.z/ONE);
		
		camera.rot.vx = camera.til;
		camera.rot.vy = -camera.pan;
        ApplyCamera(&camera);
	
    while (1) {
        cdb = (cdb == &db[0]) ? &db[1] : &db[0];

        //PrepDisplay();

        

        

        ClearOTagR(cdb->ot, OTSIZE);

        FntPrint("Main loop rn, \n \n input handle ");
        
        hbuts();

        
        FntPrint("%d, %d, %d",camera.pos.vx,camera.pos.vy,camera.pos.vz);




       


        // Sort the platform and bulb objects
		//PutObject(plat_pos, plat_rot, &Object[0]);
		
		
		// Sort our test object(s)
		//for(i=2; i<ObjectCount; i++) {	// This for-loop is not needed but its here for TMDs with multiple models
		//	PutObject(obj_pos, obj_rot, &Object[i]);
		//}
        add_cube(cdb->ot, cdb->s, &transform);

        // Wait for all drawing to finish and wait for VSync
		while(DrawSync(1));
		VSync(0);

        ClearImage(&cdb->draw.clip, 60, 120, 120);

        DrawOTag(&cdb->ot[OTSIZE - 1]);
        FntFlush(-1);
    }

    return 0;
}
