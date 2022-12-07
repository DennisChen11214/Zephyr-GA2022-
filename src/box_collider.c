#include "box_collider.h"
#include "rigidbody.h"
#include "transform.h"

#include <ode/ode.h>

void create_box_collider(box_collider_component_t* box, rigidbody_component_t* rb, dSpaceID space, float length, float width, float height)
{
	dMass m;
	dMassSetBox(&m, 5, length, width, height);
	box->box_collider = dCreateBox(space, length, width, height);
	dGeomSetBody(box->box_collider, rb->rigidbody);
	dBodySetMass(rb->rigidbody, &m);
	box->rb = rb;
}

void destroy_box_collider(box_collider_component_t* box)
{
	dGeomDestroy(box->box_collider);
}

void set_box_lengths(box_collider_component_t* box, float length, float width, float height)
{
	dGeomBoxSetLengths(box->box_collider, length, width, height);
}

vec3f_t get_box_lengths(box_collider_component_t* box)
{
	dReal length[3];
	dGeomBoxGetLengths(box->box_collider, length);
	vec3f_t lengths = (vec3f_t){ .x = length[0], .y = length[1], .z = length[2] };
	return lengths;
}

void enable_box_collider(box_collider_component_t* box)
{
	dGeomEnable(box->box_collider);
}

void disable_box_collider(box_collider_component_t* box)
{
	dGeomDisable(box->box_collider);
}

int check_collide(box_collider_component_t* col1, box_collider_component_t* col2) 
{
	dContact contact[8];
	int num_contacts = dCollide(col1->box_collider, col2->box_collider, 8, &contact[0].geom, sizeof(dContact));
	return num_contacts;
}