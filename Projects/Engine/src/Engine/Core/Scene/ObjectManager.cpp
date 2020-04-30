#include "stdafx.h"
#include "ObjectManager.h"
#include "GameObject.h"

ym::ObjectManager::ObjectManager()
{
}

ym::ObjectManager::~ObjectManager()
{
}

ym::ObjectManager* ym::ObjectManager::get()
{
	static ObjectManager objManager;
	return &objManager;
}

void ym::ObjectManager::init()
{
}

void ym::ObjectManager::destroy()
{
	for (auto& batch : this->gameObjects)
	{
		auto& objs = batch.second;
		for (auto& obj : objs)
			SAFE_DELETE(obj);
		objs.clear();
	}
	this->gameObjects.clear();
}

void ym::ObjectManager::update()
{
	if (this->shouldRemove.empty() == false)
	{
		for (GameObject*& gameObject : this->shouldRemove)
		{
			auto it = this->gameObjects.find(gameObject->getModel());
			if (it != this->gameObjects.end())
			{
				gameObject->destroy();
				it->second.erase(std::remove(it->second.begin(), it->second.end(), gameObject), it->second.end());
				if (it->second.empty())
					this->gameObjects.erase(gameObject->getModel());
			}
		}
		this->shouldRemove.clear();
	}
}

ym::GameObject* ym::ObjectManager::createGameObject(const glm::vec3& pos, Model* model)
{
	GameObject* gameObject = new GameObject();
	gameObject->init(pos, model);

	auto it = this->gameObjects.find(model);
	if (it != this->gameObjects.end())
		it->second.push_back(gameObject);
	else
	{
		std::vector<GameObject*> gos = { gameObject };
		this->gameObjects[model] = gos;
	}

	return gameObject;
}

ym::GameObject* ym::ObjectManager::createGameObject(const glm::mat4& transform, Model* model)
{
	GameObject* gameObject = new GameObject();
	gameObject->init(transform, model);

	auto it = this->gameObjects.find(model);
	if (it != this->gameObjects.end())
		it->second.push_back(gameObject);
	else
	{
		std::vector<GameObject*> gos = { gameObject };
		this->gameObjects[model] = gos;
	}

	return gameObject;
}

bool ym::ObjectManager::removeGameObject(GameObject* gameObject)
{
	auto it = this->gameObjects.find(gameObject->getModel());
	if (it != this->gameObjects.end())
	{
		this->shouldRemove.push_back(gameObject);
		return true;
	}
	return false;
}

std::unordered_map<ym::Model*, std::vector<ym::GameObject*>>& ym::ObjectManager::getGameObjects()
{
	return this->gameObjects;
}
