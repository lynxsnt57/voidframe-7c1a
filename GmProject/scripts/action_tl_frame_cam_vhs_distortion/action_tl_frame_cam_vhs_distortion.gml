/// action_tl_frame_cam_vhs_distortion(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_vhs_distortion(val, add)
{
	tl_value_set_start(action_tl_frame_cam_vhs_distortion, true)
	tl_value_set(e_value.CAM_VHS_DISTORTION, val / 100, add)
	tl_value_set_done()
}