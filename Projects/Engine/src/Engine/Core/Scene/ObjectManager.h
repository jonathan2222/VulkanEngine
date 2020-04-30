#pragma once

#include "stdafx.h"

namespace ym
{
	class Model;
	class GameObject;
	class ObjectManager
	{
	public:
		ObjectManager();
		~ObjectManager();

		static ObjectManager* get();

		void init();
		void destroy();
		void update();

		GameObject* createGameObject(const glm::vec3& pos, Model* model);
		GameObject* createGameObject(const glm::mat4& transform, Model* model);
		bool removeGameObject(GameObject* gameObject);

		std::unordered_map<Model*, std::vector<GameObject*>>& getGameObjects();

	private:
		std::unordered_map<Model*, std::vector<GameObject*>> gameObjects;
		std::vector<GameObject*> shouldRemove;
	};
}