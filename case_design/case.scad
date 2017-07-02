Bx=25;     //batter length
By=15;
Bz=3;
Cx=25;     //circuit length
Cy=15;
Cz=3;

b=3;    //border

x=Bx+(2*b);    //if battery is wider than circuit
y=By+(2*b);
z=Bz+Cz+b+1;

r=2;     //radius of curvature


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
    