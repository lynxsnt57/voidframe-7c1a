/// shader_high_fog_apply_set(fogbuffer, color)
/// @arg fogbuffer
/// @arg color

function shader_high_fog_apply_set(fogbuffer, color)
{
	texture_set_stage(sampler_map[?"uFogBuffer"], surface_get_texture(fogbuffer))
	render_set_uniform_color("uFogColor", color, 1)
}