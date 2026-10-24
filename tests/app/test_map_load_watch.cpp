// Spotting map loads from disc reads: the sector ranges of a stage library and the sector source that feeds the signal.
// Runs on synthetic libraries; nothing here needs the game's files.

#include <gtest/gtest.h>

#include <cstring>
#include <memory>

#include "present/map_load_watch.h"

using namespace dw3app;
using dw3::texrep::StagePack;
using dw3::texrep::StageLibrary;

namespace {

// Maps S1 at sectors 100-109, S2 at 120-129 and S0 at 10-19, added out of order.
std::shared_ptr<StageLibrary> make_library(bool finished = true) {
    auto lib = std::make_shared<StageLibrary>();
    StagePack pack;
    lib->add_map("S1", pack, 100, 10);
    lib->add_map("S2", pack, 120, 10);
    lib->add_map("S0", pack, 10, 10);
    lib->add_map("NoPlace", pack);
    if (finished) lib->finish();
    return lib;
}

class FakeDisc final : public dw3::SectorSource {
public:
    uint32_t sector_count() const override { return 500; }
    bool read_raw(uint32_t lba, uint8_t out[2352]) override {
        std::memset(out, static_cast<int>(lba & 0xFF), 2352);
        return lba < 500;
    }
};

}  // namespace

TEST(StageLibraryExtents, Boundaries) {
    auto lib = make_library();
    EXPECT_EQ(lib->map_at_lba(100), 0);
    EXPECT_EQ(lib->map_at_lba(109), 0);
    EXPECT_EQ(lib->map_at_lba(120), 1);
    EXPECT_EQ(lib->map_at_lba(129), 1);
    EXPECT_EQ(lib->map_at_lba(10), 2);
    EXPECT_EQ(lib->map_at_lba(19), 2);
    EXPECT_EQ(lib->map_at_lba(110), -1);  // gap
    EXPECT_EQ(lib->map_at_lba(119), -1);
    EXPECT_EQ(lib->map_at_lba(9), -1);    // before the first
    EXPECT_EQ(lib->map_at_lba(130), -1);  // after the last
}

TEST(StageLibraryExtents, NotReady) {
    auto lib = make_library(false);
    EXPECT_EQ(lib->map_at_lba(100), -1);
    lib->finish();
    EXPECT_EQ(lib->map_at_lba(100), 0);
}

TEST(StageLibraryExtents, MapIndex) {
    auto lib = make_library();
    EXPECT_EQ(lib->map_index("S1"), 0);
    EXPECT_EQ(lib->map_index("NoPlace"), 3);
    EXPECT_EQ(lib->map_index("S9"), -1);
}

TEST(MapLoadWatch, PassesBytesThrough) {
    FakeDisc disc;
    auto watch = std::make_shared<MapLoadWatch>();
    watch->attach(make_library());
    WatchedSectorSource source(disc, watch);
    EXPECT_EQ(source.sector_count(), 500u);
    for (uint32_t lba : {0u, 105u, 125u, 499u, 500u}) {
        uint8_t a[2352], b[2352];
        EXPECT_EQ(source.read_raw(lba, a), disc.read_raw(lba, b));
        EXPECT_EQ(std::memcmp(a, b, sizeof a), 0);
    }
}

TEST(MapLoadWatch, PostsEachMapOncePerChange) {
    FakeDisc disc;
    auto watch = std::make_shared<MapLoadWatch>();
    auto signal = watch->signal();
    WatchedSectorSource source(disc, watch);
    uint8_t buf[2352];
    source.read_raw(100, buf);  // no library yet
    EXPECT_EQ(signal->take(), -1);

    watch->attach(make_library());
    source.read_raw(100, buf);
    source.read_raw(101, buf);
    source.read_raw(300, buf);  // not a map: keeps the current one
    source.read_raw(102, buf);
    EXPECT_EQ(signal->take(), 0);
    EXPECT_EQ(signal->take(), -1);
    source.read_raw(121, buf);
    EXPECT_EQ(signal->take(), 1);
    source.read_raw(105, buf);
    EXPECT_EQ(signal->take(), 0);
}

TEST(MapLoadWatch, NewLibraryReportsAgain) {
    FakeDisc disc;
    auto watch = std::make_shared<MapLoadWatch>();
    WatchedSectorSource source(disc, watch);
    uint8_t buf[2352];
    watch->attach(make_library());
    source.read_raw(100, buf);
    EXPECT_EQ(watch->signal()->take(), 0);
    watch->attach(make_library());
    source.read_raw(100, buf);
    EXPECT_EQ(watch->signal()->take(), 0);
    watch->attach(make_library(false));  // still indexing: nothing is posted
    source.read_raw(120, buf);
    EXPECT_EQ(watch->signal()->take(), -1);
    watch->attach(nullptr);
    source.read_raw(120, buf);
    EXPECT_EQ(watch->signal()->take(), -1);
}
