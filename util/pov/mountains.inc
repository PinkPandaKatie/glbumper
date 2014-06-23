


#declare greengrass = pigment {
    wrinkles
    color_map {
        [ 0.0 color <0,0,0> ]
        [ 0.2 color <0,0,0> ]
        [ 1.0 color <0.1, 0.7, 0.1>*0.6 ]
    }
    scale 0.003
    translate 200
}

#if (0)
    #declare redgrass = pigment {
        wrinkles
        color_map {
            [ 0.0 color <0,0,0> ]
            [ 0.2 color <0,0,0> ]
            [ 1.0 color <0.5, 0.2, 0.1>*0.8 ]
        }
        scale 0.001
        translate 400
    }
#else
    #local col=<1.0, 0.4, 0.2>;
    #declare redgrass = pigment {
        wrinkles
        color_map {
            [ 0.0 color col*0.1 ]
            [ 0.2 color col*0.1 ]
            [ 1.0 color col*0.3 ]
        }
        scale 0.005
        translate -40
    }

#end
#declare rock = pigment {
    wrinkles
    color_map {
        [ 0.0 color White*0.1 ]
        [ 0.2 color White*0.1 ]
        [ 1.0 color White*0.25 ]
    }
    scale 0.005
    translate -30
}
#if (nograss=1)
    //#declare greengrass=rock
#end

#macro rockwith (tex,thres) 
#declare rscale=60;
#declare inter=0.2;
pigment {
    wrinkles
    pigment_map {
        [ 0.0 rock scale rscale]
        [ thres rock scale rscale]
        [ thres+inter tex scale rscale]
        [ 1.0 tex scale rscale]
    }
    scale 1/rscale
    translate 4000
}

#end

#macro rocktex(rthgreen, rthred,  rthrg)
#local rockwithred = rockwith(redgrass,rthred)
#local rockwithgreen = rockwith(greengrass,rthgreen)

#local rpscale=10;
//pigment {
    wrinkles
    pigment_map {
        [ 0.0 rockwithred scale rpscale]
        [ rthrg rockwithred scale rpscale]
        [ rthrg+0.3 rockwithgreen scale rpscale]
        [ 1.0 rockwithgreen scale rpscale]
    }
    scale 1/rpscale
    translate 2000

    //}
#end

/*#declare grass = pigment {
  wrinkles
  pigment_map {
  [ 0.0 greengrass scale gscale]
  [ 0.5 greengrass scale gscale]
  [ 0.9 redgrass scale gscale]
  [ 1.0 redgrass scale gscale]
  }
  scale 1/gscale
  translate 1000
  }*/



/*#declare valscale=0.25;

#declare valley = pigment {
  wrinkles
  pigment_map {
  [ 0.0 grass scale 1/valscale ]
  [ 0.55 grass scale 1/valscale ]
  [ 0.65 rock  scale 1/valscale ]
  [ 1.0 rock  scale 1/valscale ]
  }
  scale valscale
  
  }*/
#declare peak = pigment {
    granite
    color_map {
        [ 0.0 color <0,0,0> ]
        [ 1.0 color White*0.75 ]
    }
    scale 0.01
}


/*
gradient y
color_map {
  [0.0 color <0.6, 0.2, 0.0> ]
  [0.7  color <0.2, 0.3, 0.0> ]
  [1.0 color <0.2, 0.3, 0.1> ]
  }
//translate -0.5*y
//scale <1,-1,1>
//translate 0.5*y
//turbulence .7
//omega 0.35
//lambda 1.25
//octaves 5
*/


#declare mapscale=0.5;
#declare mapofs=0.25;
height_field {
    png "mtn_img.png"       
    smooth
    pigment { 
        gradient y
        pigment_map {
            [0.0 rock ]
            [0.0*mapscale+mapofs rocktex(0.25,0.35,0.0)  ]
            [0.2*mapscale+mapofs rocktex(0.30,0.35,0.1)  ]
            [0.4*mapscale+mapofs rocktex(0.35,0.25,0.2)  ]
            [0.6*mapscale+mapofs rocktex(0.40,0.25,0.3)  ]
            [0.8*mapscale+mapofs rocktex(0.45,0.25,0.4)  ]
            [1.0*mapscale+mapofs rock]
            [1.0 rock]
        }
        scale <0.5,1,0.5>
        
        /*	
  pigment_map {
  [0.0 valley]
  [0.4 valley]
  [0.5 peak]
  [1.0 peak]
  }
  scale 0.7*/
    }
    //normal { granite }
    finish { ambient 0 }
    translate <-0.5,0.7,-0.5>
    scale <20,4,20>  
    translate <0,-2.5,0>
}
