

#include "../../proj/tl_common.h"
#include "drivers/StdRequestType.h"
#include "drivers/StdDescriptors.h"
#include "sniffer_usbdesc.h"

#define SNIFFER_ID_VENDOR				0x0451			// for report 
#define SNIFFER_ID_PRODUCT				0x16AE
#define SNIFFER_ID_VERSION				0x7425
#define SNIFFER_STRING_VENDOR			L"Texas Instruments"
#define SNIFFER_STRING_PRODUCT			L"CC2531 USB Dongle"
#define SNIFFER_STRING_SERIAL			L"TLSR8869"

// request parameters
/** Language descriptor structure. This descriptor, located in FLASH memory, is returned when the host requests
 *  the string descriptor with index 0 (the first index). It is actually an array of 16-bit integers, which indicate
 *  via the language ID table available at USB.org what languages the device supports for its string descriptors.
 */
static const USB_Descriptor_String_t language_desc = { {
		sizeof(USB_Descriptor_Header_t) + 2, DTYPE_String
	},
	{ LANGUAGE_ID_ENG }
};

/** Manufacturer descriptor string. This is a Unicode string containing the manufacturer's details in human readable
 *  form, and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
static const USB_Descriptor_String_t vendor_desc = { {
		sizeof(USB_Descriptor_Header_t)
		+ sizeof(SNIFFER_STRING_VENDOR) - 2, DTYPE_String
	}, // Header
	SNIFFER_STRING_VENDOR
};

/** Product descriptor string. This is a Unicode string containing the product's details in human readable form,
 *  and is read out upon request by the host when the appropriate string ID is requested, listed in the Device
 *  Descriptor.
 */
static const USB_Descriptor_String_t product_desc = { {
		sizeof(USB_Descriptor_Header_t) + sizeof(SNIFFER_STRING_PRODUCT) - 2,
		DTYPE_String
	}, // Header
	SNIFFER_STRING_PRODUCT
};

/** Serial number string. This is a Unicode string containing the device's unique serial number, expressed as a
 *  series of uppercase hexadecimal digits.
 */
static const USB_Descriptor_String_t serial_desc = { {
		sizeof(USB_Descriptor_Header_t)
		+ sizeof(SNIFFER_STRING_SERIAL) - 2, DTYPE_String
	}, // Header
	SNIFFER_STRING_SERIAL
};


static USB_Descriptor_Device_t device_desc = { {
		sizeof(USB_Descriptor_Device_t), DTYPE_Device
	}, // Header
	0x0200, // USBSpecification, USB 2.0
	USB_CSCP_NoDeviceClass, // Class
	USB_CSCP_NoDeviceSubclass, // SubClass
	USB_CSCP_NoDeviceProtocol, // Protocol
	8, // Endpoint0Size, Maximum Packet Size for Zero Endpoint. Valid Sizes are 8, 16, 32, 64
	SNIFFER_ID_VENDOR, // VendorID
	SNIFFER_ID_PRODUCT, // ProductID
	SNIFFER_ID_VERSION/*0x0100*/, // .ReleaseNumber
	SNIFFER_USB_STRING_VENDOR, 	// .ManufacturerStrIndex
	SNIFFER_USB_STRING_PRODUCT, // .ProductStrIndex
	0, 	// .SerialNumStrIndex, iSerialNumber
	1
};

static const Sniffer_USB_Descriptor_Configuration_t
configuration_desc = { { {
			sizeof(USB_Descriptor_Configuration_Header_t),
			DTYPE_Configuration
		}, // Length, type
		sizeof(Sniffer_USB_Descriptor_Configuration_t), // TotalLength: variable
		1, // NumInterfaces
		1, // Configuration index
		NO_DESCRIPTOR, // Configuration String
		USB_CONFIG_ATTR_RESERVED, // Attributes
		USB_CONFIG_POWER_MA(100) // MaxPower = 100mA
	},
	// printer_interface
	{	{ sizeof(USB_Descriptor_Interface_t), DTYPE_Interface },
		0, 0, 	// AlternateSetting
		1, 		// bNumEndpoints
		0xff,
		0xff, 	// bInterfaceSubClass -> Control
		0xff,	// bInterfaceProtocol
		0 		// iInterface,  same as iProduct in USB_Descriptor_Device_t, or else not working
	},
	// printer_in_endpoint
	{	{ sizeof(USB_Descriptor_Endpoint_t), DTYPE_Endpoint }, // length, bDescriptorType
		0x83, // endpoint id
		0x02, 	// endpoint type
		0x0040, 		// wMaxPacketSize
		5 				// bInterval
	},
};

#define		SNIFFER_ID_FLASH_ADDR			0x5000

void sniffer_usbIDinit(){
	u16 *id = (u16 *) SNIFFER_ID_FLASH_ADDR;
	if (*id == 0xffff)
	{
		u16 idv = rand();
		flash_write_page(SNIFFER_ID_FLASH_ADDR,2,(u8 *)(&idv));
	}
	device_desc.ReleaseNumber = *id;
}
u8 *sniffer_usbdesc_get_language(void) {
	return (u8 *)(&language_desc);
}

u8 *sniffer_usbdesc_get_vendor(void) {
	return (u8 *)(&vendor_desc);
}

u8 *sniffer_usbdesc_get_product(void) {
	return (u8 *)(&product_desc);
}
u8 *sniffer_usbdesc_get_serial(void) {
	return (u8 *)(&serial_desc);
}

u8 *sniffer_usbdesc_get_device(void) {
	return (u8 *)(&device_desc);
}

u8 *sniffer_usbdesc_get_configuration(void) {
	return (u8 *)(&configuration_desc);
}



