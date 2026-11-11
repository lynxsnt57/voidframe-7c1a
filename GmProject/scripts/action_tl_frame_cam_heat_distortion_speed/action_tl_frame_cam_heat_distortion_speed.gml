/// action_tl_frame_cam_heat_distortion_speed(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_heat_distortion_speed(val, add)
{
	tl_value_set_start(action_tl_frame_cam_heat_distortion_speed, true)
	tl_value_set(e_value.CAM_HEAT_DISTORTION_SPEED, val / 100, add)
	tl_value_set_done()
}