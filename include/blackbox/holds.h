#pragma once

#include <set>
#include <string>

// BLACKBOX: reads {"held": ["N101", ...]} written by the bridge every couple of seconds.
class Holds {
public:
	explicit Holds(std::string path) : path(std::move(path)) {}

	// Re-reads the file if more than intervalSec has passed since the last read.
	void poll(double nowSec, double intervalSec = 2.0);

	bool isHeld(const std::string& tail) const { return held.count(tail) > 0; }
	const std::set<std::string>& all() const { return held; }

private:
	std::string path;
	std::set<std::string> held;
	double lastRead = -1e9;
};
