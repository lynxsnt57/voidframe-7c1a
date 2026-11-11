/// action_tl_frame_cam_dof_bokeh_strength(value, add)
/// @arg value
/// @arg add

function action_tl_frame_cam_dof_bokeh_strength(val, add)
{
	tl_value_set_start(action_tl_frame_cam_dof_bokeh_strength, true)
	tl_value_set(e_value.CAM_DOF_BOKEH_STRENGTH, val / 100, add)
	tl_value_set_done()
}
