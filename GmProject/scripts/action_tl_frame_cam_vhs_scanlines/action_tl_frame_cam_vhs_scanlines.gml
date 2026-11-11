/// action_tl_frame_cam_vhs_scanlines(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_vhs_scanlines(val, add)
{
	tl_value_set_start(action_tl_frame_cam_vhs_scanlines, true)
	tl_value_set(e_value.CAM_VHS_SCANLINES, val / 100, add)
	tl_value_set_done()
}
