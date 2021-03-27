#include "tower_aoi.h"
#include "tower_entity.h"

TowerAOIEntity::TowerAOIEntity(TowerAOI* aoi, int uid, float x, float z) {
	aoi_ = aoi;
	uid_ = uid;
	pos_[0] = x;
	pos_[1] = z;
}

void TowerAOIEntity::Move(float x, float z) {

}