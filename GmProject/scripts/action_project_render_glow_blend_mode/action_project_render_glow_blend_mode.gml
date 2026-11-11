/// action_project_render_glow_blend_mode(mode)
/// @arg mode

function action_project_render_glow_blend_mode(mode)
{
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_glow_blend_mode, project_render_glow_blend_mode, mode, true)
	
	project_render_glow_blend_mode = mode
}
