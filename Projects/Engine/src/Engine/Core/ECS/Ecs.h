#pragma once

// https://docs.unity3d.com/Packages/com.unity.entities@0.1/manual/ecs_core.html

namespace ym
{
	class ECS
	{
	public:
		void addEntity(/*List of components*/);
		void removeEntity(/*Handler to entity*/);
		void removeComponentFromEntity(/*Handler to entity and component type*/);
	private:
		// An entity (A set of components) can be changed and thus moved to a new archtype.
		// Archtype (All entities with the same type of components)
		// Archtype chunk (A memory block of archtypes, A archtype can have many archtype chunks, Always packed!)
		// Systems can be in groups. The update order (of entities within the group or of the groups) can be changed by the programmer!
	};
}