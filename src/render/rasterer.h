#pragma once

#include "vertex.h"
#include "bounding_box.h"
#include "color.h"

float signedArea(const Vertex& v0, const Vertex& v1, const Vertex& v2);

BoundingBox boundingBox(const Vertex& v0, const Vertex& v1, const Vertex& v2);

Color interpolateColor(const Color& c0, const Color& c1, const Color& c2, float w0, float w1, float w2);
