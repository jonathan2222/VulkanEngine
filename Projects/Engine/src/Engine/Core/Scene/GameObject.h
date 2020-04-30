#pragma once

#include "stdafx.h"
#include "Model/Model.h"

namespace ym
{
	class GameObject
	{
	public:
		GameObject();
		~GameObject();

		void init(const glm::vec3& pos, Model* modelPtr);
		void init(const glm::mat4& transform, Model* modelPtr);
		void destroy();

		void setTransform(const glm::mat4& transform);

		glm::vec3 getPos() const;
		void setPos(const glm::vec3& pos);
		glm::mat4 getTransform() const;

		Model* getModel();

	private:
		glm::vec3 pos;
		glm::mat4 transform;
		Model* modelPtr;
	};
}