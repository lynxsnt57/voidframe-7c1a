/// action_tl_frame_cam_black_lines_size(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_black_lines_size(val, add)
{
	tl_value_set_start(action_tl_frame_cam_black_lines_size, true)
	tl_value_set(e_value.CAM_BLACK_LINES_SIZE, val / 100, add)
	tl_value_set_done()
}