// Persistence of Vision Ray Tracer Scene Description File
// File: ?.pov
// Vers: 3.1
// Desc: Basic Scene Example
// Date: mm/dd/yy
// Auth: ?[esp]
//

  #include "colors.inc"
  #include "skies.inc"
  #include "shapes.inc"    
  #include "finish.inc"
  #include "glass.inc"
  #include "metals.inc"
  #include "stones.inc"
  #include "woods.inc"
  #include "textures.inc"
  #include "lens.inc"
  #include "hands.inc"
  #include "vmath.inc"  
  #include "consts.inc"  

global_settings
{
  assumed_gamma 1.0
  
}


#declare usekubecam =true;
#declare myclock = clock;

#macro debugf (fl,a,b)
#debug concat(str (fl,a,b),"\n")
#end

#macro debugfn (fl,a,b)
#debug str (fl,a,b)
#end

#macro debugv (vc,a,b)
#debug concat("<",str (vc.x,a,b),",",str (vc.y,a,b),",",str (vc.z,a,b),">","\n")
#end

#declare k6=floor(clock*6+0.5);
#if (usekubecam) 
    #include "kubecam.inc"
    kubecam(<0,2,0>,k6)
    debugf(k6,5,60)
    #declare myclock=0.2;
#end

background { color Green }

light_source 
{
  vaxis_rotate(<500,0,-50>,z,135)
  color rgb 1
	shadowless
}                                     
/*light_source 
{
  vaxis_rotate(<500,0,50>,z,45)
  color rgb 0.4
  shadowless
}                                     
light_source 
{
  vaxis_rotate(<500,0,-50>,z,45)
  color rgb 0.4
  shadowless
}                                     
light_source 
{
  vaxis_rotate(<500,0,50>,z,135)
  color rgb 0.4
  shadowless
}                                     
light_source 
{
  vaxis_rotate(<500,0,-50>,z,135)
  color rgb 0.4
  shadowless
}*/

#declare nograss=1;
#include "mountains.inc"

