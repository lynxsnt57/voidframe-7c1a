/// action_lib_shape_subdivision(value, add)
/// @arg value
/// @arg add

function action_lib_shape_subdivision(val, add)
{
	if (!history_undo && !history_redo)
		history_set_var(action_lib_shape_subdivision, temp_edit.shape_subdivision, temp_edit.shape_subdivision * add + val, true)
	
	with (temp_edit)
	{
		shape_subdivision = shape_subdivision * add + val
		temp_update_shape()
	}
	
	lib_preview.update = true
}
