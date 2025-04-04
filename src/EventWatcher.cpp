#include "EventWatcher.h"

#include <SKSE/SKSE.h>
#include <SkyrimScripting/Logging.h>

#include "BGSLocationEx.h"
#include "DiscoverableLocations.h"
#include "EventHandler.h"
#include "PlayerMapMarkers.h"

class EventSink : public RE::BSTEventSink<RE::LocationDiscovery::Event>,
                  public RE::BSTEventSink<RE::LocationCleared::Event>,
                  public RE::BSTEventSink<RE::TESActorLocationChangeEvent> {
public:
    static EventSink* instance() {
        static EventSink singleton;
        return &singleton;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::LocationDiscovery::Event* event, RE::BSTEventSource<RE::LocationDiscovery::Event>*) override {
        auto* discoverableLocationInfo = GetDiscoverableLocationInfo();
        auto  foundLocation            = discoverableLocationInfo->discoverableMapMarkersToLocations.find(event->mapMarkerData);
        if (foundLocation != discoverableLocationInfo->discoverableMapMarkersToLocations.end()) {
            Log("[Event] Location Discovered: {}", foundLocation->second->GetFormID());
            EventHandler::OnLocationDiscovered(foundLocation->second);
        } else {
            Log("[Event] Discovered location (not found in discoverable locations list) - {}", event->mapMarkerData->locationName.GetFullName());
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::LocationCleared::Event* event, RE::BSTEventSource<RE::LocationCleared::Event>*) override {
        auto* location = BGSLocationEx::GetLastChecked();
        if (location && location->IsLoaded()) {
            Log("[Event] Location Cleared: {} - {}", location->GetName(), location->GetFormID());
            EventHandler::OnLocationCleared(location);
        }
        return RE::BSEventNotifyControl::kContinue;
    }

    RE::BSEventNotifyControl ProcessEvent(const RE::TESActorLocationChangeEvent* event, RE::BSTEventSource<RE::TESActorLocationChangeEvent>*) override {
        if (event && event->actor && event->actor->IsPlayerRef()) UpdateSaveGameToIncludeDiscoveredPlayerMapMarkers();
        return RE::BSEventNotifyControl::kContinue;
    }
};

void WatchForEvents() {
    auto& trampoline = SKSE::GetTrampoline();
    trampoline.create(256);

    RE::LocationDiscovery::GetEventSource()->AddEventSink(EventSink::instance());

    // This is for the BGSLocationEx::GetLastChecked() function (used in Location Cleared event)
    BGSLocationEx::Install(trampoline);

    RE::LocationCleared::GetEventSource()->AddEventSink(EventSink::instance());

    RE::ScriptEventSourceHolder::GetSingleton()->AddEventSink<RE::TESActorLocationChangeEvent>(EventSink::instance());
}
