/// action_project_render_dof_affect_glow(value)
/// @arg value

function action_project_render_dof_affect_glow(val)
{
	
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_dof_affect_glow, project_render_dof_affect_glow, val, 1)
		
	project_render_dof_affect_glow = val
	render_samples = -1
}
