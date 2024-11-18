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
#include "dep/3D.h"


#define ARRAY_SIZE(a) (sizeof(a) / sizeof(*(a)))

#define OTSIZE 4096
#define SCREEN_Z 512
#define CUBESIZE 196

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
    int speed= 6;
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
    ReadcdInit();
    ResetGraph(0);
    InitGeom();
    SetGraphDebug(0);
    PadInit(0);
    
    opencd();  
    cd_read_file("GRID.TMD", &CDData[0]);
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

    
    INIT();
    

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

	ObjectCount += LoadTMD(CDData[0], &Object[0], 0);

    camera.pos.vx = -(camera.x/ONE);
		camera.pos.vy = -(camera.y/ONE);
		camera.pos.vz = -(camera.z/ONE);
		
		camera.rot.vx = camera.til;
		camera.rot.vy = -camera.pan;
        ApplyCamera(&camera);
	
    while (1) {
        cdb = (cdb == &db[0]) ? &db[1] : &db[0];

        

        

        ClearOTagR(cdb->ot, OTSIZE);

        FntPrint("Main loop rn, \n \n input handle ");
        
        hbuts();

        
        FntPrint("%d, %d, %d",camera.pos.vx,camera.pos.vy,camera.pos.vz);




        RenderObject(grid.position, grid.rotation, &Object[0]);
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
