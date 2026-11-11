/// action_tl_frame_cam_vhs_noise(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_vhs_noise(val, add)
{
	tl_value_set_start(action_tl_frame_cam_vhs_noise, true)
	tl_value_set(e_value.CAM_VHS_NOISE, val / 100, add)
	tl_value_set_done()
}
