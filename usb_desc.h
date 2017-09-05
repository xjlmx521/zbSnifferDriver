#pragma once

enum {
	SNIFFER_USB_STRING_LANGUAGE = 0,
	SNIFFER_USB_STRING_VENDOR,
	SNIFFER_USB_STRING_PRODUCT,
	SNIFFER_USB_STRING_SERIAL,
};

typedef struct {
	USB_Descriptor_Configuration_Header_t Config;
	USB_Descriptor_Interface_t intf;
	USB_Descriptor_Endpoint_t endp;
} Sniffer_USB_Descriptor_Configuration_t;

u8 *sniffer_usbdesc_get_language(void);
u8 *sniffer_usbdesc_get_vendor(void);
u8 *sniffer_usbdesc_get_product(void);
u8 *sniffer_usbdesc_get_serial(void);
u8 *sniffer_usbdesc_get_device(void);
u8 *sniffer_usbdesc_get_configuration(void);

