#include "ecs.h"
#include "fs.h"
#include "gpu.h"
#include "heap.h"
#include "render.h"
#include "timer_object.h"
#include "transform.h"
#include "wm.h"
#include "rigidbody.h"
#include "box_collider.h"
#include "debug.h"

#include <ode/ode.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

typedef struct camera_component_t
{
	mat4f_t projection;
	mat4f_t view;
} camera_component_t;

typedef struct model_component_t
{
	gpu_mesh_info_t* mesh_info;
	gpu_shader_info_t* shader_info;
} model_component_t;

typedef struct player_component_t
{
	int index;
} player_component_t;

typedef struct traffic_component_t
{
	float scale;
	int lane;
} traffic_component_t;

typedef struct name_component_t
{
	char name[32];
} name_component_t;

typedef struct dynamics_t
{
	dWorldID world;
	dSpaceID space;
	dJointGroupID contact_group;
} dynamics_t;

typedef struct final_game_t
{
	heap_t* heap;
	fs_t* fs;
	wm_window_t* window;
	render_t* render;

	timer_object_t* timer;

	dynamics_t* dynamics;

	float aspect;
	float height;
	float width;

	ecs_t* ecs;
	int transform_type;
	int camera_type;
	int model_type;
	int player_type;
	int traffic_type;
	int name_type;
	int rigidbody_type;
	int box_collider_type;

	ecs_entity_ref_t player_ent;
	ecs_entity_ref_t lane1_ents[5];
	ecs_entity_ref_t lane2_ents[5];
	ecs_entity_ref_t lane3_ents[5];
	ecs_entity_ref_t camera_ent;

	gpu_mesh_info_t player_mesh;
	gpu_mesh_info_t traffic1_mesh;
	gpu_mesh_info_t traffic2_mesh;
	gpu_mesh_info_t traffic3_mesh;
	gpu_shader_info_t cube_shader;
	gpu_shader_info_t traffic2_shader;
	gpu_shader_info_t traffic3_shader;
	fs_work_t* vertex_shader_work;
	fs_work_t* fragment_shader_work;
} final_game_t;

static void load_resources(final_game_t* game);
static void unload_resources(final_game_t* game);
static void spawn_player(final_game_t* game, int index);
static void spawn_camera(final_game_t* game);
static void spawn_traffic(final_game_t* game);
static void update_players(final_game_t* game);
static void update_traffic(final_game_t* game);
static void draw_models(final_game_t* game);


final_game_t* final_game_create(heap_t* heap, fs_t* fs, wm_window_t* window, render_t* render, float aspect, float height)
{
	final_game_t* game = heap_alloc(heap, sizeof(final_game_t), 8);
	game->heap = heap;
	game->fs = fs;
	game->window = window;
	game->render = render;

	game->timer = timer_object_create(heap, NULL);
	
	game->aspect = aspect;
	game->height = height;
	game->width = aspect * height;

	game->ecs = ecs_create(heap);
	game->transform_type = ecs_register_component_type(game->ecs, "transform", sizeof(transform_component_t), _Alignof(transform_component_t));
	game->camera_type = ecs_register_component_type(game->ecs, "camera", sizeof(camera_component_t), _Alignof(camera_component_t));
	game->model_type = ecs_register_component_type(game->ecs, "model", sizeof(model_component_t), _Alignof(model_component_t));
	game->player_type = ecs_register_component_type(game->ecs, "player", sizeof(player_component_t), _Alignof(player_component_t));
	game->traffic_type = ecs_register_component_type(game->ecs, "traffic", sizeof(traffic_component_t), _Alignof(traffic_component_t));
	game->name_type = ecs_register_component_type(game->ecs, "name", sizeof(name_component_t), _Alignof(name_component_t));
	game->rigidbody_type = ecs_register_component_type(game->ecs, "rigidbody", sizeof(rigidbody_component_t), _Alignof(rigidbody_component_t));
	game->box_collider_type = ecs_register_component_type(game->ecs, "box_collider", sizeof(box_collider_component_t), _Alignof(box_collider_component_t));

	//Physics-related initialization
	game->dynamics = heap_alloc(heap, sizeof(dynamics_t), 8);
	dInitODE();
	game->dynamics->world = dWorldCreate();
	game->dynamics->space = dHashSpaceCreate(0);
	game->dynamics->contact_group = dJointGroupCreate(0);
	//dWorldSetGravity(game->dynamics->world, 0, 0, -9.8f);

	load_resources(game);
	spawn_player(game, 0);
	spawn_traffic(game);
	spawn_camera(game);

	return game;

}

void final_game_destroy(final_game_t* game)
{
	/*dGeomDestroy(sphgeom);
	dGeomDestroy(world_mesh);*/

	dJointGroupEmpty(game->dynamics->contact_group);
	dJointGroupDestroy(game->dynamics->contact_group);
	dSpaceDestroy(game->dynamics->space);
	dWorldDestroy(game->dynamics->world);
	dCloseODE();
	heap_free(game->heap, game->dynamics);

	ecs_destroy(game->ecs);
	timer_object_destroy(game->timer);
	unload_resources(game);
	heap_free(game->heap, game);
}

void final_game_fixed_update(final_game_t* game)
{
	float simstep = 1.0f / 60.0f; // 60 simulation steps per second
	float dt = (float)timer_object_get_delta_ms(game->timer) * 0.001f;

	int nrofsteps = (int)ceilf(dt / simstep);

	for (int i = 0; i < nrofsteps; i++)
	{
		uint32_t key_mask = wm_get_key_mask(game->window);

		uint64_t k_query_mask = (1ULL << game->transform_type) | (1ULL << game->player_type);

		uint64_t k2_query_mask = (1ULL << game->traffic_type);
		for (ecs_query_t query = ecs_query_create(game->ecs, k_query_mask, 0);
			ecs_query_is_valid(game->ecs, &query);
			ecs_query_next(game->ecs, &query))
		{
			box_collider_component_t* player_col = ecs_query_get_component(game->ecs, &query, game->box_collider_type);
			transform_component_t* player_transform = ecs_query_get_component(game->ecs, &query, game->transform_type);
			rigidbody_component_t* player_rb = ecs_query_get_component(game->ecs, &query, game->rigidbody_type);
			for (ecs_query_t query2 = ecs_query_create(game->ecs, k2_query_mask, 0);
				ecs_query_is_valid(game->ecs, &query2);
				ecs_query_next(game->ecs, &query2))
			{
				box_collider_component_t* traffic_col = ecs_query_get_component(game->ecs, &query2, game->box_collider_type);
				dContactGeom contacts[32];
				int num_contacts = dCollide(player_col->box_collider, traffic_col->box_collider, 32, contacts, sizeof(dContactGeom));
				if (num_contacts > 0) 
				{
					player_transform->transform.translation.z = game->height - 1;
					set_rigidbody_position(player_rb, player_transform->transform.translation);
				}
			}
		}

		dWorldQuickStep(game->dynamics->world, simstep);
		dJointGroupEmpty(game->dynamics->contact_group);
	}
}

void final_game_update(final_game_t* game)
{
	timer_object_update(game->timer);
	final_game_fixed_update(game);
	ecs_update(game->ecs);
	update_players(game);
	update_traffic(game);
	draw_models(game);
	render_push_done(game->render);
}

static void load_resources(final_game_t* game)
{
	game->vertex_shader_work = fs_read(game->fs, "shaders/triangle.vert.spv", game->heap, false, false);
	game->fragment_shader_work = fs_read(game->fs, "shaders/triangle.frag.spv", game->heap, false, false);
	game->cube_shader = (gpu_shader_info_t)
	{
		.vertex_shader_data = fs_work_get_buffer(game->vertex_shader_work),
		.vertex_shader_size = fs_work_get_size(game->vertex_shader_work),
		.fragment_shader_data = fs_work_get_buffer(game->fragment_shader_work),
		.fragment_shader_size = fs_work_get_size(game->fragment_shader_work),
		.uniform_buffer_count = 1,
	};

	static vec3f_t player_verts[] =
	{
		{ -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f,  0.0f },
		{  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f,  0.0f },
		{  1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f,  0.0f },
		{ -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f,  0.0f },
		{ -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f,  0.0f },
		{  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f,  0.0f },
		{  1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f,  0.0f },
		{ -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f,  0.0f },
	};
	static uint16_t cube_indices[] =
	{
		0, 1, 2,
		2, 3, 0,
		1, 5, 6,
		6, 2, 1,
		7, 6, 5,
		5, 4, 7,
		4, 0, 3,
		3, 7, 4,
		4, 5, 1,
		1, 0, 4,
		3, 2, 6,
		6, 7, 3
	};
	game->player_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = player_verts,
		.vertex_data_size = sizeof(player_verts),
		.index_data = cube_indices,
		.index_data_size = sizeof(cube_indices),
	};

	static vec3f_t traffic1_verts[] =
	{
		{ -1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f,  0.0f },
		{  1.0f, -1.0f,  1.0f }, { 1.0f, 0.0f,  0.0f },
		{  1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f,  0.0f },
		{ -1.0f,  1.0f,  1.0f }, { 1.0f, 0.0f,  0.0f },
		{ -1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f,  0.0f },
		{  1.0f, -1.0f, -1.0f }, { 1.0f, 0.0f,  0.0f },
		{  1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f,  0.0f },
		{ -1.0f,  1.0f, -1.0f }, { 1.0f, 0.0f,  0.0f },
	};

	game->traffic1_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = traffic1_verts,
		.vertex_data_size = sizeof(traffic1_verts),
		.index_data = cube_indices,
		.index_data_size = sizeof(cube_indices),
	};

	static vec3f_t traffic2_verts[] =
	{
		{ -1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f,  1.0f },
		{  1.0f, -1.0f,  1.0f }, { 0.0f, 0.0f,  1.0f },
		{  1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f,  1.0f },
		{ -1.0f,  1.0f,  1.0f }, { 0.0f, 0.0f,  1.0f },
		{ -1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f,  1.0f },
		{  1.0f, -1.0f, -1.0f }, { 0.0f, 0.0f,  1.0f },
		{  1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f,  1.0f },
		{ -1.0f,  1.0f, -1.0f }, { 0.0f, 0.0f,  1.0f },
	};

	game->traffic2_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = traffic2_verts,
		.vertex_data_size = sizeof(traffic2_verts),
		.index_data = cube_indices,
		.index_data_size = sizeof(cube_indices),
	};

	static vec3f_t traffic3_verts[] =
	{
		{ -1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f,  1.0f },
		{  1.0f, -1.0f,  1.0f }, { 0.0f, 1.0f,  1.0f },
		{  1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f,  1.0f },
		{ -1.0f,  1.0f,  1.0f }, { 0.0f, 1.0f,  1.0f },
		{ -1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f,  1.0f },
		{  1.0f, -1.0f, -1.0f }, { 0.0f, 1.0f,  1.0f },
		{  1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f,  1.0f },
		{ -1.0f,  1.0f, -1.0f }, { 0.0f, 1.0f,  1.0f },
	};

	game->traffic3_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = traffic3_verts,
		.vertex_data_size = sizeof(traffic3_verts),
		.index_data = cube_indices,
		.index_data_size = sizeof(cube_indices),
	};
}

static void unload_resources(final_game_t* game)
{
	fs_work_destroy(game->fragment_shader_work);
	fs_work_destroy(game->vertex_shader_work);
}

static void spawn_player(final_game_t* game, int index)
{
	uint64_t k_player_ent_mask =
		(1ULL << game->transform_type) |
		(1ULL << game->model_type) |
		(1ULL << game->player_type) |
		(1ULL << game->name_type);
	game->player_ent = ecs_entity_add(game->ecs, k_player_ent_mask);

	transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->player_ent, game->transform_type, true);
	transform_identity(&transform_comp->transform);
	transform_comp->transform.translation.z = game->height - 1;

	name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->player_ent, game->name_type, true);
	strcpy_s(name_comp->name, sizeof(name_comp->name), "player");

	player_component_t* player_comp = ecs_entity_get_component(game->ecs, game->player_ent, game->player_type, true);
	player_comp->index = index;

	model_component_t* model_comp = ecs_entity_get_component(game->ecs, game->player_ent, game->model_type, true);
	model_comp->mesh_info = &game->player_mesh;
	model_comp->shader_info = &game->cube_shader;

	rigidbody_component_t* rigidbody_comp = ecs_entity_get_component(game->ecs, game->player_ent, game->rigidbody_type, true);
	create_rigidbody(game->dynamics->world, rigidbody_comp);

	set_rigidbody_position(rigidbody_comp, transform_comp->transform.translation);

	box_collider_component_t* box_collider_comp = ecs_entity_get_component(game->ecs, game->player_ent, game->box_collider_type, true);
	create_box_collider(box_collider_comp, rigidbody_comp, 2, 2, 2);
	dSpaceAdd(game->dynamics->space, box_collider_comp->box_collider);
}

static void spawn_traffic(final_game_t* game)
{
	uint64_t k_traffic_ent_mask =
		(1ULL << game->transform_type) |
		(1ULL << game->model_type) |
		(1ULL << game->traffic_type) |
		(1ULL << game->name_type);

	float lane1_types[_countof(game->lane1_ents)] = { 3.0f, 1.0f, 3.0f, 2.0f, 2.0f };
	float lane2_types[_countof(game->lane2_ents)] = { 1.0f, 2.0f, 3.0f, 1.0f, 3.0f };
	float lane3_types[_countof(game->lane3_ents)] = { 1.0f, 3.0f, 2.0f, 2.0f, 1.0f };

	//Spawn all the traffic lanes according to the type array initialized earlier
	for (int i = 0; i < _countof(game->lane1_ents); i++)
	{
		game->lane1_ents[i] = ecs_entity_add(game->ecs, k_traffic_ent_mask);
		transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->transform_type, true);
		transform_identity(&transform_comp->transform);
		transform_comp->transform.translation.z = game->height - 3.5f;
		if (i == 0) 
		{
			transform_comp->transform.translation.y = -game->width + lane1_types[0] + 2;
		}
		else 
		{
			transform_component_t* prev_transform_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i - 1], game->transform_type, true);
			transform_comp->transform.translation.y = prev_transform_comp->transform.translation.y + lane1_types[i - 1] + 4 + lane1_types[i];
		}
		transform_comp->transform.scale.y = lane1_types[i];

		traffic_component_t* traffic_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->traffic_type, true);
		traffic_comp->scale = lane1_types[i];
		traffic_comp->lane = 1;
		
		name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->name_type, true);

		char index[2];
		sprintf_s(index, sizeof(index), "%d", i);
		strcpy_s(name_comp->name, sizeof(name_comp->name), "lane1_");
		strcat_s(name_comp->name, sizeof(name_comp->name), index);

		model_component_t* model_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->model_type, true);
		if (lane1_types[i] == 1)
		{
			model_comp->mesh_info = &game->traffic1_mesh;
		}
		else if (lane1_types[i] == 2)
		{
			model_comp->mesh_info = &game->traffic2_mesh;
		}
		else 
		{
			model_comp->mesh_info = &game->traffic3_mesh;
		}
		model_comp->shader_info = &game->cube_shader;

		rigidbody_component_t* rigidbody_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->rigidbody_type, true);
		create_rigidbody(game->dynamics->world, rigidbody_comp);
		set_rigidbody_position(rigidbody_comp, transform_comp->transform.translation);
		float speed = -2.0f;
		set_rigidbody_linear_velocity(rigidbody_comp, vec3f_scale(vec3f_right(), speed));

		box_collider_component_t* box_collider_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->box_collider_type, true);
		create_box_collider(box_collider_comp, rigidbody_comp, 2, lane1_types[i] * 2, 2);
		dSpaceAdd(game->dynamics->space, box_collider_comp->box_collider);
	}
	for (int i = 0; i < _countof(game->lane2_ents); i++)
	{
		game->lane2_ents[i] = ecs_entity_add(game->ecs, k_traffic_ent_mask);
		transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->lane2_ents[i], game->transform_type, true);
		transform_identity(&transform_comp->transform);
		transform_comp->transform.translation.z = game->height - 8.0f;
		if (i == 0)
		{
			transform_comp->transform.translation.y = -game->width + lane2_types[0] + 2;
		}
		else
		{
			transform_component_t* prev_transform_comp = ecs_entity_get_component(game->ecs, game->lane2_ents[i - 1], game->transform_type, true);
			transform_comp->transform.translation.y = prev_transform_comp->transform.translation.y + lane2_types[i - 1] + 4 + lane2_types[i];
		}
		transform_comp->transform.scale.y = lane2_types[i];

		traffic_component_t* traffic_comp = ecs_entity_get_component(game->ecs, game->lane2_ents[i], game->traffic_type, true);
		traffic_comp->scale = lane2_types[i];
		traffic_comp->lane = 2;
		
		name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->name_type, true);

		char index[2];
		sprintf_s(index, sizeof(index), "%d", i);
		strcpy_s(name_comp->name, sizeof(name_comp->name), "lane2_");
		strcat_s(name_comp->name, sizeof(name_comp->name), index);
		
		model_component_t* model_comp = ecs_entity_get_component(game->ecs, game->lane2_ents[i], game->model_type, true);
		if (lane2_types[i] == 1)
		{
			model_comp->mesh_info = &game->traffic1_mesh;
		}
		else if (lane2_types[i] == 2)
		{
			model_comp->mesh_info = &game->traffic2_mesh;
		}
		else
		{
			model_comp->mesh_info = &game->traffic3_mesh;
		}
		model_comp->shader_info = &game->cube_shader;

		rigidbody_component_t* rigidbody_comp = ecs_entity_get_component(game->ecs, game->lane2_ents[i], game->rigidbody_type, true);
		create_rigidbody(game->dynamics->world, rigidbody_comp);
		set_rigidbody_position(rigidbody_comp, transform_comp->transform.translation);
		float speed = -2.0f;
		set_rigidbody_linear_velocity(rigidbody_comp, vec3f_scale(vec3f_right(), speed));

		box_collider_component_t* box_collider_comp = ecs_entity_get_component(game->ecs, game->lane2_ents[i], game->box_collider_type, true);
		create_box_collider(box_collider_comp, rigidbody_comp, 2, lane2_types[i] * 2, 2);
		dSpaceAdd(game->dynamics->space, box_collider_comp->box_collider);
	}
	for (int i = 0; i < _countof(game->lane3_ents); i++)
	{
		game->lane3_ents[i] = ecs_entity_add(game->ecs, k_traffic_ent_mask);
		transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->lane3_ents[i], game->transform_type, true);
		transform_identity(&transform_comp->transform);
		transform_comp->transform.translation.z = game->height - 12.5f;
		if (i == 0)
		{
			transform_comp->transform.translation.y = -game->width + lane3_types[0] + 2;
		}
		else
		{
			transform_component_t* prev_transform_comp = ecs_entity_get_component(game->ecs, game->lane3_ents[i - 1], game->transform_type, true);
			transform_comp->transform.translation.y = prev_transform_comp->transform.translation.y + lane3_types[i - 1] + 4 + lane3_types[i];
		}
		transform_comp->transform.scale.y = lane3_types[i];
		
		traffic_component_t* traffic_comp = ecs_entity_get_component(game->ecs, game->lane3_ents[i], game->traffic_type, true);
		traffic_comp->scale = lane3_types[i];
		traffic_comp->lane = 3;

		name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->lane1_ents[i], game->name_type, true);

		char index[2];
		sprintf_s(index, sizeof(index), "%d", i);
		strcpy_s(name_comp->name, sizeof(name_comp->name), "lane3_");
		strcat_s(name_comp->name, sizeof(name_comp->name), index);

		model_component_t* model_comp = ecs_entity_get_component(game->ecs, game->lane3_ents[i], game->model_type, true);
		if (lane3_types[i] == 1)
		{
			model_comp->mesh_info = &game->traffic1_mesh;
		}
		else if (lane3_types[i] == 2)
		{
			model_comp->mesh_info = &game->traffic2_mesh;
		}
		else
		{
			model_comp->mesh_info = &game->traffic3_mesh;
		}
		model_comp->shader_info = &game->cube_shader;

		rigidbody_component_t* rigidbody_comp = ecs_entity_get_component(game->ecs, game->lane3_ents[i], game->rigidbody_type, true);
		create_rigidbody(game->dynamics->world, rigidbody_comp);
		set_rigidbody_position(rigidbody_comp, transform_comp->transform.translation);
		float speed = -2.0f;
		set_rigidbody_linear_velocity(rigidbody_comp, vec3f_scale(vec3f_right(), speed));

		box_collider_component_t* box_collider_comp = ecs_entity_get_component(game->ecs, game->lane3_ents[i], game->box_collider_type, true);
		create_box_collider(box_collider_comp, rigidbody_comp, 2, lane3_types[i] * 2, 2);
		dSpaceAdd(game->dynamics->space, box_collider_comp->box_collider);
	}
}

static void spawn_camera(final_game_t* game)
{
	uint64_t k_camera_ent_mask =
		(1ULL << game->camera_type) |
		(1ULL << game->name_type);
	game->camera_ent = ecs_entity_add(game->ecs, k_camera_ent_mask);

	name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->camera_ent, game->name_type, true);
	strcpy_s(name_comp->name, sizeof(name_comp->name), "camera");

	camera_component_t* camera_comp = ecs_entity_get_component(game->ecs, game->camera_ent, game->camera_type, true);
	mat4f_make_orthographic(&camera_comp->projection, game->aspect, game->height * 2, 0.1f, 100.0f);

	vec3f_t eye_pos = vec3f_scale(vec3f_forward(), -5.0f);
	vec3f_t forward = vec3f_forward();
	vec3f_t up = vec3f_up();
	mat4f_make_lookat(&camera_comp->view, &eye_pos, &forward, &up);
}

static void update_players(final_game_t* game)
{
	float dt = (float)timer_object_get_delta_ms(game->timer) * 0.001f;

	uint32_t key_mask = wm_get_key_mask(game->window);

	uint64_t k_query_mask = (1ULL << game->transform_type) | (1ULL << game->player_type);

	for (ecs_query_t query = ecs_query_create(game->ecs, k_query_mask, 0);
		ecs_query_is_valid(game->ecs, &query);
		ecs_query_next(game->ecs, &query))
	{
		transform_component_t* player_transform_comp = ecs_query_get_component(game->ecs, &query, game->transform_type);
		player_component_t* player_comp = ecs_query_get_component(game->ecs, &query, game->player_type);
		rigidbody_component_t* player_rigidbody_comp = ecs_query_get_component(game->ecs, &query, game->rigidbody_type);

		player_transform_comp->transform.translation = get_rigidbody_position(player_rigidbody_comp);

		//Reset the player back to start if they reached the end
		if (player_transform_comp->transform.translation.z < -(game->height - 1))
		{
			player_transform_comp->transform.translation.z = game->height - 1;
			set_rigidbody_position(player_rigidbody_comp, player_transform_comp->transform.translation);
		}

		//Handle input actions and move
		float speed = 4.0f;
		vec3f_t vel = vec3f_zero();
		if (key_mask & k_key_up)
		{
			vel.z += -speed;
		}
		if ((key_mask & k_key_down) && player_transform_comp->transform.translation.z < game->height - 1)
		{
			vel.z += speed;
		}
		if (key_mask & k_key_left)
		{
			vel.y += -speed;
		}
		if (key_mask & k_key_right)
		{
			vel.y += speed;
		}
		set_rigidbody_linear_velocity(player_rigidbody_comp, vel);

	}
}

static void update_traffic(final_game_t* game)
{
	float dt = (float)timer_object_get_delta_ms(game->timer) * 0.001f;

	uint32_t key_mask = wm_get_key_mask(game->window);

	uint64_t k_query_mask = (1ULL << game->traffic_type);

	//Get each lane width
	float lane1_width = 0.0f;
	float lane2_width = 0.0f;
	float lane3_width = 0.0f;
	for (ecs_query_t query = ecs_query_create(game->ecs, k_query_mask, 0);
		ecs_query_is_valid(game->ecs, &query);
		ecs_query_next(game->ecs, &query))
	{
		traffic_component_t* traffic_comp = ecs_query_get_component(game->ecs, &query, game->traffic_type);
		if (traffic_comp->lane == 1) 
		{
			lane1_width += traffic_comp->scale * 2 + 4;
		}
		else if(traffic_comp->lane == 2) 
		{
			lane2_width += traffic_comp->scale * 2 + 4;
		}
		else
		{
			lane3_width += traffic_comp->scale * 2 + 4;
		}
	}

	//Move each traffic rect left
	for (ecs_query_t query = ecs_query_create(game->ecs, k_query_mask, 0);
		ecs_query_is_valid(game->ecs, &query);
		ecs_query_next(game->ecs, &query))
	{
		transform_component_t* transform_comp = ecs_query_get_component(game->ecs, &query, game->transform_type);
		traffic_component_t* traffic_comp = ecs_query_get_component(game->ecs, &query, game->traffic_type);
		rigidbody_component_t* rigidbody_comp = ecs_query_get_component(game->ecs, &query, game->rigidbody_type);
		name_component_t* name_comp = ecs_query_get_component(game->ecs, &query, game->name_type);

		transform_comp->transform.translation = get_rigidbody_position(rigidbody_comp);

		if (transform_comp->transform.translation.y < -game->width - transform_comp->transform.scale.y)
		{
			if (traffic_comp->lane == 1)
			{
				transform_comp->transform.translation.y += lane1_width;
			}
			else if (traffic_comp->lane == 2)
			{
				transform_comp->transform.translation.y += lane2_width;
			}
			else
			{
				transform_comp->transform.translation.y += lane3_width;
			}
			set_rigidbody_position(rigidbody_comp, transform_comp->transform.translation);
		}

	}
}

static void draw_models(final_game_t* game)
{
	uint64_t k_camera_query_mask = (1ULL << game->camera_type);
	for (ecs_query_t camera_query = ecs_query_create(game->ecs, k_camera_query_mask, 0);
		ecs_query_is_valid(game->ecs, &camera_query);
		ecs_query_next(game->ecs, &camera_query))
	{
		camera_component_t* camera_comp = ecs_query_get_component(game->ecs, &camera_query, game->camera_type);

		uint64_t k_model_query_mask = (1ULL << game->transform_type) | (1ULL << game->model_type);
		for (ecs_query_t query = ecs_query_create(game->ecs, k_model_query_mask, 0);
			ecs_query_is_valid(game->ecs, &query);
			ecs_query_next(game->ecs, &query))
		{
			transform_component_t* transform_comp = ecs_query_get_component(game->ecs, &query, game->transform_type);
			model_component_t* model_comp = ecs_query_get_component(game->ecs, &query, game->model_type);
			ecs_entity_ref_t entity_ref = ecs_query_get_entity(game->ecs, &query);

			struct
			{
				mat4f_t projection;
				mat4f_t model;
				mat4f_t view;
			} uniform_data;
			uniform_data.projection = camera_comp->projection;
			uniform_data.view = camera_comp->view;
			transform_to_matrix(&transform_comp->transform, &uniform_data.model);
			gpu_uniform_buffer_info_t uniform_info = { .data = &uniform_data, sizeof(uniform_data) };

			render_push_model(game->render, &entity_ref, model_comp->mesh_info, model_comp->shader_info, &uniform_info);
		}
	}
}
