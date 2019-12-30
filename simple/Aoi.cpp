
#include "Aoi.h"
#include <stdlib.h>
#include <math.h>
#include "glut.h"


const GLfloat R = 0.5f;
const GLfloat pi = 3.1415926536f;

Aoi::Aoi(float x, float z, float speed, AoiContext* context) :m_pos(x, z) {
	m_context = context;
	m_speed = speed;
	m_radius = 5;
	m_ref = 0;
	m_id = context->m_countor++;
}

Aoi::~Aoi() {
}

void Aoi::RandomTarget() {
	this->SetTarget(rand() % (int)m_context->m_width, rand() % (int)m_context->m_height);
}

void Aoi::SetTarget(float x, float z) {
	m_target.m_x = x;
	m_target.m_z = z;
}

void Aoi::Update(float interval) {
	float dt = m_pos.Distance(m_target);
	if (dt <= 5) {
		RandomTarget();
	} else {
		float dt = m_speed * interval;
		m_pos.MoveForward(m_target, dt);
	}
}

void Aoi::Draw() {
	glBegin(GL_POLYGON);
	if (m_ref > 0) {
		glColor3f(0.0f, 0.5f, 1.0f);
	} else {
		glColor3f(0.5f, 1.0f, 1.0f);
	}
	int n = 10;
	for (int i = 0; i < n; i++) {
		glVertex2f(m_pos.m_x + m_radius*cos(2 * pi / n*i), m_pos.m_z + m_radius*sin(2 * pi / n*i));
	}
	glEnd();
}

void Aoi::Ref() {
	m_ref++;
}

void Aoi::DeRef() {
	m_ref--;
}