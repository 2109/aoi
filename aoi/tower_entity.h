#ifndef TOWER_AOI_EITITY_H
#define TOWER_AOI_EITITY_H

#include "interface.h"

namespace AOI {
	namespace Tower {
		struct AOI;
		struct Entity : public IAOIEntity {
			Entity* prev_;
			Entity* next_;
			AOI* aoi_;
			int uid_;
			uint8_t mask_;
			float pos_[2];

			Entity();

			Entity(AOI* aoi, int uid, uint8_t mask);

			virtual void Move(float x, float z);

			void Enter(float x, float z);

			void Leave();
		};
	}
}

#endif

