/// action_tl_frame_modifier_shake_offset_automatic(enable)
/// @arg enable

function action_tl_frame_modifier_shake_offset_automatic(enable)
{
	tl_value_set_start(action_tl_frame_modifier_shake_offset_automatic, false)
	tl_value_set(e_value.MODIFIER_SHAKE_OFFSET_AUTOMATIC, enable, false)
	tl_value_set_done()
}
