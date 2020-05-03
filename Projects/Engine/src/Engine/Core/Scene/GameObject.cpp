#include "stdafx.h"
#include "GameObject.h"

ym::GameObject::GameObject() : pos(0.f, 0.f, 0.f), modelPtr(nullptr), transform(1.f)
{
}

ym::GameObject::~GameObject()
{
}

void ym::GameObject::init(const glm::vec3& pos, Model* modelPtr)
{
	setPos(pos);
	this->modelPtr = modelPtr;
}

void ym::GameObject::init(const glm::mat4& transform, Model* modelPtr)
{
	this->transform = transform;
	this->pos = transform[3];
	this->modelPtr = modelPtr;
}

void ym::GameObject::destroy()
{
	this->pos = glm::vec3(0.f, 0.f, 0.f);
	this->modelPtr = nullptr;
}

void ym::GameObject::setTransform(const glm::mat4& transform)
{
	this->transform = transform;
}

glm::vec3 ym::GameObject::getPos() const
{
	return this->pos;
}

void ym::GameObject::setPos(const glm::vec3& pos)
{
	this->pos = pos;
	this->transform = this->transform * glm::translate(glm::mat4(1.f), pos);
}

glm::mat4 ym::GameObject::getTransform() const
{
	return this->transform;
}

ym::Model* ym::GameObject::getModel()
{
	return this->modelPtr;
}
