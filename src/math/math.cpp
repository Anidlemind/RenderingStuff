#include "math.h"

#include <cmath>

bool areEqual(float a, float b, float epsilon) { return std::abs(a - b) < epsilon; }
