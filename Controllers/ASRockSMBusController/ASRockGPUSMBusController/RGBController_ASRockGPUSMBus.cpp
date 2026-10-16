/*---------------------------------------------------------*\
| RGBController_ASRockGPUSMBus.cpp                          |
|                                                           |
|   RGBController for ASRock GPU RGB controllers            |
|                                                           |
|   Based on SignalRGB ASRock GPU.js plugin                 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_ASRockGPUSMBus.h"

/**------------------------------------------------------------------*\
    @name ASRock GPU SMBus
    @category GPU
    @type I2C
    @save :x:
    @direct :white_check_mark:
    @effects :white_check_mark:
    @detectors DetectASRockGPUSMBusControllers
    @comment
\*-------------------------------------------------------------------*/

RGBController_ASRockGPUSMBus::RGBController_ASRockGPUSMBus(ASRockGPUSMBusController* controller_ptr)
{
    controller  = controller_ptr;

    name        = controller->GetDeviceName();
    vendor      = "ASRock";
    type        = DEVICE_TYPE_GPU;
    description = "ASRock GPU RGB Controller";
    location    = controller->GetDeviceLocation();

    mode Direct;
    Direct.name       = "Direct";
    Direct.value      = 0;
    Direct.flags      = MODE_FLAG_HAS_PER_LED_COLOR;
    Direct.color_mode = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    AddHardwareMode("Static",         ASROCK_GPU_MODE_STATIC,         true,  false);
    AddHardwareMode("Breathing",      ASROCK_GPU_MODE_BREATHING,      true,  true);
    AddHardwareMode("Flashing",       ASROCK_GPU_MODE_FLASHING,       true,  true);
    AddHardwareMode("Color Cycle",    ASROCK_GPU_MODE_COLOR_CYCLE,    false, true);
    AddHardwareMode("Spectrum Cycle", ASROCK_GPU_MODE_SPECTRUM_CYCLE, false, true);
    AddHardwareMode("Ping Pong",      ASROCK_GPU_MODE_PING_PONG,      true,  true);
    AddHardwareMode("Fill and Drain", ASROCK_GPU_MODE_FILL_DRAIN,     true,  true);
    AddHardwareMode("Fill",           ASROCK_GPU_MODE_FILL,           true,  true);
    AddHardwareMode("Spin",           ASROCK_GPU_MODE_SPIN,           true,  true);
    AddHardwareMode("Alternating",    ASROCK_GPU_MODE_ALTERNATING,    true,  true);
    AddHardwareMode("Spectrum Spin",  ASROCK_GPU_MODE_SPECTRUM_SPIN,  false, true);
    AddHardwareMode("Rainbow Wave",   ASROCK_GPU_MODE_RAINBOW_WAVE,   false, true);
    AddHardwareMode("Rainbow Spin",   ASROCK_GPU_MODE_RAINBOW_SPIN,   false, true);

    mode Off;
    Off.name       = "Off";
    Off.value      = ASROCK_GPU_MODE_OFF;
    Off.flags      = 0;
    Off.color_mode = MODE_COLORS_NONE;
    modes.push_back(Off);

    SetupZones();
}

void RGBController_ASRockGPUSMBus::AddHardwareMode(const char* mode_name, uint8_t mode_value, bool has_color, bool has_speed)
{
    mode new_mode;
    new_mode.name           = mode_name;
    new_mode.value          = mode_value;
    new_mode.flags          = MODE_FLAG_HAS_BRIGHTNESS;
    new_mode.brightness_min = ASROCK_GPU_BRIGHTNESS_MIN;
    new_mode.brightness_max = ASROCK_GPU_BRIGHTNESS_MAX;
    new_mode.brightness     = ASROCK_GPU_BRIGHTNESS_MAX;

    if(has_color)
    {
        new_mode.flags     |= MODE_FLAG_HAS_MODE_SPECIFIC_COLOR;
        new_mode.color_mode = MODE_COLORS_MODE_SPECIFIC;
        new_mode.colors_min = 1;
        new_mode.colors_max = 1;
        new_mode.colors.resize(1, ToRGBColor(255, 0, 0));
    }
    else
    {
        new_mode.color_mode = MODE_COLORS_NONE;
    }

    if(has_speed)
    {
        new_mode.flags     |= MODE_FLAG_HAS_SPEED;
        new_mode.speed_min  = ASROCK_GPU_SPEED_SLOWEST;
        new_mode.speed_max  = ASROCK_GPU_SPEED_FASTEST;
        new_mode.speed      = ASROCK_GPU_SPEED_DEFAULT;
    }

    modes.push_back(new_mode);
}

RGBController_ASRockGPUSMBus::~RGBController_ASRockGPUSMBus()
{
    Shutdown();

    delete controller;
}

void RGBController_ASRockGPUSMBus::SetupZones()
{
    /*-----------------------------------------------------*\
    | Set up zones - one zone per channel                   |
    \*-----------------------------------------------------*/
    for(unsigned int i = 0; i < controller->num_channels; i++)
    {
        uint8_t channel_idx = controller->enabled_channels[i];
        
        zone new_zone;
        new_zone.type = ZONE_TYPE_SINGLE;
        new_zone.leds_min = 1;
        new_zone.leds_max = 1;
        new_zone.leds_count = 1;
        
        /*-----------------------------------------------------*\
        | Channel names from SignalRGB                          |
        \*-----------------------------------------------------*/
        if(channel_idx == ASROCK_GPU_CHANNEL_ARGB_HEADER)
        {
            new_zone.name = "ARGB Header";
        }
        else if(channel_idx == ASROCK_GPU_CHANNEL_TOP_SIDE)
        {
            new_zone.name = "Top Side";
        }
        else if(channel_idx == ASROCK_GPU_CHANNEL_FAN)
        {
            new_zone.name = "Fan";
        }
        else
        {
            new_zone.name = "Channel " + std::to_string(channel_idx);
        }
        
        zones.push_back(new_zone);
        
        /*-----------------------------------------------------*\
        | Set up LED for this zone                              |
        \*-----------------------------------------------------*/
        led new_led;
        new_led.name = new_zone.name;
        leds.push_back(new_led);
    }

    SetupColors();
}

void RGBController_ASRockGPUSMBus::ResizeZone(int /*zone*/, int /*new_size*/)
{
    /*-----------------------------------------------------*\
    | This device does not support resizing zones           |
    \*-----------------------------------------------------*/
}

void RGBController_ASRockGPUSMBus::DeviceUpdateLEDs()
{
    /*-----------------------------------------------------*\
    | Per-LED colors only apply in Direct mode              |
    \*-----------------------------------------------------*/
    if(modes[active_mode].color_mode != MODE_COLORS_PER_LED)
    {
        return;
    }

    for(unsigned int i = 0; i < controller->num_channels; i++)
    {
        uint8_t channel_idx = controller->enabled_channels[i];
        RGBColor color = colors[i];
        
        controller->SetChannelColor(channel_idx, 
                                    RGBGetRValue(color),
                                    RGBGetGValue(color),
                                    RGBGetBValue(color));
    }
}

void RGBController_ASRockGPUSMBus::UpdateZoneLEDs(int zone)
{
    /*-----------------------------------------------------*\
    | Per-LED colors only apply in Direct mode              |
    \*-----------------------------------------------------*/
    if(modes[active_mode].color_mode != MODE_COLORS_PER_LED)
    {
        return;
    }

    uint8_t channel_idx = controller->enabled_channels[zone];
    RGBColor color = colors[zone];
    
    controller->SetChannelColor(channel_idx,
                                RGBGetRValue(color),
                                RGBGetGValue(color),
                                RGBGetBValue(color));
}

void RGBController_ASRockGPUSMBus::UpdateSingleLED(int led)
{
    UpdateZoneLEDs(led);
}

void RGBController_ASRockGPUSMBus::DeviceUpdateMode()
{
    const mode& current = modes[active_mode];

    if(current.color_mode == MODE_COLORS_PER_LED)
    {
        DeviceUpdateLEDs();
        return;
    }

    RGBColor color      = (current.color_mode == MODE_COLORS_MODE_SPECIFIC) ? current.colors[0] : 0;
    uint8_t  speed      = (current.flags & MODE_FLAG_HAS_SPEED)      ? current.speed      : ASROCK_GPU_SPEED_DEFAULT;
    uint8_t  brightness = (current.flags & MODE_FLAG_HAS_BRIGHTNESS) ? current.brightness : ASROCK_GPU_BRIGHTNESS_MAX;

    for(unsigned int i = 0; i < controller->num_channels; i++)
    {
        controller->SetChannelMode(controller->enabled_channels[i],
                                   current.value,
                                   RGBGetRValue(color),
                                   RGBGetGValue(color),
                                   RGBGetBValue(color),
                                   speed,
                                   brightness);
    }
}
