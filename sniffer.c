#include "../../proj/tl_common.h"

/**********************************************************************
 * INCLUDES
 */
#include "sniffer.h"

/**********************************************************************
 * LOCAL TYPES
 */


#define RX_BUF_NUM    8
#define RF_GET_TIMESTAMP(p)    ( p[8] | (p[9]<<8) | (p[10]<<16) | (p[11]<<24) )

u8 push_data_flag = 0;
static u8 rf_rdPtr = 0;
static u8 rf_wrPtr = 0;

u8	rf_rxBuffer[RX_BUF_NUM][160];

/**********************************************************************
 * LOCAL FUNCTIONS
 */

u8 sniffer_dataPending(){
	return (rf_rdPtr != rf_wrPtr);
}
void sniffer_rfInit(){
	RF_PowerLevelSet(RF_POWER_7dBm);
	RF_RxBufferSet(rf_rxBuffer[0],160,0);
	RF_TrxStateSet(RF_MODE_TX,0);//receive packet
}

/*********************************************************************
 * @fn      sniffer_task
 *
 * @brief   Main task for sniffer
 *
 * @param   None
 *
 * @return  None
 */

_attribute_ram_code_ void sniffer_task(){
	if(rf_rdPtr != rf_wrPtr){

		u8 *rxBuffAddr = rf_rxBuffer[rf_rdPtr++ &(RX_BUF_NUM-1)];

		u8 *pdu = &rxBuffAddr[13];

		u8 len = rxBuffAddr[12];
		u32 tick = RF_GET_TIMESTAMP(rxBuffAddr);
		if((rxBuffAddr[rxBuffAddr[0]+3] & 0x01) == 0x0){//CRC ok
			pdu[len - 1] = 0xeb;
		}
		else{
			pdu[len - 1] = 0;
		}
		u8 lqi = rf_getLqi(rxBuffAddr[4]);
		if(lqi<36){
			lqi = 0;
		}else{
			lqi = (lqi -36)*3/8;
		}
		pdu[len - 2] = lqi;
		sniffer_usb_send_pkt(pdu, len, tick);

	}else{
		push_data_flag = 0;
	}

}


_attribute_ram_code_ void irq_sniffer(void ){
	u16  src_rf = reg_rf_irq_status;
	if(src_rf & FLD_RF_IRQ_RX){
		u8 *buff_rx0 = rf_rxBuffer[rf_wrPtr& (RX_BUF_NUM-1)];
		if(buff_rx0[0] < 160 && (buff_rx0[0] == buff_rx0[12]+13))
			//set rx buffer address
			REG_ADDR16(0x508) = (unsigned short)(rf_rxBuffer[++rf_wrPtr & (RX_BUF_NUM-1)]);
	}
	reg_rf_irq_status = 0xffff;//Clear RX TX irq src
}


