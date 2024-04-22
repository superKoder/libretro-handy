#include <stdlib.h>
#include <libretro.h>

#include "system.h"
#include "lynxdef.h"

#define HANDY_MP_VER    "0.1.0"
#define ROM_FILE    "lynxboot.img"

void handy_log(enum retro_log_level level, const char *format, ...);
