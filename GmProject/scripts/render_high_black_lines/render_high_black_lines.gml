/// render_high_black_lines(basesurf)
/// @arg basesurf

function render_high_black_lines(prevsurf)
{
    var resultsurf = render_high_get_apply_surf();
    
    surface_set_target(resultsurf)
    {
        draw_clear_alpha(c_black, 0);
        
        // Draw the original surface first
        draw_surface_exists(prevsurf, 0, 0);
        
        // Get black lines parameters from camera
        var line_size = render_camera.value[e_value.CAM_BLACK_LINES_SIZE];
        var line_rotation = render_camera.value[e_value.CAM_BLACK_LINES_ROTATION];
        var offset_y = render_camera.value[e_value.CAM_BLACK_LINES_OFFSET_Y];
        var line_color = render_camera.value[e_value.CAM_BLACK_LINES_COLOR];
        
        // Calculate line thickness in pixels
        var line_thickness = (line_size + ((render_post_kernel - 1) / (render_height / 5))) * render_height;
        
        // Calculate Y offset in pixels
        var pixel_offset_y = offset_y * render_height * 0.01;
        
        // Draw colored lines directly to surface
        draw_set_color(line_color);

        // Precompute center and rotation
        var cx = render_width * 0.5;
        var cy = render_height * 0.5 + pixel_offset_y;
        var ang = line_rotation;

        // Half-thickness
        var half_t = line_thickness * 1;

        // Rotation cos/sin
        var c = dcos(ang);
        var s = dsin(ang);

        // Draw two big parallelogram bars that simulate rotation
        var size = render_width * 3; // extend far enough
		
		// Top Bars
		var ypos = -render_height
        var x1 = -size, y1 = ypos - half_t;
        var x2 =  size, y2 = ypos - half_t;
        var x3 =  size, y3 = ypos + half_t;
        var x4 = -size, y4 = ypos + half_t;

        // Rotate and translate manually
        var p1x = cx + x1 * c - y1 * s;
        var p1y = cy + x1 * s + y1 * c;
        var p2x = cx + x2 * c - y2 * s;
        var p2y = cy + x2 * s + y2 * c;
        var p3x = cx + x3 * c - y3 * s;
        var p3y = cy + x3 * s + y3 * c;
        var p4x = cx + x4 * c - y4 * s;
        var p4y = cy + x4 * s + y4 * c;

        draw_primitive_begin(pr_trianglestrip);
        draw_vertex(p1x, p1y);
        draw_vertex(p2x, p2y);
        draw_vertex(p4x, p4y);
        draw_vertex(p3x, p3y);
        draw_primitive_end();
		
		// Bottom Bars
		ypos = render_height
        x1 = -size
		y1 = ypos - half_t;
        x2 =  size
		y2 = ypos - half_t;
        x3 =  size
		y3 = ypos + half_t;
        x4 = -size
		y4 = ypos + half_t;

        // Rotate and translate manually
        p1x = cx + x1 * c - y1 * s;
        p1y = cy + x1 * s + y1 * c;
        p2x = cx + x2 * c - y2 * s;
        p2y = cy + x2 * s + y2 * c;
        p3x = cx + x3 * c - y3 * s;
        p3y = cy + x3 * s + y3 * c;
        p4x = cx + x4 * c - y4 * s;
        p4y = cy + x4 * s + y4 * c;

        draw_primitive_begin(pr_trianglestrip);
        draw_vertex(p1x, p1y);
        draw_vertex(p2x, p2y);
        draw_vertex(p4x, p4y);
        draw_vertex(p3x, p3y);
        draw_primitive_end();
    }
    surface_reset_target();
    
    return resultsurf;
}