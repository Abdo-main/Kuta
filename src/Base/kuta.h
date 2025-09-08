#pragma  once

#include "main.h"

bool kuta_init(Settings *settings);

void renderer_init(void);

void load_glbs(const char* models_files[]);

void load_texture(const char* textures_files[]);

void renderer_deinit(void);

void begin_frame(void);

void end_frame(void);

bool running();

void kuta_deinit(void);


