// Script assets have changed for v2.3.0 see
// https://help.yoyogames.com/hc/en-us/articles/360005277377 for more information
function render_high_remove_background(surf)
{
	var prevsurf, alphasurf;
	render_surface[1] = surface_require(render_surface[1], render_width, render_height)
	render_surface[2] = surface_require(render_surface[2], render_width, render_height)
	prevsurf = render_surface[1]
	alphasurf = render_surface[2]
	
	surface_set_target(prevsurf)
	{
		draw_clear_alpha(c_black, 0)
		draw_surface_exists(surf, 0 ,0)
	}
	surface_reset_target()
	
	surface_set_target(alphasurf)
	{
		draw_clear_alpha(c_black, 0)
		render_world_start()
		render_world(e_render_mode.ALPHA_FIX)
		render_world_done()
	}
	surface_reset_target()
	
	// Remove Background if needed
	surface_set_target(surf)
	{
		draw_clear_alpha(c_black, 0)
		
		render_shader_obj = shader_map[?shader_remove_background]
		with (render_shader_obj)
		{
			shader_set(shader)
			shader_remove_background_set(prevsurf, alphasurf)
		}
		
		draw_surface_exists(prevsurf, 0, 0)
		with (render_shader_obj)
			shader_clear()
	}
	surface_reset_target()
	
	return surf
}