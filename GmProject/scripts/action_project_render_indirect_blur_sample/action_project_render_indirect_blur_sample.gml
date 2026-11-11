/// action_project_render_indirect_blur_sample(size)
/// @arg size

function action_project_render_indirect_blur_sample(val, add)
{
	if (!history_undo && !history_redo)
		history_set_var(action_project_render_indirect_blur_sample, project_render_indirect_blur_sample, project_render_indirect_blur_sample * add + val, 1)
		
	project_render_indirect_blur_sample = project_render_indirect_blur_sample * add + val
	render_samples = -1
}