#pragma once
class Vector2 {
public:
	Vector2(float x, float z);
	Vector2();
	~Vector2();

	float Distance(Vector2& to);

	void MoveForward(Vector2& to, float pass);

	void Lerp(Vector2& to, float ratio);
public:
	float m_x;
	float m_z;
};

