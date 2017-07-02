Bx=25;     //battery size
By=15;
Bz=3;

Cx=25;     //circuit size
Cy=15;
Cz=3;

b=3;    //border width
r=2;    //radius of curvature

x=Bx+(2*b);    //case size
y=By+(2*b);
z=Bz+Cz+b;

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

        translate([b,b,b]) cube([Bx,By,Bz+Cz+2]);
    
    }
    cube([x,y,z]);
}