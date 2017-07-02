include <dimensions.scad>;

x=Cx+(2*b);    //case size
y=Cy+(2*b);
z=Cz+Bz+b;

intersection() {
    difference() {
        translate([r,r,r]) hull() {
            sphere(r);
            translate([x-(2*r),0,0]) sphere(r);
            translate([0,y-(2*r),0]) sphere(r);
            translate([0,0,z-r]) sphere(r);
            translate([x-(2*r),y-(2*r),0]) sphere(r);
            translate([x-(2*r),0,z-r]) sphere(r);
            translate([0,y-(2*r),z-r]) sphere(r);
            translate([x-(2*r),y-(2*r),z-r]) sphere(r);
        }

        translate([b,b,b]) cube([Bx,By,Bz+0.1]);
        translate([b,b,b+Bz]) cube([Cx,Cy,Cz+0.1]);
        translate([-0.1,b+9.5,b+Bz]) cube([b+0.2,7.5,2]);
    
    }
    cube([x,y,z]);
}

