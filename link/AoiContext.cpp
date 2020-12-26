
#include "AoiContext.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "AoiEntity.h"
#include "AoiTrigger.h"

AoiContext::AoiContext(float width, float height, float range) {
	m_width = width;
	m_height = height;
	m_range = range;
	m_countor = 1;
	m_context = create_aoi_ctx();
}


AoiContext::~AoiContext() {
}

void AoiContext::CreateTrigger() {
	Aoi* aoi = new AoiTrigger(rand() % (int)m_width, rand() % (int)m_height, rand() % 10 + 5, rand() % (int)m_range + 2, this);
	aoi->RandomTarget();
	m_trigger_list[aoi->m_id] = aoi;
	aoi->Enter();
}

void AoiContext::CreateEntity() {
	Aoi* aoi = new AoiEntity(rand() % (int)m_width, rand() % (int)m_height, rand() % 10 + 5, this);
	aoi->RandomTarget();
	m_entity_list[aoi->m_id] = aoi;
	aoi->Enter();
}


void AoiContext::OnEntityEnter(int self, int other, void* ud) {
	AoiContext* inst = (AoiContext*)ud;
	inst->RefEntity(other);
}

void AoiContext::OnEntityLeave(int self, int other, void* ud) {
	AoiContext* inst = (AoiContext*)ud;
	inst->DeRefEntity(other);
}

void AoiContext::RefEntity(int uid) {
	std::map<int, Aoi*>::iterator iter = m_entity_list.find(uid);
	if (iter != m_entity_list.end()) {
		Aoi* aoi = iter->second;
		aoi->Ref();
	} else {
		assert(0);
	}

}

void AoiContext::DeRefEntity(int uid) {
	std::map<int, Aoi*>::iterator iter = m_entity_list.find(uid);
	if (iter != m_entity_list.end()) {
		Aoi* aoi = iter->second;
		aoi->DeRef();
	} else {
		assert(0);
	}
}


void AoiContext::Update(float interval) {
	std::map<int, Aoi*>::iterator iter = m_entity_list.begin();
	for (; iter != m_entity_list.end(); iter++) {
		Aoi* aoi = iter->second;
		aoi->Update(interval);
	}

	iter = m_trigger_list.begin();
	for (; iter != m_trigger_list.end(); iter++) {
		Aoi* aoi = iter->second;
		aoi->Update(interval);
	}
}

void AoiContext::Draw() {
	std::map<int, Aoi*>::iterator iter = m_entity_list.begin();
	for (; iter != m_entity_list.end(); iter++) {
		Aoi* aoi = iter->second;
		aoi->Draw();
	}

	iter = m_trigger_list.begin();
	for (; iter != m_trigger_list.end(); iter++) {
		Aoi* aoi = iter->second;
		aoi->Draw();
	}
}