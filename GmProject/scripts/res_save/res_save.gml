/// res_save()
/// @desc Copies the file into the project directory.

function res_save()
{
	if (type != e_res_type.FROM_WORLD)
		file_copy_lib(load_folder + "/" + filename, save_folder + "/" + filename)
	
	// Save model's own textures
	if (type = e_res_type.MODEL)
	{
		// Copy internal geometry alongside the .mimodel on import and Save As.
		if (model_format = e_model_format.MIMODEL && model_file != null)
		{
			for (var p = 0; p < ds_list_size(model_file.file_part_list); p++)
			{
				var part = model_file.file_part_list[|p]
				if (part.shape_list != null)
				{
					for (var s = 0; s < ds_list_size(part.shape_list); s++)
					{
						var shape = part.shape_list[|s]
						if (shape.type = "glb_mesh")
						{
							directory_create_lib(save_folder + "/" + filename_dir(shape.mesh_filename))
							file_copy_lib(load_folder + "/" + shape.mesh_filename, save_folder + "/" + shape.mesh_filename)
							if (shape.skin_filename != "")
								file_copy_lib(load_folder + "/" + shape.skin_filename, save_folder + "/" + shape.skin_filename)
						}
					}
				}
			}
		}
		var name, key;
		name = filename_new_ext(filename, "")
		
		if (model_texture_map != null)
		{
			key = ds_map_find_first(model_texture_map)
			while (!is_undefined(key))
			{
				var fn = key;
				if (model_format = e_model_format.BLOCK) // Export to a folder with the model's name
					fn = name + "/" + key + ".png"
			
				directory_create_lib(save_folder + "/" + filename_dir(fn))
				texture_export(model_texture_map[?key], save_folder + "/" + fn)
			
				key = ds_map_find_next(model_texture_map, key)
			}
		}
		
		if (model_texture_material_map != null)
		{
			key = ds_map_find_first(model_texture_material_map)
			while (!is_undefined(key))
			{
				var fn = key;
				if (model_format = e_model_format.BLOCK) // Export to a folder with the model's name
					fn = name + "/" + key + ".png"
			
				directory_create_lib(save_folder + "/" + filename_dir(fn))
				texture_export(model_texture_material_map[?key], save_folder + "/" + fn)
			
				key = ds_map_find_next(model_texture_material_map, key)
			}
		}
		
		if (model_tex_normal_map != null)
		{
			key = ds_map_find_first(model_tex_normal_map)
			while (!is_undefined(key))
			{
				var fn = key;
				if (model_format = e_model_format.BLOCK) // Export to a folder with the model's name
					fn = name + "/" + key + ".png"
			
				directory_create_lib(save_folder + "/" + filename_dir(fn))
				texture_export(model_tex_normal_map[?key], save_folder + "/" + fn)
			
				key = ds_map_find_next(model_tex_normal_map, key)
			}
		}
	}
}