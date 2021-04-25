#pragma once

int shmemTelemetryInit(std::string process_id);
void shmemTelemetrySend(char state, double x, double y, double w, double h);
