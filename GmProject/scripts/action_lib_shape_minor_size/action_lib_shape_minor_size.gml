/// action_lib_shape_minor_size(value, add)
/// @arg value
/// @arg add

function action_lib_shape_minor_size(val, add)
{
	if (!history_undo && !history_redo)
		history_set_var(action_lib_shape_minor_size, temp_edit.shape_minor_size, temp_edit.shape_minor_size * add + val, true)
	
	with (temp_edit)
	{
		shape_minor_size = shape_minor_size * add + val
		temp_update_shape()
	}
	
	lib_preview.update = true
}
