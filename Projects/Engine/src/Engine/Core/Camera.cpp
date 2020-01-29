#include "stdafx.h"
#include "Camera.h"

#include <glm/gtx/transform.hpp>
#include "Display/Display.h"

ym::Camera::Camera(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& up, float nearPlane, float farPlane, float fov) :
	m_pos(pos), m_dir(dir), m_up(up), m_nearPlane(nearPlane), m_farPlane(farPlane), m_fov(fov), m_proj(1.0), m_view(1.0)
{

}

float ym::Camera::getNearPlane() const
{
	return m_nearPlane;
}

float ym::Camera::getFarPlane() const
{
	return m_farPlane;
}

glm::vec3 ym::Camera::getPos() const
{
	return m_pos;
}

glm::vec3 ym::Camera::getDir() const
{
	return m_dir;
}

void ym::Camera::updateView()
{
	YM_PROFILER_FUNCTION();
	m_view = glm::lookAtRH(m_pos, m_pos + m_dir, m_up);
}

void ym::Camera::updateProj()
{
	YM_PROFILER_FUNCTION();
	// TODO: Make it possible to change the type of projection!!
	float aspectRatio = Display::get()->getAspectRatio();
	m_proj = glm::perspectiveRH(m_fov, aspectRatio, m_nearPlane, m_farPlane);
}

glm::vec3 ym::Camera::getUp() const
{
	return m_up;
}

glm::vec3 ym::Camera::getRight() const
{
	return glm::normalize(glm::cross(m_dir, m_up));
}

void ym::Camera::setOrientaion(glm::vec3 up, glm::vec3& right)
{
	YM_PROFILER_FUNCTION();
	m_up = glm::normalize(up);
	m_dir = glm::normalize(glm::cross(up, right));
	updateView();
}

void ym::Camera::setPosition(glm::vec3 pos)
{
	m_pos = pos;
	updateView();
}

glm::mat4 ym::Camera::getView() const
{
	return m_view;
}

glm::mat4 ym::Camera::getProj() const
{
	return m_proj;
}
