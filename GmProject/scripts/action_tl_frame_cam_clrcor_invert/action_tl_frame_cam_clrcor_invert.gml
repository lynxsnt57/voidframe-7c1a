/// action_tl_frame_cam_clrcor_invert(enable)
/// @arg enable

function action_tl_frame_cam_clrcor_invert(enable)
{
	tl_value_set_start(action_tl_frame_cam_clrcor_invert, false)
	tl_value_set(e_value.CAM_COLOR_INVERT, enable, false)
	tl_value_set_done()
}
