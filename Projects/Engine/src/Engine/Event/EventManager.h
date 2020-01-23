#pragma once

#include <functional>
#include <typeindex>

#include "Event.h"
#include "MemberFunction.h"

#include <unordered_map>
#include "../Defines.h"
#include "spdlog/spdlog.h"
namespace ym
{
	class EventManager
	{
	public:
		using SubscriberList = std::vector<Ptr(MemberFunctionHandle)>;
		using EventGroups = std::unordered_map<std::type_index, SubscriberList>;
		using Level = std::pair<size_t, Ptr(EventGroups)>;

		/*
		Get a static instance of this class.
		*/
		static EventManager& Get();

		/*
		Bind a member function of an instance to be called when a specific event is called.
		Arguments:
			instance: Pointer to the class.
			function: Pointer to the member function.
			level: The level of which this function will be called in.
		*/
		template<class T, class EventType>
		void Subscribe(T* instance, bool(T::* function)(EventType*), size_t level = 0);

		/*
		Unbind a member function.
		Arguments:
			instance: Pointer to the class.
			function: Pointer to the member function.
		*/
		template<class T, class EventType>
		void Unsubscribe(T* instance, bool(T::* function)(EventType*));

		/*
		Will publish an event to all which subscribes to it. It will begin at the specified level.
		A subscriber will be called if:
			- is on the same level,
			- is on a lower level and no subscriber in a higher level return false.
		Arguments:
			evnt: Pointer to the event which will be published.
			level: The level to begin calling at.
		*/
		template<class EventType>
		void Publish(EventType* evnt, size_t level = SIZE_MAX);

		/*
		================ DEBUG ================
		Print all subscribers in each event group for each level.
		*/
		void PrintInfo() const;

	private:
		/*
		================ DEBUG ================
		Print all subscribers in each event group for a specific level.
		*/
		void PrintSubscriberInfo(Level& level) const;

		// This should always be sorted in descending order.
		mutable std::vector<Level> levelSubscribers;
	};

	template<class T, class EventType>
	inline void EventManager::Subscribe(T* instance, bool(T::* function)(EventType*), size_t level)
	{
		// Find if level already exists.
		std::vector<Level>::iterator it = std::find_if(levelSubscribers.begin(), levelSubscribers.end(), [level](const Level & e) {
			return e.first == level;
			});

		// Level does not exist, create one.
		bool created = false;
		if (it == levelSubscribers.end())
		{
			Level level(level, MakePtr(EventGroups, NOP));
			levelSubscribers.push_back(std::move(level));
			// Assign new iterator.
			it = --levelSubscribers.end();
			created = true;
		}
		// ===================== DEBUG =====================
		else
		{
			size_t target = MemberFunctionHandle::getId<T, EventType>(instance);
			EventGroups& eventGroups = *it->second;
			std::type_index index = typeid(EventType);
			auto it = eventGroups.find(index);
			if (it != eventGroups.end())
			{
				// Search for targeted member function.
				SubscriberList& list = (it)->second;
				SubscriberList::iterator funcIt = find_if(list.begin(), list.end(), [target](const Ptr(MemberFunctionHandle) & e) {
					return e->id == target;
					});
				if (funcIt != list.end())
				{
					spdlog::warn("({0}) Tried to subscrib to event({1:s}), subscriber already exists!", typeid(T).name(), index.name());
					return;
				}
			}
		}
		// ==================== /DEBUG =====================

		// Add member function to the list in the level.
		std::type_index index = typeid(EventType);
		EventGroups& eventGroups = *it->second;
		MemberFunction<T, EventType> memberFunction(instance, function);
		std::unique_ptr<MemberFunction<T, EventType>> ptr = std::make_unique<MemberFunction<T, EventType>>(instance, function);
		eventGroups[index].push_back(std::move(ptr));

		// Sort by level in descending order if level was created.
		if (created)
		{
			std::sort(this->levelSubscribers.begin(), this->levelSubscribers.end(), [](const Level & a, const Level & b) {
				return a.first > b.first;
				});
		}
	}

	template<class T, class EventType>
	inline void EventManager::Unsubscribe(T* instance, bool(T::* function)(EventType*))
	{
		bool existed = false; // <========================= DEBUG

		// Return if no subscribers.
		if (this->levelSubscribers.empty()) return;

		size_t target = MemberFunctionHandle::getId<T, EventType>(instance);

		// Get index of the type.
		std::type_index index = typeid(EventType);
		std::vector<Level>::iterator lvlIt = this->levelSubscribers.begin();
		for (; lvlIt != this->levelSubscribers.end(); lvlIt++)
		{
			Level& level = *lvlIt;

			// Fetch all subscribers from that level
			EventGroups& eventGroups = *level.second;

			// Check if there are any subscribers to this event type.
			auto it = eventGroups.find(index);
			if (it != eventGroups.end())
			{
				// Search for targeted member function.
				SubscriberList& list = (it)->second;
				SubscriberList::iterator funcIt = find_if(list.begin(), list.end(), [target](const Ptr(MemberFunctionHandle) & e) {
					return e->id == target;
					});
				if (funcIt != list.end())
				{
					spdlog::info("({0}) Unsubscribed to event({1:s})", typeid(T).name(), index.name());

					// Remove function from list of subscribers.
					list.erase(funcIt);
					existed = true; // <========================= DEBUG
					// If list becomes empty, remove that event mapping.
					if (list.empty())
					{
						eventGroups.erase(index);
						// If all event in the map are empty remove that level.
						if (eventGroups.empty())
						{
							this->levelSubscribers.erase(lvlIt);
							// Sort by level in descending order.
							std::sort(this->levelSubscribers.begin(), this->levelSubscribers.end(), [](const Level & a, const Level & b) {
								return a.first > b.first;
								});
						}
					}
					return;
				}
			}
		}

		// ============= DEBUG =================
		if (!existed) spdlog::warn("({0}) Tried to unsubscrib to event({1:s}), subscriber was not found!", typeid(T).name(), index.name());
	}

	template<class EventType>
	inline void EventManager::Publish(EventType* evnt, size_t level)
	{
		// Return if no subscribers.
		if (this->levelSubscribers.empty()) return;

		// Get index of the type.
		std::type_index index = typeid(EventType);

		if (level == SIZE_MAX)	spdlog::info("Published event({0:s}) from max level", index.name());
		else					spdlog::info("Published event({0:s}) from level {1:d}", index.name(), level);

		bool succeeded = true;
		// Find closest level lower or equal to the specified level. Go though level, if succeeded, go to the next.
		for (Level& obj : this->levelSubscribers)
		{
			if (obj.first <= level)
			{
				// Fetch all subscribers from that level
				EventGroups& eventGroups = *obj.second;

				// Check if there are any subscribers to this event type.
				auto it = eventGroups.find(index);
				if (it != eventGroups.end())
				{
					// Call all subscribers for this event.
					auto& list = *it;
					for (Ptr(MemberFunctionHandle)& sub : list.second)
					{
						succeeded = sub->exec(evnt) && succeeded;
					}
				}
			}

			// If one subscriber returned false, do not propagated down to next level.
			if (!succeeded) return;
		}
	}
}