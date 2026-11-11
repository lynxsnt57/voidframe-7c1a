/// action_project_render_glow(enable)
/// @arg enable

function action_project_render_glow(enable)
{
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_glow, project_render_glow, enable, true)
	
	project_render_glow = enable
}


function action_project_glow_crisp_core(enable)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_glow_crisp_core, project_glow_crisp_core, enable, true)
    project_glow_crisp_core = enable
}

function action_project_glow_less_blur(enable)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_glow_less_blur, project_glow_less_blur, enable, true)
    project_glow_less_blur = enable
}
