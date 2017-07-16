difference() 
{
cube([40,40,10]);
translate([0,15,9])
//translate([0,50,9])
    linear_extrude(height=2)
            text("OpenSeizureDetector",
                size=2.8,
                font="Liberation Sans:style=bold");
}

translate([50,0,0])
    linear_extrude(height = 40)
        import("OSD_Logo.dxf");
