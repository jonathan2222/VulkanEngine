#include "stdafx.h"
#include "EventManager.h"

namespace ym
{
	EventManager& EventManager::Get()
	{
		static EventManager eventManager;
		return eventManager;
	}

	void EventManager::PrintInfo() const
	{
		for (Level& level : levelSubscribers)
			PrintSubscriberInfo(level);
	}

	void EventManager::PrintSubscriberInfo(Level & level) const
	{
		spdlog::debug("[Level] {0}", level.first);

		// Fetch all subscribers from the level.
		EventGroups& eventGroups = *level.second;

		// Go through each subscriber for each event type and print its info.
		for (auto& eventGroup : eventGroups)
		{
			spdlog::debug("\t[Event] {0}", eventGroup.first.name());
			SubscriberList& list = eventGroup.second;
			for (Ptr(MemberFunctionHandle)& sub : list)
			{
				spdlog::debug("\t\t[Subscriber] Class: {0}, Id: {1:d}", sub->name, sub->id);
			}
		}
	}
}
