#pragma once

#include "vertex.h"
#include "bounding_box.h"
#include "color.h"

BoundingBox boundingBox(const Vertex2D& v0, const Vertex2D& v1, const Vertex2D& v2);

Color interpolateColor(const Color& c0, const Color& c1, const Color& c2, float w0, float w1, float w2);

float interpolateDepth(const Vertex2D& a, const Vertex2D&b, const Vertex2D& c, float w0, float w1, float w2);