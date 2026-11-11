/// vbuffer_create_torus(radius, tex1, tex2, hflip, vflip, detail, smooth, invert, tex_mapped, [major_radius, minor_radius])
/// @arg radius
/// @arg tex1
/// @arg tex2
/// @arg hflip
/// @arg vflip
/// @arg detail
/// @arg smooth
/// @arg invert
/// @arg tex_mapped
/// @arg [major_radius]
/// @arg [minor_radius]

function vbuffer_create_torus(rad, tex1, tex2, hflip, vflip, detail, smooth, invert, tex_mapped, major_radius = 1.0, minor_radius = 0.4)
{
    vbuffer_start();
    
    var texsize = point2D_sub(tex2, tex1);
    var n = negate(invert);

    var R = rad * major_radius; // major radius
    var r = rad * minor_radius; // minor radius
	
	if (detail > 8)
		detail = floor(detail / 2) * 2
	
    var major_segments = detail;
    var minor_segments = max(detail / 2, 4);

    for (var i = 0; i < major_segments; i++)
    {
        var i0 = i / major_segments;
        var i1 = (i + 1) / major_segments;

        var a0 = i0 * 2 * pi;
        var a1 = i1 * 2 * pi;

        var cosA0 = cos(a0), sinA0 = sin(a0);
        var cosA1 = cos(a1), sinA1 = sin(a1);

        for (var j = 0; j < minor_segments; j++)
        {
            var j0 = j / minor_segments;
            var j1 = (j + 1) / minor_segments;

            var b0 = j0 * 2 * pi;
            var b1 = j1 * 2 * pi;

            var cosB0 = cos(b0), sinB0 = sin(b0);
            var cosB1 = cos(b1), sinB1 = sin(b1);

            // ---- POSITIONS ----
            var p1x = (R + r * cosB0) * cosA0;
            var p1y = (R + r * cosB0) * sinA0;
            var p1z =  r * sinB0;

            var p2x = (R + r * cosB0) * cosA1;
            var p2y = (R + r * cosB0) * sinA1;
            var p2z =  r * sinB0;

            var p3x = (R + r * cosB1) * cosA0;
            var p3y = (R + r * cosB1) * sinA0;
            var p3z =  r * sinB1;

            var p4x = (R + r * cosB1) * cosA1;
            var p4y = (R + r * cosB1) * sinA1;
            var p4z =  r * sinB1;

            // ---- NORMALS ----
            var n1x = cosB0 * cosA0;
            var n1y = cosB0 * sinA0;
            var n1z = sinB0;

            var n2x = cosB0 * cosA1;
            var n2y = cosB0 * sinA1;
            var n2z = sinB0;

            var n3x = cosB1 * cosA0;
            var n3y = cosB1 * sinA0;
            var n3z = sinB1;

            var n4x = cosB1 * cosA1;
            var n4y = cosB1 * sinA1;
            var n4z = sinB1;

            if (!smooth)
            {
                var fx = (n1x + n2x + n3x + n4x) * 0.25;
                var fy = (n1y + n2y + n3y + n4y) * 0.25;
                var fz = (n1z + n2z + n3z + n4z) * 0.25;
                n1x=fx;
				n2x=fx;
				n3x=fx;
				n4x=fx;
                n1y=fy;
				n2y=fy;
				n3y=fy;
				n4y=fy;
                n1z=fz;
				n2z=fz;
				n3z=fz;
				n4z=fz;
            }

            // ---- UV (Blender unwrap) ----
            var u1 = tex1[X] + i0 * texsize[X];
            var u2 = tex1[X] + i1 * texsize[X];

            var v1 = tex1[Y] + j0 * texsize[Y];
            var v2 = tex1[Y] + j1 * texsize[Y];

            if (hflip) { var t=u1; u1=u2; u2=t; }
            if (vflip) { var t=v1; v1=v2; v2=t; }

            // ---- TRIANGLES ----
            if (invert)
            {
                vertex_add(p1x,p1y,p1z, n1x*n,n1y*n,n1z*n, u1,v1);
                vertex_add(p3x,p3y,p3z, n3x*n,n3y*n,n3z*n, u1,v2);
                vertex_add(p2x,p2y,p2z, n2x*n,n2y*n,n2z*n, u2,v1);

                vertex_add(p2x,p2y,p2z, n2x*n,n2y*n,n2z*n, u2,v1);
                vertex_add(p3x,p3y,p3z, n3x*n,n3y*n,n3z*n, u1,v2);
                vertex_add(p4x,p4y,p4z, n4x*n,n4y*n,n4z*n, u2,v2);
            }
            else
            {
                vertex_add(p1x,p1y,p1z, n1x*n,n1y*n,n1z*n, u1,v1);
                vertex_add(p2x,p2y,p2z, n2x*n,n2y*n,n2z*n, u2,v1);
                vertex_add(p3x,p3y,p3z, n3x*n,n3y*n,n3z*n, u1,v2);

                vertex_add(p2x,p2y,p2z, n2x*n,n2y*n,n2z*n, u2,v1);
                vertex_add(p4x,p4y,p4z, n4x*n,n4y*n,n4z*n, u2,v2);
                vertex_add(p3x,p3y,p3z, n3x*n,n3y*n,n3z*n, u1,v2);
            }
        }
    }
    
    return vbuffer_done();
}
