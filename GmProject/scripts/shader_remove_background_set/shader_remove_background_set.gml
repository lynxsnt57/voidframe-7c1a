/// shader_remove_background_set()
function shader_remove_background_set(Mainsurf, alphasurf)
{
	texture_set_stage(sampler_map[?"uPrevSurface"], surface_get_texture(Mainsurf));
	texture_set_stage(sampler_map[?"uAlphaSurface"], surface_get_texture(alphasurf));
	
	gpu_set_texrepeat_ext(sampler_map[?"uPrevSurface"], false);
	gpu_set_texrepeat_ext(sampler_map[?"uAlphaSurface"], false);
}