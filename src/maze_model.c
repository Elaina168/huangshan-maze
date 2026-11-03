/* 圆球扫掠、沿墙滑动及球心区域判定；不读取时钟或外设。 */
#include "maze_model.h"
#include <math.h>
#include <stdbool.h>

typedef struct { float t, nx, ny; bool found; } contact_t;

/* 取运动线段首次进入圆的比例，包含区域起点与切点。 */
static bool circle_entry(float x, float y, float dx, float dy,
                         maze_circle_t c, float *t)
{
    double ox=x-c.x, oy=y-c.y;
    double cc=ox*ox+oy*oy-(double)c.radius*c.radius;
    if (cc<=0) { *t=0; return true; }
    double aa=(double)dx*dx+(double)dy*dy;
    if (aa==0) return false;
    double bb=ox*dx+oy*dy;
    if (bb>=0) return false;
    double disc=bb*bb-aa*cc;
    if (disc<0) return false;
    /* 等价于(-bb-sqrt(disc))/aa，避免接近圆边界时相减消去。 */
    double first=cc/(-bb+sqrt(disc));
    if (first>1) return false;
    *t=(float)first;
    return true;
}

static void contact(contact_t *hit, float t, float nx, float ny, float dx, float dy)
{
    /* 切向或离墙的运动不阻挡；容差只用于浮点接触，避免反复零时碰撞。 */
    if (t<0 || t>1 || dx*nx+dy*ny>=-0.000001f) return;
    if (!hit->found || t<hit->t) *hit=(contact_t){t,nx,ny,true};
}

static void wall_contact(contact_t *hit, float x, float y, float dx, float dy,
                         float radius, maze_rect_t w)
{
    float right=w.x+w.width, bottom=w.y+w.height;
    /* 先以整段圆球的保守包围盒排除远墙，避免动态子步对全图反复做double圆角求交。
     * 保留接触边界和浮点余量；通过筛选后的精确扫掠与响应完全沿用原算法。 */
    const float margin=0.001f;
    if(fmaxf(x,x+dx)+radius+margin<w.x || fminf(x,x+dx)-radius-margin>right ||
       fmaxf(y,y+dy)+radius+margin<w.y || fminf(y,y+dy)-radius-margin>bottom) return;
    if (dx!=0)
    {
        float nx=dx>0?-1:1;
        float edge=dx>0?w.x-radius:right+radius;
        float t=(edge-x)/dx, cy=y+t*dy;
        if (cy>=w.y && cy<=bottom) contact(hit,t,nx,0,dx,dy);
    }
    if (dy!=0)
    {
        float ny=dy>0?-1:1;
        float edge=dy>0?w.y-radius:bottom+radius;
        float t=(edge-y)/dy, cx=x+t*dx;
        if (cx>=w.x && cx<=right) contact(hit,t,0,ny,dx,dy);
    }
    for (unsigned i=0;i<4;i++)
    {
        float cx=(i&1)?right:w.x, cy=(i&2)?bottom:w.y, t;
        if (!circle_entry(x,y,dx,dy,(maze_circle_t){cx,cy,radius},&t)) continue;
        float ox=x+t*dx-cx, oy=y+t*dy-cy;
        /* 每个圆仅保留对应外角四分之一，直边与圆角共享切点。 */
        if (((i&1)?ox>=0:ox<=0) && ((i&2)?oy>=0:oy<=0))
            contact(hit,t,ox/radius,oy/radius,dx,dy);
    }
}

static contact_t first_wall(const maze_model_t *m, float dx, float dy)
{
    contact_t hit={0};
    const maze_level_t *l=m->level;
    float r=l->ball_radius;
    if (dx<0) contact(&hit,(r-m->x)/dx,1,0,dx,dy);
    if (dx>0) contact(&hit,(l->width-r-m->x)/dx,-1,0,dx,dy);
    if (dy<0) contact(&hit,(r-m->y)/dy,0,1,dx,dy);
    if (dy>0) contact(&hit,(l->height-r-m->y)/dy,0,-1,dx,dy);
    for (unsigned i=0;i<l->wall_count;i++)
        wall_contact(&hit,m->x,m->y,dx,dy,r,l->walls[i]);
    return hit;
}

static bool first_region(const maze_model_t *m, float dx, float dy, float limit,
                          float *time, maze_model_result_t *result)
{
    bool found=false;
    const maze_level_t *l=m->level;
    for (unsigned i=0;i<=l->trap_count;i++)
    {
        maze_circle_t c=i==l->trap_count?l->goal:l->traps[i];
        float t;
        if (circle_entry(m->x,m->y,dx,dy,c,&t) && t<=limit && (!found || t<*time))
        {
            *time=t;
            *result=i==l->trap_count?MAZE_MODEL_WON:MAZE_MODEL_LOST;
            found=true;
        }
    }
    return found;
}

/* 相位与位置只由有效游玩时间求值，暂停时无累积误差。 */
static maze_dynamic_view_t dynamic_at(const maze_dynamic_t *d, uint64_t us)
{
    maze_dynamic_view_t v={d->area,2,false};
    double ms=(double)us/1000+d->phase_ms;
    if(d->kind==MAZE_DYNAMIC_LASER || d->kind==MAZE_DYNAMIC_GATE)
    {
        double t=fmod(ms,(double)d->off_ms+d->warning_ms+d->on_ms);
        v.phase=t<d->off_ms?0:t<d->off_ms+d->warning_ms?1:2;
        v.blocked=d->kind==MAZE_DYNAMIC_GATE && v.phase==2;
    }
    else if(d->kind==MAZE_DYNAMIC_PATROL)
    {
        float dx=d->end.x-d->area.x,dy=d->end.y-d->area.y;
        double travel=hypot(dx,dy)*1000/d->speed;
        double t=fmod(ms,2*(travel+d->dwell_ms));
        double f=t<d->dwell_ms?0:t<d->dwell_ms+travel?(t-d->dwell_ms)/travel:
            t<2*d->dwell_ms+travel?1:1-(t-2*d->dwell_ms-travel)/travel;
        v.area.x+=(float)(dx*f);v.area.y+=(float)(dy*f);v.blocked=true;
    }
    return v;
}
void maze_model_dynamic_view(const maze_model_t *m,unsigned i,maze_dynamic_view_t *v)
{
    *v=dynamic_at(&m->level->dynamics[i],m->dynamic_us);
    if(m->level->dynamics[i].kind==MAZE_DYNAMIC_GATE && (m->gate_pending&(1u<<i)))
    { v->phase=1;v->blocked=false; }
}
static bool overlaps(float x,float y,float r,maze_rect_t w)
{
    float dx=x-fmaxf(w.x,fminf(x,w.x+w.width));
    float dy=y-fmaxf(w.y,fminf(y,w.y+w.height));
    return dx*dx+dy*dy<(r-0.001f)*(r-0.001f);
}
static bool touches(float x,float y,float r,maze_rect_t w)
{
    float dx=x-fmaxf(w.x,fminf(x,w.x+w.width));
    float dy=y-fmaxf(w.y,fminf(y,w.y+w.height));
    return dx*dx+dy*dy<=r*r+0.00001f;
}
/* 危险区包含切向接触，不能复用墙体忽略擦边的响应过滤。 */
static float hazard_entry(float x,float y,float dx,float dy,float r,maze_rect_t a)
{
    if(touches(x,y,r,a)) return 0;
    float best=2;
    for(unsigned axis=0;axis<2;axis++)
    {
        float v=axis?dy:dx;if(v==0) continue;
        float edge=axis?(v>0?a.y-r:a.y+a.height+r):(v>0?a.x-r:a.x+a.width+r);
        float t=(edge-(axis?y:x))/v,other=(axis?x:y)+t*(axis?dx:dy);
        float lo=axis?a.x:a.y,hi=lo+(axis?a.width:a.height);
        if(t>=0 && t<=1 && other>=lo && other<=hi && t<best) best=t;
    }
    for(unsigned k=0;k<4;k++)
    {
        float cx=a.x+((k&1)?a.width:0),cy=a.y+((k&2)?a.height:0),t;
        if(!circle_entry(x,y,dx,dy,(maze_circle_t){cx,cy,r},&t)) continue;
        float ox=x+t*dx-cx,oy=y+t*dy-cy;
        if(((k&1)?ox>=0:ox<=0)&&((k&2)?oy>=0:oy<=0)&&t<best) best=t;
    }
    return best;
}
static void magnetic_force(const maze_model_t *m,float *ax,float *ay)
{
    *ax=*ay=0;
    for(unsigned i=0;i<m->level->dynamic_count;i++)
    {
        const maze_dynamic_t *d=&m->level->dynamics[i];
        if(d->kind!=MAZE_DYNAMIC_MAGNET) continue;
        float dx=d->area.x+d->area.width/2-m->x,dy=d->area.y+d->area.height/2-m->y;
        float r=hypotf(dx,dy);
        if(r==0 || r>=d->outer_radius) continue;
        float force=d->force*(r<=d->inner_radius?r/d->inner_radius:
            (d->outer_radius-r)/(d->outer_radius-d->inner_radius));
        *ax+=dx/r*force;*ay+=dy/r*force;
    }
    float f=hypotf(*ax,*ay);
    if(f>MAZE_MAGNET_FORCE_MAX) {*ax*=MAZE_MAGNET_FORCE_MAX/f;*ay*=MAZE_MAGNET_FORCE_MAX/f;}
}

void maze_model_reset(maze_model_t *m, const maze_level_t *level)
{
    *m=(maze_model_t){.level=level,.x=level->start.x,.y=level->start.y};
}

bool maze_model_respawn(maze_model_t *m)
{
    if (m->result!=MAZE_MODEL_LOST || !m->checkpoint) return false;
    maze_rect_t a=m->level->mechanisms[m->checkpoint-1].area;
    m->x=a.x+a.width/2; m->y=a.y+a.height/2;
    m->vx=m->vy=m->step_elapsed_ms=0;
    m->keys=m->checkpoint_keys; m->switches=m->checkpoint_switches;
    m->broken=m->checkpoint_broken;
    m->dynamic_us=m->checkpoint_dynamic_us;m->gate_pending=m->checkpoint_gate_pending;
    m->result=MAZE_MODEL_RUNNING;
    return true;
}

/* 普通关卡保持旧路径；机关关卡逐次处理最早接触，危险同时间优先。 */
static void mechanism_motion(maze_model_t *m, float dt_ms)
{
    const maze_level_t *l=m->level;
    float remaining=(float)dt_ms;
    for (unsigned n=0;n<2*MAZE_MAP_MAX_MECHANISMS+8;n++)
    {
        float dx=m->vx*remaining/1000,dy=m->vy*remaining/1000;
        contact_t hit=first_wall(m,dx,dy);
        unsigned blocker=l->mechanism_count,trigger=l->mechanism_count;
        float obstacle_vx=0,obstacle_vy=0;
        float trigger_time=2;
        for (unsigned i=0;i<l->mechanism_count;i++)
        {
            maze_mechanism_t o=l->mechanisms[i];
            bool solid=o.kind==MAZE_MECH_KEY_DOOR?!(m->keys&(1u<<o.link)):
                o.kind==MAZE_MECH_SWITCH_DOOR?!(m->switches&(1u<<o.link)):
                o.kind==MAZE_MECH_BREAKABLE?!(m->broken&(1u<<i)):false;
            if (solid)
            {
                contact_t candidate={0};
                wall_contact(&candidate,m->x,m->y,dx,dy,l->ball_radius,o.area);
                if (candidate.found && (!hit.found || candidate.t<hit.t))
                { hit=candidate; blocker=i; }
            }
            bool active=o.kind==MAZE_MECH_KEY?!(m->keys&(1u<<o.link)):
                o.kind==MAZE_MECH_SWITCH?!(m->switches&(1u<<o.link)):
                o.kind==MAZE_MECH_CHECKPOINT?m->checkpoint!=i+1:false;
            if (active)
            {
                maze_circle_t c={o.area.x+o.area.width/2,o.area.y+o.area.height/2,
                    fminf(o.area.width,o.area.height)/2+l->ball_radius};
                float t;
                if (circle_entry(m->x,m->y,dx,dy,c,&t) && t<trigger_time)
                { trigger_time=t;trigger=i; }
            }
        }
        bool laser_hit=false;
        float laser_t=2;
        for(unsigned i=0;i<l->dynamic_count;i++)
        {
            const maze_dynamic_t *d=&l->dynamics[i];
            maze_dynamic_view_t v=dynamic_at(d,m->dynamic_us);
            if(d->kind==MAZE_DYNAMIC_GATE)
            {
                if(v.phase!=2) m->gate_pending&=~(1u<<i);
                else if(overlaps(m->x,m->y,l->ball_radius,v.area)) m->gate_pending|=1u<<i;
                else m->gate_pending&=~(1u<<i);
                if(m->gate_pending&(1u<<i)) v.blocked=false;
            }
            if(d->kind==MAZE_DYNAMIC_LASER)
            {
                float entry=v.phase==2?hazard_entry(m->x,m->y,dx,dy,l->ball_radius,v.area):2;
                if(entry<=1 && dynamic_at(d,m->dynamic_us+(uint64_t)llroundf(remaining*entry*1000)).phase!=2) entry=2;
                maze_dynamic_view_t end=dynamic_at(d,m->dynamic_us+(uint64_t)llroundf(remaining*1000));
                /* 相变终点危险先于同刻检查点/物品/终点提交。 */
                if(end.phase==2 && touches(m->x+dx,m->y+dy,l->ball_radius,end.area) && entry>1) entry=1;
                if(entry<laser_t) {laser_t=entry;laser_hit=entry<=1;}
            }
            if(!v.blocked) continue;
            maze_dynamic_view_t next=dynamic_at(d,m->dynamic_us+(uint64_t)llroundf(remaining*1000));
            float ox=next.area.x-v.area.x,oy=next.area.y-v.area.y;
            contact_t h={0};wall_contact(&h,m->x,m->y,dx-ox,dy-oy,l->ball_radius,v.area);
            if(h.found && (!hit.found || h.t<hit.t))
            {
                hit=h;blocker=l->mechanism_count;
                obstacle_vx=ox*1000/remaining;obstacle_vy=oy*1000/remaining;
            }
        }
        float limit=hit.found?hit.t:1,t=0;
        if (trigger_time<limit) limit=trigger_time;
        if(laser_hit && laser_t<=limit) limit=laser_t;
        maze_model_result_t result=MAZE_MODEL_RUNNING;
        if (first_region(m,dx,dy,limit,&t,&result) && !(laser_hit && laser_t<=t && result==MAZE_MODEL_WON))
        {
            m->x+=dx*t;m->y+=dy*t;m->step_elapsed_ms+=remaining*t;
            m->dynamic_us+=(uint64_t)llroundf(remaining*t*1000);
            m->loss_reason=MAZE_LOSS_TRAP;m->vx=m->vy=0;m->result=result;return;
        }
        m->x+=dx*limit;m->y+=dy*limit;
        m->step_elapsed_ms+=remaining*limit;
        m->dynamic_us+=(uint64_t)llroundf(remaining*limit*1000);remaining*=1-limit;
        if(laser_hit && laser_t==limit)
        {m->vx=m->vy=0;m->result=MAZE_MODEL_LOST;m->loss_reason=MAZE_LOSS_LASER;return;}
        if (hit.found && hit.t==limit)
        {
            float normal=(m->vx-obstacle_vx)*hit.nx+(m->vy-obstacle_vy)*hit.ny;
            /* 相对扫掠后保留亚像素间隙，避免float累计把下一步起点放进实体。 */
            if(l->dynamic_count){m->x+=hit.nx*0.0002f;m->y+=hit.ny*0.0002f;}
            if (blocker<l->mechanism_count && l->mechanisms[blocker].kind==MAZE_MECH_BREAKABLE &&
                -normal>=MAZE_BREAK_SPEED)
            {
                m->broken|=1u<<blocker;
                m->vx*=MAZE_BREAK_RETAIN;m->vy*=MAZE_BREAK_RETAIN;
            }
            else { m->vx-=normal*hit.nx;m->vy-=normal*hit.ny; }
        }
        else if (trigger<l->mechanism_count && trigger_time==limit)
        {
            maze_mechanism_t o=l->mechanisms[trigger];
            if (o.kind==MAZE_MECH_KEY) m->keys|=1u<<o.link;
            else if (o.kind==MAZE_MECH_SWITCH) m->switches|=1u<<o.link;
            else
            {
                m->checkpoint=trigger+1;
                m->checkpoint_keys=m->keys;m->checkpoint_switches=m->switches;
                m->checkpoint_broken=m->broken;
                m->checkpoint_dynamic_us=m->dynamic_us;m->checkpoint_gate_pending=m->gate_pending;
            }
        }
        else return;
        if(remaining<0.00001f) return;
    }
    m->vx=m->vy=0;m->step_elapsed_ms=(float)dt_ms;
}

void maze_model_step(maze_model_t *m, float input_x, float input_y, uint32_t dt_ms)
{
    m->step_elapsed_ms=0;
    if (m->result!=MAZE_MODEL_RUNNING) return;
    if(m->level->dynamic_count)
    {
        float total=0;
        uint64_t left=(uint64_t)dt_ms*1000;
        while(left && m->result==MAZE_MODEL_RUNNING)
        {
            uint64_t slice=left<1000?left:1000;
            for(unsigned i=0;i<m->level->dynamic_count;i++)
            {
                const maze_dynamic_t *d=&m->level->dynamics[i];
                if(d->kind==MAZE_DYNAMIC_MAGNET) continue;
                double boundaries[4],period;
                if(d->kind==MAZE_DYNAMIC_PATROL)
                {
                    double travel=hypot(d->end.x-d->area.x,d->end.y-d->area.y)*1000000/d->speed;
                    boundaries[0]=d->dwell_ms*1000.;boundaries[1]=boundaries[0]+travel;
                    boundaries[2]=boundaries[1]+d->dwell_ms*1000.;period=boundaries[3]=boundaries[2]+travel;
                }
                else {boundaries[0]=d->off_ms*1000.;boundaries[1]=(d->off_ms+d->warning_ms)*1000.;
                    period=boundaries[2]=boundaries[3]=(d->off_ms+d->warning_ms+d->on_ms)*1000.;}
                double phase=fmod((double)m->dynamic_us+d->phase_ms*1000.,period);
                for(unsigned j=0;j<4;j++) if(boundaries[j]>phase+0.01)
                {uint64_t gap=(uint64_t)ceil(boundaries[j]-phase);if(gap<slice) slice=gap;break;}
            }
            float ax,ay;magnetic_force(m,&ax,&ay);
            float magnitude=hypotf(input_x,input_y),scale=MAZE_ACCEL_MAX/fmaxf(MAZE_INPUT_FULL_RAW,magnitude);
            maze_surface_t surface=maze_level_surface_at(m->level,m->x,m->y);
            float drag=surface==MAZE_SURFACE_ICE?MAZE_DRAG_ICE:surface==MAZE_SURFACE_ROUGH?MAZE_DRAG_ROUGH:MAZE_DRAG_NORMAL;
            float dt=(float)slice/1000000;
            m->vx=(m->vx+(input_x*scale+ax)*dt)/(1+drag*dt);
            m->vy=(m->vy+(input_y*scale+ay)*dt)/(1+drag*dt);
            float speed=hypotf(m->vx,m->vy);
            if(speed>MAZE_SPEED_MAX){m->vx*=MAZE_SPEED_MAX/speed;m->vy*=MAZE_SPEED_MAX/speed;}
            else if(magnitude==0 && ax==0 && ay==0 && speed<MAZE_STOP_SPEED) m->vx=m->vy=0;
            uint64_t slice_start=m->dynamic_us;
            m->step_elapsed_ms=0;mechanism_motion(m,(float)slice/1000);
            if(m->result==MAZE_MODEL_RUNNING) m->dynamic_us=slice_start+slice;
            total+=m->step_elapsed_ms;left-=slice;
            /* 移动块把球夹在静态墙上时，不能留下重叠继续游玩。 */
            if(m->result==MAZE_MODEL_RUNNING)
                for(unsigned i=0;i<m->level->dynamic_count;i++)
                    if(m->level->dynamics[i].kind==MAZE_DYNAMIC_PATROL &&
                        overlaps(m->x,m->y,m->level->ball_radius,dynamic_at(&m->level->dynamics[i],m->dynamic_us).area))
                    {m->result=MAZE_MODEL_LOST;m->loss_reason=MAZE_LOSS_CRUSH;m->vx=m->vy=0;break;}
        }
        /* 末端恰好相变也立即生效，不能推迟到下一次物理调用。 */
        if(m->result!=MAZE_MODEL_LOST)
            for(unsigned i=0;i<m->level->dynamic_count;i++)
            {
                const maze_dynamic_t *d=&m->level->dynamics[i];
                maze_dynamic_view_t v=dynamic_at(d,m->dynamic_us);
                if(d->kind==MAZE_DYNAMIC_LASER && v.phase==2 && touches(m->x,m->y,m->level->ball_radius,v.area))
                {m->result=MAZE_MODEL_LOST;m->loss_reason=MAZE_LOSS_LASER;m->vx=m->vy=0;}
                if(d->kind==MAZE_DYNAMIC_GATE)
                {
                    if(v.phase!=2) m->gate_pending&=~(1u<<i);
                    else if(overlaps(m->x,m->y,m->level->ball_radius,v.area)) m->gate_pending|=1u<<i;
                }
            }
        m->step_elapsed_ms=total;return;
    }
    float magnitude=hypotf(input_x,input_y);
    float scale=MAZE_ACCEL_MAX/fmaxf(MAZE_INPUT_FULL_RAW,magnitude);
    float dt=(float)dt_ms/1000;
    maze_surface_t surface=maze_level_surface_at(m->level,m->x,m->y);
    float drag=surface==MAZE_SURFACE_ICE?MAZE_DRAG_ICE:
        surface==MAZE_SURFACE_ROUGH?MAZE_DRAG_ROUGH:MAZE_DRAG_NORMAL;
    /* 输入施加加速度，隐式阻尼保留惯性；切换地面不直接重设速度。 */
    m->vx=(m->vx+input_x*scale*dt)/(1+drag*dt);
    m->vy=(m->vy+input_y*scale*dt)/(1+drag*dt);
    float speed=hypotf(m->vx,m->vy);
    if (speed>MAZE_SPEED_MAX)
    {
        m->vx*=MAZE_SPEED_MAX/speed;
        m->vy*=MAZE_SPEED_MAX/speed;
    }
    else if (magnitude==0 && speed<MAZE_STOP_SPEED) m->vx=m->vy=0;
    if (m->level->mechanism_count) { mechanism_motion(m,dt_ms); return; }
    float remaining=(float)dt_ms;
    for (unsigned n=0;n<4;n++)
    {
        float dx=m->vx*remaining/1000, dy=m->vy*remaining/1000;
        contact_t hit=first_wall(m,dx,dy);
        float limit=hit.found?hit.t:1, t=0;
        maze_model_result_t result=MAZE_MODEL_RUNNING;
        if (first_region(m,dx,dy,limit,&t,&result))
        {
            m->x+=dx*t; m->y+=dy*t;
            m->step_elapsed_ms+=remaining*t;
            m->vx=0; m->vy=0; m->result=result;
            return;
        }
        m->x+=dx*limit; m->y+=dy*limit;
        m->step_elapsed_ms+=remaining*limit;
        remaining*=1-limit;
        if (!hit.found) return;
        float normal=m->vx*hit.nx+m->vy*hit.ny;
        m->vx-=normal*hit.nx;
        m->vy-=normal*hit.ny;
    }
    /* 接触预算耗尽即停止，剩余时间不产生未验证的位移。 */
    m->vx=0; m->vy=0;
    m->step_elapsed_ms=(float)dt_ms;
}
