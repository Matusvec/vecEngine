#include "blackbox/telemetry.h"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <chrono>
#include <cstdio>
#include <ctime>

namespace {
sockaddr_in destination{};

std::string isoNowUtc() {
	using namespace std::chrono;
	auto now = system_clock::now();
	auto ms = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
	std::time_t t = system_clock::to_time_t(now);
	std::tm tm{};
	gmtime_r(&t, &tm);
	char buf[32];
	std::snprintf(buf, sizeof buf, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
	              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
	              tm.tm_hour, tm.tm_min, tm.tm_sec, (int)ms.count());
	return buf;
}
}  // namespace

Telemetry::Telemetry(const std::string& host, int port) {
	sock = ::socket(AF_INET, SOCK_DGRAM, 0);
	destination.sin_family = AF_INET;
	destination.sin_port = htons((uint16_t)port);
	::inet_pton(AF_INET, host.c_str(), &destination.sin_addr);
}

Telemetry::~Telemetry() {
	if (sock >= 0) ::close(sock);
}

void Telemetry::send(const std::string& tail, const std::string& surface,
                     float commandedDeg, float measuredDeg) {
	if (sock < 0) return;
	char line[256];
	int n = std::snprintf(line, sizeof line,
	                      "{\"ts\":\"%s\",\"tail\":\"%s\",\"surface\":\"%s\","
	                      "\"commanded_deg\":%.3f,\"measured_deg\":%.3f}",
	                      isoNowUtc().c_str(), tail.c_str(), surface.c_str(),
	                      commandedDeg, measuredDeg);
	if (n < 0 || (size_t)n >= sizeof line) return;  // never send past the buffer
	// ponytail: drop on failure, the bridge tolerates gaps and nobody wants a blocking sim
	::sendto(sock, line, (size_t)n, 0, (sockaddr*)&destination, sizeof destination);
}
