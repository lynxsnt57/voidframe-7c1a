/// action_tl_frame_cam_outline_radius(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_outline_radius(val, add)
{
	tl_value_set_start(action_tl_frame_cam_outline_radius, true)
	tl_value_set(e_value.CAM_OUTLINE_RADIUS, val, add)
	tl_value_set_done()
}
