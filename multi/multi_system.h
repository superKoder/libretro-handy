#ifndef HANDY_MP_MULTI_SYSTEM_H_
#define HANDY_MP_MULTI_SYSTEM_H_
#pragma once

#include "handy.h"
#include "layout.h"

#include <vector>
#include <memory>

using DisplayBufferPointer = uint8_t *;
using DisplayBufferProvidingCallback = std::function<DisplayBufferPointer()>;

using ButtonState = uint32_t;
using ButtonFeedCallback = ButtonState (*)(int player);

using CSystemVect = std::vector<std::unique_ptr<CSystem>>;

enum class Rotation : UBYTE
{
    // Values must match
    Invalid = MIKIE_BAD_MODE,

    None = MIKIE_NO_ROTATE,
    Left = MIKIE_ROTATE_L,
    Flip = MIKIE_ROTATE_B,
    Right = MIKIE_ROTATE_R,

    Auto = 255,
};

enum class PixelFormat : UBYTE
{
    RGB8 = MIKIE_PIXEL_FORMAT_8BPP,
    BGR16_555 = MIKIE_PIXEL_FORMAT_16BPP_BGR555,
    RGB16_555 = MIKIE_PIXEL_FORMAT_16BPP_555,
    RGB16_565 = MIKIE_PIXEL_FORMAT_16BPP_565,
    RGB24 = MIKIE_PIXEL_FORMAT_24BPP,
    RGB32 = MIKIE_PIXEL_FORMAT_32BPP,
};

using FileStreamPath = char const *;

/**
 * Represents multiple consoles.
 */
class MultiSystem
{
public:
    MultiSystem(Layout layout,
                FileStreamPath bios_path,
                FileStreamPath eeprom_path,
                bool use_emu,
                ButtonFeedCallback button_callback);

    ~MultiSystem();

    void BootGame(FileStreamPath game_path,
                  const uint8_t *game_data,
                  size_t game_size,
                  bool connect_comlynx);

    void UnbootGame();

    void Overclock(int times);

    Rotation GetCartRotation();

    void UpdateButtons();
    void UpdateComLynx();

    void FetchAudioSamples();

    void DisplaySetAttributes(Rotation rotate,
                              PixelFormat format,
                              unsigned pitch,
                              DisplayBufferProvidingCallback buffer_provider);

    bool IsAnySkippingFrame() const;
    bool IsNoneSkippingFrame() const;
    void SetIsSkippingFrame(bool);

    void NoteLastCycleCounts();
    void CatchUpAllSystems(ULONG cycles_per_frame, unsigned overclock);
    void CatchUpSystem(int player, ULONG cycles_per_frame, unsigned overclock);

    void SetAudioEnabled(bool enabled);
    int16_t *GetAudioBuffer();
    ULONG GetAudioBufferPointer();
    void SetAudioBufferPointer(ULONG);

    size_t ContextSize() const;

    bool ContextLoad(LSS_FILE *fp);

    bool ContextSave(LSS_FILE *fp);

    void SaveEEPROM();

    void Reset();

    UBYTE *GetRamPointer(void);

    CSystem *GetSystem(int player);

private:
    Layout const layout_;
    FileStreamPath path_bios_;
    FileStreamPath path_eeprom_;
    bool use_emu_;
    ButtonFeedCallback cb_button_feed_;

    CSystemVect systems_;
    CSystem *first_system_ = {};
    bool comlynx_connected_ = {};
};

#endif // HANDY_MP_MULTI_SYSTEM_H_
