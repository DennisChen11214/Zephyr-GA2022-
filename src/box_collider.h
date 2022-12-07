#pragma once

#include <ode/ode.h>

typedef struct rigidbody_component_t rigidbody_component_t;

//Handle to a box collider component that contains a corresponding rigid body component and the ID of the collider
typedef struct box_collider_component_t
{
	rigidbody_component_t* rb;
	dGeomID box_collider;
} box_collider_component_t;

typedef struct vec3f_t vec3f_t;

//Creates a box collider with a given rigidbody, space, and side lengths
void create_box_collider(box_collider_component_t* box, rigidbody_component_t* rb, dSpaceID space, float length, float width, float height);

//Destroys a box collider
void destroy_box_collider(box_collider_component_t* box);

//Sets the side lengths of a box collider
void set_box_lengths(box_collider_component_t* box, float length, float width, float height);

//Returns a vector of the side lengths of a box collider
vec3f_t get_box_lengths(box_collider_component_t* box);

//Enables a box collider to be considered for collisions
void enable_box_collider(box_collider_component_t* box);

//Disables a box collider and has it be ignored when doing collisions
void disable_box_collider(box_collider_component_t* box);

//Returns the number of points at which the 2 objects collide
int check_collide(box_collider_component_t* col1, box_collider_component_t* col2);