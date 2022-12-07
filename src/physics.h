#pragma once
#include "final_game.h"
#include "timer_object.h"

#include <ode/ode.h>

typedef struct dynamics_t
{
	dWorldID world;
	dSpaceID space;
	dJointGroupID contact_group;
} dynamics_t;

//Initializes a physics world
void physics_initialize(dynamics_t* dynamics);

//Ends a physics world
void physics_end(dynamics_t* dynamics);

//Goes through a certain amount of physics updates, calling the given function each physics frame with the given argument
void physics_fixed_update(dynamics_t* dynamics, timer_object_t* timer, void(*func)(void*), void* data);

//Spawns a floor plane at a specified z-value
void physics_spawn_floor(dSpaceID space, float z_value);