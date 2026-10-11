#include "mpu6050.h"
#include <string.h>
static bool due(uint32_t now, uint32_t deadline) { return (int32_t)(now-deadline)>=0; }
void mpu6050_decode(const uint8_t b[6], int32_t mg[3]) {
    for (unsigned i=0; i<3; ++i) {
        uint16_t bits=((uint16_t)b[i*2]<<8)|b[i*2+1];
        int32_t signed_raw=bits>=32768u ? (int32_t)bits-65536 : bits;
        mg[i]=signed_raw*1000/16384;
    }
}
void mpu6050_init(mpu6050_t *m, sensor_bus_t bus, uint32_t now) {
    memset(m,0,sizeof(*m)); m->bus=bus; m->state=MPU_ID; m->deadline=now;
}
void mpu6050_request(mpu6050_t *m, bool bad_address) {
    if (bad_address) m->probe_requested=true; else m->refresh_requested=true;
}
static void fail(mpu6050_t *m, uint32_t now, bus_status_t error) {
    m->waiting=false; m->online=false; m->valid=false; m->errors++;
    m->last_error=error; m->state=MPU_RETRY; m->deadline=now+1000;
}
void mpu6050_tick(mpu6050_t *m, uint32_t now) {
    if(m->waiting) {
        bus_status_t result=m->bus.poll(m->bus.context);
        if(result==BUS_PENDING) return; /* Platform owns the bounded transfer timeout. */
        m->waiting=false;
        if(result!=BUS_OK) { fail(m,now,result); return; }
        switch(m->state) {
        case MPU_ID:
            m->id=m->bytes[0];
            if(m->id!=0x68) { fail(m,now,BUS_ERROR); return; }
            m->state=MPU_WAKE; break;
        case MPU_WAKE: m->state=MPU_FILTER; break;
        case MPU_FILTER: m->state=MPU_RANGE; break;
        case MPU_RANGE: m->state=MPU_SETTLE; m->deadline=now+100; break;
        case MPU_SAMPLE:
            mpu6050_decode(m->bytes,m->mg); m->valid=true; m->sequence++;
            m->last_sample=now; m->state=MPU_READY; m->deadline=now+100; break;
        case MPU_PROBE:
            /* An unexpected response at 0x69 is reported too; never use it as panel data. */
            fail(m,now,BUS_ERROR); break;
        default: fail(m,now,BUS_ERROR); break;
        }
        return;
    }
    if(m->state==MPU_RETRY) {
        if(!due(now,m->deadline)) return;
        m->state=MPU_ID;
    }
    if(m->state==MPU_SETTLE) {
        if(!due(now,m->deadline)) return;
        m->online=true; m->state=MPU_READY; m->deadline=now;
    }
    if(m->state==MPU_READY) {
        if(m->probe_requested) { m->probe_requested=false; m->state=MPU_PROBE; }
        else if(m->refresh_requested) { m->refresh_requested=false; m->state=MPU_ID; }
        else { if(!due(now,m->deadline)) return; m->state=MPU_SAMPLE; }
    }
    bool read=true; uint8_t addr=0x68, reg=0x75; uint16_t length=1;
    switch(m->state) {
    case MPU_WAKE: read=false; reg=0x6b; m->bytes[0]=0; break;
    case MPU_FILTER: read=false; reg=0x1a; m->bytes[0]=3; break;
    case MPU_RANGE: read=false; reg=0x1c; m->bytes[0]=0; break;
    case MPU_SAMPLE: reg=0x3b; length=6; break;
    case MPU_PROBE: addr=0x69; break;
    default: break;
    }
    bus_status_t result=m->bus.start(m->bus.context,read,addr,reg,m->bytes,length);
    if(result==BUS_OK) m->waiting=true;
    else if(result!=BUS_BUSY) fail(m,now,result);
}
