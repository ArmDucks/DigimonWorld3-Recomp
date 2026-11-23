// Separate music and effects levels: tells the SPU which of its voices play music and which play effects, and applies the
// two volume options. The game's sound driver keeps the sequence that started each voice in a table in its memory (see
// audio_mix.h), which is read when a voice is keyed on.
#pragma once

#include "controller.h"
#include "dw3/machine.h"
#include "dw3/spu.h"

namespace dw3app::ingame {

class AudioGroups final : public dw3::VoiceClassifier {
public:
    // The machine and the controller must outlive this object; it installs itself on the machine's SPU.
    AudioGroups(dw3::Machine& machine, SettingsController& controller);
    ~AudioGroups() override;
    AudioGroups(const AudioGroups&) = delete;
    AudioGroups& operator=(const AudioGroups&) = delete;

    // Pushes audio.music_volume and audio.sfx_volume to the SPU; call after either changes.
    void apply();
    // Keep the score interpreter and music voices at the same fixed-rate clock.
    // Returns false if the guest function does not match the known libsnd routine.
    bool set_fast_forward_music(int divisor);

    dw3::VoiceGroup classify(int voice) override;

private:
    friend struct AudioSequenceHook;
    dw3::Machine& machine_;
    SettingsController& controller_;
    int music_divisor_ = 1;
};

}  // namespace dw3app::ingame
