/// action_tl_glow(enable)
/// @arg enable

function action_tl_glow(enable)
{
	if (history_undo)
	{
		with (history_data)
			for (var t = 0; t < save_var_amount; t++)
				with (save_id_find(save_var_save_id[t]))
					glow = other.save_var_old_value[t]
	}
	else if (history_redo)
	{
		with (history_data)
			for (var t = 0; t < save_var_amount; t++)
				with (save_id_find(save_var_save_id[t]))
					glow = other.save_var_new_value[t]
	}
	else
	{
		var hobj = history_save_var_start(action_tl_glow, false);
		
		with (obj_timeline)
			if (selected)
				action_tl_glow_tree(id, enable, hobj)
	}
}


// Create a normal point light as a child of the selected object. It has no mesh.
function action_emitter_light_create(target = null)
{
    var hobj, light;
    if (history_undo)
    {
        with (history_data)
        {
            with (save_id_find(spawn_save_id))
            {
                tl_remove_clean()
                instance_destroy()
            }
        }
    }
    else
    {
        if (history_redo)
        {
            hobj = history_data
            target = save_id_find(hobj.emitter_parent_save_id)
        }
        else
        {
            if (target = null || !instance_exists(target)) return 0
            hobj = history_set(action_emitter_light_create)
            hobj.emitter_parent_save_id = save_id_get(target)
        }
        if (target = null || !instance_exists(target)) return 0
        light = new_tl(e_tl_type.POINT_LIGHT)
        with (light)
        {
            name = "Object light"
            emitter_attached = true
            shadows = true
            tl_set_parent(target)
            tl_update_display_name()
        }
        if (history_redo)
            light.save_id = hobj.spawn_save_id
        else
            hobj.spawn_save_id = save_id_get(light)
    }
    tl_update_list()
    tl_update_matrix()
}
