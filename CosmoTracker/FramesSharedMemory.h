#pragma once

enum {
	SHMEM_OK,
	SHMEM_BUSY,
	SHMEM_THE_SAME_FRAME
};

int shmemFramesInit(std::string process_id);
int shmemFramesReceive(int *frame_width, int *frame_height, int *frame_number, int *frame_size);
unsigned char * shmemFramesGetBuffer();
