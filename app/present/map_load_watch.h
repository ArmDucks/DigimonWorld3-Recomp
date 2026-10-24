#pragma once

// Spotting a field map being loaded from the sectors the game reads. Each map is one file on the disc, so a read that
// lands inside a map's file tells which map the game is about to show, early enough for the renderer to prepare that
// map's pictures while the game is still on its load screen (see stage_library.h for the sector ranges).

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>

#include "dw3/cdrom.h"
#include "dw3/texrep/stage_library.h"

namespace dw3app {

// The most recent map the game started to load, handed from the game thread to whoever polls for it.
class MapLoadSignal {
public:
    void post(int map) { pending_ = map; }
    // The map index posted since the last call, or -1 when nothing new arrived.
    int take() { return pending_.exchange(-1); }

private:
    std::atomic<int> pending_{-1};
};

// The state shared between the sector source the drive reads through and the code that owns the stage library: which
// library to look sectors up in (it changes with the settings) and where to post what it finds.
class MapLoadWatch {
public:
    std::shared_ptr<MapLoadSignal> signal() const { return signal_; }
    // Replaces the library (null stops the lookups). A different library starts a new run of maps.
    void attach(std::shared_ptr<dw3::texrep::StageLibrary> library);
    // Called for every sector the game reads.
    void on_read(uint32_t lba);

private:
    std::shared_ptr<MapLoadSignal> signal_ = std::make_shared<MapLoadSignal>();
    std::mutex mutex_;
    std::shared_ptr<dw3::texrep::StageLibrary> library_;
    int last_map_ = -1;
};

// Passes every read to the source it wraps unchanged and tells the watch about it.
class WatchedSectorSource final : public dw3::SectorSource {
public:
    WatchedSectorSource(dw3::SectorSource& inner, std::shared_ptr<MapLoadWatch> watch) : inner_(inner), watch_(std::move(watch)) {}
    uint32_t sector_count() const override { return inner_.sector_count(); }
    bool read_raw(uint32_t lba, uint8_t out[2352]) override;

private:
    dw3::SectorSource& inner_;
    std::shared_ptr<MapLoadWatch> watch_;
};

}  // namespace dw3app
