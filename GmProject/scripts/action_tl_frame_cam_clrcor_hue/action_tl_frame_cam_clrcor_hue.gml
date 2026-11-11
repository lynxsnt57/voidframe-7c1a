/// action_tl_frame_cam_clrcor_hue(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_clrcor_hue(val, add)
{
	tl_value_set_start(action_tl_frame_cam_clrcor_hue, true)
	tl_value_set(e_value.CAM_COLOR_HUE, val / 100, add)
	tl_value_set_done()
}
