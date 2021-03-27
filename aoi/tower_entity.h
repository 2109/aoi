#ifndef TOWER_AOI_EITITY_H
#define TOWER_AOI_EITITY_H

#include "interface.h"
struct TowerAOIEntity : public IAOIEntity {
	TowerAOI* aoi_;
	int uid_;
	float pos_[2];

	TowerAOIEntity(TowerAOI* aoi, int uid, float x, float z);

	virtual void Move(float x, float z);
};


#endif

