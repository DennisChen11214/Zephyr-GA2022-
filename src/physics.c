#include "physics.h"

static void near_callback(void* data, dGeomID o1, dGeomID o2);

void physics_initialize(dynamics_t* dynamics)
{
	dInitODE2(0);
	dynamics->world = dWorldCreate();
	dynamics->space = dHashSpaceCreate(0);
	dynamics->contact_group = dJointGroupCreate(0);
	dWorldSetGravity(dynamics->world, 0, 0, -9.81f);
	dWorldSetCFM(dynamics->world, 1e-5f);
	dWorldSetContactMaxCorrectingVel(dynamics->world, 0.1f);
	dWorldSetContactSurfaceLayer(dynamics->world, 0.001f);
	dWorldSetLinearDamping(dynamics->world, 0.00001f);
	dWorldSetAngularDamping(dynamics->world, 0.005f);
	dWorldSetMaxAngularSpeed(dynamics->world, 200);

	dThreadingImplementationID threading = dThreadingAllocateMultiThreadedImplementation();
	dThreadingThreadPoolID pool = dThreadingAllocateThreadPool(4, 0, dAllocateFlagBasicData, NULL);
	dThreadingThreadPoolServeMultiThreadedImplementation(pool, threading);
	dWorldSetStepThreadingImplementation(dynamics->world, dThreadingImplementationGetFunctions(threading), threading);
}

void physics_end(dynamics_t* dynamics)
{
	dJointGroupEmpty(dynamics->contact_group);
	dJointGroupDestroy(dynamics->contact_group);
	dSpaceDestroy(dynamics->space);
	dWorldDestroy(dynamics->world);
	dCloseODE();
}


void physics_fixed_update(dynamics_t* dynamics, timer_object_t* timer, void(*func)(void*), void* data)
{
	float simstep = 1.0f / 60.0f; // 60 simulation steps per second
	float dt = (float)timer_object_get_delta_ms(timer) * 0.001f;

	int nrofsteps = (int)ceilf(dt / simstep);

	for (int i = 0; i < nrofsteps; i++)
	{
		//Calls the given function each physics frame
		if (func != NULL) 
		{
			func(data);
		}
		dSpaceCollide(dynamics->space, dynamics, &near_callback);
		dWorldQuickStep(dynamics->world, simstep);
		dJointGroupEmpty(dynamics->contact_group);
	}
}

void physics_spawn_floor(dSpaceID space, float z_value)
{
	dCreatePlane(space, 0, 0, -z_value, 0);
}

//Handles natural collision between objects
static void near_callback(void* data, dGeomID o1, dGeomID o2)
{
	dWorldID world = ((dynamics_t*)data)->world;
	dJointGroupID contact_group = ((dynamics_t*)data)->contact_group;
	dBodyID b1 = dGeomGetBody(o1);
	dBodyID b2 = dGeomGetBody(o2);

	dContact contact[8];
	for (int i = 0; i < 8; i++) {
		contact[i].surface.mode = dContactBounce | dContactSoftCFM;
		contact[i].surface.mu = 0;
		contact[i].surface.mu2 = 0;
		contact[i].surface.bounce = 0.01f;
		contact[i].surface.bounce_vel = 0.1f;
		contact[i].surface.soft_cfm = 0.0001f;
	}
	int numc = dCollide(o1, o2, 8, &contact[0].geom, sizeof(dContact));
	if (numc > 0) {
		for (int i = 0; i < numc; i++)
		{
			dJointID c = dJointCreateContact(world, contact_group, contact + i);
			dJointAttach(c, b1, b2);
		}
	}
}