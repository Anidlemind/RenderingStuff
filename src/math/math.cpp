#include "math/math.h"

#include <algorithm>
#include <cmath>

bool AreEqual(float a, float b, float epsilon) {
  return std::abs(a - b) < epsilon;
}

float SignedArea(float ax, float ay, float bx, float by, float cx, float cy) {
  return (bx - ax) * (cy - ay) - (by - ay) * (cx - ax);
}

Vec2& Vec2::operator+=(const Vec2& other) {
  x += other.x;
  y += other.y;
  return *this;
}

Vec2& Vec2::operator-=(const Vec2& other) {
  x -= other.x;
  y -= other.y;
  return *this;
}

Vec2& Vec2::operator*=(float s) {
  x *= s;
  y *= s;
  return *this;
}

Vec2& Vec2::operator/=(float s) {
  x /= s;
  y /= s;
  return *this;
}

Vec2 Vec2::operator+(const Vec2& other) const {
  return {x + other.x, y + other.y};
}

Vec2 Vec2::operator-(const Vec2& other) const {
  return {x - other.x, y - other.y};
}

Vec2 Vec2::operator-() const { return {-x, -y}; }

Vec2 Vec2::operator*(float s) const { return {x * s, y * s}; }

Vec2 Vec2::operator/(float s) const { return {x / s, y / s}; }

float Vec2::Length() const { return std::sqrt(x * x + y * y); }

Vec2 operator*(float s, const Vec2& v) { return v * s; }

Vec3& Vec3::operator+=(const Vec3& other) {
  x += other.x;
  y += other.y;
  z += other.z;
  return *this;
}

Vec3& Vec3::operator-=(const Vec3& other) {
  x -= other.x;
  y -= other.y;
  z -= other.z;
  return *this;
}

Vec3& Vec3::operator*=(float s) {
  x *= s;
  y *= s;
  z *= s;
  return *this;
}

Vec3& Vec3::operator/=(float s) {
  x /= s;
  y /= s;
  z /= s;
  return *this;
}

Vec3 Vec3::operator+(const Vec3& other) const {
  return {x + other.x, y + other.y, z + other.z};
}

Vec3 Vec3::operator-(const Vec3& other) const {
  return {x - other.x, y - other.y, z - other.z};
}

Vec3 Vec3::operator-() const { return {-x, -y, -z}; }

Vec3 Vec3::operator*(float s) const { return {x * s, y * s, z * s}; }

Vec3 Vec3::operator/(float s) const { return {x / s, y / s, z / s}; }

float Vec3::Length() const { return std::sqrt(x * x + y * y + z * z); }

Vec3 operator*(float s, const Vec3& v) { return v * s; }

Vec4& Vec4::operator+=(const Vec4& other) {
  x += other.x;
  y += other.y;
  z += other.z;
  w += other.w;
  return *this;
}

Vec4& Vec4::operator-=(const Vec4& other) {
  x -= other.x;
  y -= other.y;
  z -= other.z;
  w -= other.w;
  return *this;
}

Vec4& Vec4::operator*=(float s) {
  x *= s;
  y *= s;
  z *= s;
  w *= s;
  return *this;
}

Vec4& Vec4::operator/=(float s) {
  x /= s;
  y /= s;
  z /= s;
  w /= s;
  return *this;
}

Vec4 Vec4::operator+(const Vec4& other) const {
  return {x + other.x, y + other.y, z + other.z, w + other.w};
}

Vec4 Vec4::operator-(const Vec4& other) const {
  return {x - other.x, y - other.y, z - other.z, w - other.w};
}

Vec4 Vec4::operator-() const { return {-x, -y, -z, -w}; }

Vec4 Vec4::operator*(float s) const { return {x * s, y * s, z * s, w * s}; }

Vec4 Vec4::operator/(float s) const { return {x / s, y / s, z / s, w / s}; }

float Vec4::Length() const { return std::sqrt(x * x + y * y + z * z + w * w); }

Vec4 operator*(float s, const Vec4& v) { return v * s; }

float Dot(const Vec2& a, const Vec2& b) { return a.x * b.x + a.y * b.y; }

float Dot(const Vec3& a, const Vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float Dot(const Vec4& a, const Vec4& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec3 Cross(const Vec3& a, const Vec3& b) {
  return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

Vec2 Normalize(const Vec2& v) {
  const float length = v.Length();
  if (length > 0 && std::isfinite(length)) {
    return v / length;
  }
  const float magnitude = std::max(std::abs(v.x), std::abs(v.y));
  if (!std::isfinite(v.x) || !std::isfinite(v.y) || magnitude == 0) {
    return {};
  }
  const auto scaled = v / magnitude;
  return scaled / scaled.Length();
}

Vec3 Normalize(const Vec3& v) {
  const float length = v.Length();
  if (length > 0 && std::isfinite(length)) {
    return v / length;
  }
  const float magnitude =
      std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z)});
  if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z) ||
      magnitude == 0) {
    return {};
  }
  const auto scaled = v / magnitude;
  return scaled / scaled.Length();
}

Vec4 Normalize(const Vec4& v) {
  const float length = v.Length();
  if (length > 0 && std::isfinite(length)) {
    return v / length;
  }
  const float magnitude =
      std::max({std::abs(v.x), std::abs(v.y), std::abs(v.z), std::abs(v.w)});
  if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z) ||
      !std::isfinite(v.w) || magnitude == 0) {
    return {};
  }
  const auto scaled = v / magnitude;
  return scaled / scaled.Length();
}

Mat4 Mat4::Identity() { return Mat4{}; }

Mat4 operator*(const Mat4& a, const Mat4& b) {
  Mat4 res;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a.At(k, row) * b.At(col, k);
      }
      res.At(col, row) = sum;
    }
  }
  return res;
}

Vec4 operator*(const Mat4& mat, const Vec4& v) {
  Vec4 res;
  res.x = mat.At(0, 0) * v.x + mat.At(1, 0) * v.y + mat.At(2, 0) * v.z +
          mat.At(3, 0) * v.w;
  res.y = mat.At(0, 1) * v.x + mat.At(1, 1) * v.y + mat.At(2, 1) * v.z +
          mat.At(3, 1) * v.w;
  res.z = mat.At(0, 2) * v.x + mat.At(1, 2) * v.y + mat.At(2, 2) * v.z +
          mat.At(3, 2) * v.w;
  res.w = mat.At(0, 3) * v.x + mat.At(1, 3) * v.y + mat.At(2, 3) * v.z +
          mat.At(3, 3) * v.w;
  return res;
}

Mat4 Translation(const Vec3& t) {
  return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, t.x, t.y, t.z, 1};
}

Mat4 Scale(const Vec3& s) {
  return {s.x, 0, 0, 0, 0, s.y, 0, 0, 0, 0, s.z, 0, 0, 0, 0, 1};
}

Mat4 Scale(float s) { return Scale({s, s, s}); }

Mat4 RotationX(float radians) {
  float c = std::cos(radians);
  float s = std::sin(radians);

  return {1, 0, 0, 0, 0, c, s, 0, 0, -s, c, 0, 0, 0, 0, 1};
}

Mat4 RotationY(float radians) {
  float c = std::cos(radians);
  float s = std::sin(radians);

  return {c, 0, -s, 0, 0, 1, 0, 0, s, 0, c, 0, 0, 0, 0, 1};
}

Mat4 RotationZ(float radians) {
  float c = std::cos(radians);
  float s = std::sin(radians);

  return {c, s, 0, 0, -s, c, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}

Mat4 Perspective(float fov_y, float aspect, float near, float far) {
  float f = 1.0f / std::tan(fov_y * 0.5f);

  return {f / aspect,
          0,
          0,
          0,
          0,
          f,
          0,
          0,
          0,
          0,
          (far + near) / (near - far),
          -1.0f,
          0,
          0,
          (2.0f * far * near) / (near - far),
          0.0f};
}

Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
  Vec3 f = Normalize(center - eye);
  Vec3 s = Normalize(Cross(f, up));
  Vec3 u = Cross(s, f);

  return {s.x, u.x, -f.x, 0, s.y,          u.y,          -f.y,        0,
          s.z, u.z, -f.z, 0, -Dot(s, eye), -Dot(u, eye), Dot(f, eye), 1};
}

Vec3 TransformDirection(const Mat4& m, const Vec3& v) {
  return {m.At(0, 0) * v.x + m.At(1, 0) * v.y + m.At(2, 0) * v.z,
          m.At(0, 1) * v.x + m.At(1, 1) * v.y + m.At(2, 1) * v.z,
          m.At(0, 2) * v.x + m.At(1, 2) * v.y + m.At(2, 2) * v.z};
}
