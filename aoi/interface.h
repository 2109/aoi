#ifndef IAOI_INTERFACE_H
#define IAOI_INTERFACE_H

struct IAOIEntity {
	virtual void Move(float x, float z) = 0;
	
	virtual void Enter(float x, float z) = 0;

	virtual void Leave() = 0;
};

struct IAOITrigger {
	virtual void Move(float x, float z) = 0;

	virtual void Enter(float x, float z) = 0;

	virtual void Leave() = 0;

	virtual void OnEnter(IAOIEntity* entity) = 0;

	virtual void OnLeave(IAOIEntity* entity) = 0;
};

struct IAOI {
	virtual int AddEntity(IAOIEntity* entity) = 0;
	virtual int RemoveEntity(IAOIEntity* entity) = 0;
	virtual int MoveEntity(IAOIEntity* entity, float x, float z) = 0;

	virtual int AddTrigger(IAOITrigger* entity) = 0;
	virtual int RemoveTrigger(IAOITrigger* entity) = 0;
	virtual int MoveTrigger(IAOIEntity* entity, float x, float z) = 0;
};

#endif

