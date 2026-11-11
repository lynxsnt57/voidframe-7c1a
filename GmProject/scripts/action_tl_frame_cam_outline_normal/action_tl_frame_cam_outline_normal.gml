/// action_tl_frame_cam_outline_normal(enable)
/// @arg enable

function action_tl_frame_cam_outline_normal(enable)
{
	tl_value_set_start(action_tl_frame_cam_outline_normal, false)
	tl_value_set(e_value.CAM_OUTLINE_NORMAL, enable, false)
	tl_value_set_done()
}
