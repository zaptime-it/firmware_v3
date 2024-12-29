#pragma once

#include <Adafruit_GFX.h>
#include <Arduino.h>
#include "fonts.hpp"

const uint8_t Oswald_Medium20pt7bBitmaps_Gzip[] = {
  0x1f,0x8b,0x08,0x00,0xf7,0xa4,0x71,0x67,0x02,0xff,0xad,0x58,
  0xcf,0x8e,0xdb,0xb8,0x19,0xa7,0xa0,0xa2,0xbc,0x04,0xe1,0xb5,
  0x87,0x81,0x98,0x37,0xe8,0x1e,0xb3,0x80,0x22,0xf6,0x51,0xda,
  0x37,0x48,0xb1,0x87,0x3a,0x18,0x45,0xd4,0xc0,0x07,0xdf,0x56,
  0x2f,0xb0,0x88,0x5f,0x63,0x81,0xa6,0x19,0x1a,0x2e,0xe0,0x4b,
  0xb1,0x7e,0x81,0x36,0xa6,0xa1,0x83,0x6f,0x2b,0x1a,0x3e,0x98,
  0x86,0x39,0x64,0x7f,0x94,0xec,0x19,0x4f,0x32,0x49,0x16,0x45,
  0xc5,0xb1,0xf4,0xf1,0xff,0xc7,0xef,0xef,0x8f,0x43,0x42,0x7c,
  0x8e,0xff,0xde,0x7c,0xff,0xcb,0x0f,0x3f,0x6d,0xbe,0xff,0xd7,
  0x8b,0x24,0x56,0xbd,0x5c,0xbe,0xe9,0x5e,0x6d,0x7e,0xde,0x67,
  0xed,0x4f,0x3f,0xfc,0x48,0x58,0x43,0x27,0xe3,0xc5,0xac,0x55,
  0x5b,0x3d,0x32,0xf9,0xa8,0xc8,0xb3,0x8c,0x31,0xd1,0xcf,0x34,
  0x5b,0x6d,0xcd,0x68,0x94,0xe7,0x32,0xdc,0x86,0x55,0xe8,0xc2,
  0xae,0x9d,0xad,0xb5,0x31,0x23,0x9b,0x8f,0xb2,0x9c,0x65,0xb4,
  0xa1,0x8b,0xf1,0x7c,0xd6,0x6a,0x52,0x93,0x3f,0x91,0x17,0xc4,
  0x27,0x61,0x16,0xb6,0xc1,0xba,0xca,0xf1,0x8a,0xdd,0xd2,0xd5,
  0xb8,0x53,0xa9,0x49,0x1c,0x09,0x44,0x28,0x66,0xd2,0x33,0x81,
  0x96,0x8a,0x88,0x9c,0x79,0x1a,0xc6,0x61,0x76,0xd4,0x77,0xd6,
  0x49,0x2f,0x3c,0x0b,0xa9,0x27,0x7f,0x20,0xcf,0xc8,0xef,0x49,
  0x4a,0x58,0x9d,0x11,0x66,0x69,0x4d,0x43,0xa2,0xd2,0xa0,0x5f,
  0x13,0x6b,0x73,0x92,0x17,0x8c,0x30,0x3a,0xae,0x53,0xb0,0x41,
  0xc0,0x19,0xc9,0xb3,0x2b,0x34,0x34,0x24,0x05,0xff,0xc4,0x58,
  0x4d,0x8a,0xe2,0x25,0x61,0x21,0x23,0x49,0x58,0x4c,0xac,0x58,
  0xcf,0x43,0xaa,0xb6,0x41,0x93,0x7c,0x64,0xc9,0xf3,0x22,0x23,
  0x69,0x33,0x51,0x49,0xbb,0x36,0x64,0x64,0x4b,0x72,0x55,0x70,
  0xc2,0xd8,0x44,0xa5,0x73,0x34,0x68,0x3b,0x22,0x23,0x8c,0xc8,
  0x68,0xa8,0xe9,0x4d,0x50,0x89,0x12,0x86,0x18,0x6a,0xa8,0x4e,
  0x03,0x09,0x86,0x7b,0xda,0x6a,0x5b,0x72,0x36,0x5e,0x9b,0xb2,
  0x60,0x93,0x7a,0x6b,0x78,0x9e,0x74,0x44,0xe8,0xd4,0xe2,0x30,
  0xac,0x4e,0x6d,0x1e,0x26,0x1f,0xb6,0xfb,0x95,0xd8,0x6e,0x3e,
  0x14,0xfb,0xb9,0xb7,0x72,0x4a,0x77,0xba,0x2c,0xf9,0x26,0xf8,
  0x10,0x96,0xce,0x06,0x06,0x91,0xfe,0xfa,0xab,0xa6,0x8c,0x8b,
  0xaa,0x2c,0xed,0x97,0x9e,0xb2,0x2c,0x85,0xe0,0x54,0x97,0x82,
  0xcf,0x35,0x36,0xc3,0x9e,0x33,0x63,0xe3,0x77,0x02,0x46,0xef,
  0x89,0x81,0x62,0xcd,0x72,0x31,0x57,0xac,0x7e,0x99,0xa8,0x1f,
  0xff,0xf8,0x37,0x6c,0xd0,0x5a,0x56,0x7b,0x36,0x3a,0xb6,0x3f,
  0x3e,0xfb,0x73,0x5a,0x53,0xf2,0x9c,0x5c,0x91,0x97,0xe4,0x75,
  0xaf,0x50,0x7f,0x5f,0x25,0x3a,0x51,0x68,0xf8,0xe9,0xd7,0x3d,
  0xc1,0xc7,0xe1,0x57,0x93,0x51,0xaa,0xae,0x88,0x66,0xf8,0xd6,
  0x79,0xa2,0x9f,0x93,0xd7,0x14,0x5f,0x95,0x41,0x08,0xe4,0x65,
  0x8a,0xef,0xd0,0x75,0x95,0xe0,0x7b,0xee,0xa2,0x86,0x41,0x6b,
  0x4d,0x58,0x84,0xdd,0xfa,0x68,0xde,0x5a,0xe1,0x38,0xea,0xbd,
  0x56,0x83,0x0e,0x46,0x7e,0xa1,0x61,0x45,0x97,0x8b,0x0d,0x0c,
  0xc5,0x48,0xc3,0x4d,0x3a,0xdb,0xde,0x45,0xe6,0x2a,0x4e,0x67,
  0xa6,0xfc,0xca,0xcb,0x70,0x27,0xbc,0xf4,0x52,0x3a,0x61,0xf9,
  0xa9,0x08,0x22,0x48,0x01,0xf9,0x3b,0xe2,0x12,0x9b,0x9a,0x14,
  0xea,0xa2,0x8a,0x29,0x5e,0xf3,0x5a,0xa0,0x39,0x76,0x40,0xfa,
  0x7d,0x61,0x46,0x58,0x09,0x3b,0x73,0x95,0x15,0xf6,0xb4,0x02,
  0x89,0xf3,0xab,0xd4,0xa5,0x16,0xc5,0xc1,0xfa,0xaa,0xbe,0xe5,
  0x7e,0x75,0xb0,0x5b,0x05,0x8f,0x0d,0x1d,0x37,0xc4,0x13,0x51,
  0x33,0x4d,0x6d,0xe2,0x49,0xa8,0x85,0xce,0x2c,0xfd,0x90,0x74,
  0xf5,0x4e,0xe7,0x36,0x2b,0x68,0x33,0x6e,0xb5,0xb1,0x79,0xd4,
  0x0d,0x28,0x1b,0xce,0x0f,0xb8,0x83,0x6d,0x68,0x62,0xef,0x89,
  0x33,0x3f,0xc1,0x5b,0xf2,0x50,0x0e,0x26,0x58,0xe8,0xe1,0xc4,
  0x1d,0xe1,0xe7,0x72,0xe6,0xc4,0x15,0xd2,0xf7,0x07,0x37,0x83,
  0xcc,0x27,0x61,0xd1,0xee,0xf4,0x01,0x32,0x2f,0x7a,0x67,0x22,
  0xbc,0x66,0xff,0xa4,0xa1,0x0d,0x87,0x68,0x6d,0x15,0x8f,0xce,
  0xf7,0x48,0xe6,0xd3,0x5e,0xe6,0xeb,0x41,0xe6,0x67,0xe6,0x5c,
  0xaa,0x59,0x5d,0x24,0x96,0x2a,0x4e,0xca,0xc4,0x30,0x55,0x10,
  0x9b,0x6a,0x5e,0x83,0xa6,0x4a,0x44,0xfa,0xa1,0x3b,0x35,0xac,
  0x66,0x26,0xb2,0x22,0x7d,0x55,0xb9,0xe2,0x2c,0x43,0x27,0xca,
  0xa2,0xac,0xb0,0x87,0x1d,0xfa,0xca,0xe2,0x42,0x3f,0xa7,0x11,
  0xe0,0xdd,0x0b,0xc7,0xa0,0x9d,0xd3,0x98,0xca,0xdd,0xeb,0xe0,
  0x61,0x14,0x54,0x1b,0x64,0x10,0x2b,0x36,0xbd,0x38,0xbd,0xb8,
  0x3c,0x7b,0xb4,0x55,0x12,0x9f,0x34,0x46,0xa7,0xfb,0x1a,0x21,
  0xd1,0xbc,0xaf,0xfe,0xfa,0xee,0x35,0x79,0xf1,0x8c,0x8e,0xb7,
  0x90,0xc0,0x42,0x1b,0x9e,0x3a,0xd1,0x7a,0xae,0x2c,0x25,0x2f,
  0x86,0xf3,0x9a,0x61,0x70,0xef,0x0a,0xf5,0x33,0x82,0xf3,0x06,
  0x0a,0xcd,0x4a,0x62,0x04,0xdb,0xf9,0xd0,0x68,0xfb,0x1c,0x91,
  0x8c,0x69,0xb9,0x0d,0x7b,0x44,0xbb,0x00,0x41,0xe3,0x58,0x42,
  0x95,0xc4,0x26,0x91,0x77,0xd8,0x94,0x83,0x18,0x20,0x99,0x92,
  0x98,0x44,0xa7,0x8a,0xd6,0xc3,0x8a,0xbd,0xf1,0x51,0xcd,0x14,
  0x41,0x68,0x43,0xfc,0x51,0x84,0x07,0x8b,0x1f,0x4c,0x06,0xb6,
  0x28,0xa0,0x77,0x4e,0x08,0xcc,0x8d,0xe4,0x3c,0xd9,0x34,0x2c,
  0x39,0x2e,0x9a,0x71,0x68,0x17,0x37,0xc1,0xec,0x67,0xd6,0x5c,
  0xcf,0xec,0xe8,0x95,0xb6,0x79,0x66,0x6c,0xc1,0x46,0xb0,0x74,
  0x57,0xb0,0xb1,0xcf,0xe8,0xcc,0xb3,0x54,0x75,0xf4,0x46,0xef,
  0xc7,0xca,0x5c,0xc3,0xf5,0xdf,0x18,0xeb,0x72,0x2b,0x7d,0x9e,
  0xf3,0xce,0x65,0x74,0xef,0xd8,0x8d,0xb1,0xa9,0x22,0x24,0x89,
  0x27,0x2b,0xa3,0x1c,0xf0,0x63,0x08,0xc7,0x69,0x98,0x2a,0x9c,
  0x53,0x83,0x07,0x83,0x58,0xa8,0x48,0x6a,0x88,0x44,0x0f,0x42,
  0xb0,0x84,0x16,0x13,0xc4,0x61,0xcd,0x5c,0xb2,0xaf,0x0b,0x43,
  0x57,0xe4,0xa8,0x8b,0x92,0x2e,0x6b,0x6b,0x38,0x4f,0x5b,0xed,
  0x86,0x28,0x27,0x11,0xaf,0xda,0xe0,0x64,0x68,0x6e,0xb6,0x46,
  0x14,0x69,0xa7,0xde,0x5a,0x76,0x9b,0xec,0x95,0x08,0x5a,0xa0,
  0x6f,0x17,0x9c,0x93,0xb7,0x68,0x3e,0x9a,0xc2,0x21,0x74,0x0f,
  0x04,0xbf,0x0d,0xcb,0x30,0x87,0x2d,0x07,0xc1,0x3a,0x15,0x8c,
  0x70,0x08,0xe0,0x41,0xc1,0xe8,0x06,0x02,0xf6,0x17,0x42,0x1c,
  0xd3,0x06,0xb8,0x52,0x0c,0x0c,0xdb,0xe0,0x2a,0x3e,0xa5,0xa7,
  0x75,0x6e,0xd3,0xd0,0xcf,0xc2,0x49,0x18,0x72,0x07,0x79,0x20,
  0xb0,0x45,0xec,0xea,0xc7,0x74,0xea,0x60,0xab,0xc8,0x20,0x2c,
  0x5c,0x98,0x4b,0x7e,0xe8,0xb0,0xe9,0x69,0xf0,0x37,0x89,0xd9,
  0xd1,0xc0,0x58,0xc4,0xc0,0xcf,0xc9,0x2b,0x4e,0x2a,0xbe,0xff,
  0xa0,0x53,0x38,0x2c,0xbf,0xbc,0x6c,0xec,0x3f,0xe1,0xd1,0xa3,
  0xaa,0xd4,0xf0,0xda,0x51,0x2d,0x12,0xcb,0x50,0x09,0x2b,0x0f,
  0x33,0xba,0x1d,0x2a,0x8f,0x7a,0x50,0x39,0x9f,0xdd,0xe3,0xec,
  0xe9,0x23,0x9e,0x2f,0x8e,0x2c,0x03,0x47,0x68,0x85,0x80,0x2e,
  0x64,0x88,0x31,0x77,0x16,0x8e,0xb2,0xf4,0xdb,0xf0,0x5e,0x2c,
  0x3f,0x11,0xef,0x67,0xc4,0x03,0x77,0x5f,0x1e,0x13,0x89,0xf0,
  0xc4,0xf3,0x8d,0x78,0x3c,0x33,0x4e,0x60,0xde,0xd1,0x21,0xf8,
  0xb2,0xd5,0xcd,0xce,0x54,0x15,0x5b,0xce,0xb6,0xb6,0x12,0x6c,
  0xd2,0x1a,0x58,0xd1,0x74,0x8c,0xb6,0xd8,0x05,0x05,0x61,0xa3,
  0x3a,0x58,0x1e,0xd2,0xa3,0xc6,0xb0,0xc5,0xcc,0x62,0xee,0x64,
  0x1d,0x57,0xc0,0xd8,0x38,0x11,0x1a,0xe5,0xb7,0xe9,0x1e,0x23,
  0x3f,0x11,0xff,0x37,0x3f,0x27,0x6e,0x2b,0x72,0x67,0x31,0x9f,
  0xad,0x94,0x94,0xc9,0x9d,0xa3,0x7b,0xc3,0x57,0x2a,0xc8,0xf1,
  0x9d,0x67,0x7b,0x2b,0x56,0xba,0xbb,0x9e,0xfc,0xa3,0xcb,0xf6,
  0xef,0xaf,0x3f,0x6e,0xbb,0xeb,0xc5,0xab,0x4d,0xbe,0x7f,0xb7,
  0xfb,0x78,0xbd,0x3c,0x74,0x62,0xf3,0x73,0x68,0x5a,0x2f,0x1a,
  0xeb,0x45,0x6b,0xc3,0x64,0xee,0x39,0xb3,0xa5,0x58,0x9b,0x30,
  0x19,0xfb,0xe7,0xcc,0x32,0x47,0x7d,0x1a,0x6e,0x7a,0xdb,0xb5,
  0xc2,0xf3,0x10,0xa3,0x6f,0x1b,0xd5,0x12,0x3e,0xca,0x4e,0xec,
  0x57,0x87,0x0e,0x81,0x5b,0x40,0x57,0x0d,0x2c,0x7f,0x7d,0x8e,
  0xb8,0x51,0x55,0x35,0x64,0x0b,0x03,0x65,0x17,0xda,0xbe,0xfd,
  0x6d,0x86,0xf9,0x40,0xdc,0x45,0x4b,0x6f,0x4e,0x96,0x6e,0x04,
  0xc8,0x1d,0xd2,0x84,0xb8,0x4d,0x3f,0xb5,0xf4,0x5b,0xd6,0x85,
  0x63,0xcc,0x21,0xa0,0x1f,0xbb,0xce,0x67,0xc4,0xff,0x8b,0x9f,
  0x98,0xd4,0x54,0x12,0x73,0x1b,0x25,0x44,0x45,0x3f,0xbc,0xe7,
  0x2e,0x3c,0x38,0xff,0x30,0xf5,0xc4,0x1d,0x34,0xce,0x16,0xed,
  0xd6,0xba,0x42,0xb0,0xe5,0x7c,0x67,0x9c,0xe3,0x82,0x76,0xf0,
  0xc3,0xea,0xec,0xfc,0x9c,0xe9,0x21,0x53,0xfb,0xca,0x9e,0xb2,
  0x0c,0x2c,0x0c,0xe9,0x57,0x22,0x76,0x09,0x8d,0x0c,0x88,0x5c,
  0x9d,0x0c,0x0d,0x62,0x74,0x9f,0x85,0xbc,0x88,0x18,0x32,0x66,
  0x99,0x7b,0x03,0xd6,0x08,0xdb,0x69,0x04,0x04,0x35,0x07,0x8b,
  0x7d,0x14,0xff,0x56,0xf5,0xb7,0xcb,0xe1,0x53,0x62,0xff,0x48,
  0x32,0x96,0x02,0x0c,0xd7,0x6f,0x0d,0x2f,0xd9,0x2a,0xdd,0xa8,
  0x83,0x29,0x0a,0xd6,0x4c,0xda,0x01,0xbf,0xd1,0xc9,0x6c,0xab,
  0x4b,0xcb,0x0b,0x04,0xdd,0x9d,0x2e,0x0c,0x7b,0x1f,0x31,0xa6,
  0x8a,0x27,0x03,0xb8,0x90,0x35,0x8f,0xa0,0x72,0x50,0x96,0xcd,
  0xa8,0xa7,0x37,0xfb,0x1b,0xfd,0x46,0xdb,0xa2,0x14,0x6c,0xca,
  0xe6,0x9b,0xb9,0x39,0x18,0x17,0x2b,0x0d,0x9b,0xcf,0x17,0xd6,
  0x1c,0x90,0x73,0xe5,0x94,0x4d,0x37,0xf3,0xcd,0xf7,0xe6,0xf0,
  0x4b,0x99,0xed,0x3e,0xd2,0xf7,0xfb,0x9b,0xee,0x95,0x96,0x4b,
  0xcb,0xd7,0x9e,0x5a,0x79,0xe3,0xb9,0x16,0x13,0xc3,0xd6,0x80,
  0x3a,0x12,0xee,0xaa,0x44,0xf4,0x1d,0x38,0x10,0x52,0x19,0xce,
  0x9f,0xea,0x12,0xe6,0x64,0x2d,0xe2,0xff,0x1c,0x49,0x88,0xd3,
  0x8d,0x2a,0x2d,0x9b,0x42,0x77,0xdc,0xa6,0x31,0x89,0xd1,0x1e,
  0xe4,0x2a,0xb0,0x25,0x09,0x20,0x1b,0x98,0x54,0xdc,0xa5,0x5d,
  0xfd,0xd6,0xf6,0x29,0xc3,0x02,0xbc,0xcd,0xb5,0xeb,0xa7,0x68,
  0x04,0x95,0xae,0x3e,0x18,0xa8,0x75,0x40,0xcc,0x73,0xdd,0x2f,
  0x8c,0x83,0x5a,0xe4,0x9a,0xa0,0x99,0xed,0xd3,0x4f,0xbf,0x16,
  0x8b,0xc8,0x88,0x13,0xe0,0xa2,0x12,0xf0,0x28,0x89,0x06,0x45,
  0xd5,0x65,0x93,0xf4,0xe1,0x0e,0x76,0x03,0x38,0x12,0xb3,0x6f,
  0x89,0x3c,0xab,0x23,0x59,0x0c,0x24,0xeb,0x49,0x77,0x22,0x49,
  0x24,0xa1,0x43,0x76,0x11,0xc3,0x1e,0x63,0xe6,0x2f,0x11,0xc1,
  0x9b,0x14,0xf7,0x0d,0xbc,0xae,0x00,0x77,0xcf,0x2f,0x7d,0x7a,
  0xe5,0xe7,0x17,0x01,0x2e,0x3e,0xbd,0x5e,0x26,0xfd,0x36,0xda,
  0x8c,0xf2,0x0c,0x37,0x92,0xd9,0x57,0x09,0x19,0x3c,0xb0,0x82,
  0x06,0x40,0x30,0x11,0x41,0x65,0xa3,0x6c,0x54,0x94,0x79,0x0e,
  0xdc,0x54,0xd8,0x73,0xe4,0x05,0xec,0xcf,0x8b,0xec,0x39,0x83,
  0xf1,0x6b,0x44,0x97,0xcd,0x72,0xc5,0x3e,0xf0,0x12,0xe7,0x4c,
  0x23,0xee,0x69,0xf7,0xcb,0xe6,0x1d,0xaa,0xf6,0xad,0x39,0xb6,
  0x11,0x9b,0x2c,0x37,0x4d,0x53,0x7f,0x66,0xc0,0xdb,0xb6,0xdb,
  0xa0,0x73,0x75,0x02,0x2e,0x46,0x6a,0x58,0xe3,0x38,0xd0,0x87,
  0xea,0x3c,0x74,0xab,0xf0,0xc1,0x97,0x30,0x01,0xa4,0x96,0x5b,
  0x6f,0x11,0xb0,0x26,0x9e,0x5b,0x19,0x91,0x5e,0x0f,0xf8,0xfa,
  0x1a,0x1a,0x43,0x21,0x6d,0x98,0x79,0x12,0x41,0xf4,0x19,0xde,
  0xf4,0xa8,0x3a,0x8f,0x0a,0xc1,0xd3,0x78,0x78,0x9d,0x95,0x06,
  0x2b,0xcc,0x31,0x9c,0x5d,0x56,0x97,0x01,0xf7,0xc5,0xe9,0xed,
  0x79,0x93,0xe3,0x69,0x93,0xe1,0xa4,0x3d,0xaa,0x3c,0x6d,0xe2,
  0x87,0x4d,0xe8,0xda,0xc9,0x29,0xb2,0x05,0x00,0x4d,0x3f,0x06,
  0x17,0x27,0x90,0x05,0x55,0x36,0xda,0xe6,0x25,0x09,0xc4,0x75,
  0x63,0xff,0x12,0xde,0x07,0x5f,0x54,0x9f,0xeb,0xb2,0x77,0x48,
  0x84,0x5c,0x8d,0xbb,0x49,0x0d,0x07,0x94,0x08,0xd7,0xb3,0x60,
  0xb4,0x7b,0xcd,0xde,0x03,0xac,0xc7,0x00,0x84,0x38,0x3d,0x0e,
  0x4f,0x88,0x6e,0xb7,0xe9,0x31,0x5f,0x3c,0xd5,0xd3,0xa2,0xbb,
  0xaf,0x0e,0xb8,0xf3,0xb3,0x74,0xa9,0x78,0x13,0x03,0x1b,0x10,
  0xd8,0x40,0xb8,0x4a,0x3c,0x41,0x40,0x30,0x9d,0xbe,0x44,0xfe,
  0xa7,0x12,0x91,0x6e,0x85,0xe2,0xac,0x3d,0xd8,0xa3,0x39,0x02,
  0xd5,0x44,0xf4,0x1e,0x8c,0x3b,0xdd,0x0a,0x51,0x62,0x3c,0xec,
  0x23,0x62,0xf8,0xc2,0x83,0x61,0x47,0xe7,0x4f,0x30,0xa6,0xe2,
  0xf0,0xe4,0xff,0xe5,0xef,0xb0,0x3b,0x02,0xb8,0x40,0x73,0xec,
  0x2b,0x1a,0xee,0xab,0xf5,0xa7,0x56,0xf4,0xd9,0x27,0x88,0x41,
  0xc1,0x76,0xbb,0xdf,0x45,0x50,0x3d,0x79,0x52,0xa2,0xb1,0xba,
  0x0e,0x7b,0x58,0xae,0x2c,0xca,0x47,0x81,0x99,0x64,0x95,0xec,
  0xaf,0x87,0x5f,0xe3,0x66,0x13,0xa2,0x47,0xac,0xfa,0x2b,0xdf,
  0xbd,0xad,0x5a,0x29,0x3a,0xcc,0xf4,0xf2,0x09,0x23,0x02,0x89,
  0x68,0x86,0x45,0xb8,0x2f,0xaf,0xd7,0x61,0xcc,0x49,0x95,0xb8,
  0xd4,0x51,0x0b,0x07,0x8d,0x46,0xfe,0x9d,0x9c,0x85,0xc9,0xea,
  0xe4,0x00,0xe7,0x59,0xfd,0x15,0xda,0x3d,0xb1,0x96,0xa0,0x2e,
  0x34,0xc6,0x3f,0x71,0xfa,0x4f,0x64,0x21,0xc3,0xdd,0xdd,0x7e,
  0xa3,0x5c,0xef,0xc6,0xeb,0xfd,0xbc,0x9b,0x34,0x53,0x5e,0xe0,
  0xc4,0xf6,0xa0,0x77,0xb3,0x4d,0x2f,0x0c,0xb0,0x80,0x50,0xad,
  0x64,0xed,0xe3,0x51,0x88,0x19,0xc9,0xf9,0xdc,0x17,0x7c,0x6b,
  0xf6,0x4d,0xf3,0xbe,0x7c,0xd3,0xee,0x96,0xfc,0xdd,0x1b,0xb7,
  0xeb,0x36,0xd3,0xbf,0xe5,0xd7,0x7f,0x6f,0xff,0xe3,0xb3,0x8d,
  0x09,0x62,0xb2,0x75,0xa2,0xd1,0x4e,0xd2,0x16,0x57,0x99,0x1b,
  0x5b,0xb2,0xb1,0x2e,0x71,0x67,0xe3,0x65,0x56,0x14,0x45,0x2e,
  0x72,0x5e,0x72,0x1b,0x97,0x8e,0xff,0xcf,0x78,0x08,0x44,0x08,
  0x45,0xc8,0x23,0x79,0x35,0x04,0x22,0x8b,0x41,0x28,0xa0,0xf3,
  0x22,0xce,0x2a,0xf2,0xac,0xcc,0x62,0xd3,0x88,0x8d,0xd8,0x79,
  0x76,0x5f,0x52,0x3d,0x14,0xa1,0x50,0x10,0x36,0xe4,0x31,0x62,
  0x5d,0xc3,0x81,0x6e,0x81,0x72,0x55,0x91,0x3e,0x26,0xa1,0x1c,
  0x8c,0x50,0x74,0xb1,0x75,0xb8,0x15,0x8c,0x91,0xc8,0x1e,0x5e,
  0xbc,0x41,0xde,0x10,0x8f,0xdb,0xf0,0x72,0x7c,0x61,0x5c,0xf2,
  0x94,0x85,0x0b,0xd6,0xe2,0x37,0x07,0x72,0x3c,0xbf,0x00,0x45,
  0x31,0x9e,0x2d,0xb6,0x26,0x26,0x9b,0x8b,0x0e,0xe0,0xb9,0x66,
  0x4e,0xf8,0xef,0xc4,0x2f,0x21,0x48,0xff,0xc2,0x91,0xef,0xfe,
  0x0b,0x06,0x62,0xc2,0xe3,0x11,0x13,0x00,0x00,
};

const GFXglyph Oswald_Medium20pt7bGlyphs[] PROGMEM = {
  {     0,   1,   1,  10,    0,    0 },   // 0x20 ' '
  {     1,   5,  32,   9,    2,  -31 },   // 0x21 '!'
  {    21,  11,  11,  12,    1,  -31 },   // 0x22 '"'
  {    37,  17,  32,  20,    2,  -31 },   // 0x23 '#'
  {   105,  17,  39,  19,    1,  -34 },   // 0x24 '$'
  {   188,  34,  32,  37,    2,  -31 },   // 0x25 '%'
  {   324,  19,  32,  23,    2,  -31 },   // 0x26 '&'
  {   400,   4,  11,   6,    1,  -31 },   // 0x27 '''
  {   406,   8,  39,  13,    3,  -31 },   // 0x28 '('
  {   445,   9,  39,  12,    1,  -31 },   // 0x29 ')'
  {   489,  13,  14,  16,    2,  -31 },   // 0x2A '*'
  {   512,  15,  16,  17,    1,  -23 },   // 0x2B '+'
  {   542,   5,  10,   8,    2,   -4 },   // 0x2C ','
  {   549,  10,   3,  12,    1,  -12 },   // 0x2D '-'
  {   553,   5,   5,   9,    2,   -4 },   // 0x2E '.'
  {   557,  13,  32,  16,    1,  -31 },   // 0x2F '/'
  {   609,  17,  32,  21,    2,  -31 },   // 0x30 '0'
  {   677,  10,  32,  15,    1,  -31 },   // 0x31 '1'
  {   717,  16,  32,  19,    2,  -31 },   // 0x32 '2'
  {   781,  16,  32,  19,    2,  -31 },   // 0x33 '3'
  {   845,  18,  32,  20,    1,  -31 },   // 0x34 '4'
  {   917,  16,  32,  19,    2,  -31 },   // 0x35 '5'
  {   981,  17,  32,  20,    2,  -31 },   // 0x36 '6'
  {  1049,  14,  32,  16,    1,  -31 },   // 0x37 '7'
  {  1105,  16,  32,  20,    2,  -31 },   // 0x38 '8'
  {  1169,  16,  32,  20,    2,  -31 },   // 0x39 '9'
  {  1233,   6,  18,   9,    2,  -20 },   // 0x3A ':'
  {  1247,   6,  25,  10,    2,  -21 },   // 0x3B ';'
  {  1266,  11,  17,  15,    2,  -24 },   // 0x3C '<'
  {  1290,  13,  11,  17,    2,  -21 },   // 0x3D '='
  {  1308,  12,  17,  15,    2,  -24 },   // 0x3E '>'
  {  1334,  15,  32,  19,    2,  -31 },   // 0x3F '?'
  {  1394,  33,  37,  36,    2,  -31 },   // 0x40 '@'
  {  1547,  19,  32,  20,    1,  -31 },   // 0x41 'A'
  {  1623,  18,  32,  22,    2,  -31 },   // 0x42 'B'
  {  1695,  18,  32,  21,    2,  -31 },   // 0x43 'C'
  {  1767,  18,  32,  22,    2,  -31 },   // 0x44 'D'
  {  1839,  14,  32,  17,    2,  -31 },   // 0x45 'E'
  {  1895,  13,  32,  16,    2,  -31 },   // 0x46 'F'
  {  1947,  18,  32,  22,    2,  -31 },   // 0x47 'G'
  {  2019,  18,  32,  23,    2,  -31 },   // 0x48 'H'
  {  2091,   5,  32,  11,    3,  -31 },   // 0x49 'I'
  {  2111,  10,  33,  13,    0,  -31 },   // 0x4A 'J'
  {  2153,  19,  32,  21,    2,  -31 },   // 0x4B 'K'
  {  2229,  14,  32,  16,    2,  -31 },   // 0x4C 'L'
  {  2285,  22,  32,  27,    2,  -31 },   // 0x4D 'M'
  {  2373,  17,  32,  21,    2,  -31 },   // 0x4E 'N'
  {  2441,  18,  32,  22,    2,  -31 },   // 0x4F 'O'
  {  2513,  18,  32,  21,    2,  -31 },   // 0x50 'P'
  {  2585,  18,  38,  22,    2,  -31 },   // 0x51 'Q'
  {  2671,  18,  32,  22,    2,  -31 },   // 0x52 'R'
  {  2743,  16,  32,  19,    2,  -31 },   // 0x53 'S'
  {  2807,  15,  32,  17,    1,  -31 },   // 0x54 'T'
  {  2867,  18,  32,  22,    2,  -31 },   // 0x55 'U'
  {  2939,  18,  32,  20,    1,  -31 },   // 0x56 'V'
  {  3011,  26,  32,  28,    1,  -31 },   // 0x57 'W'
  {  3115,  19,  32,  20,    0,  -31 },   // 0x58 'X'
  {  3191,  19,  32,  19,    0,  -31 },   // 0x59 'Y'
  {  3267,  15,  32,  17,    1,  -31 },   // 0x5A 'Z'
  {  3327,   9,  39,  13,    2,  -31 },   // 0x5B '['
  {  3371,  13,  32,  16,    1,  -31 },   // 0x5C '\'
  {  3423,   9,  39,  12,    1,  -31 },   // 0x5D ']'
  {  3467,  16,  13,  18,    1,  -31 },   // 0x5E '^'
  {  3493,  14,   4,  14,    0,    3 },   // 0x5F '_'
  {  3500,   8,   8,  11,    2,  -31 },   // 0x60 '`'
  {  3508,  15,  23,  17,    1,  -22 },   // 0x61 'a'
  {  3552,  15,  32,  19,    2,  -31 },   // 0x62 'b'
  {  3612,  14,  23,  17,    2,  -22 },   // 0x63 'c'
  {  3653,  15,  32,  19,    2,  -31 },   // 0x64 'd'
  {  3713,  14,  23,  17,    2,  -22 },   // 0x65 'e'
  {  3754,  11,  31,  12,    1,  -30 },   // 0x66 'f'
  {  3797,  18,  31,  18,    1,  -23 },   // 0x67 'g'
  {  3867,  15,  32,  19,    2,  -31 },   // 0x68 'h'
  {  3927,   6,  31,  10,    2,  -30 },   // 0x69 'i'
  {  3951,   9,  37,  10,   -1,  -30 },   // 0x6A 'j'
  {  3993,  16,  32,  18,    2,  -31 },   // 0x6B 'k'
  {  4057,   6,  32,  10,    2,  -31 },   // 0x6C 'l'
  {  4081,  24,  23,  28,    2,  -22 },   // 0x6D 'm'
  {  4150,  15,  23,  19,    2,  -22 },   // 0x6E 'n'
  {  4194,  14,  23,  18,    2,  -22 },   // 0x6F 'o'
  {  4235,  15,  30,  19,    2,  -22 },   // 0x70 'p'
  {  4292,  15,  30,  19,    2,  -22 },   // 0x71 'q'
  {  4349,  11,  23,  14,    2,  -22 },   // 0x72 'r'
  {  4381,  14,  23,  16,    1,  -22 },   // 0x73 's'
  {  4422,  11,  29,  13,    1,  -28 },   // 0x74 't'
  {  4462,  14,  23,  19,    2,  -22 },   // 0x75 'u'
  {  4503,  15,  23,  16,    0,  -22 },   // 0x76 'v'
  {  4547,  21,  23,  23,    1,  -22 },   // 0x77 'w'
  {  4608,  16,  23,  16,    0,  -22 },   // 0x78 'x'
  {  4654,  16,  29,  16,    0,  -22 },   // 0x79 'y'
  {  4712,  13,  23,  15,    1,  -22 },   // 0x7A 'z'
  {  4750,  10,  40,  13,    2,  -31 },   // 0x7B '{'
  {  4800,   4,  38,  10,    3,  -31 },   // 0x7C '|'
  {  4819,  10,  40,  14,    2,  -31 },   // 0x7D '}'
  {  4869,  16,   6,  18,    1,  -18 } }; // 0x7E '~'

// const GFXfont Oswald_Medium20pt7b PROGMEM = {
//   (uint8_t  *)Oswald_Medium20pt7bBitmaps,
//   (GFXglyph *)Oswald_Medium20pt7bGlyphs,
//   0x20, 0x7E, 58 };

// // Approx. 5553 bytes


// Font properties
static constexpr FontData Oswald_Medium20pt7b_Properties = {
    Oswald_Medium20pt7bBitmaps_Gzip,
    Oswald_Medium20pt7bGlyphs,
    sizeof(Oswald_Medium20pt7bBitmaps_Gzip),
    5553,  // Original size
    0x20,   // First char
    0x7E,   // Last char
    58    // yAdvance
};
