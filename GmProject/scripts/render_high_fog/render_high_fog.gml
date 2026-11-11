/// render_high_fog(basesurf)
/// @arg basesurf

function render_high_fog(basesurf)
{
	var fogsurf, prevsurf, fogheightsurf;
	render_surface[1] = surface_require(render_surface[1], render_width, render_height)
	render_surface[2] = surface_require(render_surface[2], render_width, render_height)
	render_surface[3] = surface_require(render_surface[3], render_width, render_height)
	fogsurf = render_surface[1]
	fogheightsurf = render_surface[2]
	prevsurf = render_surface[3]
	
	// Get fog strength
	if (!project_render_performance_mode || !project_render_performance_mode_skipfog)
	{
		surface_set_target_ext(0, fogsurf)
		surface_set_target_ext(1, fogheightsurf)
		{
			draw_clear(c_black)
			render_world_start()
			render_world(e_render_mode.HIGH_FOG)
			render_world_done()
		}
		surface_reset_target()
	} else {
		surface_copy(fogsurf, 0, 0, render_surface_depth)
	}
	
	// Copy into separate surface
	surface_set_target(prevsurf)
	{
		draw_clear_alpha(c_black, 0)
		draw_surface_exists(basesurf, 0, 0)
	}
	surface_reset_target()
	
	// Apply fog
	surface_set_target(basesurf)
	{
		draw_clear_alpha(c_black, 0)
		draw_surface_exists(prevsurf, 0, 0)
		
		// Draw fog
		if (background_fog_show)
		{
			// Draw height fog
			if (background_fog_height_show)
			{
				render_shader_obj = shader_map[?shader_high_fog_apply]
				with (render_shader_obj)
				{
					shader_set(shader)
					shader_high_fog_apply_set(fogheightsurf, app.background_fog_height_color_final)
				}
				draw_blank(0, 0, render_width, render_height)
				with (render_shader_obj)
					shader_clear()
			}
			
			// Draw fog
			render_shader_obj = shader_map[?shader_high_fog_apply]
			with (render_shader_obj)
			{
				shader_set(shader)
				shader_high_fog_apply_set(fogsurf, app.background_fog_object_color_final)
			}
			draw_blank(0, 0, render_width, render_height)
			with (render_shader_obj)
				shader_clear()
		}
		
		// Alpha fix
		render_set_projection_ortho(0, 0, render_width, render_height, 0)
		gpu_set_blendmode_ext(bm_src_color, bm_one)
		draw_box(0, 0, render_width, render_height, false, c_black, 1)
		gpu_set_blendmode(bm_normal)
		/*
		if (app.project_render_legacy_rendering)
		{
			if (render_background)
				draw_box(0, 0, render_width, render_height, false, c_black, 1)
			else
			{
				render_world_start()
				render_world(e_render_mode.ALPHA_FIX)
				render_world_done()
			}
		} else {
			draw_surface(render_surface_alpha_fix, 0, 0)
		}
		gpu_set_blendmode(bm_normal)
		*/
	}
	surface_reset_target()
}