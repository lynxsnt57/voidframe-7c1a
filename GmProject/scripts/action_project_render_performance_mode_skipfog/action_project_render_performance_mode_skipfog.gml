/// action_project_render_performance_mode_skipsky(value)
/// @arg value

function action_project_render_performance_mode_skipfog(val)
{
	
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_performance_mode_skipfog, project_render_performance_mode_skipfog, val, 1)
		
	project_render_performance_mode_skipfog = val
	render_samples = -1
}
