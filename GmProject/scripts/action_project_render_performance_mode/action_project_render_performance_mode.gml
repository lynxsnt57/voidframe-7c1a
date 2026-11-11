/// action_project_render_performance_mode(value)
/// @arg value

function action_project_render_performance_mode(val)
{
	
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_performance_mode, project_render_performance_mode, val, 1)
		
	project_render_performance_mode = val
	render_samples = -1
}
