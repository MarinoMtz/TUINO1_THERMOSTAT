#ifndef _GMX_BG96_H_
#define _GMX_BG96_H_

#include <Arduino.h>

#define GMX_BG96_OK                      0 
#define GMX_BG96_KO                      1


#define BG96_DEFAULT_TIMEOUT          1000
#define BG96_BOOT_TIMEOUT             6000
#define BG96_COPS_TIMEOUT             60000


#define BG96_BAND_B1                0x1
#define BG96_BAND_B2                0x2
#define BG96_BAND_B3                0x4
#define BG96_BAND_B4                0x8
#define BG96_BAND_B5                0x10
#define BG96_BAND_B8                0x80
#define BG96_BAND_B12               0x800
#define BG96_BAND_B13               0x1000
#define BG96_BAND_B18               0x20000
#define BG96_BAND_B19               0x40000
#define BG96_BAND_B20               0x80000
#define BG96_BAND_B26               0x2000000
#define BG96_BAND_B28               8000000
#define BG96_BAND_B39               0x4000000000



uint8_t gmxBG96_init();
byte gmxBG96_getIMEI(String& IMEI);
//uint8_t gmxBG96_getIMEI(char *data, uint8_t max_size);
uint8_t gmxBG96_getIMSI(char *data, uint8_t max_size);


uint8_t gmxBG96_setGSM(char *apn);
uint8_t gmxBG96_setNBIoT(char *apn, uint32_t band);
uint8_t gmxBG96_setNBIoT_A(char *apn, uint32_t band);
uint8_t gmxBG96_IoT_Attach(char *apn, uint32_t band);

uint8_t gmxBG96_BellLabs_Sequence();

uint8_t gmxBG96_isNetworkAttached(int *attach_status);

uint8_t gmxBG96_TXData(char *data, int data_len);
uint8_t gmxBG96_SocketClose();
uint8_t gmxBG96_SocketOpen();


uint8_t gmxBG96_signalQuality(int *rssi, int *ber );

void gmxBG96_print_config();


#endif 
