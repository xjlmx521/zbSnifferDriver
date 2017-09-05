

#include "../../proj/tl_common.h"
#include "drivers/StdRequestType.h"
#include "drivers/StdDescriptors.h"
#include "sniffer_usbdesc.h"
#include "drivers/usbhw_i.h"


enum {
	SNIFFER_USB_IRQ_SETUP_REQ = 0,
	SNIFFER_USB_IRQ_DATA_REQ,
};

enum {
	SNIFFER_USB_EDP_BULK_IN = 3,	//  TI use endpoint 3, must be 3
};

static USB_Request_Header_t control_request;
static u8 *g_response = 0;
static u16 g_response_len = 0;
static int sniffer_g_stall = 0;


_attribute_ram_code_ u8 sniffer_usbIdle(){
	return (!usbhw_is_ep_busy(SNIFFER_USB_EDP_BULK_IN));
}
void sniffer_usb_send_response(void) {
	u16 n;

	if(g_response_len < 8) {
		n = g_response_len;
	}
	else {
		n = 8;
	}
	printf("...send len %d\n",n);
	g_response_len -= n;
	usbhw_reset_ctrl_ep_ptr();
	while(n-- > 0) {
		usbhw_write_ctrl_ep_data(*g_response);
		++g_response;
	}
}

void sniffer_usb_prepare_desc_data(void) {
	u8 value_l = (control_request.wValue) & 0xff;
	u8 value_h = (control_request.wValue >> 8) & 0xff;

	g_response = 0;
	g_response_len = 0;

	switch(value_h) {

		case DTYPE_Device:
			g_response = sniffer_usbdesc_get_device();
			g_response_len = sizeof(USB_Descriptor_Device_t);
			break;

		case DTYPE_Configuration:
			g_response = sniffer_usbdesc_get_configuration();
			g_response_len = sizeof(Sniffer_USB_Descriptor_Configuration_t);
			break;

		case DTYPE_String:
			if(SNIFFER_USB_STRING_LANGUAGE == value_l) {
				g_response = sniffer_usbdesc_get_language();
				g_response_len = *g_response;//sizeof(LANGUAGE_ID_ENG);
			}
			else if(SNIFFER_USB_STRING_VENDOR == value_l) {
				g_response = sniffer_usbdesc_get_vendor();
				g_response_len = *g_response;//sizeof(STRING_VENDOR);
			}
			else if(SNIFFER_USB_STRING_PRODUCT == value_l) {
				g_response = sniffer_usbdesc_get_product();
				g_response_len = *g_response;//sizeof(STRING_PRODUCT);
			}
			else if(SNIFFER_USB_STRING_SERIAL == value_l) {
				g_response = sniffer_usbdesc_get_serial();
				g_response_len = *g_response;//sizeof(STRING_SERIAL);
			}
			else {
				sniffer_g_stall = 1;
			}

			break;

		default:
			sniffer_g_stall = 1;
			break;

	}

	printf("gresponse len %d, wlength %d\n",g_response_len,control_request.wLength);

	if(control_request.wLength < g_response_len) {
		g_response_len = control_request.wLength;
	}

	return;
}

void sniffer_usb_handle_in_class_intf_req() {
	u8 property = control_request.bRequest;

	switch(property) {
		case 0x00:
			usbhw_write_ctrl_ep_data(0x00);
			break;

		default:
			sniffer_g_stall = 1;
			break;
	}

}




#define		LOGICCHANNEL_TO_PHYSICAL(p)					(((p)-10)*5)
u8 remind_packetLen = 0;
u8 *sendAddr = 0;
u8 sniffer_rf_channel;
u8 sniffer_rf_capture_started = 0;
u8 sendRspData = 0;


_attribute_ram_code_ void sniffer_sendRemainPkt(void ){

	usbhw_reset_ep_ptr(SNIFFER_USB_EDP_BULK_IN);
	u8 len = remind_packetLen;
	u8 *txAddr = sendAddr;
	if(remind_packetLen > 64){
		len = 64;
		sendAddr += 64;
	}
	remind_packetLen -= len;
	usbhw_reset_ep_ptr(SNIFFER_USB_EDP_BULK_IN);
	for(u8 i=0;i<len;i++){
		usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, txAddr[i]);	// raw pkt
	}
	usbhw_data_ep_ack(SNIFFER_USB_EDP_BULK_IN);
}

_attribute_ram_code_ void sniffer_usb_handle_request(u8 data_request) {
	u8 bmRequestType = control_request.bmRequestType;
	u8 bRequest = control_request.bRequest;

	usbhw_reset_ctrl_ep_ptr();



	switch(bmRequestType) {
		case(REQDIR_DEVICETOHOST | REQTYPE_STANDARD | REQREC_DEVICE)://0x80
			if(REQ_GetDescriptor == bRequest) {
				if(SNIFFER_USB_IRQ_SETUP_REQ == data_request) {
					sniffer_usb_prepare_desc_data();
				}

				sniffer_usb_send_response();
			}

			break;

		case(REQDIR_DEVICETOHOST | REQTYPE_CLASS | REQREC_INTERFACE):
			sniffer_usb_handle_in_class_intf_req();
			break;

		case(REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_INTERFACE):
			break;

		case(REQDIR_DEVICETOHOST | REQTYPE_VENDOR | REQREC_DEVICE):	// 0xc0
			if(SNIFFER_USB_IRQ_SETUP_REQ == data_request) {
				if(0xc0 == bRequest) {					// Get board version
					usbhw_reset_ctrl_ep_ptr();
					usbhw_write_ctrl_ep_data(0x31);
					usbhw_write_ctrl_ep_data(0x25);
					usbhw_write_ctrl_ep_data(0x31);
					usbhw_write_ctrl_ep_data(0x05);
					usbhw_write_ctrl_ep_data(0x02);
					usbhw_write_ctrl_ep_data(0x00);
					usbhw_write_ctrl_ep_data(0x01);
					usbhw_write_ctrl_ep_data(0x00);

				}
				else if(0xc6 == bRequest) {			//
					usbhw_reset_ctrl_ep_ptr();
					sendRspData++;
					if(sendRspData<4)
						usbhw_write_ctrl_ep_data(0x01);
					else{
						usbhw_write_ctrl_ep_data(0x04);
						sendRspData=0;
					}
				}
			}

			break;

		case(REQDIR_HOSTTODEVICE | REQTYPE_VENDOR | REQREC_DEVICE):	// 0x40
			if(SNIFFER_USB_IRQ_SETUP_REQ == data_request) {
				if(0xc5 == bRequest) {
					u8 val = control_request.wIndex;
				}
				else if(0xc9 == bRequest) {
				}
				else if(0xd0 == bRequest) {					// start capture
					printf("start capture\n");
					gpio_write(LED1,0);
					gpio_write(LED2,1);
					sniffer_rf_capture_started = 1;
					RF_TrxStateSet(RF_MODE_RX,sniffer_rf_channel);
				}
				else if(0xd1 == bRequest) {					// stop capture
					gpio_write(LED1,1);
					gpio_write(LED2,0);
					sniffer_rf_capture_started = 0;
					RF_TrxStateSet(RF_MODE_TX,sniffer_rf_channel);//receive packet
				}
			}
			else {
				if(0xd2 == bRequest) {
					if(0 == control_request.wIndex) {			// set channel
						sniffer_rf_channel = LOGICCHANNEL_TO_PHYSICAL(usbhw_read_ctrl_ep_data());
						//RF_TrxStateSet(RF_MODE_RX,LOGICCHANNEL_TO_PHYSICAL(sniffer_rf_channel));
					}
					else if(6 == control_request.wIndex) {

					}
				}
			}

			break;

		default:
			sniffer_g_stall = 1;
			break;
	}



	return;
}

_attribute_ram_code_ void sniffer_usb_handle_ctl_ep_setup() {
	usbhw_reset_ctrl_ep_ptr();
	control_request.bmRequestType = usbhw_read_ctrl_ep_data();
	control_request.bRequest = usbhw_read_ctrl_ep_data();
	control_request.wValue = usbhw_read_ctrl_ep_u16();
	control_request.wIndex = usbhw_read_ctrl_ep_u16();
	control_request.wLength = usbhw_read_ctrl_ep_u16();
	sniffer_g_stall = 0;
	sniffer_usb_handle_request(SNIFFER_USB_IRQ_SETUP_REQ);

	if(sniffer_g_stall)
	{ usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_STALL); }
	else
	{ usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK); }
}

_attribute_ram_code_ void sniffer_usb_handle_ctl_ep_data(void) {
	usbhw_reset_ctrl_ep_ptr();
	sniffer_g_stall = 0;
	sniffer_usb_handle_request(SNIFFER_USB_IRQ_DATA_REQ);

	if(sniffer_g_stall)
	{ usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_STALL); }
	else
	{ usbhw_write_ctrl_ep_ctrl(FLD_EP_DAT_ACK); }
}

void sniffer_usb_handle_ctl_ep_status() {
	if(sniffer_g_stall)
	{ usbhw_write_ctrl_ep_ctrl(FLD_EP_STA_STALL); }
	else
	{ usbhw_write_ctrl_ep_ctrl(FLD_EP_STA_ACK); }
}



#define		SNIFFER_HDR_LEN			8


_attribute_ram_code_ void sniffer_usb_send_pkt(u8 *pkt, u8 len, u32 t) {

	if(!sniffer_rf_capture_started || remind_packetLen) { return; }

	usbhw_reset_ep_ptr(SNIFFER_USB_EDP_BULK_IN);

	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, 0);				// pkt  status
	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, len + 5);		// pkt len
	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, 0);			// pkt  len
	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, t);			// time stamp
	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, (t >> 8));	// time stamp
	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, (t >> 16));			// time stamp
	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, (t >> 24));	// time stamp
	usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, len);		// payload len
	//HDR length 8

	if(len + SNIFFER_HDR_LEN > 64){
		remind_packetLen = len + SNIFFER_HDR_LEN - 64;
		len = 64 - SNIFFER_HDR_LEN;
		sendAddr = pkt+len;
	}

	for(int i = 0; i < len; i++) {
		usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, pkt[i]);	// raw pkt
	}

	//const s8 rssi_min = 94;
	//usbhw_write_ep_data(SNIFFER_USB_EDP_BULK_IN, rssi_min + rssi);

	usbhw_data_ep_ack(SNIFFER_USB_EDP_BULK_IN);
}



_attribute_ram_code_ void sniffer_usb_handle_irq(void) {

	u32 irq = usbhw_get_ctrl_ep_irq();

	if(irq & FLD_CTRL_EP_IRQ_SETUP) {
		usbhw_clr_ctrl_ep_irq(FLD_CTRL_EP_IRQ_SETUP);
		sniffer_usb_handle_ctl_ep_setup();
	}

	if(irq & FLD_CTRL_EP_IRQ_DATA) {
		usbhw_clr_ctrl_ep_irq(FLD_CTRL_EP_IRQ_DATA);
		sniffer_usb_handle_ctl_ep_data();
	}

	if(irq & FLD_CTRL_EP_IRQ_STA) {
		usbhw_clr_ctrl_ep_irq(FLD_CTRL_EP_IRQ_STA);
		sniffer_usb_handle_ctl_ep_status();
	}

	if(reg_irq_src & FLD_IRQ_USB_RST_EN) {		//USB reset
		reg_irq_src3 = BIT(1);					//Clear USB reset flag
	}

	sniffer_g_stall = 0;

}

void sniffer_usb_init() {
	usbhw_enable_manual_interrupt(FLD_CTRL_EP_AUTO_STD | FLD_CTRL_EP_AUTO_DESC);
	reg_usb_ep8_fifo_mode = 0;
	sniffer_usbIDinit();
}


