#include "debug.h"
#include "fs.h"
#include "heap.h"
#include "render.h"
#include "simple_game.h"
#include "final_game.h"
#include "timer.h"
#include "wm.h"

int main(int argc, const char* argv[])
{
	debug_set_print_mask(k_print_info | k_print_warning | k_print_error);
	debug_install_exception_handler();

	timer_startup();

	heap_t* heap = heap_create(2 * 1024 * 1024);
	fs_t* fs = fs_create(heap, 8);
	wm_window_t* window = wm_create(heap);
	render_t* render = render_create(heap, window);

	final_game_t* game = final_game_create(heap, fs, window, render, 16.0f/9.0f, 8.0f);

	while (!wm_pump(window))
	{
		final_game_update(game);
	}

	/* XXX: Shutdown render before the game. Render uses game resources. */
	render_destroy(render);

	final_game_destroy(game);

	wm_destroy(window);
	fs_destroy(fs);
	heap_destroy(heap);

	return 0;
}
