#pragma once

#include "Vector2.h"
#include "AoiContext.h"
class Aoi
{
public:
	Aoi(float x,float z,float speed,AoiContext* context);
	~Aoi();

	void RandomTarget();

	void SetTarget(float x, float z);

	virtual void Enter() = 0;

	virtual void Update(float interval);

	virtual void Draw();

	virtual void Ref();
	virtual void DeRef();

public:
	float m_radius;
	float m_speed;
	int m_id;
	int m_ref;
	AoiContext* m_context;
	Vector2 m_pos;
	Vector2 m_target;
};

