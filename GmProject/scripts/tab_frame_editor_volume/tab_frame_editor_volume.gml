/// Volume keyframe controls

function action_tl_frame_volume_density(val, add)
{
	tl_value_set_start(action_tl_frame_volume_density, true)
	tl_value_set(e_value.VOLUME_DENSITY, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_volume_scattering(val, add)
{
	tl_value_set_start(action_tl_frame_volume_scattering, true)
	tl_value_set(e_value.VOLUME_SCATTERING, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_volume_absorption(val, add)
{
	tl_value_set_start(action_tl_frame_volume_absorption, true)
	tl_value_set(e_value.VOLUME_ABSORPTION, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_volume_anisotropy(val, add)
{
	tl_value_set_start(action_tl_frame_volume_anisotropy, true)
	tl_value_set(e_value.VOLUME_ANISOTROPY, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_volume_edge(val, add)
{
	tl_value_set_start(action_tl_frame_volume_edge, true)
	tl_value_set(e_value.VOLUME_EDGE, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_volume_noise(val, add)
{
	tl_value_set_start(action_tl_frame_volume_noise, true)
	tl_value_set(e_value.VOLUME_NOISE_AMOUNT, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_volume_noise_scale(val, add)
{
	tl_value_set_start(action_tl_frame_volume_noise_scale, true)
	tl_value_set(e_value.VOLUME_NOISE_SCALE, val, add)
	tl_value_set_done()
}

function action_tl_frame_volume_samples(val, add)
{
    tl_value_set_start(action_tl_frame_volume_samples, true)
    tl_value_set(e_value.VOLUME_SAMPLES, val, add)
    tl_value_set_done()
}

function action_tl_frame_volume_shape(val)
{
    tl_value_set_start(action_tl_frame_volume_shape, true)
    tl_value_set(e_value.VOLUME_SHAPE, val, false)
    tl_value_set_done()
}

function action_tl_frame_volume_phase(val)
{
    tl_value_set_start(action_tl_frame_volume_phase, true)
    tl_value_set(e_value.VOLUME_PHASE, val, false)
    tl_value_set_done()
}

function action_tl_frame_volume_gradient(val)
{
    tl_value_set_start(action_tl_frame_volume_gradient, true)
    tl_value_set(e_value.VOLUME_GRADIENT, val, false)
    tl_value_set_done()
}

function action_tl_frame_volume_top_color(val)
{
    tl_value_set_start(action_tl_frame_volume_top_color, true)
    tl_value_set(e_value.VOLUME_TOP_COLOR, val, false)
    tl_value_set_done()
}

function action_tl_frame_volume_emission_color(val)
{
    tl_value_set_start(action_tl_frame_volume_emission_color, true)
    tl_value_set(e_value.VOLUME_EMISSION_COLOR, val, false)
    tl_value_set_done()
}

function action_tl_frame_volume_noise_world(val)
{
    tl_value_set_start(action_tl_frame_volume_noise_world, true)
    tl_value_set(e_value.VOLUME_NOISE_WORLD, val, false)
    tl_value_set_done()
}

function action_tl_frame_volume_emission(val, add)
{
    tl_value_set_start(action_tl_frame_volume_emission, true)
    tl_value_set(e_value.VOLUME_EMISSION, val / 100, add)
    tl_value_set_done()
}

function action_tl_frame_volume_detail(val, add)
{
    tl_value_set_start(action_tl_frame_volume_detail, true)
    tl_value_set(e_value.VOLUME_NOISE_DETAIL, val / 100, add)
    tl_value_set_done()
}

function action_tl_frame_volume_contrast(val, add)
{
    tl_value_set_start(action_tl_frame_volume_contrast, true)
    tl_value_set(e_value.VOLUME_NOISE_CONTRAST, val, add)
    tl_value_set_done()
}

function action_tl_frame_volume_threshold(val, add)
{
    tl_value_set_start(action_tl_frame_volume_threshold, true)
    tl_value_set(e_value.VOLUME_NOISE_THRESHOLD, val / 100, add)
    tl_value_set_done()
}

function action_tl_frame_volume_softness(val, add)
{
    tl_value_set_start(action_tl_frame_volume_softness, true)
    tl_value_set(e_value.VOLUME_NOISE_SOFTNESS, val / 100, add)
    tl_value_set_done()
}

function action_tl_frame_volume_seed(val, add)
{
    tl_value_set_start(action_tl_frame_volume_seed, true)
    tl_value_set(e_value.VOLUME_NOISE_SEED, val, add)
    tl_value_set_done()
}

function action_tl_frame_volume_wind_speed(val, add)
{
    tl_value_set_start(action_tl_frame_volume_wind_speed, true)
    tl_value_set(e_value.VOLUME_WIND_SPEED, val, add)
    tl_value_set_done()
}

function action_tl_frame_volume_wind_x(val, add)
{
    tl_value_set_start(action_tl_frame_volume_wind_x, true)
    tl_value_set(e_value.VOLUME_WIND_X, val, add)
    tl_value_set_done()
}

function action_tl_frame_volume_wind_y(val, add)
{
    tl_value_set_start(action_tl_frame_volume_wind_y, true)
    tl_value_set(e_value.VOLUME_WIND_Y, val, add)
    tl_value_set_done()
}

function action_tl_frame_volume_wind_z(val, add)
{
    tl_value_set_start(action_tl_frame_volume_wind_z, true)
    tl_value_set(e_value.VOLUME_WIND_Z, val, add)
    tl_value_set_done()
}

function action_tl_frame_volume_wind_influence(val, add)
{
    tl_value_set_start(action_tl_frame_volume_wind_influence, true)
    tl_value_set(e_value.VOLUME_WIND_INFLUENCE, val / 100, add)
    tl_value_set_done()
}

function action_tl_frame_volume_turbulence(val, add)
{
    tl_value_set_start(action_tl_frame_volume_turbulence, true)
    tl_value_set(e_value.VOLUME_TURBULENCE, val / 100, add)
    tl_value_set_done()
}

function action_tl_frame_volume_turbulence_scale(val, add)
{
    tl_value_set_start(action_tl_frame_volume_turbulence_scale, true)
    tl_value_set(e_value.VOLUME_TURBULENCE_SCALE, val, add)
    tl_value_set_done()
}

// Presets are one-shot edits to existing values, not a persistent mode.
// Color, light response, volume shape and wind direction are left untouched.
function action_tl_frame_volume_preset(val)
{
    if (val < 0 || val > 3) return 0
    var scale = 2.0, detail = 0.15, contrast = 1.0, threshold = 0.25, softness = 0.35;
    var speed = 0.10, turbulence = 0.25, turbulence_scale = 1.8;
    if (val = 1) { scale = 3.0; detail = 0.25; contrast = 1.5; threshold = 0.45; softness = 0.25; speed = 0.25; turbulence = 0.65; turbulence_scale = 2.0; }
    if (val = 2) { scale = 4.0; detail = 0.3; contrast = 1.8; threshold = 0.58; softness = 0.18; speed = 0.2; turbulence = 0.6; turbulence_scale = 2.5; }
    if (val = 3) { scale = 5.0; detail = 0.5; contrast = 1.5; threshold = 0.4; softness = 0.2; speed = 0.45; turbulence = 0.9; turbulence_scale = 3.0; }
    tl_value_set_start(action_tl_frame_volume_preset, false)
    tl_value_set(e_value.VOLUME_NOISE_AMOUNT, 1.0, false)
    tl_value_set(e_value.VOLUME_NOISE_SCALE, scale, false)
    tl_value_set(e_value.VOLUME_NOISE_DETAIL, detail, false)
    tl_value_set(e_value.VOLUME_NOISE_CONTRAST, contrast, false)
    tl_value_set(e_value.VOLUME_NOISE_THRESHOLD, threshold, false)
    tl_value_set(e_value.VOLUME_NOISE_SOFTNESS, softness, false)
    tl_value_set(e_value.VOLUME_WIND_SPEED, speed, false)
    tl_value_set(e_value.VOLUME_WIND_INFLUENCE, 1.0, false)
    tl_value_set(e_value.VOLUME_TURBULENCE, turbulence, false)
    tl_value_set(e_value.VOLUME_TURBULENCE_SCALE, turbulence_scale, false)
    tl_value_set_done()
}

function tab_frame_editor_volume()
{
    tab_control_menu()
    draw_button_menu("frameeditorvolumepreset", e_menu.LIST, dx, dy, dw, 24, -1, text_get("frameeditorvolumepresetchoose"), action_tl_frame_volume_preset)
    tab_next()
    tab_control_menu()
    draw_button_menu("frameeditorvolumeshape", e_menu.LIST, dx, dy, dw, 24, tl_edit.value[e_value.VOLUME_SHAPE], text_get("frameeditorvolumeshape" + string(tl_edit.value[e_value.VOLUME_SHAPE])), action_tl_frame_volume_shape)
    tab_next()
    tab_control_menu()
    draw_button_menu("frameeditorvolumephase", e_menu.LIST, dx, dy, dw, 24, tl_edit.value[e_value.VOLUME_PHASE], text_get("frameeditorvolumephase" + string(tl_edit.value[e_value.VOLUME_PHASE])), action_tl_frame_volume_phase)
    tab_next()
    tab_control_switch()
    draw_switch("frameeditorvolumegradient", dx, dy, tl_edit.value[e_value.VOLUME_GRADIENT], action_tl_frame_volume_gradient)
    tab_next()
	tab_control_color()
	draw_button_color("frameeditorvolumecolor", dx, dy, dw, tl_edit.value[e_value.LIGHT_COLOR], c_white, false, action_tl_frame_light_color)
	tab_next()
    if (tl_edit.value[e_value.VOLUME_GRADIENT])
    {
        tab_control_color()
        draw_button_color("frameeditorvolumetopcolor", dx, dy, dw, tl_edit.value[e_value.VOLUME_TOP_COLOR], c_white, false, action_tl_frame_volume_top_color)
        tab_next()
    }
	tab_control_dragger()
	draw_dragger("frameeditorvolumedensity", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_DENSITY] * 100, 1, 0, 100, 0, 1, tab.volume.tbx_density, action_tl_frame_volume_density)
	tab_next()
	tab_control_dragger()
	draw_dragger("frameeditorvolumescattering", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_SCATTERING] * 100, 1, 0, no_limit, 0, 1, tab.volume.tbx_scattering, action_tl_frame_volume_scattering)
	tab_next()
	tab_control_dragger()
	draw_dragger("frameeditorvolumeabsorption", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_ABSORPTION] * 100, 1, 0, no_limit, 0, 1, tab.volume.tbx_absorption, action_tl_frame_volume_absorption)
	tab_next()
	tab_control_dragger()
	draw_dragger("frameeditorvolumeanisotropy", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_ANISOTROPY] * 100, 1, 0, 100, 0, 1, tab.volume.tbx_anisotropy, action_tl_frame_volume_anisotropy)
	tab_next()
	tab_control_dragger()
	draw_dragger("frameeditorvolumeedge", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_EDGE] * 100, 1, 0, 100, 0, 1, tab.volume.tbx_edge, action_tl_frame_volume_edge)
	tab_next()
	tab_control_dragger()
	draw_dragger("frameeditorvolumesamples", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_SAMPLES], 1, 8, 128, 32, 1, tab.volume.tbx_samples, action_tl_frame_volume_samples)
	tab_next()
    tab_control_color()
    draw_button_color("frameeditorvolumeemissioncolor", dx, dy, dw, tl_edit.value[e_value.VOLUME_EMISSION_COLOR], c_white, false, action_tl_frame_volume_emission_color)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumeemission", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_EMISSION] * 100, 1, 0, 1000, 0, 1, tab.volume.tbx_emission, action_tl_frame_volume_emission)
    tab_next()
    tab_control_switch()
    draw_button_collapse("volume_noise", collapse_map[?"volume_noise"], null, true, "frameeditorvolumenoisegroup")
    tab_next()
    if (collapse_map[?"volume_noise"])
    {
        tab_collapse_start()
	tab_control_dragger()
	draw_dragger("frameeditorvolumenoise", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_NOISE_AMOUNT] * 100, 1, 0, 100, 0, 1, tab.volume.tbx_noise, action_tl_frame_volume_noise)
	tab_next()
	tab_control_dragger()
	draw_dragger("frameeditorvolumenoisescale", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_NOISE_SCALE], 0.05, 0.01, no_limit, 0, 0.05, tab.volume.tbx_noise_scale, action_tl_frame_volume_noise_scale)
	tab_next()
    tab_control_switch()
    draw_switch("frameeditorvolumenoiseworld", dx, dy, tl_edit.value[e_value.VOLUME_NOISE_WORLD], action_tl_frame_volume_noise_world)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumedetail", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_NOISE_DETAIL] * 100, 1, 0, 100, 0, 1, tab.volume.tbx_detail, action_tl_frame_volume_detail)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumecontrast", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_NOISE_CONTRAST], 0.05, 0, 10, 1, 0.05, tab.volume.tbx_contrast, action_tl_frame_volume_contrast)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumethreshold", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_NOISE_THRESHOLD] * 100, 1, 0, 100, 0, 1, tab.volume.tbx_threshold, action_tl_frame_volume_threshold)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumesoftness", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_NOISE_SOFTNESS] * 100, 1, 0, 100, 25, 1, tab.volume.tbx_softness, action_tl_frame_volume_softness)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumeseed", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_NOISE_SEED], 1, 0, 999999, 0, 1, tab.volume.tbx_seed, action_tl_frame_volume_seed)
    tab_next()
        tab_collapse_end(false)
    }
    tab_control_switch()
    draw_button_collapse("volume_motion", collapse_map[?"volume_motion"], null, true, "frameeditorvolumemotiongroup")
    tab_next()
    if (collapse_map[?"volume_motion"])
    {
        tab_collapse_start()
    tab_control_dragger()
    draw_dragger("frameeditorvolumewindspeed", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_WIND_SPEED], 0.01, -1000, 1000, 0, 0.01, tab.volume.tbx_wind_speed, action_tl_frame_volume_wind_speed)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumewindx", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_WIND_X], 0.05, -10, 10, 1, 0.05, tab.volume.tbx_wind_x, action_tl_frame_volume_wind_x)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumewindy", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_WIND_Y], 0.05, -10, 10, 0, 0.05, tab.volume.tbx_wind_y, action_tl_frame_volume_wind_y)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumewindz", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_WIND_Z], 0.05, -10, 10, 0, 0.05, tab.volume.tbx_wind_z, action_tl_frame_volume_wind_z)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumewindinfluence", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_WIND_INFLUENCE] * 100, 1, 0, 100, 100, 1, tab.volume.tbx_wind_influence, action_tl_frame_volume_wind_influence)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumeturbulence", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_TURBULENCE] * 100, 1, 0, 100, 0, 1, tab.volume.tbx_turbulence, action_tl_frame_volume_turbulence)
    tab_next()
    tab_control_dragger()
    draw_dragger("frameeditorvolumeturbulencescale", dx, dy, dragger_width, tl_edit.value[e_value.VOLUME_TURBULENCE_SCALE], 0.05, 0.01, 100, 2.5, 0.05, tab.volume.tbx_turbulence_scale, action_tl_frame_volume_turbulence_scale)
    tab_next()
        tab_collapse_end(false)
    }
}
