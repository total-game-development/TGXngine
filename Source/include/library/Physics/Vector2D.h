#pragma once

#include <cassert>
#include <cmath>
#include <initializer_list>
#include <numbers>
#include <ostream>
#include <random>
#include <span>
#include "WorldState.h"
#include <string>
#include <vector>
#include "Logs.h"

namespace TGX
{
class Vector2D
{
public:
	float x;
	float y;

	constexpr Vector2D() : x(0), y(0) {}
	constexpr Vector2D(float _x, float _y) : x(_x), y(_y) {}
	constexpr Vector2D(const Vector2D &) = default;
	constexpr Vector2D(Vector2D &&) noexcept = default;
	constexpr Vector2D(std::span<const float, 2> s) : x(s[0]), y(s[1]) {}
	explicit Vector2D(const std::vector<float> &v)
	{
		assert(v.size() >= 2);

		x = v[0];
		y = v[1];
	}
	explicit Vector2D(std::initializer_list<float> list)
	{
		assert(list.size() >= 2);

		const auto *it = list.begin();
		x = *it++;
		y = *it;
	}

	constexpr Vector2D &operator=(const Vector2D &) = default;
	constexpr Vector2D &operator=(float v)
	{
		x = y = v;
		return *this;
	}
	constexpr Vector2D &operator=(std::span<const float, 2> s)
	{
		x = s[0];
		y = s[1];
		return *this;
	}
	Vector2D &operator=(const std::vector<float> &v)
	{
		assert(v.size() >= 2);

		x = v[0];
		y = v[1];
		return *this;
	}
	constexpr Vector2D &operator=(std::initializer_list<float> list)
	{
		assert(list.size() >= 2);

		const auto *it = list.begin();
		x = *it++;
		y = *it;
		return *this;
	}

	constexpr bool operator==(const Vector2D &other) const
	{
		return x == other.x && y == other.y;
	}
	constexpr bool operator!=(const Vector2D &other) const
	{
		return !(*this == other);
	}
	bool Equals(const Vector2D &other, const float epsilon = std::numeric_limits<float>::epsilon()) const
	{
		return std::fabs(x - other.x) <= epsilon && std::fabs(y - other.y) <= epsilon;
	}

	constexpr Vector2D operator+(const Vector2D &other) const
	{
		return Vector2D(x + other.x, y + other.y);
	}
	constexpr Vector2D operator-(const Vector2D &other) const
	{
		return Vector2D(x - other.x, y - other.y);
	}
	constexpr Vector2D operator*(float scalar) const
	{
		return Vector2D(x * scalar, y * scalar);
	}
	constexpr friend Vector2D operator*(float scalar, const Vector2D &v)
	{
		return v * scalar;
	}
	constexpr Vector2D operator/(float scalar) const
	{
		assert(scalar != 0.0f);

		return Vector2D(x / scalar, y / scalar);
	}
	constexpr Vector2D operator-() const
	{
		return Vector2D(-x, -y);
	}
	constexpr Vector2D &operator+=(const Vector2D &other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}
	constexpr Vector2D &operator-=(const Vector2D &other)
	{
		x -= other.x;
		y -= other.y;
		return *this;
	}
	constexpr Vector2D &operator*=(const Vector2D &other)
	{
		x *= other.x;
		y *= other.y;
		return *this;
	}
	constexpr Vector2D &operator/=(const Vector2D &other)
	{
		assert(other.x != 0.0f && other.y != 0.0f);

		x /= other.x;
		y /= other.y;
		return *this;
	}
	constexpr Vector2D &operator+=(float scalar)
	{
		x += scalar;
		y += scalar;
		return *this;
	}
	constexpr Vector2D &operator-=(float scalar)
	{
		x -= scalar;
		y -= scalar;
		return *this;
	}
	constexpr Vector2D &operator*=(float scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}
	constexpr Vector2D &operator/=(float scalar)
	{
		assert(scalar != 0.0f);

		x /= scalar;
		y /= scalar;
		return *this;
	}
	constexpr Vector2D &Add(const Vector2D &other)
	{
		return *this += other;
	}
	constexpr Vector2D &Add(const float scalar)
	{
		return *this += scalar;
	}
	constexpr Vector2D &Sub(const Vector2D &other)
	{
		return *this -= other;
	}
	constexpr Vector2D &Sub(const float scalar)
	{
		return *this -= scalar;
	}
	constexpr Vector2D &Mul(const Vector2D &other)
	{
		return *this *= other;
	}
	constexpr Vector2D &Mul(const float scalar)
	{
		return *this *= scalar;
	}
	constexpr Vector2D &Div(const Vector2D &other)
	{
		return *this /= other;
	}
	constexpr Vector2D &Div(const float scalar)
	{
		return *this /= scalar;
	}

	float Length() const
	{
		return std::sqrt(LengthSquared());
	}
	constexpr float LengthSquared() const
	{
		return (x * x) + (y * y);
	}
	Vector2D Normal() const
	{
		float len = Length();
		if (len == 0)
		{
			return Vector2D(0, 0);
		}
		float r = 1 / len;
		return Vector2D(x * r, y * r);
	}
	Vector2D &Normalize()
	{
		float len = Length();
		if (len == 0)
		{
			*this = 0;
			return *this;
		}
		float r = 1 / len;
		*this *= r;
		return *this;
	}

	constexpr float Dot(const Vector2D &other) const
	{
		return (x * other.x) + (y * other.y);
	}
	constexpr float Cross(const Vector2D &other) const
	{
		return (x * other.y) - (y * other.x);
	}
	float Heading() const
	{
		return std::atan2(y, x);
	}
	Vector2D Limit(float max) const
	{
		if (LengthSquared() > max * max)
		{
			return this->Normal() * max;
		}
		return *this;
	}
	Vector2D &SetMagnitude(float length)
	{
		Normalize();
		*this *= length;
		return *this;
	}

	void Display() const
	{
		Log::Debug(Str());
	}
	std::string Str() const
	{
		return "{" + std::to_string(x) + ", " + std::to_string(y) + "}";
	}
	friend std::ostream &operator<<(std::ostream &os, const Vector2D &vec)
	{
		os << vec.Str();
		return os;
	}

	static Vector2D Random()
	{
		std::uniform_real_distribution<float> dist(0.0f, 2.0f * std::numbers::pi_v<float>);

		return FromAngle(dist(WorldState::GetInstance().Random()));
	}
	static Vector2D FromAngle(float angle, float length = 1.0f)
	{
		return Vector2D(length * std::cos(angle), length * std::sin(angle));
	}
};

} // namespace TGX
