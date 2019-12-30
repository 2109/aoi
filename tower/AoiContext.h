#pragma once

extern "C" {
#include "tower/tower-aoi.h"
}

#include <map>

class Aoi;

class AoiContext {
public:
	AoiContext(float width, float height, float range);
	~AoiContext();

	static void OnEntityEnter(int self, int other, void* ud);

	static void OnEntityLeave(int self, int other, void* ud);

	static void OnTriggerEnter(int self, int other, void* ud);

	static void OnTriggerLeave(int self, int other, void* ud);


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
	struct aoi* m_context;
	std::map<int, int> m_entity_status;
	std::map<int, Aoi*> m_entity_list;
	std::map<int, Aoi*> m_trigger_list;
};

