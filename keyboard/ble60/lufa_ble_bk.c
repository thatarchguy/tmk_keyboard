/* 
 * Copyright 2012 Jun Wako <wakojun@gmail.com>
 * This file is based on:
 *     LUFA-120219/Demos/Device/Lowlevel/KeyboardMouse
 *     LUFA-120219/Demos/Device/Lowlevel/GenericHID
 */

/*
             LUFA Library
     Copyright (C) Dean Camera, 2012.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2012  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Copyright 2010  Denver Gingerich (denver [at] ossguy [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaim all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

#include "report.h"
#include "host.h"
#include "host_driver.h"
#include "keyboard.h"
#include "action.h"
#include "led.h"
#include "sendchar.h"
#include "debug.h"
#ifdef SLEEP_LED_ENABLE
#include "sleep_led.h"
#endif
#include "suspend.h"
#include "timer.h"
#include "hardware_uart.c"
#include "descriptor.h"
#include "lufa.h"

/*
 *0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF
 *0xA8 hid_code : 00 00 00 00 00 00 00 00
 *0xA9 hid_code : xx 00 00 00 00 00 00 00
 *0xAA hid_code : xx 00 xx 00 00 00 00 00 / xx 00 00 xx 00 00 00 00...
 *.......
 */
#define HID_LEN             0xA8    //0xa8-0xaf  hid report 
#define BLE_NAME_LEN        0x13    //ble name length  20 byte
#define BLE_CHANGE_NAME     0XA5    //ble update new name
#define BLE_PAIRING         0xA6    //ble pairing
#define BLE_DISCONT         0xA7    //ble disconnect

uint8_t keyboard_idle = 0;
uint8_t keyboard_protocol = 1;
uint8_t send_mode = 0;               //0=BLE ,1=USB ,2=BLE_COMMAND
uint8_t ble_command_mode = 0;        //0=chang_name ,1=pairing
uint8_t name_cache[BLE_NAME_LEN];    
uint8_t name_cache_p = 0;
uint8_t last_send_mode = 0;          //save last mode
uint8_t pairing_press = 0;           // 2 send 0xa6

bool cable_into = false;
uint32_t last_act = 0;               //last action time sleep 
//uint32_t sleep_time = 0x124F80;      //after 20 minutes sleep
uint32_t sleep_time = 0x004e20;      //after 20s sleep
bool sleeping = false;

bool isFirstConnect = false;        //First connect suspend

static uint8_t keyboard_led_stats = 0;

static report_keyboard_t keyboard_report_sent;

/* Host driver */
static uint8_t keyboard_leds(void);
static void send_keyboard(report_keyboard_t *report);
static void send_mouse(report_mouse_t *report);
static void send_system(uint16_t data);
static void send_consumer(uint16_t data);
static uint8_t key2inputasc(uint8_t key,bool is_shift);
host_driver_t lufa_driver = {
    keyboard_leds,
    send_keyboard,
    send_mouse,
    send_system,
    send_consumer
};


/*******************************************************************************
 * Console
 ******************************************************************************/
#ifdef CONSOLE_ENABLE
static void Console_Task(void)
{7773
    /* Device must be connected and configured for the task to run */
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    uint8_t ep = Endpoint_GetCurrentEndpoint();

#if 0
    // TODO: impl receivechar()/recvchar()
    Endpoint_SelectEndpoint(CONSOLE_OUT_EPNUM);

    /* Check to see if a packet has been sent from the host */
    if (Endpoint_IsOUTReceived())
    {
        /* Check to see if the packet contains data */
        if (Endpoint_IsReadWriteAllowed())
        {
            /* Create a temporary buffer to hold the read in report from the host */
            uint8_t ConsoleData[CONSOLE_EPSIZE];
 
            /* Read Console Report Data */
            Endpoint_Read_Stream_LE(&ConsoleData, sizeof(ConsoleData), NULL);
 
            /* Process Console Report Data */
            //ProcessConsoleHIDReport(ConsoleData);
        }

        /* Finalize the stream transfer to send the last packet */
        Endpoint_ClearOUT();
    }
#endif

    /* IN packet */
    Endpoint_SelectEndpoint(CONSOLE_IN_EPNUM);
    if (!Endpoint_IsEnabled() || !Endpoint_IsConfigured()) {
        Endpoint_SelectEndpoint(ep);
        return;
    }

    // fill empty bank
    while (Endpoint_IsReadWriteAllowed())
        Endpoint_Write_8(0);

    // flash senchar packet
    if (Endpoint_IsINReady()) {
        Endpoint_ClearIN();
    }

    Endpoint_SelectEndpoint(ep);
}
#else
static void Console_Task(void)
{
}
#endif


/*******************************************************************************
 * USB Events
 ******************************************************************************/
/*
 * Event Order of Plug in:
 * 0) EVENT_USB_Device_Connect
 * 1) EVENT_USB_Device_Suspend
 * 2) EVENT_USB_Device_Reset
 * 3) EVENT_USB_Device_Wake
*/
void EVENT_USB_Device_Connect(void)
{
    isFirstConnect = true;
    uart_print("Connect# ");
}

void EVENT_USB_Device_Disconnect(void)
{
    uart_print("Disconnect# ");
}

void EVENT_USB_Device_Reset(void)
{
    uart_print("Reset# ");
}

void EVENT_USB_Device_Suspend()
{
    uart_print("Suspend# ");
    if(isFirstConnect){
        isFirstConnect=false;
        return;
    }
    cable_into = true;
    uint8_t timeout=200;
    while(timeout)
    {
        if (PINF & (1<<PF4)) {
            timeout--;
        }
        else{
            cable_into = false;
            break;
        }
        _delay_ms(10);
    }
    if (PINF & (1<<PF4)) {
        uart_print("cable_into# ");
    }
    else{
        uart_print("notcable_into# ");
    }
    uart_print("Suspend$ ");
    _delay_ms(10);
    if(cable_into){
        #ifdef BACKLIGHT_ENABLE
        backlight_set(0);
        #endif
        return;
    }
    else{
        timer_clear();
        last_act = timer_read32();
        last_send_mode = send_mode;
        send_mode=0;
        #ifdef BACKLIGHT_ENABLE
        backlight_set(0);
        #endif
    }

    #ifdef SLEEP_LED_ENABLE
        sleep_led_enable();
    #endif
}

void EVENT_USB_Device_WakeUp()
{
    uart_print("WakeUp# ");
    if(sleeping){
        sleeping=false;
        cable_into=true;
        USB_Device_SendRemoteWakeup();
        return;
    }
    suspend_wakeup_init();
    #ifdef SLEEP_LED_ENABLE
        sleep_led_disable();
        // NOTE: converters may not accept this
        led_set(host_keyboard_leds());
    #endif
}

void EVENT_USB_Device_StartOfFrame(void)
{
    Console_Task();
}

/** Event handler for the USB_ConfigurationChanged event.
 * This is fired when the host sets the current configuration of the USB device after enumeration.
 */
void EVENT_USB_Device_ConfigurationChanged(void)
{
    uart_print("ConfigurationChanged# ");
    timer_clear();
    last_act = timer_read32();
    sleeping=false;
    last_send_mode = send_mode;
    send_mode=1;
    #ifdef BACKLIGHT_ENABLE
    backlight_init();
    #endif
    
    cable_into=true;

    bool ConfigSuccess = true;

    /* Setup Keyboard HID Report Endpoints */
    ConfigSuccess &= ENDPOINT_CONFIG(KEYBOARD_IN_EPNUM, EP_TYPE_INTERRUPT, ENDPOINT_DIR_IN,
                                     KEYBOARD_EPSIZE, ENDPOINT_BANK_SINGLE);

#ifdef MOUSE_ENABLE
    /* Setup Mouse HID Report Endpoint */
    ConfigSuccess &= ENDPOINT_CONFIG(MOUSE_IN_EPNUM, EP_TYPE_INTERRUPT, ENDPOINT_DIR_IN,
                                     MOUSE_EPSIZE, ENDPOINT_BANK_SINGLE);
#endif

#ifdef EXTRAKEY_ENABLE
    /* Setup Extra HID Report Endpoint */
    ConfigSuccess &= ENDPOINT_CONFIG(EXTRAKEY_IN_EPNUM, EP_TYPE_INTERRUPT, ENDPOINT_DIR_IN,
                                     EXTRAKEY_EPSIZE, ENDPOINT_BANK_SINGLE);
#endif

#ifdef CONSOLE_ENABLE
    /* Setup Console HID Report Endpoints */
    ConfigSuccess &= ENDPOINT_CONFIG(CONSOLE_IN_EPNUM, EP_TYPE_INTERRUPT, ENDPOINT_DIR_IN,
                                     CONSOLE_EPSIZE, ENDPOINT_BANK_DOUBLE);
#if 0
    ConfigSuccess &= ENDPOINT_CONFIG(CONSOLE_OUT_EPNUM, EP_TYPE_INTERRUPT, ENDPOINT_DIR_OUT,
                                     CONSOLE_EPSIZE, ENDPOINT_BANK_SINGLE);
#endif
#endif

#ifdef NKRO_ENABLE
    /* Setup NKRO HID Report Endpoints */
    ConfigSuccess &= ENDPOINT_CONFIG(NKRO_IN_EPNUM, EP_TYPE_INTERRUPT, ENDPOINT_DIR_IN,
                                     NKRO_EPSIZE, ENDPOINT_BANK_SINGLE);
#endif
}

/*
Appendix G: HID Request Support Requirements

The following table enumerates the requests that need to be supported by various types of HID class devices.

Device type     GetReport   SetReport   GetIdle     SetIdle     GetProtocol SetProtocol
------------------------------------------------------------------------------------------
Boot Mouse      Required    Optional    Optional    Optional    Required    Required
Non-Boot Mouse  Required    Optional    Optional    Optional    Optional    Optional
Boot Keyboard   Required    Optional    Required    Required    Required    Required
Non-Boot Keybrd Required    Optional    Required    Required    Optional    Optional
Other Device    Required    Optional    Optional    Optional    Optional    Optional
*/
/** Event handler for the USB_ControlRequest event.
 *  This is fired before passing along unhandled control requests to the library for processing internally.
 */
void EVENT_USB_Device_ControlRequest(void)
{
    uint8_t* ReportData = NULL;
    uint8_t  ReportSize = 0;

    /* Handle HID Class specific requests */
    switch (USB_ControlRequest.bRequest)
    {
        case HID_REQ_GetReport:
            if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                Endpoint_ClearSETUP();

                // Interface
                switch (USB_ControlRequest.wIndex) {
                case KEYBOARD_INTERFACE:
                    // TODO: test/check
                    ReportData = (uint8_t*)&keyboard_report_sent;
                    ReportSize = sizeof(keyboard_report_sent);
                    break;
                }

                /* Write the report data to the control endpoint */
                Endpoint_Write_Control_Stream_LE(ReportData, ReportSize);
                Endpoint_ClearOUT();
            }

            break;
        case HID_REQ_SetReport:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {

                // Interface
                switch (USB_ControlRequest.wIndex) {
                case KEYBOARD_INTERFACE:
#ifdef NKRO_ENABLE
                case NKRO_INTERFACE:
#endif
                    Endpoint_ClearSETUP();

                    while (!(Endpoint_IsOUTReceived())) {
                        if (USB_DeviceState == DEVICE_STATE_Unattached)
                          return;
                    }
                    keyboard_led_stats = Endpoint_Read_8();

                    Endpoint_ClearOUT();
                    Endpoint_ClearStatusStage();
                    break;
                }

            }

            break;

        case HID_REQ_GetProtocol:
            if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                if (USB_ControlRequest.wIndex == KEYBOARD_INTERFACE) {
                    Endpoint_ClearSETUP();
                    while (!(Endpoint_IsINReady()));
                    Endpoint_Write_8(keyboard_protocol);
                    Endpoint_ClearIN();
                    Endpoint_ClearStatusStage();
                }
            }

            break;
        case HID_REQ_SetProtocol:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                if (USB_ControlRequest.wIndex == KEYBOARD_INTERFACE) {
                    Endpoint_ClearSETUP();
                    Endpoint_ClearStatusStage();

                    keyboard_protocol = ((USB_ControlRequest.wValue & 0xFF) != 0x00);
#ifdef NKRO_ENABLE
                    keyboard_nkro = !!keyboard_protocol;
#endif
                    clear_keyboard();
                }
            }

            break;
        case HID_REQ_SetIdle:
            if (USB_ControlRequest.bmRequestType == (REQDIR_HOSTTODEVICE | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                Endpoint_ClearSETUP();
                Endpoint_ClearStatusStage();

                keyboard_idle = ((USB_ControlRequest.wValue & 0xFF00) >> 8);
            }

            break;
        case HID_REQ_GetIdle:
            if (USB_ControlRequest.bmRequestType == (REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE))
            {
                Endpoint_ClearSETUP();
                while (!(Endpoint_IsINReady()));
                Endpoint_Write_8(keyboard_idle);
                Endpoint_ClearIN();
                Endpoint_ClearStatusStage();
            }

            break;
    }
}

/*******************************************************************************
 * Host driver 
 ******************************************************************************/
static uint8_t keyboard_leds(void)
{
    return keyboard_led_stats;
}

static void send_keyboard(report_keyboard_t *report)
{
    uint8_t timeout = 255;
    pairing_press = 0; //reset pairing status
    //last_act = timer_read32();
    /*
    uart_transmit(report->mods);
    uart_transmit(0x00); 
    uart_transmit(report->keys[0]);
    uart_transmit(report->keys[1]);
    uart_transmit(report->keys[2]);
    uart_transmit(report->keys[3]);
    uart_transmit(report->keys[4]);
    uart_transmit(report->keys[5]);
    */
    if(send_mode == 0)
    {
        last_act = timer_read32();
        uint8_t len = 0;
        uint8_t i = 0;
        uint8_t start_hid=HID_LEN;
        if(report->mods!=0x00)
        {
            len = 1 ;
        }
        for(i = 0;i<6; i++)
        {
            if(report->keys[5-i]!=0x00)
            {
                len = (5-i) + 2;
                break;
            }
        }
        start_hid=HID_LEN+len;
        uart_transmit(start_hid);
        for(i = 0;i<len; i++)
        {
            if(i==0)
            {
                uart_transmit(report->mods);
            }
            else 
            {
                uart_transmit(report->keys[i-1]);
            }
        }
        return;
    }
    else if(send_mode == 2)
    {
        if(ble_command_mode == 0)
        {
            bool is_shift = (report->mods & 0x02) || (report->mods & 0x20);
            uint8_t tmp_key = report->keys[0];
            uint8_t tmp_status = 0; //0=continue ,1=exit ,2=update
            if (tmp_key == KC_ESC)
            {
                tmp_status = 1;
            }
            else if(tmp_key == KC_ENT)
            {
                tmp_status = 2;
            }
            else if(tmp_key == KC_BSPACE)
            {
                name_cache_p = name_cache_p ? name_cache_p - 1 : 0;
                name_cache[name_cache_p]=0x00;
            }
            else
            {
                uint8_t asc = key2inputasc(report->keys[0],is_shift);
                if (asc != 0x00)
                {
                    name_cache[name_cache_p]=asc;
                    name_cache_p++;
                    if(name_cache_p >=BLE_NAME_LEN)
                    {
                        tmp_status = 2;
                    }
                }
            }
            if(tmp_status >= 1)
            {
                if(tmp_status == 2 && name_cache_p > 0)
                {
                    uart_transmit(BLE_CHANGE_NAME);
                    uint8_t i = 0;
                    for(i = 0;i<BLE_NAME_LEN; i++)
                    {
                        if(name_cache[i]==0x00)
                        {
                            break;
                        }
                        uart_transmit(name_cache[i]);
                    }
                    uart_transmit(0x00);
                }
                send_mode = last_send_mode;
                ble_command_mode = 0;
                name_cache_p = 0;
                return;
            }
        }
    }

    //uart_transmit(USB_DeviceState);
    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    /* Select the Keyboard Report Endpoint */
#ifdef NKRO_ENABLE
    if (keyboard_nkro) {
        /* Report protocol - NKRO */
        Endpoint_SelectEndpoint(NKRO_IN_EPNUM);

        /* Check if write ready for a polling interval around 1ms */
        while (timeout-- && !Endpoint_IsReadWriteAllowed()) _delay_us(4);
        if (!Endpoint_IsReadWriteAllowed()) return;

        /* Write Keyboard Report Data */
        Endpoint_Write_Stream_LE(report, NKRO_EPSIZE, NULL);
    }
    else
#endif
    {
        /* Boot protocol */
        Endpoint_SelectEndpoint(KEYBOARD_IN_EPNUM);

        /* Check if write ready for a polling interval around 10ms */
        while (timeout-- && !Endpoint_IsReadWriteAllowed()) _delay_us(40);
        if (!Endpoint_IsReadWriteAllowed()) return;

        /* Write Keyboard Report Data */
        Endpoint_Write_Stream_LE(report, KEYBOARD_EPSIZE, NULL);
    }

    /* Finalize the stream transfer to send the last packet */
    Endpoint_ClearIN();

    keyboard_report_sent = *report;
}

static void send_mouse(report_mouse_t *report)
{
#ifdef MOUSE_ENABLE
    uint8_t timeout = 255;

    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    /* Select the Mouse Report Endpoint */
    Endpoint_SelectEndpoint(MOUSE_IN_EPNUM);

    /* Check if write ready for a polling interval around 10ms */
    while (timeout-- && !Endpoint_IsReadWriteAllowed()) _delay_us(40);
    if (!Endpoint_IsReadWriteAllowed()) return;

    /* Write Mouse Report Data */
    Endpoint_Write_Stream_LE(report, sizeof(report_mouse_t), NULL);

    /* Finalize the stream transfer to send the last packet */
    Endpoint_ClearIN();
#endif
}

static void send_system(uint16_t data)
{
    uint8_t timeout = 255;
    if(data == CHANGE_SEND_MODE)
    {
        last_send_mode = send_mode;
        if (PINF & (1<<PF4)) 
        { 
            send_mode=send_mode==1?0:1;
        }
        else
        {
            send_mode=0;
        }
        return;
    }
    else if(data == CHANGE_BLE_NAME)
    {
        last_send_mode = send_mode;
        send_mode = 2;
        ble_command_mode = 0;
        name_cache_p = 0;
        for(uint8_t i = 0;i<BLE_NAME_LEN;i++)
        {
            name_cache[i]=0x00;
        }
    }
    else if(data == PAIRING_BLE)
    {
        pairing_press++;
        if(pairing_press==2)
        {
            uart_transmit(BLE_PAIRING);
        }
        return;
    }
    else if(data == DISCONT_BLE)
    {
        uart_transmit(BLE_DISCONT);
    }
    else if(data == DONT_SEND_REPORT)
    {
        return;
    }
    

    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    report_extra_t r = {
        .report_id = REPORT_ID_SYSTEM,
        .usage = data
    };
    Endpoint_SelectEndpoint(EXTRAKEY_IN_EPNUM);

    /* Check if write ready for a polling interval around 10ms */
    while (timeout-- && !Endpoint_IsReadWriteAllowed()) _delay_us(40);
    if (!Endpoint_IsReadWriteAllowed()) return;

    Endpoint_Write_Stream_LE(&r, sizeof(report_extra_t), NULL);
    Endpoint_ClearIN();
}

static void send_consumer(uint16_t data)
{
    uint8_t timeout = 255;
    if(send_mode == 0){
        uint8_t consumer_key=0;
        switch (data) {
            case AUDIO_MUTE:
                consumer_key = 0xE8;
                break;
            case AUDIO_VOL_DOWN:
                consumer_key = 0xE9;
                break;
            case AUDIO_VOL_UP:
                consumer_key = 0xEA;
                break;
            case TRANSPORT_NEXT_TRACK:
                consumer_key = 0xEB;
                break;
            case TRANSPORT_PREV_TRACK:
                consumer_key = 0xEC;
                break;
            case TRANSPORT_STOP:
                consumer_key = 0xED;
                break;            
            case TRANSPORT_EJECT:
                consumer_key = 0xEE;
                break;
            case TRANSPORT_PLAY_PAUSE:
                consumer_key = 0xEF;
                break;
            case AL_CC_CONFIG:
                consumer_key = 0xF1;
                break;
            case AL_EMAIL:
                consumer_key = 0xF2;
                break;
            case AL_CALCULATOR:
                consumer_key = 0xF3;
                break;
            case AL_LOCAL_BROWSER:
                consumer_key = 0xF4;
                break;
            case AC_SEARCH:
                consumer_key = 0xF5;
                break;
            case AC_HOME:
                consumer_key = 0xF6;
                break;
            case AC_BACK:
                consumer_key = 0xF7;
                break;
            case AC_FORWARD:
                consumer_key = 0xF8;
                break;
            case AC_STOP:
                consumer_key = 0xF9;
                break;
            case AC_REFRESH:
                consumer_key = 0xFA;
                break;
            case AC_BOOKMARKS:
                consumer_key = 0xFB;
                break;

        }
        if(consumer_key!=0)
        {
            uart_transmit(0xAA);
            uart_transmit(0x00);
            uart_transmit(consumer_key); 
        }
        else
        {
            uart_transmit(0xA8);
        }
        return;
    }

    if (USB_DeviceState != DEVICE_STATE_Configured)
        return;

    report_extra_t r = {
        .report_id = REPORT_ID_CONSUMER,
        .usage = data
    };
    Endpoint_SelectEndpoint(EXTRAKEY_IN_EPNUM);

    /* Check if write ready for a polling interval around 10ms */
    while (timeout-- && !Endpoint_IsReadWriteAllowed()) _delay_us(40);
    if (!Endpoint_IsReadWriteAllowed()) return;

    Endpoint_Write_Stream_LE(&r, sizeof(report_extra_t), NULL);
    Endpoint_ClearIN();
}


/*******************************************************************************
 * sendchar
 ******************************************************************************/
#ifdef CONSOLE_ENABLE
#define SEND_TIMEOUT 5
int8_t sendchar(uint8_t c)
{
    // Not wait once timeouted.
    // Because sendchar() is called so many times, waiting each call causes big lag.
    static bool timeouted = false;

    if (USB_DeviceState != DEVICE_STATE_Configured)
        return -1;

    uint8_t ep = Endpoint_GetCurrentEndpoint();
    Endpoint_SelectEndpoint(CONSOLE_IN_EPNUM);
    if (!Endpoint_IsEnabled() || !Endpoint_IsConfigured()) {
        Endpoint_SelectEndpoint(ep);
        return -1;
    }

    if (timeouted && !Endpoint_IsReadWriteAllowed()) {
        Endpoint_SelectEndpoint(ep);
        return - 1;
    }

    timeouted = false;

    uint8_t timeout = SEND_TIMEOUT;
    uint16_t prevFN = USB_Device_GetFrameNumber();
    while (!Endpoint_IsReadWriteAllowed()) {
        switch (USB_DeviceState) {
        case DEVICE_STATE_Unattached:
        case DEVICE_STATE_Suspended:
            return -1;
        }
        if (Endpoint_IsStalled()) {
            Endpoint_SelectEndpoint(ep);
            return -1;
        }
        if (prevFN != USB_Device_GetFrameNumber()) {
            if (!(timeout--)) {
                timeouted = true;
                Endpoint_SelectEndpoint(ep);
                return -1;
            }
            prevFN = USB_Device_GetFrameNumber();
        }
    }

    Endpoint_Write_8(c);

    // send when bank is full
    if (!Endpoint_IsReadWriteAllowed())
        Endpoint_ClearIN();

    Endpoint_SelectEndpoint(ep);
    return 0;
}
#else
int8_t sendchar(uint8_t c)
{
    return 0;
}
#endif

// convert keycode into ascii charactor
static uint8_t key2inputasc(uint8_t key,bool is_shift)
{
    uint8_t ret = 0;
    uint8_t offset_len = 0;
    if (key >= KC_A && key <= KC_Z)                 //a-z
    {
        offset_len = key - KC_A;
        ret = is_shift ? (0x41 + offset_len) : (0x61 + offset_len);
    }
    else if (key >= KC_1 && key <= KC_0)            //0-9
    {
        offset_len = key - KC_1;
        uint8_t num_char[10] ={'!','@','#','$','%','^','&','*','(',')'};
        ret = is_shift ? num_char[offset_len] : (key==KC_0) ? 0x30 : (0x31 + offset_len);
    }
    else if(key >= KC_SPACE && key <= KC_SLASH)     //-_+=[{}]\|;:'"`~,<.>/?
    {   
        uint8_t symbol_top[13] =    {' ','-','=','[',']','\\',' ',';','\'','`',',','.','/'};
        uint8_t symbol_bottom[13] = {' ','_','+','{','}','|' ,' ',':','"' ,'~','<','>','?'};
        offset_len = key - KC_SPACE;
        ret = is_shift ? symbol_bottom[offset_len] : symbol_top[offset_len];
    }
    else
    {
        ret = 0;
    }
    
    return ret;
}
/*******************************************************************************
 * main
 ******************************************************************************/
static void setup_mcu(void)
{
    /* Disable watchdog if enabled by bootloader/fuses */
    MCUSR &= ~(1 << WDRF);
    wdt_disable();

    /* Disable clock division */
    clock_prescale_set(clock_div_1);
            // disable JTAG
    MCUCR = (1<<JTD);
    MCUCR = (1<<JTD);
    DDRF  &= ~(1<<PF4);
    PORTF &= ~(1<<PF4);
}
static void setup_usb(void)
{
    // Leonardo needs. Without this USB device is not recognized.
    //USB_Disable();

    USB_Init();

    // for Console_Task
    USB_Device_EnableSOFEvents();
    print_set_sendchar(sendchar);
}
static void setup_uart()
{
    uart_init();
    unsigned char receivedChar = '0';
    //uart_print("hi body# ");
}

int main(void)  __attribute__ ((weak));
int main(void)
{
    setup_mcu();
    setup_usb();
    keyboard_setup();
    setup_uart();
    if (PINF & (1<<PF4)) {
        uart_print("cable_into# ");
    }
    else{
        uart_print("notcable_into# ");

    }
   
    sei();
    //last_act = timer_read32();
    keyboard_init();
    uart_print("hi body2# ");

#ifdef BACKLIGHT_ENABLE
    if(!cable_into)
    {
        backlight_set(0);
    }
#endif
    host_set_driver(&lufa_driver);

 #ifdef SLEEP_LED_ENABLE
    sleep_led_init();
#endif
    while (1)
    {
        
        if (cable_into)
        {
            if(send_mode!=0){
                while (USB_DeviceState == DEVICE_STATE_Suspended) 
                {
                    suspend_power_down();
                    if (USB_Device_RemoteWakeupEnabled && suspend_wakeup_condition()) 
                    {
                        USB_Device_SendRemoteWakeup();
                    }
                }
            }
        }
        else
        {
            if(sleeping == false && timer_elapsed32(last_act) > sleep_time ) 
            {
                uart_print("sleep");
                //uart_transmit(BLE_DISCONT);
                sleeping = true;
                _delay_ms(10); 
            }
            while (sleeping) 
            {
                suspend_power_down();
                if (suspend_wakeup_condition())
                {
                    timer_clear();
                    last_act = timer_read32();
                    sleeping = false;
                    //suspend_wakeup_init();
                    break;
                }
            }
        }
        
        keyboard_task();

        #if !defined(INTERRUPT_CONTROL_ENDPOINT)
            USB_USBTask();
        #endif
    }
    
}

/*
    MCUSR &= ~(1 << WDRF);
    MCUCR |= (1 << JTD);
    MCUCR |= (1 << JTD); //close jtag


*/
