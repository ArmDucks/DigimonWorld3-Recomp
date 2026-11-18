// Integration tests against the user's Digimon World 3 (USA) disc. Skipped when absent.

#include <gtest/gtest.h>

#include <algorithm>

#include "../game_disc.h"
#include "psxdisc/disc.h"
#include "psxdisc/iso9660.h"
#include "psxdisc/psxexe.h"

using namespace psxdisc;

TEST(RealDisc, OpensAndHasExpectedSize) {
    DW3_REQUIRE_DISC();
    auto disc = Disc::open(dw3test::game_cue());
    ASSERT_TRUE(disc);
    EXPECT_EQ(disc->sector_count(), 647526768u / kRawSectorSize);
}

TEST(RealDisc, VisibleRootHasBootExe) {
    DW3_REQUIRE_DISC();
    auto disc = Disc::open(dw3test::game_cue());
    auto root = read_root(*disc);
    ASSERT_TRUE(root);
    auto files = walk(*disc, root->lba, root->size);
    ASSERT_EQ(files.size(), 3u);
    auto it = std::find_if(files.begin(), files.end(), [](auto& f) { return f.path == "SLUS_014.36"; });
    ASSERT_NE(it, files.end());
    EXPECT_EQ(it->lba, 24u);
}

TEST(RealDisc, BootExeHeader) {
    DW3_REQUIRE_DISC();
    auto disc = Disc::open(dw3test::game_cue());
    auto exe = parse_psxexe(disc->read_file(24, 919552));
    ASSERT_TRUE(exe);
    EXPECT_EQ(exe->pc, 0x80010EC4u);
    EXPECT_EQ(exe->text_addr, 0x80010000u);
    EXPECT_EQ(exe->text_size, 0xE0000u);
}

TEST(RealDisc, HiddenTreeHas263Overlays) {
    DW3_REQUIRE_DISC();
    auto disc = Disc::open(dw3test::game_cue());
    auto files = walk(*disc, 473, kForm1DataSize, "AAA/");
    EXPECT_EQ(files.size(), 2273u);
    auto pro = std::count_if(files.begin(), files.end(),
                             [](auto& f) { return f.path.size() > 4 && f.path.ends_with(".PRO"); });
    EXPECT_EQ(pro, 263);
    auto field = std::find_if(files.begin(), files.end(), [](auto& f) { return f.path == "AAA/PRO/FIELDSTG.PRO"; });
    ASSERT_NE(field, files.end());
    EXPECT_EQ(field->lba, 556u);
    EXPECT_EQ(field->size, 100168u);
}

TEST(RealDisc, HiddenRootIsFound) {
    DW3_REQUIRE_DISC();
    auto disc = Disc::open(dw3test::game_cue());
    EXPECT_EQ(find_hidden_root(*disc), 473u);
}

TEST(RealDiscEurope, HiddenTreeAndBootExecutable) {
    DW3_REQUIRE_EUROPE_DISC();
    auto disc = Disc::open(dw3test::europe_cue());
    ASSERT_TRUE(disc);
    EXPECT_EQ(find_hidden_root(*disc), 475u);
    auto files = walk(*disc, 475, kForm1DataSize, "AAA/");
    EXPECT_EQ(files.size(), 2382u);
    auto root = read_root(*disc);
    ASSERT_TRUE(root);
    const auto visible = walk(*disc, root->lba, root->size);
    auto boot = std::find_if(visible.begin(), visible.end(), [](auto& f) { return f.path == "SLES_039.36"; });
    ASSERT_NE(boot, visible.end());
    EXPECT_EQ(boot->size, 923648u);
}

TEST(RealDisc, XaAudioSectorsPresent) {
    DW3_REQUIRE_DISC();
    auto disc = Disc::open(dw3test::game_cue());
    auto h = disc->read_header(141859);  // first interleaved XA audio sector
    EXPECT_TRUE(h.submode & kSubmodeAudio);
    EXPECT_TRUE(h.submode & kSubmodeForm2);
}
