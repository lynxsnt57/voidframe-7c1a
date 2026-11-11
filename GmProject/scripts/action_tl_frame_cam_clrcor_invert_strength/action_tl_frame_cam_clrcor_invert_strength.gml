/// action_tl_frame_cam_clrcor_invert_strength(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_clrcor_invert_strength(val, add)
{
	tl_value_set_start(action_tl_frame_cam_clrcor_invert_strength, true)
	tl_value_set(e_value.CAM_COLOR_INVERT_INTENSITY, val / 100, add)
	tl_value_set_done()
}
