/// action_tl_frame_cam_black_lines_rotation(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_black_lines_rotation(val, add)
{
	tl_value_set_start(action_tl_frame_cam_black_lines_rotation, true)
	tl_value_set(e_value.CAM_BLACK_LINES_ROTATION, val, add)
	tl_value_set_done()
}