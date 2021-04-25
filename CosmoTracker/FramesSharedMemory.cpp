#include <iostream>
#include <cstring>
#include <Windows.h>

#include "FramesSharedMemory.h"

/// Shared memory size
static const int sharedMemSize = 1100000;

/// Buffer for shared memory
static unsigned char shmem_buffer[sharedMemSize];


/// Frame counter
static int frame_counter_prev = -1;

static std::string shmemName = "CosmoViewerFrame";

static LPCTSTR pBuf;

int shmemFramesInit(std::string process_id) {

	/// Add process id to shared memory name
	if (process_id != "") shmemName += "-" + process_id;

	std::wstring _shmemName = std::wstring(shmemName.begin(), shmemName.end());

	/// Open shared memory
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, _shmemName.c_str());

	/// Check errors
	if (hMapFile == nullptr) {
		fprintf(stderr, "Could not open shared memory\n");
		return 1;
	}

	/// Shared memory mapping
	pBuf = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sharedMemSize);

	/// Check errors
	if (pBuf == NULL)
	{
		fprintf(stderr, "Could not map view of file\n");
		CloseHandle(hMapFile);
		return 1;
	}

	return 0;
}


int shmemFramesReceive(int *frame_width, int *frame_height, int *frame_number, int *frame_size) {
	
	CopyMemory(shmem_buffer, (PVOID)pBuf, 11);
	char busy = shmem_buffer[0];
	if (busy) return SHMEM_BUSY;

	*frame_number = shmem_buffer[5] | (shmem_buffer[6] << 8);
	*frame_width = shmem_buffer[7] | (shmem_buffer[8] << 8);
	*frame_height = shmem_buffer[9] | (shmem_buffer[10] << 8);

	if (frame_counter_prev == *frame_number) return SHMEM_THE_SAME_FRAME;
	frame_counter_prev = *frame_number;

	*frame_size = shmem_buffer[1] + (shmem_buffer[2] << 8) + (shmem_buffer[3] << 16) + (shmem_buffer[4] << 24);

	//printf("frame %d size %d %dx%d\n", *frame_number, *frame_size, *frame_width, *frame_height);

	CopyMemory(shmem_buffer, (PVOID)pBuf, *frame_size + 11);

	return SHMEM_OK;
}

unsigned char * shmemFramesGetBuffer() {
	return shmem_buffer;
}