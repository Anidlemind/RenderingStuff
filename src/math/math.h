#pragma once

#include <cmath>

inline constexpr float kEpsilon = 1e-6f;
inline constexpr float kPi = 3.14159265358979323846f;

bool areEqual(float a, float b, float eps = kEpsilon);

inline float radians(float degrees) {
  return degrees * (kPi / 180.0f);
}

float signedArea(float ax, float ay, float bx, float by, float cx, float cy);

struct Vec2 {
  float x = 0.0f;
  float y = 0.0f;

  Vec2& operator+=(const Vec2& other);
  Vec2& operator-=(const Vec2& other);
  Vec2& operator*=(float s);
  Vec2& operator/=(float s);

  Vec2 operator+(const Vec2& other) const;
  Vec2 operator-(const Vec2& other) const;
  Vec2 operator-() const;
  Vec2 operator*(float s) const;
  Vec2 operator/(float s) const;

  float length() const;
};

Vec2 operator*(float s, const Vec2& v);

struct Vec3 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;

  Vec3& operator+=(const Vec3& other);
  Vec3& operator-=(const Vec3& other);
  Vec3& operator*=(float s);
  Vec3& operator/=(float s);

  Vec3 operator+(const Vec3& other) const;
  Vec3 operator-(const Vec3& other) const;
  Vec3 operator-() const;
  Vec3 operator*(float s) const;
  Vec3 operator/(float s) const;

  float length() const;
};

Vec3 operator*(float s, const Vec3& v);

struct Vec4 {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float w = 0.0f;

  Vec4& operator+=(const Vec4& other);
  Vec4& operator-=(const Vec4& other);
  Vec4& operator*=(float s);
  Vec4& operator/=(float s);

  Vec4 operator+(const Vec4& other) const;
  Vec4 operator-(const Vec4& other) const;
  Vec4 operator-() const;
  Vec4 operator*(float s) const;
  Vec4 operator/(float s) const;

  float length() const;
};

Vec4 operator*(float s, const Vec4& v);

float dot(const Vec2& a, const Vec2& b);
float dot(const Vec3& a, const Vec3& b);
float dot(const Vec4& a, const Vec4& b);

Vec3 cross(const Vec3& a, const Vec3& b);

Vec2 normalize(const Vec2& v);
Vec3 normalize(const Vec3& v);
Vec4 normalize(const Vec4& v);

struct Mat4 {
  float m[16] = {
    1, 0, 0, 0,
    0, 1, 0, 0,
    0, 0, 1, 0,
    0, 0, 0, 1
  };

  float&       at(int col, int row)       { return m[col * 4 + row]; }
  const float& at(int col, int row) const { return m[col * 4 + row]; }

  static Mat4 identity();
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec4 operator*(const Mat4& mat, const Vec4& v);

Mat4 translation(const Vec3& t);

Mat4 scale(const Vec3& s);
Mat4 scale(float s);

Mat4 rotationX(float radians);
Mat4 rotationY(float radians);
Mat4 rotationZ(float radians);

Mat4 perspective(float fovY, float aspect, float near, float far);
Mat4 lookAt(const Vec3& eye, const Vec3& center, const Vec3& up);