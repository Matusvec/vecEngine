#include "blackbox/holds.h"

#include <fstream>
#include <sstream>

void Holds::poll(double nowSec, double intervalSec) {
	if (nowSec - lastRead < intervalSec) return;
	lastRead = nowSec;

	std::ifstream in(path);
	if (!in) { held.clear(); return; }  // no file = nothing held
	std::stringstream ss;
	ss << in.rdbuf();
	std::string text = ss.str();

	// ponytail: not a JSON parser. Pull every quoted string between the [ ] that
	// follows "held". Enough for {"held": ["N101", "N104"]}; upgrade if the file grows fields.
	std::set<std::string> next;
	size_t start = text.find("\"held\"");
	size_t open = start == std::string::npos ? std::string::npos : text.find('[', start);
	size_t close = open == std::string::npos ? std::string::npos : text.find(']', open);
	if (close != std::string::npos) {
		size_t pos = open;
		while (true) {
			size_t q1 = text.find('"', pos);
			if (q1 == std::string::npos || q1 > close) break;
			size_t q2 = text.find('"', q1 + 1);
			if (q2 == std::string::npos || q2 > close) break;
			next.insert(text.substr(q1 + 1, q2 - q1 - 1));
			pos = q2 + 1;
		}
	}
	held = std::move(next);
}
