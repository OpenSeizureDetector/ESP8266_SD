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
z=Bz+Cz+b+r;

intersection() {
    difference() {
        hull() {
            translate([r,r,r]) sphere(r);
            translate([x-r,r,r]) sphere(r);
            translate([r,y-r,r]) sphere(r);
            translate([r,r,z-r]) sphere(r);
            translate([x-r,y-r,r]) sphere(r);
            translate([x-r,r,z-r]) sphere(r);
            translate([r,y-r,z-r]) sphere(r);
            translate([x-r,y-r,z-r]) sphere(r);
        }

        translate([b,b,b]) cube([Bx,By,Bz+Cz+2]);
    
    }
    cube([x,y,z-r]);
}