#pragma once

#include "renderer/mesh.h"

#include <string>
#include <vector>

// Minimal Wavefront OBJ loader: v, vt, vn, f (any polygon size, fan triangulated).
// Materials and groups are ignored; missing normals become flat face normals.
// The mesh is recentred on its bounding box and uniformly scaled so its width (X extent)
// equals targetWidth, then rotated by yawDeg about Y so models that face +Z or +X can be fixed
// without editing the file. Returns an empty Mesh (indexCount 0) if the file cannot be read.
Mesh loadObj(const std::string& path, float targetWidth, float yawDeg = 0.0f);

// The parsing half of loadObj, no GL calls, so it is unit-testable. Returns false if the file is unreadable or has no faces.
bool parseObj(const std::string& path, float targetWidth, float yawDeg,
              std::vector<Vertex>& verts, std::vector<unsigned int>& idx);
