#include "dw3/texrep/stage_library.h"

#include <algorithm>
#include <map>

#include "dw3/texrep/stage_dump.h"

namespace dw3::texrep {

StageLibrary::~StageLibrary() {
    if (worker_.joinable()) worker_.join();
}

void StageLibrary::add_map(std::string name, const StagePack& pack, uint32_t first_lba, uint32_t sector_count) {
    Map m;
    m.name = std::move(name);
    m.cols = pack.cols;
    m.rows = pack.rows;
    const uint32_t map_index = static_cast<uint32_t>(maps_.size());
    for (const StageTile& t : pack.tiles) {
        Tile tile;
        tile.col = t.col;
        tile.row = t.row;
        tile.width = t.width_texels();
        tile.height = t.rows;
        tile.block_hash = t.block_hash;
        tile.clut_hash = t.clut_hash;
        tile.pieces = t.pieces;
        if (t.tracked()) index_[{t.block_hash, t.clut_hash}].push_back({map_index, static_cast<uint32_t>(m.tiles.size())});
        m.tiles.push_back(std::move(tile));
    }
    maps_.push_back(std::move(m));
    if (sector_count) extents_.push_back({first_lba, first_lba + sector_count, map_index});
}

void StageLibrary::sort_extents() {
    std::sort(extents_.begin(), extents_.end(), [](const Extent& a, const Extent& b) { return a.first < b.first; });
}

void StageLibrary::finish() {
    sort_extents();
    ready_ = true;
}

int StageLibrary::map_at_lba(uint32_t lba) const {
    if (!ready_) return -1;
    auto it = std::upper_bound(extents_.begin(), extents_.end(), lba, [](uint32_t v, const Extent& e) { return v < e.first; });
    if (it == extents_.begin()) return -1;
    --it;
    return lba < it->end ? static_cast<int>(it->map) : -1;
}

int StageLibrary::map_index(std::string_view name) const {
    for (std::size_t i = 0; i < maps_.size(); ++i)
        if (maps_[i].name == name) return static_cast<int>(i);
    return -1;
}

std::span<const StageLibrary::TileRef> StageLibrary::refs(uint64_t block_hash, uint64_t clut_hash) const {
    const auto it = index_.find({block_hash, clut_hash});
    return it == index_.end() ? std::span<const TileRef>() : std::span<const TileRef>(it->second);
}

std::shared_ptr<StageLibrary> StageLibrary::open_async(const std::filesystem::path& path, std::string& error) {
    std::shared_ptr<StageSource> source = StageSource::open(path, error);
    if (!source) return nullptr;
    auto lib = std::make_shared<StageLibrary>();
    auto mutex = std::make_shared<std::mutex>();
    // StageSource keeps a file handle and is not thread safe: the loader and the indexing thread share it under a lock.
    lib->loader_ = [source, mutex](const std::string& map, StagePack& out, std::string& err) {
        std::lock_guard<std::mutex> lock(*mutex);
        std::vector<uint8_t> data;
        return source->read(map, data, err) && parse_stage_pack(data, out, err);
    };
    StageLibrary* self = lib.get();
    std::map<std::string, MapExtent> extents;
    for (MapExtent& e : source->extents()) extents[e.name] = std::move(e);
    lib->worker_ = std::thread([self, source, mutex, extents = std::move(extents)] {
        for (const std::string& name : source->maps()) {
            StagePack pack;
            std::string err;
            if (!self->loader_(name, pack, err)) continue;
            const auto found = extents.find(name);
            if (found == extents.end()) self->add_map(name, pack);
            else self->add_map(name, pack, found->second.first_lba, found->second.sector_count);
        }
        self->finish();
    });
    return lib;
}

bool StageLibrary::tile_image(uint32_t map, uint32_t tile, Image& out) {
    if (map >= maps_.size() || !loader_) return false;
    std::lock_guard<std::mutex> lock(pack_mutex_);
    if (cached_map_ != static_cast<int>(map)) {
        cached_map_ = -1;
        std::string error;
        if (!loader_(maps_[map].name, cached_pack_, error)) return false;
        cached_map_ = static_cast<int>(map);
    }
    if (tile >= cached_pack_.tiles.size()) return false;
    out = decode_stage_tile(cached_pack_.tiles[tile]);
    return true;
}

}  // namespace dw3::texrep
