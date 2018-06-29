
#include "AoiTrigger.h"
#include "glut.h"

AoiTrigger::AoiTrigger(float x, float z, float speed, float range, AoiContext* context) :Aoi(x, z, speed, context)
{
	m_range = range;
	
}


AoiTrigger::~AoiTrigger()
{
}

void AoiTrigger::Enter()
{
	m_trigger = create_trigger(m_context->m_context, m_id, m_pos.m_x, m_pos.m_z, m_range, AoiContext::OnTriggerEnter, m_context);
}

void AoiTrigger::Update(float interval) {
	Aoi::Update(interval);
	move_trigger(m_context->m_context, m_trigger, m_pos.m_x, m_pos.m_z, AoiContext::OnTriggerEnter, m_context, AoiContext::OnTriggerLeave, m_context);
}

void AoiTrigger::Draw()
{
	glColor3f(0.0f, 0.0f, 1.0f);
	Aoi::Draw();
	glColor3f(0.0f, 1.0f, 0.0f);
	glLineStipple(2, 0x5555);
	glEnable(GL_LINE_STIPPLE);
	glBegin(GL_LINES);

	float range = m_range * 5;
	glVertex3f(m_pos.m_x - range, m_pos.m_z - range, 0);
	glVertex3f(m_pos.m_x - range, m_pos.m_z + range, 0);

	glVertex3f(m_pos.m_x - range, m_pos.m_z - range, 0);
	glVertex3f(m_pos.m_x + range, m_pos.m_z - range, 0);

	glVertex3f(m_pos.m_x - range, m_pos.m_z + range, 0);
	glVertex3f(m_pos.m_x + range, m_pos.m_z + range, 0);

	glVertex3f(m_pos.m_x + range, m_pos.m_z - range, 0);
	glVertex3f(m_pos.m_x + range, m_pos.m_z + range, 0);

	glEnd();
	glDisable(GL_LINE_STIPPLE);
}
