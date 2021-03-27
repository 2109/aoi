#include <algorithm>
#include "tower_entity.h"
#include "tower_trigger.h"
#include "tower_aoi.h"

namespace AOI {
	namespace Tower {
		AOI::AOI(uint32_t w, uint32_t h, int cell) {
			if (cell > w || cell > h) {
				cell = std::min(w, h);
			}
			w_ = w;
			h_ = h;
			cell_ = cell;

			tw_ = w_ / cell_ + 1;
			th_ = h_ / cell_ + 1;

			towers_ = (Tower**)malloc(tw_ * sizeof(*towers_));
			for (uint32_t x = 0; x < tw_; x++) {
				towers_[x] = new Tower[th_];
			}
		}

		void AOI::GetRegion(int* pos, int* region, int range) {
			if (pos[0] - range < 0) {
				region[0] = 0;
				region[2] = pos[0] + range;
				if (region[2] < 0) {
					region[2] = 0;
				} else if (region[2] >= tw_) {
					region[2] = tw_ - 1;
				}
			} else if (pos[0] + range >= tw_) {
				region[2] = tw_ - 1;
				region[0] = pos[0] - range;
				if (region[0] < 0) {
					region[0] = 0;
				} else if (region[0] >= tw_) {
					region[0] = tw_ - 1;
				}
			} else {
				region[0] = pos[0] - range;
				region[2] = pos[0] + range;
			}

			if (pos[1] - range < 0) {
				region[1] = 0;
				region[3] = pos[1] + range;
				if (region[3] < 0) {
					region[3] = 0;
				} else if (region[3] >= th_) {
					region[3] = th_ - 1;
				}
			} else if (pos[1] + range >= th_) {
				region[3] = th_ - 1;
				region[1] = pos[1] - range;
				if (region[1] < 0) {
					region[1] = 0;
				} else if (region[1] >= th_) {
					region[1] = th_ - 1;
				}
			} else {
				region[1] = pos[1] - range;
				region[3] = pos[1] + range;
			}
		}

		void AOI::ForeachTriggerEnter(int64_t uid, void* value, void* ud) {
			Entity* entity = (Entity*)ud;
			Trigger* trigger = (Trigger*)value;
			if (trigger->uid_ != entity->uid_ && trigger->mask_ & entity->mask_) {
				trigger->OnEnter(entity);
			}
		}

		void AOI::ForeachTriggerLeave(int64_t uid, void* value, void* ud) {
			Entity* entity = (Entity*)ud;
			Trigger* trigger = (Trigger*)value;
			if (trigger->uid_ != entity->uid_ && trigger->mask_ & entity->mask_) {
				trigger->OnLeave(entity);
			}
		}

		int AOI::AddEntity(Entity* entity) {
			if (ValidPos(entity->pos_) < 0) {
				return -1;
			}
			Tower* tower = GetTower(entity->pos_);
			tower->Add(entity);

			hash_int64_foreach(tower->hash_, ForeachTriggerEnter, entity);
			return 0;
		}

		int AOI::RemoveEntity(Entity* entity) {
			Tower* tower = GetTower(entity->pos_);
			tower->Remove(entity);
			hash_int64_foreach(tower->hash_, ForeachTriggerLeave, entity);
			return 0;
		}

		void AOI::LinkTriggerEnter(int64_t uid, void* value, void* ud) {
			FoearchArgs* args = (FoearchArgs*)ud;
			Trigger* trigger = (Trigger*)value;

			if (trigger->uid_ != args->entity_->uid_) {
				if (trigger->next_ != NULL) {
					trigger->Remove();
				} else {
					args->link_->Add(trigger);
				}
			}
		}

		void AOI::LinkTriggerLeave(int64_t uid, void* value, void* ud) {
			FoearchArgs* args = (FoearchArgs*)ud;
			Trigger* trigger = (Trigger*)value;
			if (trigger->uid_ != args->entity_->uid_) {
				args->link_->Add(trigger);
			}
		}

		int AOI::MoveEntity(Entity* entity, float x, float z) {
			float p[2] = { x, z };
			if (ValidPos(p) < 0) {
				return -1;
			}

			Tower* otower = GetTower(entity->pos_);

			entity->pos_[0] = x;
			entity->pos_[1] = z;

			Tower* ntower = GetTower(entity->pos_);
			if (ntower == otower) {
				return 0;
			}

			otower->Remove(entity);
			ntower->Add(entity);

			Trigger leave;
			Trigger enter;

			FoearchArgs args = { &leave, entity };
			hash_int64_foreach(otower->hash_, LinkTriggerLeave, &args);

			args.link_ = &enter;
			args.entity_ = entity;
			hash_int64_foreach(otower->hash_, LinkTriggerEnter, &args);

			Trigger* trigger = leave.next_;
			for (; trigger != &leave;) {
				Trigger* cur = trigger;
				trigger = trigger->next_;
				if (cur->mask_ & entity->mask_) {
					trigger->OnLeave(entity);
				}
				cur->next_ = cur->prev_ = NULL;
			}

			trigger = enter.next_;
			for (; trigger != &enter;) {
				Trigger* cur = trigger;
				trigger = trigger->next_;
				if (cur->mask_ & entity->mask_) {
					trigger->OnEnter(entity);
				}
				cur->next_ = cur->prev_ = NULL;
			}

			return 0;
		}

		int AOI::AddTrigger(Trigger* trigger) {
			if (ValidPos(trigger->pos_) < 0) {
				return -1;
			}

			int out[2];
			Translate(trigger->pos_, out);

			int region[4];
			GetRegion(out, region, trigger->range_);

			for (int x = region[0]; x <= region[2]; x++) {
				for (int z = region[1]; z <= region[3]; z++) {
					Tower* tower = &towers_[x][z];
					hash_int64_set(tower->hash_, trigger->uid_, trigger);

					Entity* entity = tower->link_.next_;
					for (; entity != &tower->link_; entity = entity->next_) {
						if (entity->uid_ != trigger->uid_ && (trigger->mask_ & entity->mask_)) {
							trigger->OnEnter(entity);
						}
					}
				}
			}
			return 0;
		}

		int AOI::RemoveTrigger(Trigger* trigger) {
			int out[2];
			Translate(trigger->pos_, out);

			int region[4];
			GetRegion(out, region, trigger->range_);

			for (int x = region[0]; x <= region[2]; x++) {
				for (int z = region[1]; z <= region[3]; z++) {
					Tower* tower = &towers_[x][z];
					hash_int64_del(tower->hash_, trigger->uid_);

					Entity* entity = tower->link_.next_;
					for (; entity != &tower->link_; entity = entity->next_) {
						if (entity->uid_ != trigger->uid_ && (trigger->mask_ & entity->mask_)) {
							trigger->OnLeave(entity);
						}
					}
				}
			}
			return 0;
		}

		int AOI::MoveTrigger(Trigger* trigger, float x, float z) {
			float p[2] = { x, z };
			if (ValidPos(p) < 0) {
				return -1;
			}

			int oout[2];
			Translate(trigger->pos_, oout);

			trigger->pos_[0] = x;
			trigger->pos_[1] = z;

			int nout[2];
			Translate(trigger->pos_, nout);

			if (oout[0] == nout[0] && oout[1] == nout[1]) {
				return 0;
			}

			int oregion[4];
			GetRegion(oout, oregion, trigger->range_);

			int nregion[4];
			GetRegion(nout, nregion, trigger->range_);

			for (int x = oregion[0]; x <= oregion[2]; x++) {
				for (int z = oregion[1]; z <= oregion[3]; z++) {
					if (x >= nregion[0] && x <= nregion[2] && z >= nregion[1] && z <= nregion[3]) {
						continue;
					}
					Tower* tower = &towers_[x][z];
					hash_int64_del(tower->hash_, trigger->uid_);

					Entity* entity = tower->link_.next_;
					for (; entity != &tower->link_; entity = entity->next_) {
						if (entity->uid_ != trigger->uid_ && (trigger->mask_ & entity->mask_)) {
							trigger->OnLeave(entity);
						}
					}
				}
			}

			for (int x = nregion[0]; x <= nregion[2]; x++) {
				for (int z = nregion[1]; z <= nregion[3]; z++) {
					if (x >= oregion[0] && x <= oregion[2] && z >= oregion[1] && z <= oregion[3]) {
						continue;
					}
					Tower* tower = &towers_[x][z];
					hash_int64_set(tower->hash_, trigger->uid_, trigger);

					Entity* entity = tower->link_.next_;
					for (; entity != &tower->link_; entity = entity->next_) {
						if (entity->uid_ != trigger->uid_ && (trigger->mask_ & entity->mask_)) {
							trigger->OnEnter(entity);
						}
					}
				}
			}

			return 0;
		}
	}
}