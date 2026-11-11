/// recent_load()

function recent_load(minimal = false)
{
	if (!file_exists_lib(recent_file))
		return 0
	
	var recentmap = json_load(recent_file);
	
	if (recentmap = undefined)
		return 0
	
	recent_destroy()
	
	ds_list_clear(recent_list)
	var recentlist = recentmap[?"list"];
	
	for (var i = 0; i < min(ds_list_size(recentlist), 108); i++)
	{
		var projectmap = recentlist[|i];
		
		if (ds_map_valid(projectmap))
		{
			var recentobj = new_obj(obj_recent);
			
			recentobj.name = value_get_string(projectmap[?"name"], "")
			recentobj.author = value_get_string(projectmap[?"author"], "")
			recentobj.description = value_get_string(projectmap[?"description"], "")
			
			recentobj.filename = value_get_string(projectmap[?"filename"], "")
			recentobj.last_opened = value_get_real(projectmap[?"last_opened"], -1)
			recentobj.pinned = value_get_real(projectmap[?"pinned"], false)
			
			if (!minimal) {
				var thumbnailfn = filename_path(recentobj.filename) + "thumbnail.png";
				if (file_exists_lib(thumbnailfn))
					recentobj.thumbnail = texture_create(thumbnailfn)
				else
					recentobj.thumbnail = null
			} else {
				recentobj.thumbnail = null
			}
			
			ds_list_add(recent_list, recentobj)
		}
	}
	
	recent_list_amount = ds_list_size(recent_list)
	recent_update()
}
