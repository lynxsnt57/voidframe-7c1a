/// render_high_heat_distortion(basesurf)
/// @arg basesurf
/// @desc Applies heat distortion wave effect to the rendered surface

function render_high_heat_distortion(prevsurf)
{
	if (!render_camera_heat_distortion)
		return prevsurf
	
	var strength = render_camera.value[e_value.CAM_HEAT_DISTORTION_STRENGTH];
	
	if (strength <= 0)
		return prevsurf
	
	// Create temporary surface for effect
	var resultsurf = render_high_get_apply_surf();
	
	gpu_set_texfilter(true)
	
	surface_set_target(resultsurf)
	{
		draw_clear_alpha(c_black, 0)
		
		render_shader_obj = shader_map[?shader_high_heat_distortion]
		with (render_shader_obj)
			shader_use()
		draw_surface_exists(prevsurf, 0, 0)
		with (render_shader_obj)
			shader_clear()
	}
	surface_reset_target()
	
	gpu_set_texfilter(false)
	
	return resultsurf
}