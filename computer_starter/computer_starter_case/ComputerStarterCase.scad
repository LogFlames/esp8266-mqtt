wall = 2;
length = 59;
width = 32;
height = 14;
usb_hole = 8;
$fn=25;
top_scaling = 0.8;

difference() {
    cube([length + 2*wall, width + 2*wall, height + 2 * wall]);
    union() {        
        translate([wall, wall, wall]) {
            cube([length, width, height+100]);
        }
        translate([-1, wall + width/2 - usb_hole / 2, wall]) {
            intersection() {
                cube([10, usb_hole, 5.3]);
                translate([0,usb_hole/2,0]) {
                    rotate([90, 0, 90]) {
                        cylinder(10, 5.8, 5.8, center=true);
                    }
                }
            }
        }
    }
}

translate([0,-40,0]) {
    difference() {
        union() {
            cube([length + wall*2, width + wall*2, wall*top_scaling]);
            translate([wall,wall,wall*top_scaling]) {
                cube([length, width, wall*top_scaling]);
            }
        }
        union() {
            // button
            translate([9,width-wall*5,-1]) {
                cube([6.8,6.8,100]);
            }
            // stiftlist
            translate([22,width-wall-5,-1]) {
                cube([7,3,100]);
            }
            translate([35,width-wall-5,-1]) {
                cube([7,3,100]);
            }
            translate([48,width-wall-5,-1]) {
                cube([7,3,100]);
            }
        }
    }
}