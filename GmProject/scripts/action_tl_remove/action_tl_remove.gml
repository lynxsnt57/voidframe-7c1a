/// action_tl_remove()
/// @desc Removes all selected timelines.

function action_tl_remove()
{
	// Model parts can be referenced by skin weights and can own child objects.
	// Ask only for the original action, never while undoing or redoing it.
	if (!history_undo && !history_redo)
	{
		var removing_part = false;
		if (list_item_value != null && !list_item_value.selected)
			removing_part = (list_item_value.part_of != null)
		else
		{
			with (obj_timeline)
				if (selected && part_of != null)
					removing_part = true
		}
		if (removing_part && !show_question("Delete the selected model parts and their children? This may break the model or its animation. Save a backup first.\n\nContinue?"))
			return 0
	}
	if (history_undo)
	{
		with (history_data)
		{
			for (var t = 0; t < tl_save_amount; t++)
				history_restore_tl(tl_save_obj[t], null, true)
			history_restore_tl_select()
		}
	}
	else
	{
		var contexttl = null;
		
		if (!history_redo)
		{
			with (history_set(action_tl_remove))
			{
				tl_save_amount = 0
				history_save_tl_select()
				tl_context_save_id = (list_item_value = null ? null : list_item_value.save_id)
				
				// "Select" right-clicked object for removal
				if (list_item_value != null && !list_item_value.selected)
				{
					with (list_item_value)
						tl_select_single()
				}
				
				history_save_tl_tree(app)
				history_restore_tl_select()
			}
			
			contexttl = save_id_find(history[0].tl_context_save_id)
		}
		else
			contexttl = save_id_find(history_data.tl_context_save_id)
		
		// Get timelines ready for deletion
		if (contexttl = null || contexttl.selected)
		{
			with (obj_timeline)
				if (selected && !delete_ready)
					tl_remove_clean()
			
			tl_deselect_all()
		}
		else
		{
			with (contexttl)
				if (!delete_ready)
					tl_remove_clean()
		}
		
		with (obj_timeline)
			if (delete_ready)
				instance_destroy()
	}
	
	project_ik_part_array = null
	
	tl_update_list()
	tl_update_length()
	tl_update_matrix()
	
	app_update_tl_edit()
}
