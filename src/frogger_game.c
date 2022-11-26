#include "ecs.h"
#include "fs.h"
#include "gpu.h"
#include "heap.h"
#include "render.h"
#include "timer_object.h"
#include "transform.h"
#include "wm.h"

#include <ode/ode.h>

#define _USE_MATH_DEFINES
#include <math.h>
#include <string.h>


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>

typedef struct transform_component_t
{
	transform_t transform;
} transform_component_t;

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

typedef struct frogger_game_t
{
	heap_t* heap;
	fs_t* fs;
	wm_window_t* window;
	render_t* render;

	timer_object_t* timer;

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
} frogger_game_t;

static void load_resources(frogger_game_t* game);
static void unload_resources(frogger_game_t* game);
static void spawn_player(frogger_game_t* game, int index);
static void spawn_camera(frogger_game_t* game);
static void spawn_traffic(frogger_game_t* game);
static void update_players(frogger_game_t* game);
static void update_traffic(frogger_game_t* game);
static void draw_models(frogger_game_t* game);

frogger_game_t* frogger_game_create(heap_t* heap, fs_t* fs, wm_window_t* window, render_t* render, float aspect, float height)
{
	frogger_game_t* game = heap_alloc(heap, sizeof(frogger_game_t), 8);
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

	load_resources(game);
	spawn_player(game, 0);
	spawn_traffic(game);
	spawn_camera(game);

	return game;

}

void frogger_game_destroy(frogger_game_t* game)
{
	ecs_destroy(game->ecs);
	timer_object_destroy(game->timer);
	unload_resources(game);
	heap_free(game->heap, game);
}

void frogger_game_update(frogger_game_t* game)
{
	timer_object_update(game->timer);
	ecs_update(game->ecs);
	update_players(game);
	update_traffic(game);
	draw_models(game);
	render_push_done(game->render);
}

static void load_resources(frogger_game_t* game)
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

static void unload_resources(frogger_game_t* game)
{
	fs_work_destroy(game->fragment_shader_work);
	fs_work_destroy(game->vertex_shader_work);
}

static void spawn_player(frogger_game_t* game, int index)
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
}

static void spawn_traffic(frogger_game_t* game)
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
		strcpy_s(name_comp->name, sizeof(name_comp->name), "lane1_" + i);
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
		name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->lane2_ents[i], game->name_type, true);
		strcpy_s(name_comp->name, sizeof(name_comp->name), "lane2_" + i);
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
		name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->lane3_ents[i], game->name_type, true);
		strcpy_s(name_comp->name, sizeof(name_comp->name), "lane3_" + i);
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
	}
}

static void spawn_camera(frogger_game_t* game)
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

static void update_players(frogger_game_t* game)
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

		//Reset the player back to start if they reached the end
		if (player_transform_comp->transform.translation.z < -(game->height - 1))
		{
			player_transform_comp->transform.translation.z = game->height - 1;
		}

		//Handle input actions and move
		transform_t move;
		transform_identity(&move);
		float speed = 4.0f;
		if (key_mask & k_key_up)
		{
			move.translation = vec3f_add(move.translation, vec3f_scale(vec3f_up(), -dt * speed));
		}
		if ((key_mask & k_key_down) && player_transform_comp->transform.translation.z < game->height - 1)
		{
			move.translation = vec3f_add(move.translation, vec3f_scale(vec3f_up(), dt * speed));
		}
		if (key_mask & k_key_left)
		{
			move.translation = vec3f_add(move.translation, vec3f_scale(vec3f_right(), -dt * speed));
		}
		if (key_mask & k_key_right)
		{
			move.translation = vec3f_add(move.translation, vec3f_scale(vec3f_right(), dt * speed));
		}
		transform_multiply(&player_transform_comp->transform, &move);

		//Check for collisions
		uint64_t k2_query_mask = (1ULL << game->traffic_type);
		for (ecs_query_t query2 = ecs_query_create(game->ecs, k2_query_mask, 0);
			ecs_query_is_valid(game->ecs, &query2);
			ecs_query_next(game->ecs, &query2))
		{
			transform_component_t* transform_comp = ecs_query_get_component(game->ecs, &query2, game->transform_type);
			if (player_transform_comp->transform.translation.y + 1 > transform_comp->transform.translation.y - transform_comp->transform.scale.y &&
				player_transform_comp->transform.translation.y - 1 < transform_comp->transform.translation.y + transform_comp->transform.scale.y &&
				player_transform_comp->transform.translation.z + 1 > transform_comp->transform.translation.z - transform_comp->transform.scale.z &&
				player_transform_comp->transform.translation.z - 1 < transform_comp->transform.translation.z + transform_comp->transform.scale.z)
			{
				player_transform_comp->transform.translation.z = game->height - 1;
			}
		}
	}
}

static void update_traffic(frogger_game_t* game)
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
		}

		transform_t move;
		transform_identity(&move);
		float speed = 2.0f;
		move.translation = vec3f_add(move.translation, vec3f_scale(vec3f_right(), -dt * speed));
		transform_multiply(&transform_comp->transform, &move);
	}
}

static void draw_models(frogger_game_t* game)
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
