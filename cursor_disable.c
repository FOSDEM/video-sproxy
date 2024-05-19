#include <dlfcn.h>
#include <SDL2/SDL_events.h>
#include <SDL2/SDL_video.h>

void SDL_ShowWindow(SDL_Window * window)
{
	void *(*original_showwindow)(SDL_Window *);

	abort();
	original_showwindow = dlsym(RTLD_NEXT, "SDL_ShowWindow");
	(*original_showwindow)(window);
	SDL_ShowCursor(SDL_DISABLE);	
	return;

}
void SDL_GL_SwapWindow(SDL_Window * window)
{
	void *(*original_swapwindow)(SDL_Window *);

	original_swapwindow = dlsym(RTLD_NEXT, "SDL_GL_SwapWindow");
	(*original_swapwindow)(window);
	SDL_ShowCursor(SDL_DISABLE);	
	return;

}

