/// action_tl_frame_cam_black_lines_offset(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_black_lines_offset(val, add)
{
	tl_value_set_start(action_tl_frame_cam_black_lines_offset, true)
	tl_value_set(e_value.CAM_BLACK_LINES_OFFSET_Y, val, add)
	tl_value_set_done()
}