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
#include "physics.h"

#include <stdlib.h>
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

typedef struct name_component_t
{
	char name[32];
} name_component_t;

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
	int name_type;
	int rigidbody_type;
	int box_collider_type;

	int active_obstacles;
	int max_obstacles;

	float time_of_last_obstacle;
	float time_between_obstacle_spawns;
	float time_of_last_death;

	ecs_entity_ref_t player_ent;
	ecs_entity_ref_t floor_ent;
	ecs_entity_ref_t obstacle_ents[20];

	ecs_entity_ref_t camera_ent;

	gpu_mesh_info_t player_mesh;
	gpu_mesh_info_t floor_mesh;
	gpu_mesh_info_t obstacle1_mesh;
	gpu_mesh_info_t obstacle2_mesh;
	gpu_mesh_info_t obstacle3_mesh;
	gpu_shader_info_t cube_shader;
	fs_work_t* vertex_shader_work;
	fs_work_t* fragment_shader_work;
} final_game_t;

static void load_resources(final_game_t* game);
static void unload_resources(final_game_t* game);
static void spawn_player(final_game_t* game, int index);
static void spawn_camera(final_game_t* game);
static void spawn_floor(final_game_t* game);
static void update_players(final_game_t* game);
static void update_obstacles(final_game_t* game);
static void spawn_obstacles(final_game_t* game);
static void draw_models(final_game_t* game);
static void check_collisions(void* data);
static vec3f_t get_random_position();
static vec3f_t get_forward_to_random_pos(vec3f_t initial_pos);

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

	srand((unsigned)time(NULL));
	game->max_obstacles = 20;
	game->time_between_obstacle_spawns = 1000;

	game->ecs = ecs_create(heap);
	game->transform_type = ecs_register_component_type(game->ecs, "transform", sizeof(transform_component_t), _Alignof(transform_component_t));
	game->camera_type = ecs_register_component_type(game->ecs, "camera", sizeof(camera_component_t), _Alignof(camera_component_t));
	game->model_type = ecs_register_component_type(game->ecs, "model", sizeof(model_component_t), _Alignof(model_component_t));
	game->player_type = ecs_register_component_type(game->ecs, "player", sizeof(player_component_t), _Alignof(player_component_t));
	game->name_type = ecs_register_component_type(game->ecs, "name", sizeof(name_component_t), _Alignof(name_component_t));
	game->rigidbody_type = ecs_register_component_type(game->ecs, "rigidbody", sizeof(rigidbody_component_t), _Alignof(rigidbody_component_t));
	game->box_collider_type = ecs_register_component_type(game->ecs, "box_collider", sizeof(box_collider_component_t), _Alignof(box_collider_component_t));

	game->dynamics = heap_alloc(heap, sizeof(dynamics_t), 8);
	physics_initialize(game->dynamics);

	load_resources(game);
	spawn_player(game, 0);
	spawn_floor(game);
	spawn_camera(game);

	return game;

}

void final_game_destroy(final_game_t* game)
{
	physics_end(game->dynamics);
	heap_free(game->heap, game->dynamics);

	ecs_destroy(game->ecs);
	timer_object_destroy(game->timer);
	unload_resources(game);
	heap_free(game->heap, game);
}

void final_game_update(final_game_t* game)
{
	timer_object_update(game->timer);
	physics_fixed_update(game->dynamics, game->timer, check_collisions, game);
	ecs_update(game->ecs);
	update_players(game);
	update_obstacles(game);
	spawn_obstacles(game);
	draw_models(game);
	render_push_done(game->render);
}

static void check_collisions(void* data)
{
	final_game_t* game = (final_game_t*)data;

	uint32_t key_mask = wm_get_key_mask(game->window);

	uint64_t k_query_mask = (1ULL << game->rigidbody_type) | (1ULL << game->player_type);

	uint64_t k_query_mask2 = (1ULL << game->rigidbody_type);

	for (ecs_query_t query = ecs_query_create(game->ecs, k_query_mask, 0);
		ecs_query_is_valid(game->ecs, &query);
		ecs_query_next(game->ecs, &query))
	{
		transform_component_t* player_transform_comp = ecs_query_get_component(game->ecs, &query, game->transform_type);
		box_collider_component_t* player_box_collider_comp = ecs_query_get_component(game->ecs, &query, game->box_collider_type);
		player_component_t* player_comp = ecs_query_get_component(game->ecs, &query, game->player_type);
		for (ecs_query_t query2 = ecs_query_create(game->ecs, k_query_mask2, 0);
			ecs_query_is_valid(game->ecs, &query2);
			ecs_query_next(game->ecs, &query2))
		{
			transform_component_t* transform_comp = ecs_query_get_component(game->ecs, &query2, game->transform_type);
			box_collider_component_t* box_collider_comp = ecs_query_get_component(game->ecs, &query2, game->box_collider_type);
			//Checks if the player is colliding with one of the obstacles
			if (check_collide(player_box_collider_comp, box_collider_comp))
			{
				//If so, reset all the obstacles positions and print out a message saying how long the player survived
				for (int i = 0; i < _countof(game->obstacle_ents); i++)
				{
					if (ecs_is_entity_ref_valid(game->ecs, game->obstacle_ents[i], false))
					{
						transform_component_t* transform_comp2 = ecs_entity_get_component(game->ecs, game->obstacle_ents[i], game->transform_type, false);
						rigidbody_component_t* rigidbody_comp = ecs_entity_get_component(game->ecs, game->obstacle_ents[i], game->rigidbody_type, false);
						transform_comp2->transform.translation = get_random_position();
						vec3f_t forward = get_forward_to_random_pos(transform_comp2->transform.translation);
						transform_comp2->transform.rotation = quatf_look_at(forward, vec3f_up());
						set_rigidbody_position(rigidbody_comp, transform_comp2->transform.translation);
						set_rigidbody_quaternion(rigidbody_comp, transform_comp2->transform.rotation);
						set_rigidbody_linear_velocity(rigidbody_comp, vec3f_scale(forward, transform_comp2->transform.scale.y * 2));
					}
				}
				float time_survived = (float)timer_object_get_ms(game->timer) - game->time_of_last_death;
				time_survived = time_survived / 1000.0f;
				debug_print(k_print_info, "You survived for %.2f seconds! \n", time_survived);
				game->time_of_last_death = (float)timer_object_get_ms(game->timer);
				player_transform_comp->transform.translation = vec3f_zero();
			}
		}
	}
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

	static vec3f_t floor_verts[] =
	{
		{ -1.0f, -1.0f,  1.0f }, { 0.5f, 0.5f,  0.5f },
		{  1.0f, -1.0f,  1.0f }, { 0.5f, 0.5f,  0.5f },
		{  1.0f,  1.0f,  1.0f }, { 0.5f, 0.5f,  0.5f },
		{ -1.0f,  1.0f,  1.0f }, { 0.5f, 0.5f,  0.5f },
		{ -1.0f, -1.0f, -1.0f }, { 0.5f, 0.5f,  0.5f },
		{  1.0f, -1.0f, -1.0f }, { 0.5f, 0.5f,  0.5f },
		{  1.0f,  1.0f, -1.0f }, { 0.5f, 0.5f,  0.5f },
		{ -1.0f,  1.0f, -1.0f }, { 0.5f, 0.5f,  0.5f },
	};

	game->floor_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = floor_verts,
		.vertex_data_size = sizeof(floor_verts),
		.index_data = cube_indices,
		.index_data_size = sizeof(cube_indices),
	};

	static vec3f_t obstacle1_verts[] =
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

	game->obstacle1_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = obstacle1_verts,
		.vertex_data_size = sizeof(obstacle1_verts),
		.index_data = cube_indices,
		.index_data_size = sizeof(cube_indices),
	};

	static vec3f_t obstacle2_verts[] =
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

	game->obstacle2_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = obstacle2_verts,
		.vertex_data_size = sizeof(obstacle2_verts),
		.index_data = cube_indices,
		.index_data_size = sizeof(cube_indices),
	};

	static vec3f_t obstacle3_verts[] =
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

	game->obstacle3_mesh = (gpu_mesh_info_t)
	{
		.layout = k_gpu_mesh_layout_tri_p444_c444_i2,
		.vertex_data = obstacle3_verts,
		.vertex_data_size = sizeof(obstacle3_verts),
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
		(1ULL << game->name_type) |
		(1ULL << game->rigidbody_type) |
		(1ULL << game->box_collider_type);

	game->player_ent = ecs_entity_add(game->ecs, k_player_ent_mask);

	transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->player_ent, game->transform_type, true);
	transform_identity(&transform_comp->transform);

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
	create_box_collider(box_collider_comp, rigidbody_comp, game->dynamics->space, 2, 2, 2);
}

static void spawn_floor(final_game_t* game) 
{
	uint64_t k_floor_ent_mask =
		(1ULL << game->transform_type) |
		(1ULL << game->model_type) |
		(1ULL << game->name_type);
	game->floor_ent = ecs_entity_add(game->ecs, k_floor_ent_mask);

	transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->floor_ent, game->transform_type, true);
	transform_identity(&transform_comp->transform);
	transform_comp->transform.translation.z = -5;
	transform_comp->transform.scale.y = 200;
	transform_comp->transform.scale.x = 200;

	name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->floor_ent, game->name_type, true);
	strcpy_s(name_comp->name, sizeof(name_comp->name), "floor");

	model_component_t* model_comp = ecs_entity_get_component(game->ecs, game->floor_ent, game->model_type, true);
	model_comp->mesh_info = &game->floor_mesh;
	model_comp->shader_info = &game->cube_shader;

	physics_spawn_floor(game->dynamics->space, -5);
}

static void spawn_obstacles(final_game_t* game)
{
	//Only spawns up to a max limit of obstacles
	if (game->active_obstacles < game->max_obstacles && 
		timer_object_get_ms(game->timer) - game->time_of_last_obstacle > game->time_between_obstacle_spawns)
	{
		uint64_t k_obstacle_ent_mask =
			(1ULL << game->transform_type) |
			(1ULL << game->model_type) |
			(1ULL << game->name_type) |
			(1ULL << game->rigidbody_type) |
			(1ULL << game->box_collider_type);
		for (int i = 0; i < _countof(game->obstacle_ents); i++) 
		{
			if (!ecs_is_entity_ref_valid(game->ecs, game->obstacle_ents[i], false))
			{
				game->obstacle_ents[i] = ecs_entity_add(game->ecs, k_obstacle_ent_mask);
			 	int obstacle_type = rand() % 3;
				transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->obstacle_ents[i], game->transform_type, true);
				transform_identity(&transform_comp->transform);
				//Have the obstacle face a random position in the center and move toward it
				transform_comp->transform.translation = get_random_position();
				vec3f_t forward = get_forward_to_random_pos(transform_comp->transform.translation);
				transform_comp->transform.rotation = quatf_look_at(forward, vec3f_up());

				model_component_t* model_comp = ecs_entity_get_component(game->ecs, game->obstacle_ents[i], game->model_type, true);
				model_comp->shader_info = &game->cube_shader;

				name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->obstacle_ents[i], game->name_type, true);
				char index[3];
				sprintf_s(index, sizeof(index), "%d", i);
				strcpy_s(name_comp->name, sizeof(name_comp->name), "obstacle_");
				strcat_s(name_comp->name, sizeof(name_comp->name), index);

				rigidbody_component_t* rigidbody_comp = ecs_entity_get_component(game->ecs, game->obstacle_ents[i], game->rigidbody_type, true);
				create_rigidbody(game->dynamics->world, rigidbody_comp);
				set_rigidbody_position(rigidbody_comp, transform_comp->transform.translation);
				set_rigidbody_quaternion(rigidbody_comp, transform_comp->transform.rotation);
				float speed = 0;

				box_collider_component_t* box_collider_comp = ecs_entity_get_component(game->ecs, game->obstacle_ents[i], game->box_collider_type, true);
				//Three different obstacle types with different widths and speeds
				switch (obstacle_type)
				{
				case 0:
					model_comp->mesh_info = &game->obstacle1_mesh;
					speed = 8.0f;
					create_box_collider(box_collider_comp, rigidbody_comp, game->dynamics->space, 2, 2, 2);
					break;
				case 1:
					model_comp->mesh_info = &game->obstacle2_mesh;
					transform_comp->transform.scale.y = 2;
					speed = 4.0f;
					create_box_collider(box_collider_comp, rigidbody_comp, game->dynamics->space, 2, 4, 2);
					break;
				case 2:
					model_comp->mesh_info = &game->obstacle3_mesh;
					transform_comp->transform.scale.y = 3;
					speed = 2.0f;
					create_box_collider(box_collider_comp, rigidbody_comp, game->dynamics->space, 2, 6, 2);
					break;
				}
				set_rigidbody_linear_velocity(rigidbody_comp, vec3f_scale(forward, speed));
				game->active_obstacles++;
				break;
			}
		}
		game->time_of_last_obstacle = (float)timer_object_get_ms(game->timer);
	}
}

static void spawn_camera(final_game_t* game)
{
	uint64_t k_camera_ent_mask =
		(1ULL << game->camera_type) |
		(1ULL << game->transform_type) |
		(1ULL << game->name_type);
	game->camera_ent = ecs_entity_add(game->ecs, k_camera_ent_mask);

	transform_component_t* transform_comp = ecs_entity_get_component(game->ecs, game->camera_ent, game->transform_type, true);
	transform_comp->transform.translation = (vec3f_t){ .x = 30, .y = 0, .z = 15 };

	name_component_t* name_comp = ecs_entity_get_component(game->ecs, game->camera_ent, game->name_type, true);
	strcpy_s(name_comp->name, sizeof(name_comp->name), "camera");

	camera_component_t* camera_comp = ecs_entity_get_component(game->ecs, game->camera_ent, game->camera_type, true);
	mat4f_make_perspective(&camera_comp->projection, (float)M_PI / 2.0f, 16.0f / 9.0f, 0.1f, 100.0f);

	vec3f_t forward = (vec3f_t){ .x = -1, .y = 0, .z = -1 };
	vec3f_t up = (vec3f_t){ .x = -1, .y = 0, .z = 1 };
	mat4f_make_lookat(&camera_comp->view, &transform_comp->transform.translation, &forward, &up);
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
		//Handle input actions and move
		float speed = 4.0f;
		vec3f_t vel = (vec3f_t){.x = 0, .y = 0, .z = get_rigidbody_linear_velocity(player_rigidbody_comp).z};
		if (key_mask & k_key_up)
		{
			vel.x += -speed;
		}
		if (key_mask & k_key_down)
		{
			vel.x += speed;
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

static void update_obstacles(final_game_t* game) 
{
	float dt = (float)timer_object_get_delta_ms(game->timer) * 0.001f;

	uint32_t key_mask = wm_get_key_mask(game->window);

	uint64_t k_query_mask = (1ULL << game->transform_type) | (1ULL << game->box_collider_type);

	for (ecs_query_t query = ecs_query_create(game->ecs, k_query_mask, 0);
		ecs_query_is_valid(game->ecs, &query);
		ecs_query_next(game->ecs, &query))
	{
		transform_component_t* transform_comp = ecs_query_get_component(game->ecs, &query, game->transform_type);
		rigidbody_component_t* rigidbody_comp = ecs_query_get_component(game->ecs, &query, game->rigidbody_type);
		transform_comp->transform.translation = get_rigidbody_position(rigidbody_comp);
		set_rigidbody_quaternion(rigidbody_comp, transform_comp->transform.rotation);
		//Handle out-of-bounds for obstacles
		if (transform_comp->transform.translation.x > 45 || transform_comp->transform.translation.x < -45 || 
			transform_comp->transform.translation.y > 45 || transform_comp->transform.translation.y < -45 ||
			transform_comp->transform.translation.z < -10)
		{
			transform_comp->transform.translation = get_random_position();
			vec3f_t forward = get_forward_to_random_pos(transform_comp->transform.translation);
			transform_comp->transform.rotation = quatf_look_at(forward, vec3f_up());
			set_rigidbody_linear_velocity(rigidbody_comp, vec3f_scale(forward, transform_comp->transform.scale.y * 2));
		}
		set_rigidbody_position(rigidbody_comp, transform_comp->transform.translation);
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

//Returns a random position outside of a 20x20 square centered at the origin
static vec3f_t get_random_position()
{
	int quadrant = rand() % 4;
	int random_y;
	int random_x;
	int left_side = rand() % 2;
	int top_side = rand() % 2;
	if (quadrant == 0 || quadrant == 2)
	{
		random_y = rand() % 81 - 40;
		random_x = rand() % 21 + 20;
		if (top_side)
		{
			random_x *= -1;
		}
	}
	else
	{
		random_y = rand() % 21 + 20;
		random_x = rand() % 81 - 20;
		if (left_side)
		{
			random_y *= -1;
		}
	}
	return (vec3f_t) { .x = (float)random_x, .y = (float)random_y, .z = 0.0f };
}

//Returns a random direction for the obstacle to face
static vec3f_t get_forward_to_random_pos(vec3f_t initial_pos)
{
	vec3f_t random_target_pos = (vec3f_t){ .x = (float)(rand() % 21 - 10), .y = (float)(rand() % 21 - 10), .z = 0.0f };
	vec3f_t forward = vec3f_norm(vec3f_sub(random_target_pos, initial_pos));
	return forward;
}