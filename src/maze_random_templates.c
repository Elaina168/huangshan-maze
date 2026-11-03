#include "maze_random_templates.h"
#include <math.h>
maze_point_t maze_random_point(maze_point_t p,unsigned r){while(r--){float x=p.x;p.x=100-p.y;p.y=x;}return p;}
maze_rect_t maze_random_rect(maze_rect_t a,unsigned r){maze_point_t p=maze_random_point((maze_point_t){a.x,a.y},r),q=maze_random_point((maze_point_t){a.x+a.width,a.y+a.height},r);return(maze_rect_t){fminf(p.x,q.x),fminf(p.y,q.y),fabsf(q.x-p.x),fabsf(q.y-p.y)};}
void maze_random_dynamic(maze_generated_t*out,unsigned node,unsigned rotation,maze_template_kind_t kind){
 maze_dynamic_t d={0};d.kind=kind==MAZE_TEMPLATE_MAGNET?MAZE_DYNAMIC_MAGNET:kind==MAZE_TEMPLATE_LASER?MAZE_DYNAMIC_LASER:kind==MAZE_TEMPLATE_GATE?MAZE_DYNAMIC_GATE:MAZE_DYNAMIC_PATROL;
 if(kind==MAZE_TEMPLATE_MAGNET){d.area=(maze_rect_t){40,40,20,20};d.force=240;d.outer_radius=35;d.inner_radius=12;}
 else if(kind==MAZE_TEMPLATE_LASER){d.area=(maze_rect_t){48,4,4,92};d.off_ms=1800;d.warning_ms=600;d.on_ms=1200;}
 else if(kind==MAZE_TEMPLATE_GATE){d.area=(maze_rect_t){46,4,8,92};d.off_ms=2400;d.warning_ms=600;d.on_ms=1800;}
 else{d.area=(maze_rect_t){25,15,20,20};d.end=(maze_point_t){55,15};d.speed=35;d.dwell_ms=400;}
 maze_rect_t end=maze_random_rect((maze_rect_t){d.end.x,d.end.y,d.area.width,d.area.height},rotation);
 d.area=maze_random_rect(d.area,rotation);float x=(node%6)*100,y=(node/6)*100;d.area.x+=x;d.area.y+=y;
 if(kind==MAZE_TEMPLATE_PATROL)d.end=(maze_point_t){end.x+x,end.y+y};
 out->dynamic_index[node]=(uint8_t)out->definition.dynamic_count;out->types[node]=(uint8_t)kind;
 out->dynamics[out->definition.dynamic_count++]=d;
}
