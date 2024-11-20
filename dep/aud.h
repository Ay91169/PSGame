#ifndef AUD_H
#define AUD_H

#include <LIBSPU.H>

// Initialize the SPU system
void init_audio_system();

// Load a sound effect into SPU memory
void load_sound(unsigned char *adpcm_data, int size, int voice_channel);

// Play a sound effect on a specific channel
void play_sound(int voice_channel);

// Play XA audio for background music
void play_xa_audio();

#endif // AUDIO_H