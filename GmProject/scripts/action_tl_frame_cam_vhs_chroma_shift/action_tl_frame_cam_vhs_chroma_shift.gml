/// action_tl_frame_cam_vhs_chroma_shift(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_vhs_chroma_shift(val, add)
{
	tl_value_set_start(action_tl_frame_cam_vhs_chroma_shift, true)
	tl_value_set(e_value.CAM_VHS_CHROMA_SHIFT, val / 100, add)
	tl_value_set_done()
}
