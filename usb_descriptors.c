/* 
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include "tusb.h"
#include "usb_descriptors.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "class/vendor/vendor_device.h"
#include "device/usbd_pvt.h"


/* A combination of interfaces must have a unique product id, since PC will save device driver after the first plug.
 * Same VID/PID with different interface e.g MSC (first), then CDC (later) will possibly cause system error on PC.
 *
 * Auto ProductID layout's Bitmap:
 *   [MSB]       MIDI | HID | MSC | CDC          [LSB]
 */
#define _PID_MAP(itf, n)  ( (CFG_TUD_##itf) << (n) )
#define USB_PID           (0x4000 | _PID_MAP(CDC, 0) | _PID_MAP(MSC, 1) | _PID_MAP(HID, 2) | \
                           _PID_MAP(MIDI, 3) | _PID_MAP(VENDOR, 4) )

int piuio_which_device = 1; // 0 for PIUIO, 1 for LXIOv1, 2 for LXIOv2
//--------------------------------------------------------------------+
// Device Descriptors
//--------------------------------------------------------------------+
tusb_desc_device_t const desc_device_piuio =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0100, // at least 2.1 or 3.x for BOS & webUSB

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = 0xFF,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE, // TODO: This SHOULD be 8 in PIUIO

    .idVendor           = 0x0547,
    .idProduct          = 0x1002,
    .bcdDevice          = 0x0100,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

tusb_desc_device_t const desc_device_lxiov1 =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0110,

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE, // TODO: Maybe is better to put 8?

    .idVendor           = 0x0d2f,
    .idProduct          = 0x1020,
    .bcdDevice          = 0x0001,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,

    .bNumConfigurations = 0x01
};

tusb_desc_device_t const desc_device_lxiov2 =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0110,

    // Use Interface Association Descriptor (IAD) for CDC
    // As required by USB Specs IAD's subclass must be common class (2) and protocol must be IAD (1)
    .bDeviceClass       = 0xFF,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE, // TODO: Maybe is better to put 8?

    .idVendor           = 0x0d2f,
    .idProduct          = 0x1040,
    .bcdDevice          = 0x0001,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x03,

    .bNumConfigurations = 0x01
};

const tusb_desc_device_t* desc_devices[] = {
    &desc_device_piuio,
    &desc_device_lxiov1,
    &desc_device_lxiov2,
};

// Invoked when received GET DEVICE DESCRIPTOR
// Application return pointer to descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) desc_devices[piuio_which_device];
}

//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+
enum
{
    ITF_NUM_VENDOR,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN_PIUIO  (TUD_CONFIG_DESC_LEN + TUD_VENDOR_DESC_LEN)
#define CONFIG_TOTAL_LEN_LXIO   (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

#define EPNUM_VENDOR_IN_PIUIO 0x01
#define EPNUM_VENDOR_OUT_PIUIO 0x01

uint8_t const desc_configuration_piuio[] =
{
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_PIUIO, 0x00, 500),

    // Interface number, string index, EP Out & IN address, EP size
    TUD_VENDOR_DESCRIPTOR(ITF_NUM_VENDOR, 0, EPNUM_VENDOR_OUT_PIUIO, 0x80 | EPNUM_VENDOR_IN_PIUIO,
                            TUD_OPT_HIGH_SPEED ? 512 : 64)
};


uint8_t const desc_hid_report_lxio[] =
{
    TUD_HID_REPORT_DESC_GENERIC_INOUT(16)
};

// Invoked when received GET HID REPORT DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void) itf;
    return desc_hid_report_lxio;
}

uint8_t const desc_configuration_lxio[] = {
    // Config number, interface count, string index, total length, attribute, power in mA
    TUD_CONFIG_DESCRIPTOR(1, ITF_NUM_TOTAL, 0, CONFIG_TOTAL_LEN_LXIO, 0x00, 100),

    // HID Input & Output descriptor
    // Interface number, string index, protocol, report descriptor len, EP OUT & IN address, size & polling interval
    TUD_HID_INOUT_DESCRIPTOR(ITF_NUM_VENDOR, 0, HID_ITF_PROTOCOL_NONE, sizeof(desc_hid_report_lxio), EPNUM_VENDOR_OUT_LXIO, 0x80 | EPNUM_VENDOR_IN_LXIO, 16, 1)
};

const usbd_class_driver_t _piuio_driver[] = {
    {
        .name             = "VENDOR",
        .init             = vendord_init,
        .deinit           = vendord_deinit,
        .reset            = vendord_reset,
        .open             = vendord_open,
        .control_xfer_cb  = tud_vendor_control_xfer_cb,
        .xfer_cb          = vendord_xfer_cb,
        .sof              = NULL
    }
};

const usbd_class_driver_t _lxio_driver[] = {
    {
        .name             = "HID",
        .init             = hidd_init,
        .deinit           = hidd_deinit,
        .reset            = hidd_reset,
        .open             = hidd_open,
        .control_xfer_cb  = hidd_control_xfer_cb,
        .xfer_cb          = hidd_xfer_cb,
        .sof              = NULL
    }
};

// Implement callback to add our custom driver
usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count) {
    *driver_count = 1;
    if(piuio_which_device == 0) return &_piuio_driver[0];
    else return &_lxio_driver[0];
}

const uint8_t * const desc_configurations[] = {
    desc_configuration_piuio,
    desc_configuration_lxio,
    desc_configuration_lxio,
};

// Invoked when received GET CONFIGURATION DESCRIPTOR
// Application return pointer to descriptor
// Descriptor contents must exist long enough for transfer to complete
uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index; // for multiple configurations
    return desc_configurations[piuio_which_device];
}


//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

// array of pointer to string descriptors
char const* string_desc_arr_piuio [] =
{
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "Andamirn'to",                  // 1: Manufacturer
    "Andamirn'to PIUIO",            // 2: Product
};

char const* string_desc_arr_lxio [] =
{
    (const char[]) { 0x09, 0x04 },  // 0: is supported language is English (0x0409)
    "ANDAMIRO",                     // 1: Manufacturer
    "PIU HID V1.00",                // 2: Product
};

char const** string_desc_arrs [] = {
    string_desc_arr_piuio,
    string_desc_arr_lxio,
    string_desc_arr_lxio,
};

const size_t string_desc_arr_sizes [] = {
    sizeof(string_desc_arr_piuio)/sizeof(string_desc_arr_piuio[0]),
    sizeof(string_desc_arr_lxio)/sizeof(string_desc_arr_lxio[0]),
    sizeof(string_desc_arr_lxio)/sizeof(string_desc_arr_lxio[0]),
};

static uint16_t _desc_str[32];

// Invoked when received GET STRING DESCRIPTOR request
// Application return pointer to descriptor, whose contents must exist long enough for transfer to complete
uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if ( index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arrs[piuio_which_device][0], 2);
        chr_count = 1;
    }else
    {
        // Note: the 0xEE index string is a Microsoft OS 1.0 Descriptors.
        // https://docs.microsoft.com/en-us/windows-hardware/drivers/usbcon/microsoft-defined-usb-descriptors

        if ( !(index < string_desc_arr_sizes[piuio_which_device]) ) return NULL;

        const char* str = string_desc_arrs[piuio_which_device][index];

        // Cap at max char
        chr_count = (uint8_t) strlen(str);
        if ( chr_count > 31 ) chr_count = 31;

        // Convert ASCII string into UTF-16
        for(uint8_t i=0; i<chr_count; i++)
        {
            _desc_str[1+i] = str[i];
        }
    }

    // first byte is length (including header), second byte is string type
    _desc_str[0] = (uint16_t) ((TUSB_DESC_STRING << 8 ) | (2*chr_count + 2));

    return _desc_str;
}
