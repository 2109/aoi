#include <stdint.h>
#include "tower_entity.h"
#include "tower_aoi.h"


namespace AOI {
	namespace Tower {
		Entity::Entity() {
			prev_ = next_ = NULL;
			aoi_ = NULL;
			uid_ = 0;
			mask_ = 0;
			pos_[0] = pos_[1] = 0;
		}

		Entity::Entity(AOI* aoi, int uid, uint8_t mask) {
			prev_ = next_ = NULL;
			aoi_ = aoi;
			uid_ = uid;
			mask_ = mask;
			pos_[0] = pos_[1] = 0;
		}

		void Entity::Move(float x, float z) {
			aoi_->MoveEntity(this, x, z);
		}

		void Entity::Enter(float x, float z) {
			pos_[0] = x;
			pos_[1] = z;
			aoi_->AddEntity(this);
		}

		void Entity::Leave() {
			aoi_->RemoveEntity(this);
		}
	}
}