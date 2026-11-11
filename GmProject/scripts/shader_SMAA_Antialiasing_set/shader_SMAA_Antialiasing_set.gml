/// shader_SMAA_Antialiasing_set

function shader_SMAA_Antialiasing_set(surface) {
	texture_set_stage(sampler_map[?"uScreen"], surface_get_texture(surface));
	
	render_set_uniform("uPower", (1.0 - app.project_render_smaa_power) * 7.5);
	render_set_uniform_vec2("uResolution", render_width, render_height);
}