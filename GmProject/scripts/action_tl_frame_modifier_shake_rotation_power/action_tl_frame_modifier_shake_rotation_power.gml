/// action_tl_frame_modifier_shake_rotation_power(value, add)
/// @arg value
/// @arg add

function action_tl_frame_modifier_shake_rotation_power(val, add)
{
	tl_value_set_start(action_tl_frame_modifier_shake_rotation_power, true)
	tl_value_set(e_value.MODIFIER_SHAKE_ROTATION_POWER, val, add)
	tl_value_set_done()
}
