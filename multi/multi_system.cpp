#include "multi_system.h"

#include <algorithm>
#include <chrono>
#include <thread>

MultiSystem::MultiSystem(Layout layout,
                         FileStreamPath bios_path,
                         FileStreamPath eeprom_path,
                         bool use_emu,
                         ButtonFeedCallback button_callback)
    : layout_{layout}
    , path_bios_{bios_path}
    , path_eeprom_{eeprom_path}
    , use_emu_{use_emu}
    , cb_button_feed_{button_callback} {
}

MultiSystem::~MultiSystem() {
    UnbootGame();
    systems_.clear();
    first_system_ = nullptr;
    cb_button_feed_ = nullptr;
}

void MultiSystem::BootGame(FileStreamPath game_path,
                           const uint8_t *game_data,
                           size_t game_size,
                           bool connect_comlynx) {
    int const players = std::clamp(layout_.players, 1, 16);
    for (int i = 0; i < players; ++i)
    {
        systems_.push_back(std::make_unique<CSystem>(game_path,
                                                     game_data,
                                                     game_size,
                                                     path_bios_,
                                                     use_emu_,
                                                     path_eeprom_,
                                                     i));

        // This sleep is to make sure the individual Lynx systems
        // don't all boot up at once. This is required to decide
        // player 1, player 2, etc...
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    first_system_ = systems_[0].get();

    if (!connect_comlynx) {
        return;
    }

    for (int i = 0; i < players; ++i) {
        auto &system = systems_[i];

        // This sleep is to make sure the individual Lynx systems
        // don't all boot up at once. This is required to decide
        // player 1, player 2, etc...
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        system->Update();
    }
    comlynx_connected_ = true;
}

void MultiSystem::UnbootGame() {
    if (comlynx_connected_) {
        comlynx_connected_ = false;
    }
}

void MultiSystem::Overclock(int times) {
    for (auto &system : systems_) {
        system->Overclock();
    }
}

Rotation MultiSystem::GetCartRotation() {
    switch (first_system_->CartGetRotate()) {
    case CART_NO_ROTATE:
    default:
        return Rotation::None;
    case CART_ROTATE_LEFT:
        return Rotation::Left;
    case CART_ROTATE_RIGHT:
        return Rotation::Right;
    }
}

void MultiSystem::UpdateButtons() {
    for (int i = 0; i < layout_.players; ++i) {
        systems_[i]->SetButtonData(cb_button_feed_(i));
    }
}

void MultiSystem::FetchAudioSamples() {
    for (auto &system : systems_) {
        system->FetchAudioSamples();
    }
}

void MultiSystem::DisplaySetAttributes(Rotation rotate,
                                       PixelFormat format,
                                       unsigned pitch,
                                       DisplayBufferProvidingCallback buffer_provider) {
    assert(buffer_provider);
    auto const len = layout_.players;
    for (int i = 0; i < len; ++i)
    {
        auto const system_framebuffer_offset = layout_.FramebufferOffsetForPlayer(i, pitch);
        auto const system_pitch = layout_.FramebufferPitchForPlayer(i, pitch);
        auto &system = systems_[i];
        auto callback = [&system, i, buffer_provider, system_framebuffer_offset, system_pitch](ULONG objref) {
            system->mSkipFrame = 0;

            // TODO: what about `frame_available`?
            return buffer_provider() + system_framebuffer_offset;
        };

        systems_[i]->DisplaySetAttributes(
            static_cast<ULONG>(rotate),
            static_cast<ULONG>(format),
            static_cast<ULONG>(system_pitch),
            callback,
            i);
    }
}

bool MultiSystem::IsAnySkippingFrame() const {
    for (auto const &system : systems_) {
        if (system->mSkipFrame) {
            return true;
        }
    }
    return false;
}

bool MultiSystem::IsNoneSkippingFrame() const {
    for (auto const &system : systems_) {
        if (system->mSkipFrame) {
            return false;
        }
    }
    return true;
}

void MultiSystem::SetIsSkippingFrame(bool skipping) {
    for (auto &system : systems_) {
        system->mSkipFrame = skipping;
    }
}

void MultiSystem::NoteLastCycleCounts() {
    for (auto &system : systems_) {
        system->mLastRunCycleCount = system->mSystemCycleCount;
    }
}

inline static bool IsBehind(CSystem *system, ULONG cycles_per_frame) {
    return (system->mSystemCycleCount - system->mLastRunCycleCount) < cycles_per_frame;
}

inline static bool IsAnyBehind(CSystemVect const &systems, ULONG cycles_per_frame) {
    for (auto &system : systems) {
        if (IsBehind(system.get(), cycles_per_frame)) {
            return true;
        }
    }
    return false;
}

// VERSION 1: CATCH UP TOGETHER!
void MultiSystem::CatchUpAllSystems(ULONG cycles_per_frame, unsigned overclock) {
    while (IsAnyBehind(systems_, cycles_per_frame)) {
        for (auto &system : systems_) {
            if (IsBehind(system.get(), cycles_per_frame)) {
                system->Update();
            }
        }
    }
}

// // VERSION 2: DON'T CATCH UP TOGETHER
void MultiSystem::CatchUpSystem(int player, ULONG cycles_per_frame, unsigned overclock) {
    CSystem *system = systems_[player].get();
    while (IsBehind(system, cycles_per_frame)) {
        system->Update();
    }
}

void MultiSystem::SetAudioEnabled(bool enabled) {
    // TODO: only enabling the first lynx
    first_system_->mAudioEnabled = enabled;
}

int16_t *MultiSystem::GetAudioBuffer() {
    return (int16_t *)(&first_system_->mAudioBuffer);
}

ULONG MultiSystem::GetAudioBufferPointer() {
    return first_system_->mAudioBufferPointer;
}

void MultiSystem::SetAudioBufferPointer(ULONG ptr) {
    first_system_->mAudioBufferPointer = ptr;
}

size_t MultiSystem::ContextSize() const {
    return first_system_->ContextSize();
}

bool MultiSystem::ContextLoad(LSS_FILE *fp) {
    return first_system_->ContextLoad(fp);
}

bool MultiSystem::ContextSave(LSS_FILE *fp) {
    return first_system_->ContextSave(fp);
}

void MultiSystem::SaveEEPROM() {
    return first_system_->SaveEEPROM();
}

void MultiSystem::Reset() {
    for (auto &system : systems_) {
        system->Reset();
    }
}

UBYTE *MultiSystem::GetRamPointer(void) {
    return first_system_->GetRamPointer();
}

CSystem *MultiSystem::GetSystem(int player) {
    return systems_[0].get();
}
