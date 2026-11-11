/// action_tl_frame_modifier_shake_position_power(value, add)
/// @arg value
/// @arg add

function action_tl_frame_modifier_shake_position_power(val, add)
{
	tl_value_set_start(action_tl_frame_modifier_shake_position_power, true)
	tl_value_set(e_value.MODIFIER_SHAKE_POSITION_POWER, val, add)
	tl_value_set_done()
}
