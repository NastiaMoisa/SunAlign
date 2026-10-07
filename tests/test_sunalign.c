#include "sunalign.h"
#include "mpu6050.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include "fixtures/mpu6050_capture.h"
typedef struct {
    uint8_t registers[128], address[128], reg[128], data[128];
    bool read[128], pending;
    unsigned calls;
    bus_status_t start_status, result;
} fake_bus_t;
static bus_status_t fake_start(void *ctx,bool read,uint8_t addr,uint8_t reg,uint8_t *p,uint16_t len) {
    fake_bus_t *f=ctx;
    if(f->start_status!=BUS_OK) return f->start_status;
    assert(f->calls<128); unsigned i=f->calls++;
    f->address[i]=addr; f->reg[i]=reg; f->read[i]=read; f->data[i]=p[0];
    if(read) memcpy(p,f->registers+reg,len); else memcpy(f->registers+reg,p,len);
    f->pending=true; return BUS_OK;
}
static bus_status_t fake_poll(void *ctx) {
    fake_bus_t *f=ctx;
    if(f->pending) { f->pending=false; return BUS_PENDING; }
    return f->result;
}
static sensor_bus_t bus(fake_bus_t *f) { return (sensor_bus_t){f,fake_start,fake_poll}; }
static void ready(fake_bus_t *f) { memset(f,0,sizeof(*f)); f->registers[0x75]=0x68; f->registers[0x3f]=0x40; }
static void run(mpu6050_t *m,uint32_t start,uint32_t duration) {
    for(uint32_t n=0;n<duration;++n) mpu6050_tick(m,start+n);
}
static unsigned tests;
#define TEST(fn) do { fn(); tests++; puts("PASS " #fn); } while(0)
static void driver_sequence(void) {
    fake_bus_t f;ready(&f);mpu6050_t m;mpu6050_init(&m,bus(&f),0);run(&m,0,150);
    assert(m.online&&m.valid&&m.id==0x68&&m.sequence==1&&m.mg[2]==1000);
    const uint8_t regs[]={0x75,0x6b,0x1a,0x1c,0x3b};
    for(unsigned i=0;i<5;++i) { assert(f.address[i]==0x68);assert(f.reg[i]==regs[i]); }
    assert(f.read[0]&&!f.read[1]&&!f.read[2]&&!f.read[3]&&f.read[4]);
    assert(f.data[1]==0&&f.data[2]==3&&f.data[3]==0);
    unsigned calls=f.calls;run(&m,150,20);assert(f.calls==calls);
    run(&m,170,100);assert(m.sequence==2);
    mpu6050_request(&m,false);run(&m,270,150);assert(f.reg[calls+1]==0x75);
}
static void driver_errors(void) {
    for(int e=BUS_NACK;e<=BUS_ERROR;++e) {
        fake_bus_t f;ready(&f);f.result=(bus_status_t)e;mpu6050_t m;
        mpu6050_init(&m,bus(&f),0);run(&m,0,10);
        assert(!m.online&&!m.valid&&m.errors==1&&m.last_error==(bus_status_t)e);
        unsigned calls=f.calls;run(&m,10,990);assert(f.calls==calls);
        f.result=BUS_OK;run(&m,1000,160);assert(m.valid&&m.online);
    }
    fake_bus_t f;ready(&f);f.registers[0x75]=0;mpu6050_t m;
    mpu6050_init(&m,bus(&f),0);run(&m,0,10);assert(m.errors==1&&!m.valid);
    ready(&f);f.start_status=BUS_ERROR;mpu6050_init(&m,bus(&f),0);run(&m,0,1);assert(m.errors==1);
    ready(&f);f.start_status=BUS_BUSY;mpu6050_init(&m,bus(&f),0);run(&m,0,10);assert(!m.waiting&&m.errors==0);
    f.start_status=BUS_OK;run(&m,10,150);assert(m.valid);
}
static void driver_probe_and_wrap(void) {
    fake_bus_t f;ready(&f);mpu6050_t m;uint32_t t=UINT32_MAX-60;
    mpu6050_init(&m,bus(&f),t);run(&m,t,160);assert(m.valid);
    mpu6050_request(&m,true);unsigned n=f.calls;f.result=BUS_NACK;run(&m,t+160,10);
    assert(f.address[n]==0x69&&!m.valid&&m.last_error==BUS_NACK);
    f.result=BUS_OK;run(&m,t+170,1200);assert(m.valid);
    mpu6050_request(&m,true);run(&m,t+1370,10);assert(!m.valid&&m.last_error==BUS_ERROR);
}
static void decode_signed(void) {
    uint8_t b[]={0x40,0,0xc0,0,0x80,0};int32_t mg[3];
    mpu6050_decode(b,mg);assert(mg[0]==1000&&mg[1]==-1000&&mg[2]==-2000);
    b[4]=0x7f;b[5]=0xff;mpu6050_decode(b,mg);assert(mg[2]==1999);
}
static void captured_vector(void) {
    int32_t mg[3];mpu6050_decode(captured_accel,mg);
    assert(mg[0]==0&&mg[1]==0&&mg[2]==1000);
    fake_bus_t f;ready(&f);memcpy(f.registers+0x3b,captured_accel,6);
    mpu6050_t m;mpu6050_init(&m,bus(&f),0);run(&m,0,150);
    assert(m.valid&&m.mg[0]==mg[0]&&m.mg[1]==mg[1]&&m.mg[2]==mg[2]);
}
static panel_input_t input(uint32_t t) {
    panel_input_t in={.imu_ok=true,.adc_ok=true,.mg={0,0,1000},.left=1123,.right=1123,
        .imu_at=t,.adc_at=t,.sequence=1};return in;
}
static void sample(sunalign_t *a,panel_input_t *in,uint32_t t) {
    in->imu_at=t;in->adc_at=t;in->sequence++;sunalign_tick(a,in,t);
}
static void app_light(void) {
    sunalign_t a;sunalign_init(&a);panel_input_t in=input(0);sunalign_tick(&a,&in,0);
    assert(!strcmp(sunalign_horizontal(&a),"CENTER"));assert(!strcmp(sunalign_vertical(&a),"UNSET"));
    in.left=400;in.right=3500;for(unsigned n=1;n<20;++n) sample(&a,&in,n*100);
    assert(!strcmp(sunalign_horizontal(&a),"LEFT"));
    float previous=a.left;in.left=3500;in.right=400;sample(&a,&in,2000);
    assert(a.left>595&&a.left<previous); /* Filter does not jump to the new sample. */
    for(unsigned n=21;n<50;++n) sample(&a,&in,n*100);
    assert(!strcmp(sunalign_horizontal(&a),"RIGHT"));
    in.left=in.right=4095;for(unsigned n=50;n<90;++n) sample(&a,&in,n*100);
    assert(!strcmp(sunalign_horizontal(&a),"DARK"));
    in.left=in.right=3938;for(unsigned n=90;n<120;++n) sample(&a,&in,n*100);
    assert(!strcmp(sunalign_horizontal(&a),"DARK")); /* About 1 lux in the model. */
    in.left=in.right=1123;for(unsigned n=120;n<150;++n) sample(&a,&in,n*100);
    assert(!strcmp(sunalign_horizontal(&a),"CENTER"));
}
static void app_button_tilt_and_simultaneous(void) {
    sunalign_t a;sunalign_init(&a);panel_input_t in=input(100);
    sunalign_tick(&a,&in,100);in.pressed=true;sunalign_tick(&a,&in,101);
    in.pressed=false;sunalign_tick(&a,&in,110);in.pressed=true;sunalign_tick(&a,&in,120);
    sunalign_tick(&a,&in,149);assert(a.calibrations==0);sunalign_tick(&a,&in,150);assert(a.calibrations==1);
    sunalign_tick(&a,&in,151);assert(a.calibrations==1);
    in.mg[0]=500;in.mg[2]=866;in.left=400;in.right=3500;
    for(unsigned n=2;n<30;++n) sample(&a,&in,n*100);
    assert(!strcmp(sunalign_vertical(&a),"LOWER")&&!strcmp(sunalign_horizontal(&a),"LEFT"));
    in.mg[0]=-500;for(unsigned n=30;n<60;++n) sample(&a,&in,n*100);
    assert(!strcmp(sunalign_vertical(&a),"RAISE"));
    in.pressed=false;sample(&a,&in,6000);sample(&a,&in,6031);
    in.pressed=true;sample(&a,&in,6040);sample(&a,&in,6070);
    assert(a.calibrations==2&&!strcmp(sunalign_vertical(&a),"LEVEL"));
}
static void app_faults_and_wrap(void) {
    sunalign_t a;sunalign_init(&a);uint32_t t=UINT32_MAX-10;panel_input_t in=input(t);
    in.pressed=true;sunalign_tick(&a,&in,t);sunalign_tick(&a,&in,t+30);assert(a.calibrations==1);
    in.imu_ok=false;in.pressed=false;sample(&a,&in,t+40);assert(!a.healthy&&!a.filtered);
    assert(!strcmp(sunalign_horizontal(&a),"ERROR")&&!strcmp(sunalign_vertical(&a),"ERROR"));
    in.imu_ok=true;in.adc_ok=false;sample(&a,&in,t+80);assert(!a.healthy);
    in.adc_ok=true;in.mg[2]=0;sample(&a,&in,t+100);assert(!a.healthy);
    in.mg[2]=2000;sample(&a,&in,t+120);assert(!a.healthy);
    in.mg[2]=1000;sample(&a,&in,t+140);assert(a.healthy);
    sunalign_tick(&a,&in,t+441);assert(!a.healthy);
    sample(&a,&in,t+500);assert(a.healthy);
    in.adc_at=t+100;sunalign_tick(&a,&in,t+501);assert(!a.healthy);
}
int main(void) {
    TEST(driver_sequence);TEST(driver_errors);TEST(driver_probe_and_wrap);TEST(decode_signed);TEST(captured_vector);
    TEST(app_light);TEST(app_button_tilt_and_simultaneous);TEST(app_faults_and_wrap);
    printf("PASS %u test groups\n",tests);return 0;
}
