include <dimensions.scad>

x=Cx+(2*b);    //case size
y=Cy+(2*b);
z=Cz+Bz+b;

intersection() {
    translate([r,r,r]) hull() {
        sphere(r);
        translate([x-(2*r),0,0]) sphere(r);
        translate([0,y-(2*r),0]) sphere(r);
        translate([0,0,b-r]) sphere(r);
        translate([x-(2*r),y-(2*r),0]) sphere(r);
        translate([x-(2*r),0,b-r]) sphere(r);
        translate([0,y-(2*r),b-r]) sphere(r);
        translate([x-(2*r),y-(2*r),b-r]) sphere(r);
    }
    cube([x,y,b]);
}