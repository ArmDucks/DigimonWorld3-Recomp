#include "present/map_load_watch.h"

namespace dw3app {

void MapLoadWatch::attach(std::shared_ptr<dw3::texrep::StageLibrary> library) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (library == library_) return;
    library_ = std::move(library);
    last_map_ = -1;
}

void MapLoadWatch::on_read(uint32_t lba) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!library_ || !library_->ready()) return;
    const int map = library_->map_at_lba(lba);
    if (map < 0 || map == last_map_) return;
    last_map_ = map;
    signal_->post(map);
}

bool WatchedSectorSource::read_raw(uint32_t lba, uint8_t out[2352]) {
    watch_->on_read(lba);
    return inner_.read_raw(lba, out);
}

}  // namespace dw3app
