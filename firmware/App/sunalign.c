#include "sunalign.h"
#include <math.h>
#include <string.h>
void sunalign_init(sunalign_t *a) { memset(a,0,sizeof(*a)); }
static int hysteresis(int old,float value,float enter,float leave) {
    if(value>enter) return 1;
    if(value< -enter) return -1;
    if((old>0 && value>leave)||(old<0 && value< -leave)) return old;
    return 0;
}
void sunalign_tick(sunalign_t *a,const panel_input_t *in,uint32_t now) {
    bool sane=in->imu_ok && in->adc_ok && now-in->imu_at<=300 && now-in->adc_at<=300;
    float x=in->mg[0]/1000.0f,y=in->mg[1]/1000.0f,z=in->mg[2]/1000.0f;
    float g2=x*x+y*y+z*z;
    a->healthy=sane && g2>=0.64f && g2<=1.44f;
    if(a->healthy && (!a->filtered || a->last_sequence!=in->sequence)) {
        float pitch=atan2f(x,sqrtf(y*y+z*z))*57.2957795f;
        float l=4095-in->left,r=4095-in->right;
        if(!a->filtered) { a->left=l; a->right=r; a->pitch=pitch; a->filtered=true; }
        else { a->left+=(l-a->left)*0.25f; a->right+=(r-a->right)*0.25f;
               a->pitch+=(pitch-a->pitch)*0.25f; }
        a->last_sequence=in->sequence;
    }
    if(in->pressed!=a->candidate) { a->candidate=in->pressed; a->button_at=now; }
    if(a->debounced!=a->candidate && now-a->button_at>=30) {
        a->debounced=a->candidate;
        if(a->debounced && a->healthy) {
            a->zero=a->pitch; a->zero_set=true; a->calibrations++; a->vertical=0;
        }
    }
    if(!a->healthy) { a->horizontal=0; a->vertical=0; a->filtered=false; return; }
    float sum=a->left+a->right;
    a->dark=sum<500;
    a->horizontal=a->dark ? 0 : hysteresis(a->horizontal,(a->left-a->right)*1000/sum,150,80);
    a->vertical=a->zero_set ? hysteresis(a->vertical,a->pitch-a->zero,5,3) : 0;
}
const char *sunalign_horizontal(const sunalign_t *a) {
    if(!a->healthy) return "ERROR";
    if(a->dark) return "DARK";
    return a->horizontal>0 ? "LEFT" : a->horizontal<0 ? "RIGHT" : "CENTER";
}
const char *sunalign_vertical(const sunalign_t *a) {
    if(!a->healthy) return "ERROR";
    if(!a->zero_set) return "UNSET";
    return a->vertical>0 ? "LOWER" : a->vertical<0 ? "RAISE" : "LEVEL";
}
