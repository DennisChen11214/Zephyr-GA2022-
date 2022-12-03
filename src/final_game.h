#pragma once

typedef struct final_game_t final_game_t;

typedef struct fs_t fs_t;
typedef struct heap_t heap_t;
typedef struct render_t render_t;
typedef struct wm_window_t wm_window_t;

// Create an instance of final with a given aspect ratio and height
final_game_t* final_game_create(heap_t* heap, fs_t* fs, wm_window_t* window, render_t* render, float aspect, float height);

// Destroy an instance of final
void final_game_destroy(final_game_t* game);

// Per-frame update for our final game
void final_game_update(final_game_t* game);

//Physics update for our final game
void final_game_fixed_update(final_game_t* game);