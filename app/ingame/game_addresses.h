#pragma once
#include <cstdint>

namespace dw3app::ingame {

// Where the game keeps what the hooks use. Code addresses are of MAIN functions or of the mode overlays; an address of 0
// is one not known for that release, and the hook that needs it stays off.
struct GameAddresses {
    uint32_t listTaskFunction;
    uint32_t localeTableFunction;
    uint32_t rowStringIds;
    uint32_t pad;
    uint32_t encounterTick;
    uint32_t battlePending;
    uint32_t attackTask;
    uint32_t records;
    uint32_t active;
    uint32_t finaliseShare;
    uint32_t finaliseShareSignature;
    uint32_t finaliseShareSignatureAt;
    uint32_t gameMode;
    uint32_t textWindowTick;
    uint32_t measureBox;
    uint32_t voiceTable;
    uint32_t menuTask;
    uint32_t menuTaskSignature;
    uint32_t menuTaskSignatureAt;
    uint32_t runChildren;
    uint32_t runObject;
    uint32_t createWindow;
    uint32_t createCursor;
    uint32_t createFade;
    uint32_t windowTick;
    uint32_t objectCleanup;
    uint32_t playSound;
};

inline constexpr GameAddresses kUsaAddresses{
    .listTaskFunction = 0x800126FC,
    .localeTableFunction = 0x80044B58,
    .rowStringIds = 0x8003EB20,
    .pad = 0x8004AF78,
    .encounterTick = 0x80091910,
    .battlePending = 0x80098EB4 + 92,
    .attackTask = 0x8008C0BC,
    .records = 0x800A31F8,
    .active = 0x800A31F0,
    .finaliseShare = 0x80084170,
    .finaliseShareSignature = 0x8e032744,
    .finaliseShareSignatureAt = 0x800841AC,
    .gameMode = 0x800554D0,
    .textWindowTick = 0x8001A890,
    .measureBox = 0x80019C2C,
    .voiceTable = 0x800815C0,
    .menuTask = 0x80084B68,
    .menuTaskSignature = 0x10650017,
    .menuTaskSignatureAt = 0x80084B88,
    .runChildren = 0x80017ECC,
    .runObject = 0x80017DDC,
    .createWindow = 0x8001AAB4,
    .createCursor = 0x8001AFE0,
    .createFade = 0x80012070,
    .windowTick = 0x8001A890,
    .objectCleanup = 0x800142E0,
    .playSound = 0x800553F0,
};

// Europe (SLES-03936), found by matching the instructions around each USA address.
inline constexpr GameAddresses kEuropeAddresses{
    .listTaskFunction = 0x8001270C,
    .localeTableFunction = 0x80044F4C,
    .rowStringIds = 0x8003EF14,
    .pad = 0x8004B7D0,
    .encounterTick = 0x80092664,
    .battlePending = 0,
    .attackTask = 0x8008CB70,
    .records = 0x800A4470,
    .active = 0x800A4468,
    .finaliseShare = 0x80084A2C,
    .finaliseShareSignature = 0x8e032750,
    .finaliseShareSignatureAt = 0x80084A68,
    .gameMode = 0x80055D28,
    .textWindowTick = 0x8001AC14,
    .measureBox = 0x80019FB0,
    .voiceTable = 0x80081E28,
    .menuTask = 0x80085594,
    .menuTaskSignature = 0x10650017,
    .menuTaskSignatureAt = 0x800855B4,
    .runChildren = 0x80018230,
    .runObject = 0x80018140,
    .createWindow = 0x8001AE38,
    .createCursor = 0x8001B364,
    .createFade = 0x8001206C,
    .windowTick = 0x8001AC14,
    .objectCleanup = 0x80014308,
    .playSound = 0x80055C48,
};

inline const GameAddresses* g_game_addresses = &kUsaAddresses;
inline const GameAddresses& game_addresses() { return *g_game_addresses; }
inline void select_game_addresses(bool europe) { g_game_addresses = europe ? &kEuropeAddresses : &kUsaAddresses; }

}  // namespace dw3app::ingame
