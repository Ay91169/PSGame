#include <libgpu.h>
#include <libgte.h>
#include <libgs.h>


typedef struct Model
{
    VECTOR POS;
    SVECTOR ROT;
    char name[];
};
