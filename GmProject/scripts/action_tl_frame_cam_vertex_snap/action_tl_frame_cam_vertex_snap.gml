/// action_tl_frame_cam_vertex_snap(enable)

function action_tl_frame_cam_vertex_snap(enable)
{
	tl_value_set_start(action_tl_frame_cam_vertex_snap, true)
	tl_value_set(e_value.CAM_VERTEX_SNAP, enable, false)
	tl_value_set_done()
}