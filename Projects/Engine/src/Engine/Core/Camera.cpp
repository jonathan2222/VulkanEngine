#include "stdafx.h"
#include "Camera.h"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/norm.hpp"
#include "Input/Input.h"

#define FRUSTUM_SHRINK_FACTOR -5.f

namespace ym
{
	void Camera::init(float aspect, float fov, const glm::vec3& position, const glm::vec3& target, float speed, float hasteSpeed)
	{
		this->playerHeight = 2.0f;
		this->position = position;
		this->aspect = aspect;
		this->target = target;
		this->speed = speed;
		this->hasteSpeed = hasteSpeed;
		this->globalUp = glm::vec3(0.f, 1.f, 0.f);

		this->forward = glm::normalize(this->target - this->position);
		this->up = this->globalUp;
		this->right = glm::cross(this->forward, this->up);

		this->fov = fov;
		this->nearPlane = 0.1f;
		this->farPlane = 1000.f;
		this->yaw = 270;
		this->pitch = 0;
		this->roll = 0;
		this->speedFactor = 1.f;

		float tang = tan(this->fov / 2);
		this->nearHeight = this->nearPlane * tang * 2.0f;
		this->nearWidth = this->nearHeight * this->aspect;
		this->farHeight = this->farPlane * tang * 2.0f;
		this->farWidth = this->farHeight * this->aspect;

		this->planes.resize(6);
		updatePlanes();
	}

	void Camera::destroy()
	{
	}

	void Camera::update(float dt)
	{
		static Input* input = Input::get();

		if (input->isKeyPressed(Key::A))
			this->position -= this->right * this->speed * dt * this->speedFactor;
		if (input->isKeyPressed(Key::D))
			this->position += this->right * this->speed * dt * this->speedFactor;

		if (input->isKeyPressed(Key::W))
			this->position += this->forward * this->speed * dt * this->speedFactor;
		if (input->isKeyPressed(Key::S))
			this->position -= this->forward * this->speed * dt * this->speedFactor;

		if (input->isKeyPressed(Key::SPACE))
			this->position += this->globalUp * this->speed * dt * this->speedFactor;
		if (input->isKeyPressed(Key::LEFT_CONTROL))
			this->position -= this->globalUp * this->speed * dt * this->speedFactor;

		/*if (input.getKeyState(GLFW_KEY_F) == Input::KeyState::FIRST_RELEASED)
		{
			this->gravityOn ^= true;
			JAS_INFO("Toggled gravity: {}", this->gravityOn ? "On" : "Off");
		}

		if (this->gravityOn)
		{
			this->position.y = floor + this->playerHeight;
		}*/

		if (input->isKeyPressed(Key::LEFT_SHIFT))
			this->speedFactor = hasteSpeed;
		else
			this->speedFactor = 1.f;

		glm::vec2 cursorDelta = input->getCursorDelta();
		cursorDelta *= 0.1f;

		if (glm::length2(cursorDelta) > glm::epsilon<float>())
		{
			this->yaw += cursorDelta.x;
			this->pitch -= cursorDelta.y;

			//YM_LOG_WARN("yaw: {}, pitch: {}", this->yaw, this->pitch);

			if (this->pitch > 89.0f)
				this->pitch = 89.0f;
			if (this->pitch < -89.0f)
				this->pitch = -89.0f;

			glm::vec3 direction;
			direction.x = cos(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));
			direction.y = sin(glm::radians(this->pitch));
			direction.z = sin(glm::radians(this->yaw)) * cos(glm::radians(this->pitch));

			this->forward = glm::normalize(direction);
			this->right = glm::normalize(glm::cross(this->forward, this->globalUp));
			this->up = glm::cross(this->right, this->forward);

		}
		updatePlanes();
		this->target = this->position + this->forward;
	}

	void Camera::setPosition(const glm::vec3 & position)
	{
		this->position = position;
	}

	void Camera::setSpeed(float speed)
	{
		this->speed = speed;
	}

	glm::mat4 Camera::getMatrix() const
	{
		return getProjection()* getView();
	}

	glm::mat4 Camera::getProjection() const
	{
		glm::mat4 proj = glm::perspective(this->fov, this->aspect, this->nearPlane, this->farPlane);
		proj[1][1] *= -1;
		return proj;
	}

	glm::mat4 Camera::getView() const
	{
		return glm::lookAt(this->position, this->target, this->up);
	}

	glm::vec3 Camera::getPosition() const
	{
		return this->position;
	}

	glm::vec3 Camera::getDirection() const
	{
		return glm::normalize(this->target - this->position);
	}

	glm::vec3 Camera::getRight() const
	{
		return this->right;
	}

	glm::vec3 Camera::getUp() const
	{
		return this->up;
	}

	const std::vector<Camera::Plane>& Camera::getPlanes() const
	{
		return this->planes;
	}

	void Camera::updatePlanes()
	{
		glm::vec3 point;
		glm::vec3 nearCenter = this->position + this->forward * this->nearPlane;
		glm::vec3 farCenter = this->position + this->forward * this->farPlane;

		this->planes[NEAR_P].normal = this->forward;
		this->planes[NEAR_P].point = nearCenter;

		this->planes[FAR_P].normal = -this->forward;
		this->planes[FAR_P].point = farCenter;

		point = nearCenter + this->up * (this->nearHeight / 2) - this->right * (this->nearWidth / 2);
		this->planes[LEFT_P].normal = glm::normalize(glm::cross(point - this->position, this->up));
		this->planes[LEFT_P].point = point + this->planes[LEFT_P].normal * FRUSTUM_SHRINK_FACTOR;

		this->planes[TOP_P].normal = glm::normalize(glm::cross(point - this->position, this->right));
		this->planes[TOP_P].point = point;

		point = nearCenter - this->up * (this->nearHeight / 2) + this->right * (this->nearWidth / 2);
		this->planes[RIGHT_P].normal = glm::normalize(glm::cross(point - this->position, -this->up));
		this->planes[RIGHT_P].point = point + this->planes[RIGHT_P].normal * FRUSTUM_SHRINK_FACTOR;

		this->planes[BOTTOM_P].normal = glm::normalize(glm::cross(point - this->position, -this->right));
		this->planes[BOTTOM_P].point = point;
	}
}