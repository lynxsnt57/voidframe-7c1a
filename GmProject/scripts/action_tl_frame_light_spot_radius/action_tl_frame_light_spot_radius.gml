/// action_tl_frame_light_spot_radius(value, add)
/// @arg value
/// @arg add

function action_tl_frame_light_spot_radius(val, add)
{
	tl_value_set_start(action_tl_frame_light_spot_radius, true)
	tl_value_set(e_value.LIGHT_SPOT_RADIUS, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_offset_x(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_offset_x, true)
	tl_value_set(e_value.LIGHT_GOBO_OFFSET_X, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_offset_y(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_offset_y, true)
	tl_value_set(e_value.LIGHT_GOBO_OFFSET_Y, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_repeat_x(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_repeat_x, true)
	tl_value_set(e_value.LIGHT_GOBO_REPEAT_X, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_repeat_y(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_repeat_y, true)
	tl_value_set(e_value.LIGHT_GOBO_REPEAT_Y, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_enabled(enable)
{
	tl_value_set_start(action_tl_frame_light_gobo_enabled, false)
	tl_value_set(e_value.LIGHT_GOBO_ENABLED, enable, false)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_tiling(enable)
{
	tl_value_set_start(action_tl_frame_light_gobo_tiling, false)
	tl_value_set(e_value.LIGHT_GOBO_TILING, enable, false)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_invert(enable)
{
	tl_value_set_start(action_tl_frame_light_gobo_invert, false)
	tl_value_set(e_value.LIGHT_GOBO_INVERT, enable, false)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_rotation(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_rotation, true)
	tl_value_set(e_value.LIGHT_GOBO_ROTATION, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_intensity(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_intensity, true)
	tl_value_set(e_value.LIGHT_GOBO_INTENSITY, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_softness(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_softness, true)
	tl_value_set(e_value.LIGHT_GOBO_SOFTNESS, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_feather(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_feather, true)
	tl_value_set(e_value.LIGHT_GOBO_FEATHER, val / 100, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_scroll_x(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_scroll_x, true)
	tl_value_set(e_value.LIGHT_GOBO_SCROLL_X, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_scroll_y(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_scroll_y, true)
	tl_value_set(e_value.LIGHT_GOBO_SCROLL_Y, val, add)
	tl_value_set_done()
}

function action_tl_frame_light_gobo_rot_speed(val, add)
{
	tl_value_set_start(action_tl_frame_light_gobo_rot_speed, true)
	tl_value_set(e_value.LIGHT_GOBO_ROT_SPEED, val, add)
	tl_value_set_done()
}
