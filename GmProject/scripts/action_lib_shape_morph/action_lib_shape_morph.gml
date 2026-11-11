/// action_lib_shape_morph(value, add)
/// @arg value
/// @arg add

function action_lib_shape_morph(val, add)
{
	if (!history_undo && !history_redo)
		history_set_var(action_lib_shape_morph, temp_edit.shape_morph, temp_edit.shape_morph * add + val, true)
	
	with (temp_edit)
	{
		shape_morph = shape_morph * add + val
		temp_update_shape()
	}
	
	lib_preview.update = true
}
