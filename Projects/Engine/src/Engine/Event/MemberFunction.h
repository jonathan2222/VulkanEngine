#pragma once

#include <functional>
#include "Event.h"

namespace ym
{
	class MemberFunctionHandle
	{
	public:
		bool exec(Event* evnt)
		{
			return call(evnt);
		}

		size_t id;
		std::string name; // THIS IS FOR DEBUG!!

		template<class T, typename EventType>
		static size_t getId(T* instance);
	private:
		virtual bool call(Event* evnt) = 0;
	};

	template<class T, typename EventType>
	inline size_t MemberFunctionHandle::getId(T* instance)
	{
		size_t h = typeid(EventType).hash_code();
		return h - (size_t)instance;
	}

	template<class T, class EventType>
	class MemberFunction : public MemberFunctionHandle
	{
	public:
		typedef bool(T::* Function)(EventType*);

		MemberFunction(T* instance, bool(T::* function)(EventType*)) : instance(instance), function(function)
		{
			this->id = getId<T, EventType>(instance);
			this->name = typeid(T).name(); // THIS IS FOR DEBUG!!
		};

		bool call(Event* evnt) override
		{
			return (instance->*function)(static_cast<EventType*>(evnt));
		}

	private:
		T* instance;
		Function function;
	};
}