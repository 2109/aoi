#pragma once
#include "Aoi.h"
#include "AoiContext.h"
class AoiTrigger :
	public Aoi
{
public:
	AoiTrigger(float x, float z, float speed,float range, AoiContext* context);
	~AoiTrigger();
	virtual void Enter();
	virtual void Update(float interval);
	virtual void Draw();

public:
	int m_range;
};

