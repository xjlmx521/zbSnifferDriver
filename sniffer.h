#pragma once

#pragma pack(1)

typedef struct {
	u8 len;
	u8  rssi;
	u16 rsvd;
	u32 timestamp;
	u8  channel;
	u8  accesscode[4];
	u8  payload[1];
} sniffer_fmt_t;
