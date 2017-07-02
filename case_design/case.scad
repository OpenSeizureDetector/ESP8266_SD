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

rT=2;     //radius of curvature on top
rS=5;     //radius of curvature on sides

cube([x,y,z]);