#include "math.h"

#include <cmath>

bool areEqual(float a, float b, float epsilon) {
  return std::abs(a - b) < epsilon;
}

float signedArea(float ax, float ay, float bx, float by, float cx, float cy) {
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

Vec2 Vec2::operator-() const {
  return {-x, -y};
}

Vec2 Vec2::operator*(float s) const {
  return {x * s, y * s};
}

Vec2 Vec2::operator/(float s) const {
  return {x / s, y / s};
}

float Vec2::length() const {
  return std::sqrt(x * x + y * y);
}

Vec2 operator*(float s, const Vec2& v) {
  return v * s;
}

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

Vec3 Vec3::operator-() const {
  return {-x, -y, -z};
}

Vec3 Vec3::operator*(float s) const {
  return {x * s, y * s, z * s};
}

Vec3 Vec3::operator/(float s) const {
  return {x / s, y / s, z / s};
}

float Vec3::length() const {
  return std::sqrt(x * x + y * y + z * z);
}

Vec3 operator*(float s, const Vec3& v) {
  return v * s;
}

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

Vec4 Vec4::operator-() const {
  return {-x, -y, -z, -w};
}

Vec4 Vec4::operator*(float s) const {
  return {x * s, y * s, z * s, w * s};
}

Vec4 Vec4::operator/(float s) const {
  return {x / s, y / s, z / s, w / s};
}

float Vec4::length() const {
  return std::sqrt(x * x + y * y + z * z + w * w);
}

Vec4 operator*(float s, const Vec4& v) {
  return v * s;
}

float dot(const Vec2& a, const Vec2& b) {
  return a.x * b.x + a.y * b.y;
}

float dot(const Vec3& a, const Vec3& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z;
}

float dot(const Vec4& a, const Vec4& b) {
  return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec3 cross(const Vec3& a, const Vec3& b) {
  return {
    a.y * b.z - a.z * b.y,
    a.z * b.x - a.x * b.z,
    a.x * b.y - a.y * b.x
  };
}

Vec2 normalize(const Vec2& v) {
  return v / v.length();
}

Vec3 normalize(const Vec3& v) {
  return v / v.length();
}

Vec4 normalize(const Vec4& v) {
  return v / v.length();
}

Mat4 Mat4::identity() {
  return Mat4{};
}

Mat4 operator*(const Mat4& a, const Mat4& b) {
  Mat4 res;
  for (int col = 0; col < 4; ++col) {
    for (int row = 0; row < 4; ++row) {
      float sum = 0.0f;
      for (int k = 0; k < 4; ++k) {
        sum += a.at(k, row) * b.at(col, k);
      }
      res.at(col, row) = sum;
    }
  }
  return res;
}

Vec4 operator*(const Mat4& mat, const Vec4& v) {
  Vec4 res;
  res.x = mat.at(0, 0) * v.x +
          mat.at(1, 0) * v.y +
          mat.at(2, 0) * v.z +
          mat.at(3, 0) * v.w;
  res.y = mat.at(0, 1) * v.x +
          mat.at(1, 1) * v.y +
          mat.at(2, 1) * v.z +
          mat.at(3, 1) * v.w;
  res.z = mat.at(0, 2) * v.x +
          mat.at(1, 2) * v.y +
          mat.at(2, 2) * v.z +
          mat.at(3, 2) * v.w;
  res.w = mat.at(0, 3) * v.x +
          mat.at(1, 3) * v.y +
          mat.at(2, 3) * v.z +
          mat.at(3, 3) * v.w;
  return res;
}

Mat4 translation(const Vec3& t) {
  return {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    t.x, t.y, t.z, 1
  };
}

Mat4 scale(const Vec3& s) {
  return {
    s.x, 0, 0, 0,
    0, s.y, 0, 0,
    0, 0, s.z, 0,
    0, 0, 0, 1
  };
}

Mat4 scale(float s) {
  return scale({s, s, s});
}

Mat4 rotationX(float radians) {
  float c = std::cos(radians);
  float s = std::sin(radians);

  return {
    1, 0, 0, 0,
    0, c, s, 0,
    0, -s, c, 0,
    0, 0, 0, 1
  };
}

Mat4 rotationY(float radians) {
  float c = std::cos(radians);
  float s = std::sin(radians);

  return {
    c, 0, -s, 0,
    0, 1, 0, 0,
    s, 0, c, 0,
    0, 0, 0, 1
  };
}

Mat4 rotationZ(float radians) {
  float c = std::cos(radians);
  float s = std::sin(radians);

  return {
    c, s, 0, 0,
    -s, c, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };
}

Mat4 perspective(float fovY, float aspect, float near, float far) {
  float f = 1.0f / std::tan(fovY * 0.5f);

  return {
    f / aspect, 0, 0, 0,
    0, f, 0, 0,
    0, 0, (far + near) / (near - far), -1.0f,
    0, 0, (2.0f * far * near) / (near - far), 0.0f
  };
}

Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up) {
  Vec3 f = normalize(center - eye);
  Vec3 s = normalize(cross(f, up));
  Vec3 u = cross(s, f);

  return {
    s.x, u.x, -f.x, 0,
    s.y, u.y, -f.y, 0,
    s.z, u.z, -f.z, 0,
    -dot(s, eye), -dot(u, eye), dot(f, eye), 1
  };
}

Vec3 transformDirection(const Mat4& m, const Vec3& v) {
  return {
    m.at(0, 0) * v.x + m.at(1, 0) * v.y + m.at(2, 0) * v.z,
    m.at(0, 1) * v.x + m.at(1, 1) * v.y + m.at(2, 1) * v.z,
    m.at(0, 2) * v.x + m.at(1, 2) * v.y + m.at(2, 2) * v.z
  };
}
