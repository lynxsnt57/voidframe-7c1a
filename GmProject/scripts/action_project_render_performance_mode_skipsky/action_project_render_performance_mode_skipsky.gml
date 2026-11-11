/// action_project_render_performance_mode_skipsky(value)
/// @arg value

function action_project_render_performance_mode_skipsky(val)
{
	
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_performance_mode_skipsky, project_render_performance_mode_skipsky, val, 1)
		
	project_render_performance_mode_skipsky = val
	render_samples = -1
}
