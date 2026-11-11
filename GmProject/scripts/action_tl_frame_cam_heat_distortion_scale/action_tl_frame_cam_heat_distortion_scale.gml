/// action_tl_frame_cam_heat_distortion_scale(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_heat_distortion_scale(val, add)
{
	tl_value_set_start(action_tl_frame_cam_heat_distortion_scale, true)
	tl_value_set(e_value.CAM_HEAT_DISTORTION_SCALE, val / 100, add)
	tl_value_set_done()
}