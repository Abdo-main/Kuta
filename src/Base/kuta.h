#pragma  once

#include "main.h"

void kuta_init(Models *models, State *state, Config *config, BufferData *buffer_data);

void renderer_init(Models *models, State *state);

void renderer_deinit(Models *models, State *state, BufferData *buffer_data);

void kuta_deinit(Models *models, State *state, BufferData *buffer_data);
void kuta_loop(Models *models, State *state, Config *config, BufferData *buffer_data);

