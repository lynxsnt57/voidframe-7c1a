/// action_tl_frame_cam_black_lines_color(color, add)
/// @arg color
/// @arg add

function action_tl_frame_cam_black_lines_color(color)
{
	tl_value_set_start(action_tl_frame_cam_black_lines_color, true)
	tl_value_set(e_value.CAM_BLACK_LINES_COLOR, color, false)
	tl_value_set_done()
}