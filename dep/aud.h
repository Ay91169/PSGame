#ifndef AUD_H
#define AUD_H

#include <libspu.h>

int find_xa_file(const char *filename);

static int  CurPos;

void play_xa_audio(char *filename);
void testaud();

#endif







