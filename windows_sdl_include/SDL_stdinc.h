/* Fallback for Linux when SDL_mixer.h (in e.g. /usr/local) does #include "SDL_stdinc.h"
   but base SDL2 is in a different prefix. Redirect to system SDL2. */
#include <SDL2/SDL_stdinc.h>
