#include <stdint.h>
#include "tower_entity.h"
#include "tower_trigger.h"
#include "tower_aoi.h"

namespace AOI {
	namespace Tower {
		Trigger::Trigger() {
			next_ = prev_ = this;
			aoi_ = NULL;
			uid_ = 0;
			mask_ = 0;
			range_ = 0;
			pos_[0] = pos_[1] = 0;
		}

		Trigger::Trigger(AOI* aoi, int uid, uint8_t mask, int range) {
			next_ = prev_ = this;
			aoi_ = aoi;
			uid_ = uid;
			mask_ = mask;
			range_ = range;
			pos_[0] = pos_[1] = 0;
		}

		void Trigger::Move(float x, float z) {
			aoi_->MoveTrigger(this, x, z);
		}

		void Trigger::Enter(float x, float z) {
			pos_[0] = x;
			pos_[1] = z;
			aoi_->AddTrigger(this);
		}

		void Trigger::Leave() {
			aoi_->RemoveTrigger(this);
		}

		void Trigger::OnEnter(IAOIEntity* entity) {

		}

		void Trigger::OnLeave(IAOIEntity* entity) {

		}

	}
}