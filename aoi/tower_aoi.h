#ifndef TOWER_AOI_H
#define TOWER_AOI_H
#include "interface.h"
#include "hash_map.h"

namespace AOI {
	namespace Tower {
		struct Entity;
		struct Trigger;

		struct Tower {
			Entity link_;
			hash_map_t hash_;

			Tower() {
				link_.prev_ = &link_;
				link_.next_ = &link_;
				hash_ = hash_int64_create();
			}

			inline void Add(Entity* entity) {
				Entity* prev = link_.prev_;
				prev->next_ = entity;
				entity->next_ = &link_;
				entity->prev_ = prev;
				link_.prev_ = entity;
			}

			inline void Remove(Entity* entity) {
				Entity* next = entity->next_;
				Entity* prev = entity->prev_;
				next->prev_ = prev;
				prev->next_ = next;
				entity->prev_ = NULL;
				entity->next_ = NULL;
			}
		};

		struct AOI : public IAOI {

			struct FoearchArgs {
				Trigger* link_;
				Entity* entity_;
			};

			uint32_t w_;
			uint32_t h_;
			uint32_t tw_;
			uint32_t th_;
			uint32_t cell_;
			Tower** towers_;

			AOI(uint32_t w, uint32_t h, int cell);

			inline int IsValid(float* pos) {
				if (pos[0] < 0 || pos[1] < 0 || pos[0] > w_ || pos[1] > h_) {
					return -1;
				}
				return 0;
			}

			inline void Translate(float* in, int* out) {
				out[0] = in[0] / cell_;
				out[1] = in[1] / cell_;
			}

			inline Tower* GetTower(float* pos) {
				int out[2];
				Translate(pos, out);
				return &towers_[(uint32_t)out[0]][(uint32_t)out[1]];
			}

			void GetRegion(int* pos, int* region, int range);

			virtual int AddEntity(Entity* entity);

			virtual int RemoveEntity(Entity* entity);

			virtual int MoveEntity(Entity* entity, float x, float z);

			virtual int AddTrigger(Trigger* trigger);

			virtual int RemoveTrigger(Trigger* trigger);

			virtual int MoveTrigger(Trigger* trigger, float x, float z);

			static void ForeachTriggerEnter(int64_t uid, void* value, void* ud);

			static void ForeachTriggerLeave(int64_t uid, void* value, void* ud);

			static void LinkTriggerEnter(int64_t uid, void* value, void* ud);

			static void LinkTriggerLeave(int64_t uid, void* value, void* ud);
		};
	}
}

#endif