Bx=30;     //battery size
By=20;
Bz=3;

Cx=35;     //circuit size
Cy=26;
Cz=3;

b=3;    //border width
r=2;    //radius of curvature

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
    
    }
    cube([x,y,z]);
}