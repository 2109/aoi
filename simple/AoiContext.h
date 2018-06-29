#pragma once

extern "C" {
#include "simple/simple-aoi.h"
}

#include <map>

class Aoi;

class AoiContext
{
public:
	AoiContext(float width,float height, float range);
	~AoiContext();

	static void OnEntityEnter(int self, int other, void* ud);

	static void OnEntityLeave(int self, int other, void* ud);

	void CreateEntity();

	void CreateTrigger();

	void RefEntity(int uid);

	void DeRefEntity(int uid);
	
	void Update(float interval);

	void Draw();
public:
	float m_width;
	float m_height;
	float m_range;
	int m_countor;
	struct aoi_context* m_context;
	std::map<int, int> m_entity_status;
	std::map<int, Aoi*> m_entity_list;
	std::map<int, Aoi*> m_trigger_list;
};

