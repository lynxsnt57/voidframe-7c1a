/// render_world_block_map(modelmap, resource)
/// @arg modelmap
/// @arg resource
/// @desc Renders each vbuffer in the given map, with the key as the chosen texture from the resource.

function render_world_block_map(modelmap, res, meshmodel = null)
{
	var key;
	
	if (modelmap = null)
		return 0
	
	key = ds_map_find_first(modelmap)
	while (!is_undefined(key))
	{
		var vbuffer = modelmap[?key];
		if (!vbuffer_is_empty(vbuffer))
		{
			var tex;
			with (res)
				tex = res_get_model_texture(key)
			render_set_texture(tex)
			// Only experimental meshes carry this per-group SEUS material map.
			if (meshmodel != null && meshmodel.model_texture_material_map != null && filename_ext(meshmodel.filename) = ".mimesh")
			{
				var matkey = key + "_material"
				if (ds_map_exists(meshmodel.model_texture_material_map, matkey))
				{
					render_set_texture(meshmodel.model_texture_material_map[?matkey], "Material")
					render_set_uniform_int("uMaterialFormat", e_material.FORMAT_SEUS)
				}
				else
				{
					render_set_texture(spr_default_material, "Material")
					render_set_uniform_int("uMaterialFormat", e_material.FORMAT_NONE)
				}
			}
			vbuffer_render(vbuffer)
		}
		
		key = ds_map_find_next(modelmap, key)	
	}
	if (meshmodel != null && filename_ext(meshmodel.filename) = ".mimesh")
	{
		render_set_texture(spr_default_material, "Material")
		render_set_uniform_int("uMaterialFormat", e_material.FORMAT_NONE)
	}
}
