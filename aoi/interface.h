#ifndef IAOI_INTERFACE_H
#define IAOI_INTERFACE_H

struct IAOIEntity {
	virtual void Move(float x, float z) = 0;
};

struct IAOITrigger {
	virtual void Move(float x, float z) = 0;

	virtual void OnEnter(IAOIEntity* entity) = 0;

	virtual void OnLeave(IAOIEntity* entity) = 0;
};

struct IAOI {
	virtual IAOIEntity* AddEntity(IAOIEntity* entity) = 0;
	virtual void RemoveEntity(IAOIEntity* entity) = 0;

	virtual IAOITrigger* AddTrigger(IAOITrigger* entity) = 0;
	virtual void RemoveTrigger(IAOITrigger* entity) = 0;
};

#endif

