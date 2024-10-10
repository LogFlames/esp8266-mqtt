wall = 2;
length = 59;
width = 32;
height = 14;
usb_hole = 8;

difference() {
    cube([length + 2*wall, width + 2*wall, height + 2 * wall]);
    union() {        
        translate([wall, wall, wall]) {
            cube([length, width, height+100]);
        }
        translate([-1, wall + width/2 - usb_hole / 2, wall]) {
            cube([10, usb_hole, 5]);
        }
    }
}

translate([0,-40,0]) {
    difference() {
        union() {
            cube([length + wall*2, width + wall*2, wall]);
            translate([wall,wall,wall]) {
                cube([length, width, wall]);
            }
        }
        union() {
            translate([wall*5,width-wall*5,-1]) {
                cube([6.2,6.2,100]);
            }
            translate([25,width-wall-5,-1]) {
                cube([7,3,100]);
            }
        }
    }
}