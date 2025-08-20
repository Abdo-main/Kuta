#pragma once

#include "main.h"

void create_instance(State *state);
void select_physical_device(State *state);
void create_surface(State *state);
void select_queue_family(State *state);
void create_device(State *state);
void get_queue(State *state);
