/* Minimal test double: fixed-address I2C device remains connected, NACKs until
 * 3 s simulator time, then supports only registers required by SunAlign.
 * It is not the Wokwi MPU6050 model and does not model sensor power physics. */
#include "wokwi-api.h"
static pin_t ready_marker;
static bool ready, first_byte;
static uint8_t reg, regs[128];
static void activate(void *unused) {(void)unused; ready=true; pin_write(ready_marker,HIGH);}
static bool connect(void *unused,uint32_t address,bool read) {
 (void)unused; if(!ready||address!=0x68)return false; if(!read)first_byte=true; return true;
}
static uint8_t read_byte(void *unused) {(void)unused;return regs[(reg++)&0x7f];}
static bool write_byte(void *unused,uint8_t b) {
 (void)unused;if(first_byte){reg=b;first_byte=false;}else{regs[reg&0x7f]=b;reg++;}return true;
}
static void disconnect(void *unused){(void)unused;}
void chip_init(void) {
 pin_t scl=pin_init("SCL",INPUT),sda=pin_init("SDA",INPUT); ready_marker=pin_init("READY",OUTPUT_LOW);
 regs[0x75]=0x68;regs[0x3f]=0x40;
 const i2c_config_t cfg={.address=0x68,.scl=scl,.sda=sda,.connect=connect,.read=read_byte,.write=write_byte,.disconnect=disconnect};
 i2c_init(&cfg);
 const timer_config_t timer={.callback=activate,.user_data=0}; timer_t id=timer_init(&timer);timer_start(id,3000000,false);
}
