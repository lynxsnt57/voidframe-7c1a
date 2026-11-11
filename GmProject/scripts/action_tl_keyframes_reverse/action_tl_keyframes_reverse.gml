/// action_tl_keyframes_reverse()
/// @desc Reverses the selected keyframes.

function action_tl_keyframes_reverse()
{
	if (history_undo)
	{
		with (history_data)
		{
			tl_keyframes_remove()
			history_restore_keyframes()
			history_restore_tl_select()
		}
	}
	else
	{
		if (!history_redo)
		{
			with (history_set(action_tl_keyframes_reverse))
			{
				history_save_keyframes()
				history_save_tl_select()
			}
		}
		
		tl_keyframes_reverse()
	}
	
	with (obj_timeline)
		tl_update_values()
	
	tl_update_matrix()
	tl_update_length()
	
	app_update_tl_edit()
}