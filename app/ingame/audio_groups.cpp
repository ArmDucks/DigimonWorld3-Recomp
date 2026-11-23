#include "audio_groups.h"
#include "game_addresses.h"

#include "audio_mix.h"
#include "fastforward.h"
#include "dw3/recomp.h"

namespace dw3app::ingame {
namespace {

// libsnd's voice table: 56 bytes per voice, the sequence id of the note that was last keyed on it at +0x20.
constexpr uint32_t kVoiceStride = 56, kVoiceSequence = 0x20;

}  // namespace

// PsyQ's VBlank sound callback dispatches _SsSndPlay(access, sequence) once
// per playing score. Only play ticks are slowed: stop, close, voice flush,
// effect sequences and bank changes still run at guest speed.
struct AudioSequenceHook {
    static inline AudioGroups* instance = nullptr;
    static void tick(RecompContext* ctx) {
        AudioGroups& self = *instance;
        const auto access = static_cast<uint16_t>(ctx->r[4]);
        if (self.music_divisor_ > 1 && access >= kFirstMusicAccess && access < 0xFF &&
            !music_sequence_tick(self.machine_.video().frame_count(), self.music_divisor_)) return;
        self.machine_.lookup_original(game_addresses().sequencePlayTick)(ctx);
    }
};

AudioGroups::AudioGroups(dw3::Machine& machine, SettingsController& controller) : machine_(machine), controller_(controller) {
    machine_.spu().set_voice_classifier(this);
    AudioSequenceHook::instance = this;
    apply();
}

AudioGroups::~AudioGroups() {
    set_fast_forward_music(1);
    machine_.spu().set_voice_classifier(nullptr);
    AudioSequenceHook::instance = nullptr;
}

bool AudioGroups::set_fast_forward_music(int divisor) {
    const uint32_t address = game_addresses().sequencePlayTick;
    if (divisor == music_divisor_) return true;
    if (!address) return divisor <= 1;
    if (divisor <= 1) {
        machine_.override_function(address, nullptr);
        music_divisor_ = 1;
        machine_.spu().set_music_clock_divisor(1);
        return true;
    }
    // Wrapper prologue: addiu sp,-24; sw ra,16(sp). Fail closed for unknown discs.
    const bool valid = address && dw3_read32(&machine_.ctx(), address) == 0x27BDFFE8 &&
                       dw3_read32(&machine_.ctx(), address + 4) == 0xAFBF0010;
    machine_.override_function(address, valid ? &AudioSequenceHook::tick : nullptr);
    music_divisor_ = valid ? divisor : 1;
    machine_.spu().set_music_clock_divisor(music_divisor_);
    return divisor <= 1 || valid;
}

void AudioGroups::apply() {
    const Settings::Audio& audio = controller_.active().audio;
    machine_.spu().set_group_volume(dw3::VoiceGroup::Music, audio.music_volume);
    machine_.spu().set_group_volume(dw3::VoiceGroup::Effects, audio.sfx_volume);
}

dw3::VoiceGroup AudioGroups::classify(int voice) {
    const uint16_t sequence = dw3_read16(&machine_.ctx(), game_addresses().voiceTable + kVoiceStride * static_cast<uint32_t>(voice) + kVoiceSequence);
    return is_music_sequence(sequence) ? dw3::VoiceGroup::Music : dw3::VoiceGroup::Effects;
}

}  // namespace dw3app::ingame
