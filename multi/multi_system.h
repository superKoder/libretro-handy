// MIT License
//
// Copyright (c) 2024 superKoder (github.com/superKoder/)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The ABOVE COPYRIGHT notice and this permission notice SHALL BE INCLUDED in all
// copies or substantial portions of the Software.
//
// The software is provided "as is", without warranty of any kind, express or
// implied, including but not limited to the warranties of merchantability,
// fitness for a particular purpose and noninfringement. In no event shall the
// authors or copyright holders be liable for any claim, damages or other
// liability, whether in an action of contract, tort or otherwise, arising from,
// out of or in connection with the software or the use or other dealings in the
// software.

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

    Layout::Orientation GetCartRotation();

    void UpdateButtons();
    void UpdateComLynx();

    void FetchAudioSamples();

    void DisplaySetAttributes(Layout::Orientation rotate,
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
