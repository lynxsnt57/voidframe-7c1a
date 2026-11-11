/// shader_high_heat_distortion_set()

function shader_high_heat_distortion_set()
{
	// Time-based animation
	render_set_uniform("uTime", (app.timeline_marker / 10) * render_camera.value[e_value.CAM_HEAT_DISTORTION_SPEED])
	
	render_set_uniform("uStrength", render_camera.value[e_value.CAM_HEAT_DISTORTION_STRENGTH] / 100)
	render_set_uniform("uScale", render_camera.value[e_value.CAM_HEAT_DISTORTION_SCALE])
	render_set_uniform_vec2("uResolution", render_width, render_height)
}