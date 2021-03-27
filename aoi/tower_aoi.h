#ifndef TOWER_AOI_H
#define TOWER_AOI_H
#include "interface.h"
#include "hash_map.h"

struct TowerAOIEntity;
struct TowerAOI : public IAOI {
	struct Tower {
		TowerAOIEntity link_;
		hash_map_t hash_;
	};

	uint32_t width_;
	uint32_t height_;
	uint32_t cell_;
	Tower** towers_;
	uint32_t tower_x;
	uint32_t tower_z;

	virtual TowerAOIEntity* AddEntity(TowerAOIEntity* entity);
	virtual void RemoveEntity(TowerAOIEntity* entity);

	virtual TowerAOITrigger* AddTrigger(TowerAOITrigger* entity);
	virtual void RemoveTrigger(TowerAOITrigger* entity);
};
#endif