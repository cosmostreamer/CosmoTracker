#include <iostream>
#include <cstring>
#include <Windows.h>
#include <stdint.h>

#include "TelemetrySharedMemory.h"

/// Shared memory size
static const int sharedMemSize = 50;


/// Packet counter
static int counter_prev = -1;

static std::string shmemName = "CosmoViewerTrackingTelemetry";

static LPCTSTR pBuf;

int shmemTelemetryInit(std::string process_id) {

	/// Add process id to shared memory name
	if (process_id != "") shmemName += "-" + process_id;

	std::wstring _shmemName = std::wstring(shmemName.begin(), shmemName.end());

	/// Open shared memory
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, _shmemName.c_str());

	/// Check errors
	if (hMapFile == nullptr) {
		fprintf(stderr, "Could not open telemetry shared memory\n");
		return 1;
	}

	/// Shared memory mapping
	pBuf = (LPTSTR)MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, sharedMemSize);

	/// Check errors
	if (pBuf == NULL)
	{
		fprintf(stderr, "Could not map view of telemetry file\n");
		CloseHandle(hMapFile);
		return 1;
	}

	return 0;
}


void shmemTelemetrySend(char state, double x, double y, double w, double h) {

	uint16_t x_int = x * 10000;
	uint16_t y_int = y * 10000;
	uint16_t w_int = w * 10000;
	uint16_t h_int = h * 10000;

	unsigned char shmem_buffer[sharedMemSize];
	
	shmem_buffer[0] = counter_prev & 0xff;
	shmem_buffer[1] = (counter_prev >> 8) & 0xff;
	shmem_buffer[2] = x_int & 0xff;
	shmem_buffer[3] = (x_int >> 8) & 0xff;
	shmem_buffer[4] = y_int & 0xff;
	shmem_buffer[5] = (y_int >> 8) & 0xff;
	shmem_buffer[6] = w_int & 0xff;
	shmem_buffer[7] = (w_int >> 8) & 0xff;
	shmem_buffer[8] = h_int & 0xff;
	shmem_buffer[9] = (h_int >> 8) & 0xff;
	shmem_buffer[10] = state;

	CopyMemory((PVOID)pBuf, shmem_buffer, sizeof(shmem_buffer));

	counter_prev++;
}
