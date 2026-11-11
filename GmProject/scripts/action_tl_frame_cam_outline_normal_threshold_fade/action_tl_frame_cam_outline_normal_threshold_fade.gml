/// action_tl_frame_cam_outline_normal_threshold_fade(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_outline_normal_threshold_fade(val, add)
{
	tl_value_set_start(action_tl_frame_cam_outline_normal_threshold_fade, true)
	tl_value_set(e_value.CAM_OUTLINE_NORMAL_THRESHOLD_FADE, val / 100, add)
	tl_value_set_done()
}
