#include <libgs.h>


#define MAXOBJ 100
extern GsDOBJ2	Object[];
extern int ObjectCount;
int LoadTMD(u_long *tmd, GsDOBJ2 *obj, int enableLighting);
void RenderObject(VECTOR pos, SVECTOR rot, GsDOBJ2 *obj);