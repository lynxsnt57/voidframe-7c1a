/// action_tl_frame_cam_black_lines(enable)
/// @arg enable

function action_tl_frame_cam_black_lines(enable)
{
	tl_value_set_start(action_tl_frame_cam_black_lines, true)
	tl_value_set(e_value.CAM_BLACK_LINES, enable, false)
	tl_value_set_done()
}