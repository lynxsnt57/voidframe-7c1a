/// shader_vhs_set()

function shader_vhs_set()
{
	render_set_uniform_vec2("uScreenSize", render_width, render_height)
	render_set_uniform("uDistortion", render_camera.value[e_value.CAM_VHS_DISTORTION])
	render_set_uniform("uNoise", render_camera.value[e_value.CAM_VHS_NOISE])
	render_set_uniform("uScanlines", render_camera.value[e_value.CAM_VHS_SCANLINES])
	render_set_uniform("uChromaShift", render_camera.value[e_value.CAM_VHS_CHROMA_SHIFT])
}