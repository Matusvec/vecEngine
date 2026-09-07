#include "renderer/obj_loader.h"

#include <glm/gtc/matrix_transform.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

bool parseObj(const std::string& path, float targetWidth, float yawDeg,
              std::vector<Vertex>& verts, std::vector<unsigned int>& idx) {
	std::ifstream in(path);
	if (!in) return false;

	std::vector<glm::vec3> positions, normals;
	std::vector<glm::vec2> uvs;
	verts.clear();
	idx.clear();

	auto parseIndex = [](const std::string& tok, int& p, int& t, int& n) {
		p = t = n = 0;
		std::istringstream s(tok);
		std::string part;
		if (std::getline(s, part, '/') && !part.empty()) p = std::stoi(part);
		if (std::getline(s, part, '/') && !part.empty()) t = std::stoi(part);
		if (std::getline(s, part, '/') && !part.empty()) n = std::stoi(part);
	};
	auto resolve = [](int i, size_t size) -> size_t { return i > 0 ? (size_t)i - 1 : size + i; };  // negative = relative

	std::string line;
	while (std::getline(in, line)) {
		std::istringstream s(line);
		std::string kind;
		s >> kind;
		if (kind == "v") { glm::vec3 v; s >> v.x >> v.y >> v.z; positions.push_back(v); }
		else if (kind == "vn") { glm::vec3 n; s >> n.x >> n.y >> n.z; normals.push_back(n); }
		else if (kind == "vt") { glm::vec2 t; s >> t.x >> t.y; uvs.push_back(t); }
		else if (kind == "f") {
			std::vector<Vertex> face;
			std::string tok;
			while (s >> tok) {
				int p, t, n;
				parseIndex(tok, p, t, n);
				Vertex v{};
				v.position = positions[resolve(p, positions.size())];
				if (t && !uvs.empty()) v.texCoords = uvs[resolve(t, uvs.size())];
				if (n && !normals.empty()) v.normal = normals[resolve(n, normals.size())];
				face.push_back(v);
			}
			if (face.size() < 3) continue;
			if (glm::length(face[0].normal) < 0.5f) {  // no normals in the file: flat shade the face
				glm::vec3 fn = glm::normalize(glm::cross(face[1].position - face[0].position, face[2].position - face[0].position));
				for (auto& v : face) v.normal = fn;
			}
			unsigned int base = (unsigned int)verts.size();
			verts.insert(verts.end(), face.begin(), face.end());
			for (size_t k = 1; k + 1 < face.size(); ++k) { idx.push_back(base); idx.push_back(base + (unsigned)k); idx.push_back(base + (unsigned)k + 1); }
		}
	}
	if (verts.empty()) return false;

	glm::vec3 lo = verts[0].position, hi = verts[0].position;
	for (const auto& v : verts) { lo = glm::min(lo, v.position); hi = glm::max(hi, v.position); }
	glm::vec3 center = (lo + hi) * 0.5f;
	float scale = targetWidth / std::max(hi.x - lo.x, 1e-4f);
	glm::mat4 fix = glm::rotate(glm::mat4(1.0f), glm::radians(yawDeg), glm::vec3(0, 1, 0))
	              * glm::scale(glm::mat4(1.0f), glm::vec3(scale))
	              * glm::translate(glm::mat4(1.0f), -center);
	glm::mat3 normalFix = glm::mat3(glm::rotate(glm::mat4(1.0f), glm::radians(yawDeg), glm::vec3(0, 1, 0)));
	for (auto& v : verts) {
		v.position = glm::vec3(fix * glm::vec4(v.position, 1.0f));
		v.normal = glm::normalize(normalFix * v.normal);
	}
	return true;
}

Mesh loadObj(const std::string& path, float targetWidth, float yawDeg) {
	std::vector<Vertex> verts;
	std::vector<unsigned int> idx;
	return parseObj(path, targetWidth, yawDeg, verts, idx) ? Mesh(verts, idx) : Mesh();
}
