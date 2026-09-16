#ifndef _USB_CONFIG_H
#define _USB_CONFIG_H

#include "funconfig.h"
#include "ch32fun.h"

// ---------------------------------------------------------------------------
// ch32fun USBFS stack configuration
//
// BiteVault is a single-interface HID device with one bidirectional 64-byte
// interrupt endpoint pair (EP1 IN/OUT), which is exactly what the FIDO HID
// transport (CTAPHID) asks for.
// ---------------------------------------------------------------------------
#define FUSB_BUFFERS_NUMBER   3 // EP0 + EP1 IN + EP1 OUT
#define FUSB_EP1_MODE         USBFS_EP_MODE_BDIR
#define FUSB_SUPPORTS_SLEEP   0
#define FUSB_HID_INTERFACES   1
#define FUSB_HID_USER_REPORTS 1 // needed so SET_REPORT/GET_REPORT reach us
#define FUSB_USER_HANDLERS    1 // needed so interrupt OUT data reaches us
#define FUSB_CURSED_TURBO_DMA 0
#define FUSB_IO_PROFILE       0
#define FUSB_USE_HPE          FUNCONF_ENABLE_HPE
#define FUSB_USE_DMA7_COPY    0
#define FUSB_VDD_5V           FUNCONF_USE_5V_VDD
#define FUSB_FROM_RAM         0

#include "usb_defines.h"

// pid.codes test VID + the "for testing only" PID.  Get your own PID from
// https://pid.codes before you hand these out to anyone.
#define FUSB_USB_VID 0x1209
#define FUSB_USB_PID 0x0001
#define FUSB_USB_REV 0x0001
#define FUSB_STR_MANUFACTURER u"Hack Club"
#define FUSB_STR_PRODUCT      u"BiteVault"
#define FUSB_STR_SERIAL       u"0001"

static const uint8_t device_descriptor[] = {
	18,         // bLength
	1,          // bDescriptorType: Device
	0x00, 0x02, // bcdUSB: 2.00
	0x00,       // bDeviceClass: per-interface
	0x00,       // bDeviceSubClass
	0x00,       // bDeviceProtocol
	64,         // bMaxPacketSize0
	(uint8_t)(FUSB_USB_VID), (uint8_t)(FUSB_USB_VID >> 8),
	(uint8_t)(FUSB_USB_PID), (uint8_t)(FUSB_USB_PID >> 8),
	(uint8_t)(FUSB_USB_REV), (uint8_t)(FUSB_USB_REV >> 8),
	1,          // iManufacturer
	2,          // iProduct
	3,          // iSerialNumber
	1,          // bNumConfigurations
};

// The FIDO HID report descriptor, verbatim from the CTAP spec (section 11.2.9
// "HID report descriptor and device discovery").  Hosts find FIDO devices by
// matching usage page 0xF1D0 / usage 0x01, so none of this is optional.
static const uint8_t fido_report_descriptor[] = {
	0x06, 0xD0, 0xF1, // USAGE_PAGE (FIDO Alliance)
	0x09, 0x01,       // USAGE (U2F HID Authenticator Device)
	0xA1, 0x01,       // COLLECTION (Application)
	0x09, 0x20,       //   USAGE (Input Report Data)
	0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
	0x75, 0x08,       //   REPORT_SIZE (8)
	0x95, 0x40,       //   REPORT_COUNT (64)
	0x81, 0x02,       //   INPUT (Data,Var,Abs)
	0x09, 0x21,       //   USAGE (Output Report Data)
	0x15, 0x00,       //   LOGICAL_MINIMUM (0)
	0x26, 0xFF, 0x00, //   LOGICAL_MAXIMUM (255)
	0x75, 0x08,       //   REPORT_SIZE (8)
	0x95, 0x40,       //   REPORT_COUNT (64)
	0x91, 0x02,       //   OUTPUT (Data,Var,Abs)
	0xC0,             // END_COLLECTION
};

static const uint8_t config_descriptor[] = {
	/* Configuration Descriptor */
	0x09,       // bLength
	0x02,       // bDescriptorType: Configuration
	0x29, 0x00, // wTotalLength: 41
	0x01,       // bNumInterfaces
	0x01,       // bConfigurationValue
	0x00,       // iConfiguration
	0x80,       // bmAttributes: bus powered, no remote wakeup
	0x32,       // bMaxPower: 100 mA

	/* Interface Descriptor */
	0x09,       // bLength
	0x04,       // bDescriptorType: Interface
	0x00,       // bInterfaceNumber
	0x00,       // bAlternateSetting
	0x02,       // bNumEndpoints
	0x03,       // bInterfaceClass: HID
	0x00,       // bInterfaceSubClass: none (not boot)
	0x00,       // bInterfaceProtocol: none
	0x00,       // iInterface

	/* HID Descriptor */
	0x09,       // bLength
	0x21,       // bDescriptorType: HID
	0x11, 0x01, // bcdHID: 1.11
	0x00,       // bCountryCode
	0x01,       // bNumDescriptors
	0x22,       // bDescriptorType: Report
	sizeof(fido_report_descriptor), 0x00, // wDescriptorLength

	/* Endpoint Descriptor: interrupt IN */
	0x07,       // bLength
	0x05,       // bDescriptorType: Endpoint
	0x81,       // bEndpointAddress: EP1 IN
	0x03,       // bmAttributes: interrupt
	0x40, 0x00, // wMaxPacketSize: 64
	0x05,       // bInterval: 5 ms

	/* Endpoint Descriptor: interrupt OUT */
	0x07,       // bLength
	0x05,       // bDescriptorType: Endpoint
	0x01,       // bEndpointAddress: EP1 OUT
	0x03,       // bmAttributes: interrupt
	0x40, 0x00, // wMaxPacketSize: 64
	0x05,       // bInterval: 5 ms
};

struct usb_string_descriptor_struct {
	uint8_t bLength;
	uint8_t bDescriptorType;
	uint16_t wString[];
};
static const struct usb_string_descriptor_struct string0 __attribute__((section(".rodata"))) = {
	4, 3, {0x0409} // English (US)
};
static const struct usb_string_descriptor_struct string1 __attribute__((section(".rodata"))) = {
	sizeof(FUSB_STR_MANUFACTURER), 3, FUSB_STR_MANUFACTURER
};
static const struct usb_string_descriptor_struct string2 __attribute__((section(".rodata"))) = {
	sizeof(FUSB_STR_PRODUCT), 3, FUSB_STR_PRODUCT
};
static const struct usb_string_descriptor_struct string3 __attribute__((section(".rodata"))) = {
	sizeof(FUSB_STR_SERIAL), 3, FUSB_STR_SERIAL
};

static const struct descriptor_list_struct {
	uint32_t      lIndexValue;
	const uint8_t *addr;
	uint8_t       length;
} descriptor_list[] = {
	{0x00000100, device_descriptor, sizeof(device_descriptor)},
	{0x00000200, config_descriptor, sizeof(config_descriptor)},
	{0x00002200, fido_report_descriptor, sizeof(fido_report_descriptor)},
	{0x00002100, config_descriptor + 18, 9}, // HID descriptor on its own (Windows/Android ask)

	{0x00000300, (const uint8_t *)&string0, 4},
	{0x04090301, (const uint8_t *)&string1, string1.bLength},
	{0x04090302, (const uint8_t *)&string2, string2.bLength},
	{0x04090303, (const uint8_t *)&string3, string3.bLength},
};
#define DESCRIPTOR_LIST_ENTRIES ((sizeof(descriptor_list))/(sizeof(struct descriptor_list_struct)))

#endif
