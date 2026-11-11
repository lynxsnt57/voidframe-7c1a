/// action_lib_shape_major_size(value, add)
/// @arg value
/// @arg add

function action_lib_shape_major_size(val, add)
{
	if (!history_undo && !history_redo)
		history_set_var(action_lib_shape_major_size, temp_edit.shape_major_size, temp_edit.shape_major_size * add + val, true)
	
	with (temp_edit)
	{
		shape_major_size = shape_major_size * add + val
		temp_update_shape()
	}
	
	lib_preview.update = true
}
