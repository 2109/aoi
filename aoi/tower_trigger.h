#ifndef TOWER_AOI_TRIGGER_H
#define TOWER_AOI_TRIGGER_H

#include "interface.h"

namespace AOI {
	namespace Tower {
		struct AOI;
		struct Trigger : public IAOITrigger {
			Trigger* prev_;
			Trigger* next_;
			AOI* aoi_;
			int uid_;
			int range_;
			uint8_t mask_;
			float pos_[2];

			Trigger();

			Trigger(AOI* aoi, int uid, uint8_t mask, int range);

			void Add(Trigger* trigger) {
				Trigger* prev = prev_;
				prev->next_ = trigger;
				trigger->next_ = this;
				trigger->prev_ = prev;
				prev_ = trigger;
			}

			void Remove() {
				Trigger* next = next_;
				Trigger* prev = prev_;
				next->prev_ = prev;
				prev->next_ = next;
				prev_ = next_ = NULL;
			}

			virtual void Move(float x, float z);

			virtual void Enter(float x, float z);

			virtual void Leave();

			virtual void OnEnter(IAOIEntity* entity);

			virtual void OnLeave(IAOIEntity* entity);
		};

	}
}
#endif

