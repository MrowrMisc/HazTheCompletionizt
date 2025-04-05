#include "DiscoverableMapMarkers.h"

#include <SKSE/SKSE.h>
#include <SkyrimScripting/Logging.h>
#include <collections.h>

#include <atomic>
#include <chrono>

#include "JsonFiles.h"
#include "StringUtils.h"

std::unique_ptr<DiscoverableMapMarkers> g_DiscoverableMapMarkers = nullptr;
std::atomic<bool>                       g_hasLoggedFullListOfDiscoverableMapMarkers{false};
std::atomic<bool>                       g_DiscoverableMapMarkersReloading{false};
DiscoverableMapMarkers*                 GetDiscoverableMapMarkers() { return g_DiscoverableMapMarkers.get(); }

void ReloadDiscoverableMapMarkers() {
    if (g_DiscoverableMapMarkersReloading.exchange(true)) {
        Log("[Summary] Reloading of discoverable map markers already in progress, skipping...");
        return;
    }

    Log("[Summary] Reloading discoverable map markers ...");
    auto now = std::chrono::steady_clock::now();

    auto                                         locationInfo = std::make_unique<DiscoverableMapMarkers>();
    collections_map<RE::TESFile*, std::uint32_t> countOfDiscoverableMapMarkersPerFile;

    auto worldspaces = RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESWorldSpace>();
    for (auto* worldspace : worldspaces) {
        if (!worldspace) continue;

        auto* persistent = worldspace->persistentCell;
        if (persistent) {
            for (auto& refHandle : persistent->references) {
                auto* ref = refHandle.get();
                if (!ref) continue;
                auto* file = ref->GetFile(0);
                if (IgnoredMapMarkers.contains(ref->GetFormID())) {
                    Log("[Discoverable Map Marker]: Ignoring map marker with ID {:x} from {}", ref->GetLocalFormID(), file->GetFilename());
                    continue;
                }
                if (auto* extraMapMarker = ref->extraList.GetByType<RE::ExtraMapMarker>()) {
                    if (auto* mapData = extraMapMarker->mapData) {
                        if (!mapData->locationName.fullName.empty()) {
                            if (IgnoredLocationNames.contains(ToLowerCase(mapData->locationName.fullName))) {
                                Log("[Discoverable Map Marker]: Ignoring map marker with name '{}' from {:x} @ {}", mapData->locationName.GetFullName(), ref->GetLocalFormID(),
                                    file->GetFilename());
                                continue;
                            }
                            if (!g_hasLoggedFullListOfDiscoverableMapMarkers)
                                Log("[Discoverable Map Marker]: '{}' from {:x} @ {}", mapData->locationName.GetFullName(), ref->GetLocalFormID(), file->GetFilename());
                            locationInfo->AddDiscoverableMapMarker(mapData, ref);
                            countOfDiscoverableMapMarkersPerFile[file]++;
                        }
                    }
                }
            }
        }
    }

    auto durationMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - std::chrono::steady_clock::now()).count();
    Log("[Summary] Reloading discovered locations took {} ms", durationMs);
    Log("[Summary] Total discoverable locations: {}", locationInfo->GetTotalDiscoverableMapMarkersCount());
    Log("[Summary] Count of discoverable locations per file:");
    for (const auto& [file, count] : countOfDiscoverableMapMarkersPerFile) Log("[Summary] File: {} - {} discoverable locations", file->GetFilename(), count);

    g_DiscoverableMapMarkers                    = std::move(locationInfo);
    g_DiscoverableMapMarkersReloading           = false;
    g_hasLoggedFullListOfDiscoverableMapMarkers = true;
}
