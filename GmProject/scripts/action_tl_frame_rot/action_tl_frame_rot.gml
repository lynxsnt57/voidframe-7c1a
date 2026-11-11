/// action_tl_frame_rot(value, add)
/// @arg value
/// @arg add

function action_tl_frame_rot(val, add)
{
	tl_value_set_start(action_tl_frame_rot, true)
	tl_value_set(((tl_edit.value[e_value.COPY_ROT_X + axis_edit] && tl_edit.value[e_value.ROT_TARGET] != null) ? e_value.COPY_ROT_OFFSET_X : e_value.ROT_X) + axis_edit, val, add)
	tl_value_set_done()
}
