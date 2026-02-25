#include "tusb.h"
#include "usb_descriptors.h"
#include "class/hid/hid.h"
#include "class/hid/hid_device.h"
#include "device/usbd_pvt.h"

//--------------------------------------------------------------------+
// Device Descriptor (LXIO v1)
//--------------------------------------------------------------------+

tusb_desc_device_t const desc_device =
{
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0110,

    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,

    .bMaxPacketSize0    = 64,

    .idVendor           = 0x0d2f,
    .idProduct          = 0x1020,
    .bcdDevice          = 0x0001,

    .iManufacturer      = 0x01,
    .iProduct           = 0x02,
    .iSerialNumber      = 0x00,

    .bNumConfigurations = 0x01
};

// Return device descriptor
uint8_t const * tud_descriptor_device_cb(void)
{
    return (uint8_t const *) &desc_device;
}


//--------------------------------------------------------------------+
// HID Report Descriptor
//--------------------------------------------------------------------+

uint8_t const desc_hid_report[] =
{
    TUD_HID_REPORT_DESC_GENERIC_INOUT(16)
};

uint8_t const * tud_hid_descriptor_report_cb(uint8_t itf)
{
    (void) itf;
    return desc_hid_report;
}


//--------------------------------------------------------------------+
// Configuration Descriptor
//--------------------------------------------------------------------+

enum
{
    ITF_NUM_HID,
    ITF_NUM_TOTAL
};

#define CONFIG_TOTAL_LEN  (TUD_CONFIG_DESC_LEN + TUD_HID_INOUT_DESC_LEN)

uint8_t const desc_configuration[] =
{
    TUD_CONFIG_DESCRIPTOR(
        1,
        ITF_NUM_TOTAL,
        0,
        CONFIG_TOTAL_LEN,
        0x00,
        100),

    TUD_HID_INOUT_DESCRIPTOR(
        ITF_NUM_HID,
        0,
        HID_ITF_PROTOCOL_NONE,
        sizeof(desc_hid_report),
        EPNUM_VENDOR_OUT_LXIO,
        0x80 | EPNUM_VENDOR_IN_LXIO,
        16,
        1)
};

uint8_t const * tud_descriptor_configuration_cb(uint8_t index)
{
    (void) index;
    return desc_configuration;
}


//--------------------------------------------------------------------+
// HID Driver
//--------------------------------------------------------------------+

const usbd_class_driver_t hid_driver =
{
    .name             = "HID",
    .init             = hidd_init,
    .deinit           = hidd_deinit,
    .reset            = hidd_reset,
    .open             = hidd_open,
    .control_xfer_cb  = hidd_control_xfer_cb,
    .xfer_cb          = hidd_xfer_cb,
    .sof              = NULL
};

usbd_class_driver_t const *usbd_app_driver_get_cb(uint8_t *driver_count)
{
    *driver_count = 1;
    return &hid_driver;
}


//--------------------------------------------------------------------+
// String Descriptors
//--------------------------------------------------------------------+

char const* string_desc_arr[] =
{
    (const char[]) { 0x09, 0x04 }, // language English
    "ANDAMIRO",
    "PIU HID V1.00",
};

#define STRING_DESC_COUNT (sizeof(string_desc_arr)/sizeof(string_desc_arr[0]))

static uint16_t _desc_str[32];

uint16_t const* tud_descriptor_string_cb(uint8_t index, uint16_t langid)
{
    (void) langid;

    uint8_t chr_count;

    if (index == 0)
    {
        memcpy(&_desc_str[1], string_desc_arr[0], 2);
        chr_count = 1;
    }
    else
    {
        if (index >= STRING_DESC_COUNT) return NULL;

        const char* str = string_desc_arr[index];

        chr_count = strlen(str);
        if (chr_count > 31) chr_count = 31;

        for(uint8_t i=0; i<chr_count; i++)
        {
            _desc_str[1+i] = str[i];
        }
    }

    _desc_str[0] =
        (TUSB_DESC_STRING << 8 )
        | (2*chr_count + 2);

    return _desc_str;
}