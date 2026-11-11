/// action_project_render_smaa(enable)
/// @arg enable

function action_project_render_smaa(enable)
{
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_smaa, project_render_smaa, enable, true)
	
	project_render_smaa = enable
}
