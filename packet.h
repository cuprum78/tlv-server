#pragma once

#pragma pack(1)

struct tlv
{
	uint16_t type;
	uint32_t length;
	uint8_t*  value;
};

#pragma pack()

#define TYPE_HELLO   0xE110
#define TYPE_DATA    0xDA7A
#define TYPE_GOODBYE 0x0B1E
