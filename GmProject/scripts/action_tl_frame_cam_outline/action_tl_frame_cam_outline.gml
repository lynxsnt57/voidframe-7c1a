/// action_tl_frame_cam_outline(enable)
/// @arg enable

function action_tl_frame_cam_outline(enable)
{
	tl_value_set_start(action_tl_frame_cam_outline, false)
	tl_value_set(e_value.CAM_OUTLINE, enable, false)
	tl_value_set_done()
}
