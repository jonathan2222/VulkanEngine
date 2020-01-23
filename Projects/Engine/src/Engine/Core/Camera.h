#pragma once

#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

namespace ym
{
	// TODO: Make an Entity Component System and add the camera as a Entity with a system!
	class Camera
	{
	public:
		Camera(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& up, float nearPlane, float farPlane, float fov);

		float getNearPlane() const;
		float getFarPlane() const;

		glm::vec3 getPos() const;
		glm::vec3 getDir() const;

		void updateView();
		void updateProj();

		glm::vec3 getUp() const;
		glm::vec3 getRight() const;
		void setOrientaion(glm::vec3 up, glm::vec3& right);
		void setPosition(glm::vec3 pos);

		glm::mat4 getView() const;
		glm::mat4 getProj() const;

	private:
		glm::vec3 m_pos;
		glm::vec3 m_dir;
		glm::vec3 m_up;
		float m_nearPlane;
		float m_farPlane;
		float m_fov;

		glm::mat4 m_view;
		glm::mat4 m_proj;
	};
}