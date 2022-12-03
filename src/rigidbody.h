#pragma once

#include <ode/ode.h>

typedef struct transform_component_t transform_component_t;

//Handle to a rigidbody component that contains a transform component and the ID of the rigidbody
typedef struct rigidbody_component_t
{
	dBodyID rigidbody;
} rigidbody_component_t;

typedef struct vec3f_t vec3f_t;

typedef struct quatf_t quatf_t;

typedef struct mat4f_t mat4f_t;

//Creates a rigidbody given the worldID, a reference to a rigidbody component, and a transform component
void create_rigidbody(dWorldID world, rigidbody_component_t* rb);

//Destroys a given rigidbody
void destroy_rigid_body(rigidbody_component_t* rb);

//Sets the position of a rigidbody
void set_rigidbody_position(rigidbody_component_t* rb, vec3f_t position);

//Sets the rotation of a rigidbody with a quaternion
void set_rigidbody_quaternion(rigidbody_component_t* rb, quatf_t quaternion);

//Sets the rotation of a rigidbody with a vector3 of euler angles
void set_rigidbody_rotation(rigidbody_component_t* rb, vec3f_t rotation);

//Set the linear velocity of a rigidbody
void set_rigidbody_linear_velocity(rigidbody_component_t* rb, vec3f_t velocity);

//Set the angular velocity of a rigidbody
void set_rigidbody_angular_velocity(rigidbody_component_t* rb, vec3f_t velocity);

//Set the mass of a rigidbody
void set_rigidbody_mass_amount(rigidbody_component_t* rb, float mass);

//Set the mass, center of mass, and inertia tensor of a rigidbody
void set_rigidbody_mass(rigidbody_component_t* rb, float mass, vec3f_t center_of_mass, mat4f_t inertia_tensor);

//Sets the rigidbody to be dynamic
void set_rigidbody_dynamic(rigidbody_component_t* rb);

//Sets the rigidbody to be kinematic
void set_rigidbody_kinematic(rigidbody_component_t* rb);

//Set the linear damping of the rididbody
void set_rigidbody_linear_damping(rigidbody_component_t* rb, float damping);

//Set the angular damping of the rigidbody
void set_rigidbody_angular_damping(rigidbody_component_t* rb, float damping);

//Adds a vector force on a rigidbody
void add_rigidbody_force(rigidbody_component_t* rb, vec3f_t force);

//Adds a vector torque to a rigidbody
void add_rigidbody_torque(rigidbody_component_t* rb, vec3f_t torque);

//Adds a vector force on a rigidbody at a specified point 
void add_rigidbody_force_at_position(rigidbody_component_t* rb, vec3f_t force, vec3f_t position);

//Returns the position of a rigidbody
vec3f_t get_rigidbody_position(rigidbody_component_t* rb);

//Returns the rotation of a rigidbody as a quaternion
quatf_t get_rigidbody_quaternion(rigidbody_component_t* rb);

//Returns the rotation of a rigidbody as a vector
vec3f_t get_rigidbody_rotation(rigidbody_component_t* rb);

//Returns the linear velocity of a rigidbody
vec3f_t get_rigidbody_linear_velocity(rigidbody_component_t* rb);

//Returns the angular velocity of a rigidbody
vec3f_t get_rigidbody_angular_velocity(rigidbody_component_t* rb);

//Returns the mass of the rigidbody
float get_rigidbody_mass(rigidbody_component_t* rb);

//Returns 0 if the rigidbody is dynamic, 1 if it's kinematic
int get_rigidbody_is_kinematic(rigidbody_component_t* rb);

//Enables the rigidbody so it participates in the simulation
void rigidbody_enable(rigidbody_component_t* rb);

//Disables the rigidbody so it doesn't participates in the simulation
void rigidbody_disable(rigidbody_component_t* rb);

//Returns 0 if the rigidbody is disabled, 1 if it's enabled
int get_rigidbody_enabled(rigidbody_component_t* rb);

//Returns the linear damping of the rigidbody
float get_rigidbody_linear_damping(rigidbody_component_t* rb);

//Returns the angular damping of the rigidbody
float get_rigidbody_angular_damping(rigidbody_component_t* rb);
