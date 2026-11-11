/// shader_add_set(texture, amount, [color, [power]], blendmode)
/// @arg texture
/// @arg amount
/// @arg [color
/// @arg [power]]
/// @arg blendmode

function shader_add_set(texture, amount, color = c_white, strength = 1, blendmode = "add")
{
	texture_set_stage(sampler_map[?"uAddTexture"], surface_get_texture(texture))
	gpu_set_texfilter_ext(sampler_map[?"uAddTexture"], false)
	
	render_set_uniform("uAmount", amount)
	render_set_uniform_color("uBlendColor", color, 1)
	render_set_uniform_int("uBlendMode", ds_list_find_index(blend_mode_list_ex, blendmode))
	
	render_set_uniform("uPower", strength)
}
