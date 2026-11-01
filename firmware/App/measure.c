#include "measure.h"
void measure_add(measure_t *s,uint32_t x) {
    if(!s->n || x<s->min) s->min=x;
    if(x>s->max) s->max=x;
    s->sum+=x; s->n++;
}
bool cadence_take(cadence_t *c,uint32_t now) {
    if((int32_t)(now-c->next)<0) return false;
    uint32_t late=now-c->next, skip=late/c->period;
    c->missed+=skip;
    if(late>c->late_max) c->late_max=late;
    c->next+=(skip+1)*c->period;
    return true;
}
