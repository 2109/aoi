
#include "AoiTrigger.h"
#include "glut.h"

AoiTrigger::AoiTrigger(float x, float z, float speed, float range, AoiContext* context) :Aoi(x, z, speed, context) {
	m_range = range;
}


AoiTrigger::~AoiTrigger() {
}

void AoiTrigger::Enter() {
	create_trigger(m_context->m_context, m_object, m_pos.m_x, m_pos.m_z, m_range, AoiContext::OnTriggerEnter, AoiContext::OnTriggerLeave, m_context);
}

void AoiTrigger::Update(float interval) {
	Aoi::Update(interval);
	move_trigger(m_context->m_context, m_object, m_pos.m_x, m_pos.m_z, m_context);
}

void AoiTrigger::Draw() {
	glColor3f(0.0f, 0.0f, 1.0f);
	Aoi::Draw();
	glColor3f(0.0f, 1.0f, 0.0f);
	glLineStipple(2, 0x5555);
	glEnable(GL_LINE_STIPPLE);
	glBegin(GL_LINES);

	glVertex3f(m_pos.m_x - m_range, m_pos.m_z - m_range, 0);
	glVertex3f(m_pos.m_x - m_range, m_pos.m_z + m_range, 0);

	glVertex3f(m_pos.m_x - m_range, m_pos.m_z - m_range, 0);
	glVertex3f(m_pos.m_x + m_range, m_pos.m_z - m_range, 0);

	glVertex3f(m_pos.m_x - m_range, m_pos.m_z + m_range, 0);
	glVertex3f(m_pos.m_x + m_range, m_pos.m_z + m_range, 0);

	glVertex3f(m_pos.m_x + m_range, m_pos.m_z - m_range, 0);
	glVertex3f(m_pos.m_x + m_range, m_pos.m_z + m_range, 0);

	glEnd();
	glDisable(GL_LINE_STIPPLE);
}
