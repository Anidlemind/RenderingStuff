#ifndef RENDERER_SRC_MATH_MATH_H_
#define RENDERER_SRC_MATH_MATH_H_

#include <cmath>

inline constexpr float kEpsilon = 1e-6f;
inline constexpr float kPi = 3.14159265358979323846f;

bool AreEqual(float a, float b, float eps = kEpsilon);

inline float Radians(float degrees) { return degrees * (kPi / 180.0f); }

float SignedArea(float ax, float ay, float bx, float by, float cx, float cy);

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

  float Length() const;
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

  float Length() const;
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

  float Length() const;
};

Vec4 operator*(float s, const Vec4& v);

float Dot(const Vec2& a, const Vec2& b);
float Dot(const Vec3& a, const Vec3& b);
float Dot(const Vec4& a, const Vec4& b);

Vec3 Cross(const Vec3& a, const Vec3& b);

Vec2 Normalize(const Vec2& v);
Vec3 Normalize(const Vec3& v);
Vec4 Normalize(const Vec4& v);

struct Mat4 {
  float m[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};

  float& At(int col, int row) { return m[col * 4 + row]; }
  const float& At(int col, int row) const { return m[col * 4 + row]; }

  static Mat4 Identity();
};

Mat4 operator*(const Mat4& a, const Mat4& b);
Vec4 operator*(const Mat4& mat, const Vec4& v);

Mat4 Translation(const Vec3& t);

Mat4 Scale(const Vec3& s);
Mat4 Scale(float s);

Mat4 RotationX(float radians);
Mat4 RotationY(float radians);
Mat4 RotationZ(float radians);

Mat4 Perspective(float fov_y, float aspect, float near, float far);
Mat4 LookAt(const Vec3& eye, const Vec3& center, const Vec3& up);

Vec3 TransformDirection(const Mat4& m, const Vec3& v);

#endif  // RENDERER_SRC_MATH_MATH_H_
