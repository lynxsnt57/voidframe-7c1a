/// render_high_glow(basesurf, [falloff])
/// @arg basesurf
/// @arg [falloff]

function render_high_glow(prevsurf, glowfalloff = false)
{
	var glowcolorsurf, glowsurf, glowcoresurf, glowcombined, resultsurf, randomrotation;
	glowcoresurf = null
	render_surface[0] = surface_require(render_surface[0], render_width, render_height)
	render_surface[1] = surface_require(render_surface[1], render_width, render_height)
	glowcolorsurf = render_surface[0]
	glowsurf = render_surface[1]
	randomrotation = (render_post_kernel - 0.85) * 1200
	
	if (!glowfalloff)
	{
		// Draw Glow Colors and Cache it
		surface_set_target(glowcolorsurf)
		{
			draw_clear_alpha(c_black, 1)
		
			render_world_start()
			render_world(e_render_mode.COLOR_GLOW)
			render_world_done()
		
			render_set_projection_ortho(0, 0, render_width, render_height, 0)
		
			gpu_set_blendmode_ext(bm_src_color, bm_one) 
			draw_box(0, 0, render_width, render_height, false, c_black, 1)
			
			gpu_set_blendmode(bm_normal)
		}
		surface_reset_target()
		
		/*
		// Add glint to the glow surface
		if (!project_render_legacy_rendering)
		{
			surface_copy(glowsurftemp, 0, 0, glowcolorsurf)
			
			// Glint * Glow Surf
			surface_set_target(glowsurftemp)
			{
				gpu_set_blendmode_ext(bm_dest_color, bm_src_color)
				draw_surface(render_surface_glint, 0, 0)
				gpu_set_blendmode(bm_normal)
			}
			surface_reset_target()
			
			// Glint + (Glint * Glow Surf)
			surface_set_target(glowcolorsurf)
			{
				gpu_set_blendmode(bm_add)
				draw_surface(glowsurftemp, 0, 0)
				gpu_set_blendmode(bm_normal)
			}
			surface_reset_target()
		}
		*/ // USELESS
		
		surface_copy(render_surface_glow_cache, 0, 0, glowcolorsurf)
	} else {
		surface_copy(glowcolorsurf, 0, 0, render_surface_glow_cache)
	}
	
	var glowsurftemp;
	render_surface[2] = surface_require(render_surface[2], render_width, render_height)
	glowsurftemp = render_surface[2]
	
	// Save a compact core for object glow. Keep the optional falloff pass
	// unchanged so existing project falloff settings still work.
	if (!glowfalloff)
		glowcoresurf = surface_require(glowcoresurf, render_width, render_height, false)

	render_shader_obj = shader_map[?shader_blur]
	with (render_shader_obj)
		shader_set(shader)
	
	// Radius changes based on the render height to make it consistant with the size of the render
	var baseradius;
	
	if (glowfalloff)
		baseradius = ((project_render_glow_radius * project_render_glow_falloff_radius * 10) * render_height / 500 * render_post_kernel)
	else
		baseradius = ((project_render_glow_radius * 10) * render_height / 500 * render_post_kernel)
	
	gpu_set_tex_repeat(false)
	gpu_set_texfilter(true)
	
	for (var i = 0; i < 3; i++)
	{
		var radius = glowfalloff ? baseradius / (1 + 1.333 * i) : baseradius * (i = 0 ? 0.45 : (i = 1 ? 0.8 : 1.05));
		
		// Horizontal
		surface_set_target(glowsurftemp)
		{
			with (render_shader_obj)
				shader_blur_set(render_blur_kernel, radius, dsin(90 + randomrotation), dcos(90 + randomrotation))
			
			if (i = 0)
				draw_surface_exists(glowcolorsurf, 0, 0)
			else
				draw_surface_exists(glowsurf, 0, 0)
		}
		surface_reset_target()
		
		// Vertical
		surface_set_target(glowsurf)
		{
			with (render_shader_obj)
				shader_blur_set(render_blur_kernel, radius, dsin(0 + randomrotation), dcos(0 + randomrotation))
			draw_surface_exists(glowsurftemp, 0, 0)
		}
		surface_reset_target()
		if (!glowfalloff && i = 0)
			surface_copy(glowcoresurf, 0, 0, glowsurf)
	}
	
	with (render_shader_obj)
		shader_clear()

	// Blend a compact core into the broader blur. The main composite below
	// balances the combined weights to limit excess brightness.
	// This is the existing project/object Glow pass; no camera is required.
	glowcombined = glowsurf
	if (!glowfalloff)
	{
		surface_set_target(glowsurftemp)
		{
			draw_clear_alpha(c_black, 0)
			render_shader_obj = shader_map[?shader_add]
			with (render_shader_obj)
			{
				shader_set(shader)
				shader_add_set(glowcoresurf, 0.666667, c_white)
			}
			draw_surface_exists(glowsurf, 0, 0)
			with (render_shader_obj)
				shader_clear()
		}
		surface_reset_target()
		glowcombined = glowsurftemp
	}
	
	gpu_set_tex_repeat(true)
	gpu_set_texfilter(false)
	
	// Apply Glow
	resultsurf = render_high_get_apply_surf()
	
	surface_set_target(resultsurf)
	{
		draw_clear_alpha(c_black, 0)
		
		render_shader_obj = shader_map[?shader_add]
		with (render_shader_obj)
		{
			shader_set(shader)
			shader_add_set(glowcombined, glowfalloff ? app.project_render_glow_falloff_intensity : app.project_render_glow_intensity * 0.6, c_white, 1, app.project_render_glow_blend_mode)
		}
		draw_surface_exists(prevsurf, 0, 0)
		with (render_shader_obj)
			shader_clear()
	}
	surface_reset_target()
	
	// Add to lens
	if (render_camera_lens_dirt_glow)
	{
		render_surface[0] = surface_require(render_surface[0], render_width, render_height)
		prevsurf = render_surface[0]
		
		surface_set_target(prevsurf)
		{
			draw_clear_alpha(c_black, 1)
			draw_surface(render_surface_lens, 0, 0)
		}
		surface_reset_target()
		
		surface_set_target(render_surface_lens)
		{
			draw_clear_alpha(c_black, 1)
			render_shader_obj = shader_map[?shader_add]
			with (render_shader_obj)
			{
				shader_set(shader)
				shader_add_set(glowcombined, app.project_render_glow_intensity * (glowfalloff ? 1 : 0.6), c_white)
			}
			draw_surface_exists(prevsurf, 0, 0)
			with (render_shader_obj)
				shader_clear()
		}
		surface_reset_target()
	}
	
	if (surface_exists(glowcoresurf))
		surface_free(glowcoresurf)
	return resultsurf
}