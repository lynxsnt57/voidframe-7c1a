/// action_tl_frame_cam_heat_distortion(enable)
/// @arg enable

function action_tl_frame_cam_heat_distortion(enable)
{
	tl_value_set_start(action_tl_frame_cam_heat_distortion, true)
	tl_value_set(e_value.CAM_HEAT_DISTORTION, enable, false)
	tl_value_set_done()
}