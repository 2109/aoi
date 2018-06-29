
#include "AoiEntity.h"
#include "glut.h"


AoiEntity::AoiEntity(float x, float z, float speed, AoiContext* context) :Aoi(x, z, speed, context)
{

}


AoiEntity::~AoiEntity()
{
}

void AoiEntity::Enter()
{
	m_entity = aoi_enter(m_context->m_context, m_id, m_pos.m_x, m_pos.m_z, LAYER_ITEM, m_context);
}

void AoiEntity::Update(float interval) {
	Aoi::Update(interval);
	aoi_update(m_context->m_context, m_entity, m_pos.m_x, m_pos.m_z, m_context);
}

void AoiEntity::Draw()
{
	Aoi::Draw();
}
