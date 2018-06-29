
#include "AoiContext.h"
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "AoiEntity.h"
#include "AoiTrigger.h"

AoiContext::AoiContext(float width,float height)
{
	m_width = width;
	m_height = height;
	m_countor = 1;
	m_context = create_aoi_ctx();
}


AoiContext::~AoiContext()
{
}

void AoiContext::CreateTrigger()
{
	Aoi* aoi = new AoiTrigger(rand() % (int)m_width, rand() % (int)m_height, 10, 10, this);
	aoi->RandomTarget();
	m_trigger_list[aoi->m_id] = aoi;
	aoi->Enter();
}

void AoiContext::CreateEntity()
{
	Aoi* aoi = new AoiEntity(rand() % (int)m_width, rand() % (int)m_height, 10, this);
	aoi->RandomTarget();
	m_entity_list[aoi->m_id] = aoi;
	aoi->Enter();
}


void AoiContext::OnEntityEnter(int self, int other, void* ud) {
	AoiContext* inst = (AoiContext*)ud;
	inst->RefEntity(self);
}

void AoiContext::OnEntityLeave(int self, int other, void* ud) {
	AoiContext* inst = (AoiContext*)ud;
	inst->DeRefEntity(self);
}

void AoiContext::OnTriggerEnter(int self, int other, void* ud) {
	AoiContext* inst = (AoiContext*)ud;
	inst->RefEntity(other);
}

void AoiContext::OnTriggerLeave(int self, int other, void* ud) {
	AoiContext* inst = (AoiContext*)ud;
	inst->DeRefEntity(other);
}

void AoiContext::RefEntity(int uid)
{
	std::map<int, Aoi*>::iterator iter = m_entity_list.find(uid);
	if (iter != m_entity_list.end())
	{
		Aoi* aoi = iter->second;
		aoi->Ref();
	}
	else {
		std::map<int, Aoi*>::iterator iter = m_trigger_list.find(uid);
		Aoi* aoi = iter->second;
		aoi->Ref();
	}
	
}

void AoiContext::DeRefEntity(int uid)
{
	std::map<int, Aoi*>::iterator iter = m_entity_list.find(uid);
	if ( iter != m_entity_list.end() )
	{
		Aoi* aoi = iter->second;
		aoi->DeRef();
	}
	else {
		std::map<int, Aoi*>::iterator iter = m_trigger_list.find(uid);
		Aoi* aoi = iter->second;
		aoi->DeRef();
	}
}


void AoiContext::Update(float interval)
{
	std::map<int, Aoi*>::iterator iter = m_entity_list.begin();
	for ( ; iter != m_entity_list.end();iter++ )
	{
		Aoi* aoi = iter->second;
		aoi->Update(interval);
	}

	iter = m_trigger_list.begin();
	for ( ; iter != m_trigger_list.end(); iter++ )
	{
		Aoi* aoi = iter->second;
		aoi->Update(interval);
	}
}

void AoiContext::Draw()
{
	std::map<int, Aoi*>::iterator iter = m_entity_list.begin();
	for ( ; iter != m_entity_list.end(); iter++ )
	{
		Aoi* aoi = iter->second;
		aoi->Draw();
	}

	iter = m_trigger_list.begin();
	for ( ; iter != m_trigger_list.end(); iter++ )
	{
		Aoi* aoi = iter->second;
		aoi->Draw();
	}
}