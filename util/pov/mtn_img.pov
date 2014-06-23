global_settings { assumed_gamma 2.2 hf_gray_16 }

#include "colors.inc"

// a wrinkle colored plane

camera { location <0,0,0> look_at <0,0,1> up y right x }

plane { z,10
 hollow on
 pigment{wrinkles
  color_map{
   [0 White*0.3]
   [1 White*0.9]
  }
    turbulence .9
    omega 0.55
    lambda 1.25
    octaves 5
 }
 finish {ambient 0.1 }
}

// Main spotlight creates crater mountain
light_source {0 color 1.0 spotlight point_at z*10
  radius 20 falloff 22
}

/*// Dim spotlight softens outer edges further
light_source {0 color .25  spotlight point_at z*10
  radius 2 falloff 15
}*/

// Negative spotlight cuts out crater insides
light_source {0 color -.4  spotlight point_at z*10
  radius 5 falloff 9.5
}

/*light_source {0 color 1.0  spotlight point_at z*10
  radius 16 falloff 17.5
}

light_source {0 color -1.0  spotlight point_at z*10
  radius 11.5 falloff 15
}*/

