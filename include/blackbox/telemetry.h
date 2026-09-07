#pragma once

#include <string>

// BLACKBOX: fire-and-forget UDP JSON lines to the Python bridge. One line per sample.
class Telemetry {
public:
	Telemetry(const std::string& host, int port);
	~Telemetry();

	// Sends {"ts": ISO-8601 UTC, "tail", "surface", "commanded_deg", "measured_deg"}.
	void send(const std::string& tail, const std::string& surface,
	          float commandedDeg, float measuredDeg);

private:
	int sock = -1;
};
