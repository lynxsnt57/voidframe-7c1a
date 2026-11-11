/// action_tl_frame_cam_vhs(enable)
/// @arg enable

function action_tl_frame_cam_vhs(enable)
{
	tl_value_set_start(action_tl_frame_cam_vhs, false)
	tl_value_set(e_value.CAM_VHS, enable, false)
	tl_value_set_done()
}
