#include "transform.h"
#include "rigidbody.h"

void create_rigidbody(dWorldID world, rigidbody_component_t* rb)
{
	rb->rigidbody = dBodyCreate(world);
}

void destroy_rigid_body(rigidbody_component_t* rb)
{
	dBodyDestroy(rb->rigidbody);
}

void set_rigidbody_position(rigidbody_component_t* rb, vec3f_t position)
{
	dBodySetPosition(rb->rigidbody, position.x, position.y, position.z);
}

void set_rigidbody_quaternion(rigidbody_component_t* rb, quatf_t quaternion)
{
	dQuaternion quat = {quaternion.x, quaternion.y, quaternion.z, quaternion.w};
	dBodySetQuaternion(rb->rigidbody, quat);
}

void set_rigidbody_rotation(rigidbody_component_t* rb, vec3f_t rotation)
{
	dMatrix3 rot_mat;
	dRFromEulerAngles(rot_mat, rotation.x, rotation.y, rotation.z);
	dBodySetRotation(rb->rigidbody, rot_mat);
}

void set_rigidbody_linear_velocity(rigidbody_component_t* rb, vec3f_t velocity)
{
	dBodySetLinearVel(rb->rigidbody, velocity.x, velocity.y, velocity.z);
}

void set_rigidbody_angular_velocity(rigidbody_component_t* rb, vec3f_t velocity)
{
	dBodySetAngularVel(rb->rigidbody, velocity.x, velocity.y, velocity.z);
}

void set_rigidbody_mass_amount(rigidbody_component_t* rb, float mass)
{
	dMass cur_mass;
	dBodyGetMass(rb->rigidbody, &cur_mass);
	cur_mass.mass = mass;
	dBodySetMass(rb->rigidbody, &cur_mass);
}

void set_rigidbody_mass(rigidbody_component_t* rb, float mass, vec3f_t center_of_mass, mat4f_t inertia_tensor)
{
	dMass cur_mass;
	dBodyGetMass(rb->rigidbody, &cur_mass);
	dMassSetParameters(&cur_mass, mass, center_of_mass.x, center_of_mass.y, center_of_mass.z, inertia_tensor.data[0][0],
			     		inertia_tensor.data[1][1], inertia_tensor.data[2][2], inertia_tensor.data[0][1], inertia_tensor.data[0][2], inertia_tensor.data[1][2]);
}

void set_rigidbody_dynamic(rigidbody_component_t* rb)
{
	dBodySetDynamic(rb->rigidbody);
}

void set_rigidbody_kinematic(rigidbody_component_t* rb)
{
	dBodySetKinematic(rb->rigidbody);
}

void set_rigidbody_linear_damping(rigidbody_component_t* rb, float damping)
{
	dBodySetLinearDamping(rb->rigidbody, damping);
}

void set_rigidbody_angular_damping(rigidbody_component_t* rb, float damping)
{
	dBodySetAngularDamping(rb->rigidbody, damping);
}

void add_rigidbody_force(rigidbody_component_t* rb, vec3f_t force)
{
	dBodyAddForce(rb->rigidbody, force.x, force.y, force.z);
}

void add_rigidbody_torque(rigidbody_component_t* rb, vec3f_t torque)
{
	dBodyAddTorque(rb->rigidbody, torque.x, torque.y, torque.z);
}

void add_rigidbody_force_at_position(rigidbody_component_t* rb, vec3f_t force, vec3f_t position)
{
	dBodyAddForceAtPos(rb->rigidbody, force.x, force.y, force.z, position.x , position.y, position.z);
}

vec3f_t get_rigidbody_position(rigidbody_component_t* rb)
{
	const dReal* pos = dBodyGetPosition(rb->rigidbody);
	vec3f_t vec_pos = (vec3f_t){ .x = pos[0], .y = pos[1], .z = pos[2] };
	return vec_pos;
}

quatf_t get_rigidbody_quaternion(rigidbody_component_t* rb)
{
	const dReal* quat = dBodyGetQuaternion(rb->rigidbody);
	quatf_t my_quat = { .x = quat[0], .y = quat[1], .z = quat[2], .w = quat[3] };
	return my_quat;
}

vec3f_t get_rigidbody_rotation(rigidbody_component_t* rb)
{
	quatf_t	quat = get_rigidbody_quaternion(rb);
	return quatf_to_eulers(quat);
}

vec3f_t get_rigidbody_linear_velocity(rigidbody_component_t* rb)
{
	const dReal* vel = dBodyGetLinearVel(rb->rigidbody);
	vec3f_t vec_vel = (vec3f_t){ .x = vel[0], .y = vel[1], .z = vel[2] };
	return vec_vel;
}

vec3f_t get_rigidbody_angular_velocity(rigidbody_component_t* rb)
{
	const dReal* vel = dBodyGetAngularVel(rb->rigidbody);
	vec3f_t vec_vel = (vec3f_t){ .x = vel[0], .y = vel[1], .z = vel[2] };
	return vec_vel;
}

float get_rigidbody_mass(rigidbody_component_t* rb)
{
	dMass mass;
	dBodyGetMass(rb->rigidbody, &mass);
	return mass.mass;
}

int get_rigidbody_is_kinematic(rigidbody_component_t* rb)
{
	return dBodyIsKinematic(rb->rigidbody);
}

void rigidbody_enable(rigidbody_component_t* rb)
{
	dBodyEnable(rb->rigidbody);
}

void rigidbody_disable(rigidbody_component_t* rb)
{
	dBodyDisable(rb->rigidbody);
}

int get_rigidbody_enabled(rigidbody_component_t* rb)
{
	return dBodyIsEnabled(rb->rigidbody);
}

float get_rigidbody_linear_damping(rigidbody_component_t* rb)
{
	return dBodyGetLinearDamping(rb->rigidbody);
}

float get_rigidbody_angular_damping(rigidbody_component_t* rb)
{
	return dBodyGetAngularDamping(rb->rigidbody);
}