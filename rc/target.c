#ifndef _TARGET_H_
#define _TARGET_H_

#include <stdio.h>
#include <stdlib.h>
#include <kovan/camera.h>

#ifdef __cplusplus
extern "C" {
#endif

// QProcess does not correctly emulate a terminal, so output is not flushed by newline.
// This sets flush after every write, which is unfortunately slower.
__attribute__((constructor))
void __set_no_stdout_buffer() {
	setvbuf(stdout, (char *)NULL, _IONBF, 0);
}

__attribute__((constructor))
void __set_vision_config_path() {
	const char *const path = getenv("CAMERA_BASE_CONFIG_PATH");
	if(!path) return;
	set_camera_config_base_path(path);
}

#ifdef __cplusplus
}
#endif

#endif
