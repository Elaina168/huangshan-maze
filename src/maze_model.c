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
    m->broken=m->checkpoint_broken; m->result=MAZE_MODEL_RUNNING;
    return true;
}

/* 普通关卡保持旧路径；机关关卡逐次处理最早接触，危险同时间优先。 */
static void mechanism_motion(maze_model_t *m, uint32_t dt_ms)
{
    const maze_level_t *l=m->level;
    float remaining=(float)dt_ms;
    for (unsigned n=0;n<2*MAZE_MAP_MAX_MECHANISMS+8;n++)
    {
        float dx=m->vx*remaining/1000,dy=m->vy*remaining/1000;
        contact_t hit=first_wall(m,dx,dy);
        unsigned blocker=l->mechanism_count,trigger=l->mechanism_count;
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
        float limit=hit.found?hit.t:1,t=0;
        if (trigger_time<limit) limit=trigger_time;
        maze_model_result_t result=MAZE_MODEL_RUNNING;
        if (first_region(m,dx,dy,limit,&t,&result))
        {
            m->x+=dx*t;m->y+=dy*t;m->step_elapsed_ms+=remaining*t;
            m->vx=m->vy=0;m->result=result;return;
        }
        m->x+=dx*limit;m->y+=dy*limit;
        m->step_elapsed_ms+=remaining*limit;remaining*=1-limit;
        if (hit.found && hit.t==limit)
        {
            float normal=m->vx*hit.nx+m->vy*hit.ny;
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
            }
        }
        else return;
    }
    m->vx=m->vy=0;m->step_elapsed_ms=(float)dt_ms;
}

void maze_model_step(maze_model_t *m, float input_x, float input_y, uint32_t dt_ms)
{
    m->step_elapsed_ms=0;
    if (m->result!=MAZE_MODEL_RUNNING) return;
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
