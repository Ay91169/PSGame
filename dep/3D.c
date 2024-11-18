#include <libgs.h>
#include <stdio.h>
#include <malloc.h>
#include <STDLIB.H>

#include <LIBGTE.H>
#include <LIBGPU.H>

#include <LIBETC.H>
#include <LIBSPU.H>
#include <LIBDS.H>
#include <STRINGS.H>
#include <SYS/TYPES.H>
#define MAXOBJ 100
GsDOBJ2	Object[MAXOBJ];
int ObjectCount=0;

#define OT_LENGTH	12
#define OT_ENTRIES	1<<OT_LENGTH
#define PACKETMAX	2048

GsOT		orderingTable[2];
GsOT_TAG	orderingTable_TAG[2][OT_ENTRIES];
int			myActiveBuff=0;
PACKET GPUOutputPacket[2][PACKETMAX*24];







void RenderObject(VECTOR pos, SVECTOR rot, GsDOBJ2 *obj) {

	MATRIX lmtx,omtx;
	GsCOORDINATE2 coord;

	

	//Flip the Y axis so a positive value
	//is up, and a negative value is down
	pos.vy *= -1;

	// Rotate and translate the matrix according to the specified coordinates
	RotMatrix(&rot, &omtx);
	TransMatrix(&omtx, &pos);

	coord.flg = 0;

	// Apply coordinate matrix to the object
	obj->coord2 = &coord;

	// Calculate Local-World (for lighting) and Local-Screen (for projection) matrices and set both to the GTE
	GsGetLws(obj->coord2, &lmtx, &omtx);
	GsSetLightMatrix(&lmtx);
	GsSetLsMatrix(&omtx);

	// Sort the object!
	GsSortObject4(obj, &orderingTable[myActiveBuff], 14-OT_LENGTH, getScratchAddr(0));

}

int LoadTMD(u_long *tmd, GsDOBJ2 *obj, int enableLighting) {

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
	dop++;
	GsMapModelingData(dop);

	// Get object count
	dop++;
	NumObj = *dop;

	// Link object handler with the specified TMD
	dop++;
	for(i=0; i<NumObj; i++) {
		GsLinkObject4((u_long)dop, &obj[i], i);
		//connect the WORLD coordinate directly
		//GsInitCoordinate2(WORLD,obj[i].coord2);
		if (enableLighting == 0) {
			obj[i].attribute = (1<<6);	// Disables light source calculation
		}
	}

	// Return the object count found inside the TMD
	return(NumObj);

}

void loadTexture(unsigned char imageData[]) {

	// Initialize image
	GsIMAGE* tim_data;
	RECT* rect;
	RECT* crect;
	tim_data = malloc3(sizeof(GsIMAGE));
	GsGetTimInfo ((u_long *)(imageData+4),tim_data);
	rect = malloc3(sizeof(RECT));
	crect = malloc3(sizeof(RECT));

	// Load the image into the GPU memory (frame buffer)
	rect->x = tim_data->px; // x position of image in frame buffer
	rect->y = tim_data->py; // y position of image in frame buffer
	rect->w = tim_data->pw; // width in frame buffer
	rect->h = tim_data->ph; // height in frame buffer
	printf("Framebuffer info {x=%d, y=%d, w=%d, h=%d}\n", rect->x, rect->y, rect->w, rect->h);
	LoadImage(rect, tim_data->pixel);

	// Load the color lookup table (CLUT) into the GPU memory (frame buffer)
	crect->x = tim_data->cx; // x position of CLUT in frame buffer
	crect->y = tim_data->cy; // y position of CLUT in frame buffer
	crect->w = tim_data->cw; // width of CLUT in frame buffer
	crect->h = tim_data->ch; // height of CLUT in frame buffer
	printf("CLUT info {x=%d, y=%d, w=%d, h=%d}\n", crect->x, crect->y, crect->w, crect->h);
	LoadImage(crect, tim_data->clut);

	// Clean up
	free3(rect);
	free3(crect);
	free3(tim_data);
}