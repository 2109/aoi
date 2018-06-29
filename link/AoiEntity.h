#pragma once

#include "Aoi.h"


class AoiEntity : public Aoi
{
public:
	AoiEntity(float x, float z, float speed, AoiContext* context);
	~AoiEntity();

	virtual void Enter();
	virtual void Update(float interval);
	virtual void Draw();

public:
};

