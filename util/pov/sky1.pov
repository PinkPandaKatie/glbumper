#include "colors.inc"
#include "skies.inc"
#include "stones.inc"
#include "shapes.inc"
#include "metals.inc"
#include "functions.inc"
#include "rand.inc"

global_settings {
    assumed_gamma 1.0
    ambient_light 0.1
}


#declare usekubecam = true;
#declare myclock = clock;

#macro debugf (fl,a,b)
#debug concat(str (fl,a,b),"\n")
#end

#macro debugfn (fl, a, b)
#debug str (fl, a, b)
#end

#macro debugv (vc, a, b)
#debug concat("<",str (vc.x,a,b),",",str (vc.y,a,b),",",str (vc.z,a,b),">","\n")
#end

#declare k6=floor(clock * 6 + 0.5);

#if (usekubecam) 
    #include "kubecam.inc"
    kubecam(<0, 3, 0>, k6)
    debugf(k6, 5, 60)
    #declare myclock = 0.2;
#end

#declare my_cloud3 =
  pigment {
      bozo
      color_map {
          [0.0, 0.1   color red 0.85 green 0.85 blue 0.85
              color red 0.55 green 0.60 blue 0.65]
          [0.1, 0.5   color red 0.55 green 0.60 blue 0.65
              color rgb <0.184, 0.184, 0.209>*0.5 ]
          [0.5, 1.001 color rgb <0.184, 0.184, 0.209>*0.5
              color rgb <0.1, 0.1, 0.1>*0.5]
      }
      turbulence 0.65
      octaves 6
      omega 0.707
      lambda 2
      scale <6, 4, 6>
  }


sky_sphere {
    pigment {
        function { abs(y) }
        pigment_map {
            [0.01 rgb <0.847, 0.3, 0.1> ] // horizon
            [0.25 P_Cloud2 scale 0.25 rotate z*5]
            [0.60 my_cloud3 scale <0.25, 0.15, 0.25> rotate z*20]
        }
    }
}
//sky_sphere { S_Cloud1 }

#declare window_width = 0.205;
#declare window_height = 0.31;
#declare _window = box {
    <0, 0, -0.0001>, <window_width, window_height, 0>
}

#macro window() 
    
object {
    _window
#if (rand(RdmC) > 0.71)
    texture {
        pigment {
            color Yellow
        }
        finish {
            ambient 10.0
        }
    }
#else
    texture {
        pigment {
            color Black
        }
    }
#end
}
#end

#macro window_face(nx, ny, spx, spy)

#local cx = 0;
#while (cx < nx)
    #local cy = 0;
    #while (cy < ny)
        object {
            window()
            translate <cx * spx, cy * spy, 0>
        }
        #local cy = cy + 1;
    #end
    #local cx = cx + 1;
#end
#end



#macro building(xp, yp, xs, ys, h, tex)

#local minp = <xp - xs, 0, yp - ys>;
#local maxp = <xp + xs, h+2, yp + ys>;

#local wsx = window_width + 0.06;
#local wsy = window_height + 0.09;

#local nwx = floor((xs * 2) / wsx) - 1;
#local nwz = floor((ys * 2) / wsx) - 1;
#local nwy = floor((h+2) / wsy);

#local twinxwidth = (nwx - 1) * wsx + window_width;
#local twinzwidth = (nwz - 1) * wsx + window_width;
//#local twinheight = (nwy - 1) * wsy + window_height;


box {
    minp, maxp
    texture { tex
        //pigment { color Red }
        //normal { function { sin(y*10)*0.5+0.5 } }
    }
}
union {
    window_face(nwx, nwy, wsx, wsy)
    translate <xp - (twinxwidth / 2), wsy - window_height, yp - ys >
}

union {
    window_face(nwz, nwy, wsx, wsy)
    rotate 270*y
    translate <xp + xs, wsy - window_height, yp - (twinzwidth / 2) >
}

union {
    window_face(nwx, nwy, wsx, wsy)
    rotate 180*y
    translate <xp + (twinxwidth / 2), wsy - window_height, yp + ys >
}

union {
    window_face(nwz, nwy, wsx, wsy)
    rotate 90*y
    translate <xp - xs, wsy - window_height, yp + (twinzwidth / 2) >
}

#end

// Front, first row

union {

    building(-12.8, 5, 1.4, 1.2, 5.9, 
        texture { 
            T_Stone13
        } 
    )

    building(-9, 5, 2.0, 1.2, 4.7, 
        texture { 
            T_Stone42
        } 
    )

    building(-5, 5, 1.4, 1.0, 5.4, 
        texture { 
            T_Stone43
        } 
    )

    building(-2.4, 5, 1.0, 0.8, 4.2, 
        texture { 
            T_Gold_3A
            normal { function { f_granite(x * 0.5, y * 0.5, z) * 0.01 } }
        } 
    )

    building(0, 5, 0.8, 0.8, 4.8, 
        texture { 
            T_Chrome_5C 
            normal { function { f_granite(x * 0.5, y * 0.5, z) * 0.01 } }
        } 
    )

    building(2, 5, 0.8, 0.8, 6.1, 
        texture { 
            T_Chrome_5A 
        } 
    )

    building(4.1, 5, 0.8, 0.8, 5.8, 
        texture { 
            T_Stone37
        } 
    )

    building(7.2, 5.4, 1.6, 1.0, 9, 
        texture { 
            T_Stone31
        } 
    )

    building(10.5, 5.2, 1.1, 1.0, 7.0, 
        texture { 
            T_Stone39
        } 
    )

    building(13.8, 5.3, 1.1, 1.0, 7.7, 
        texture { 
            T_Stone19
        } 
    )



    building(-2.4, 7, 1.1, 0.8, 8.1, 
        texture { 
            T_Stone8
            scale 2
        } 
    )




    building(0, 7, 0.8, 0.8, 7.0, 
        texture { 
            T_Silver_5C 
            normal { function { f_granite(x * 0.5, y * 0.5, z) * 0.04 } }
        } 
    )

    translate <0, 0, 11>
}



sphere { <0,0,0>, 100000 hollow pigment { color rgbt <0,0,0,0.4> } }


plane { -y, -50
    texture {
        
        T_Cloud3
        finish {ambient 0.1}
        scale 60
        
    }
}
plane { y, -10
    texture {
        T_Cloud2
        finish {ambient 0.2}
        scale <90,1,600>
        translate x*300
        rotate -30*y
    }
}

#declare lightpos=<0,20,-80>;

light_source 
{
    lightpos
    color rgb 0.5 //<1,0.4,0.3>
}                                     

light_source 
{
    <0,200,0>
    color rgb 0.2
    shadowless
}                                     
//#include "mfog.inc"
#declare gscale = 3.0;
#declare gfunc = function { f_granite(x / gscale, y / gscale, z / gscale) }
#declare Tgrass = texture {
    pigment { color Green }
    normal { function { gfunc(x, y, z) }}
}
#declare Twater = texture {
    pigment { color rgb <0.5, 0.6, 1.0> }
    normal { granite scale 5.0 }
}

#declare rad = 500.0;
sphere {
    <0, -rad, 0>, rad
    
    texture {
        function {max(0.0, min(1.0, (-3 - z)/3.0 + gfunc(x, y, z)*2))}
        texture_map {
            [0.0  Tgrass]
            [1.0   Twater]
        }
    }
    
}

/*
sphere { 
  <0,0,0>,1
  texture{
  pigment { 
  gradient x
  color_map {
  [0.0 color Gray25]
  [0.1 color White]
  [0.9 color White]
  [1.0 color Gray25]
  }
  turbulence .7
  omega 0.8
  lambda 1.8
  octaves 4 
  }

  finish {ambient 10.0}
  }
  no_shadow
  scale 5
  translate moonpos
  }
*/
#declare nograss=0;
//#include "mountains.inc"

