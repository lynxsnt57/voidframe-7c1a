/// action_tl_frame_cam_bloom_blend_mode(enable)
/// @arg enable

function action_tl_frame_cam_bloom_blend_mode(enable)
{
	tl_value_set_start(action_tl_frame_cam_bloom_blend_mode, false)
	tl_value_set(e_value.CAM_BLOOM_BLEND_MODE, enable, false)
	tl_value_set_done()
}

