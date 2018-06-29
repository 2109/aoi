
#include "Vector2.h"
#include <math.h>

Vector2::Vector2(float x, float z)
{
	m_x = x;
	m_z = z;
}

Vector2::Vector2() {

}

Vector2::~Vector2()
{
}

float Vector2::Distance(Vector2& to) {
	return sqrt(( to.m_x - m_x ) * ( to.m_x - m_x ) + ( to.m_z - m_z ) * ( to.m_z - m_z ));
}

void Vector2::MoveForward(Vector2& to, float pass) {
	float dt = Distance(to);
	float ratio = pass / dt;
	if ( ratio > 1 )
		ratio = 1;
	this->Lerp(to, ratio);
}

void Vector2::Lerp(Vector2& to, float ratio) {
	m_x = m_x + ( to.m_x - m_x ) * ratio;
	m_z = m_z + ( to.m_z - m_z ) * ratio;
}