/// Volume render settings actions (project-level).
function action_project_volume_enabled(enable)
{
    if (!history_undo && !history_redo) history_set_var(action_project_volume_enabled, project_render_volume_enabled, enable, 1)
    project_render_volume_enabled = enable
    render_samples = -1
}

function action_project_volume_resolution(val, add)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_volume_resolution, project_render_volume_resolution, project_render_volume_resolution * add + val / 100, 1)
    else val *= 100
    project_render_volume_resolution = project_render_volume_resolution * add + val / 100
    render_samples = -1
}

function action_project_volume_quality(val, add)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_volume_quality, project_render_volume_quality, project_render_volume_quality * add + val / 100, 1)
    else val *= 100
    project_render_volume_quality = project_render_volume_quality * add + val / 100
    render_samples = -1
}

function action_project_volume_jitter(val, add)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_volume_jitter, project_render_volume_jitter, project_render_volume_jitter * add + val / 100, 1)
    else val *= 100
    project_render_volume_jitter = project_render_volume_jitter * add + val / 100
    render_samples = -1
}

function action_project_volume_shadow_samples(val, add)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_volume_shadow_samples, project_render_volume_shadow_samples, project_render_volume_shadow_samples * add + val, 1)
    project_render_volume_shadow_samples = project_render_volume_shadow_samples * add + val
    render_samples = -1
}

function action_project_volume_shadow_softness(val, add)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_volume_shadow_softness, project_render_volume_shadow_softness, project_render_volume_shadow_softness * add + val, 1)
    project_render_volume_shadow_softness = project_render_volume_shadow_softness * add + val
    render_samples = -1
}

function action_project_volume_near(val, add)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_volume_near, project_render_volume_near, project_render_volume_near * add + val, 1)
    project_render_volume_near = project_render_volume_near * add + val
    render_samples = -1
}

function action_project_volume_far(val, add)
{
    if (!history_undo && !history_redo)
        history_set_var(action_project_volume_far, project_render_volume_far, project_render_volume_far * add + val, 1)
    project_render_volume_far = project_render_volume_far * add + val
    render_samples = -1
}
