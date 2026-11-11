/// action_bench_create([edit])
/// @arg [edit]

function action_bench_create(edit = false)
{
	if (!history_undo && !history_redo && bench_settings.type = e_tl_type.VIDEO &&
		(bench_video_path = "" || !file_exists_lib(bench_video_path) || bench_video_duration <= 0))
	{
		error("videoselectfilefirst")
		return 0
	}
	
	if (history_undo)
	{
		with (history_data)
		{
			for (var s = 0; s < spawn_amount; s++)
			{
				with (save_id_find(spawn_save_id[s]))
				{
					if (object_index = obj_timeline)
						tl_remove_clean()
					
					instance_destroy()
				}
			}
		}
	}
	else
	{
		var hobj, tl;
		hobj = null
		var video_create = false
		
		if (history_redo)
		{
			history_restore_bench(history_data.bench_save_obj)
			
			if (history_data.open_editor)
			{
				tab_template_editor_update_ptype_list()
				tab_show(template_editor)
			}
		}
		else
		{
			if (bench_settings.type = e_tl_type.VIDEO)
				log("VIDEO_DIAG 01 before history_set")
			hobj = history_set(action_bench_create)
			if (bench_settings.type = e_tl_type.VIDEO)
				log("VIDEO_DIAG 02 before history_save_bench")
			hobj.bench_save_obj = history_save_bench()
			if (bench_settings.type = e_tl_type.VIDEO)
				log("VIDEO_DIAG 03 after history_save_bench")
			hobj.spawn_amount = 0
			hobj.open_editor = edit
			if (bench_settings.type = e_tl_type.VIDEO)
			{
				hobj.video_path = bench_video_path
				hobj.video_duration = bench_video_duration
				hobj.video_source_in = bench_video_source_in
				hobj.video_source_out = bench_video_source_out
				hobj.video_speed = bench_video_speed
				hobj.video_loop = bench_video_loop
				hobj.video_hold_outside = bench_video_hold_outside
			}
			
			if (edit)
				tab_show(template_editor)
		}
		video_create = (bench_settings.type == e_tl_type.VIDEO)
		
		if (type_is_timeline(bench_settings.type) && !video_create) // Timeline
		{
			tl = new_tl(bench_settings.type = e_tl_type.LIGHT_SOURCE ? bench_settings.light_type : bench_settings.type)
			with (hobj)
			{
				spawn_save_id[spawn_amount] = tl.save_id
				spawn_amount++
			}
			
			if (bench_settings.type = e_tl_type.CAMERA)
				view_second.show = true
		}
		else
		{
			with (bench_settings)
			{
				// The video track uses a surface template for its in-scene screen.
				if (video_create)
				{
					type = e_temp_type.SURFACE
					shape_type = e_shape_type.SURFACE
				}
				
				if (video_create)
					log("VIDEO_DIAG 04 before temp_duplicate")
				var temp = temp_duplicate();
				if (video_create)
					log("VIDEO_DIAG 05 after temp_duplicate")
				if (video_create)
					type = e_tl_type.VIDEO
				
				// Don't copy into template 
				if (temp.type != e_temp_type.CUBE && temp.type != e_temp_type.CYLINDER && temp.type != e_temp_type.CONE) 
					temp.shape_tex_mapped = false
				
				if (temp.type = e_temp_type.PARTICLE_SPAWNER)
					temp.pc_spawn_region_path = null
				
				with (hobj)
				{
					spawn_save_id[spawn_amount] = temp.save_id
					spawn_amount++
				}
				
				with (temp)
				{
					if (video_create)
						log("VIDEO_DIAG 06 before template cleanup")
					if (type != e_temp_type.MODEL && model != null)
					{
						model.count--
						model = null
					}
					
					if (type != e_temp_type.CHARACTER && type != e_temp_type.SPECIAL_BLOCK && type != e_temp_type.BODYPART && type != e_temp_type.MODEL)
					{
						if (model_tex != null)
							model_tex.count--
						
						if (model_tex_material != null)
							model_tex_material.count--
						
						if (model_tex_normal != null)
							model_tex_normal.count--
						
						model_tex = null
						model_tex_material = null
						model_tex_normal = null
						model_file = null
						model_part = null
						model_state = array()
					}
					
					if (type != e_temp_type.ITEM)
					{
						if (item_tex != null)
							item_tex.count--
						
						if (item_tex_material != null)
							item_tex_material.count--
						
						if (item_tex_normal != null)
							item_tex_normal.count--
						
						item_tex = null
						item_tex_material = null
						item_tex_normal = null
					}
					
					if (type != e_temp_type.BLOCK && type != e_temp_type.SCENERY)
					{
						if (block_tex != null)
							block_tex.count--
						block_tex = null
						if (block_tex_material != null)
							block_tex_material.count--
						block_tex_material = null
						if (block_tex_normal != null)
							block_tex_normal.count--
						block_tex_normal = null
						block_state = array()
					}
					
					if (type != e_temp_type.SCENERY && scenery != null)
					{
						scenery.count--
						scenery = null
					}
					
					if (!type_is_shape(type))
					{
						if (shape_tex != null)
						{
							if (shape_tex.type != e_tl_type.CAMERA)
								shape_tex.count--
							shape_tex = null
						}
						
						if (shape_tex_material != null)
						{
							shape_tex_material.count--
							shape_tex_material = null
						}
						
						if (shape_tex_normal != null)
						{
							shape_tex_normal.count--
							shape_tex_normal = null
						}
					}
					
					if (type != e_temp_type.TEXT)
					{
						if (text_font != null)
							text_font.count--
						text_font = null
					}
					
					if (video_create)
						log("VIDEO_DIAG 07 before temp_animate")
					tl = temp_animate()
					if (video_create)
						log("VIDEO_DIAG 08 after temp_animate")
					sortlist_add(app.lib_list, id)
				}
				
				temp_edit = temp
			}
			
			if (video_create)
			{
				log("VIDEO_DIAG 09 before video setup")
				with (tl)
				{
					if (app.history_redo)
					{
						video_path = history_data.video_path
						video_duration = history_data.video_duration
						video_source_in = history_data.video_source_in
						video_source_out = history_data.video_source_out
						video_speed = history_data.video_speed
						video_loop = history_data.video_loop
						video_hold_outside = history_data.video_hold_outside
					}
					type = e_tl_type.VIDEO
					if (!app.history_redo)
					{
						video_path = app.bench_video_path
						video_duration = app.bench_video_duration
						video_source_in = app.bench_video_source_in
						video_source_out = app.bench_video_source_out
						video_speed = app.bench_video_speed
						video_loop = app.bench_video_loop
						video_hold_outside = app.bench_video_hold_outside
					}
					video_start_frame = app.timeline_marker
					video_aspect = video_file_aspect(video_path)
					video_clip_retain(video_path)
					value[e_value.SCA_X] = video_aspect
					value_default[e_value.SCA_X] = video_aspect
					log("VIDEO_DIAG 10 before tl_update")
					tl_update()
					log("VIDEO_DIAG 11 before tl_keyframe_add")
					var video_key = tl_keyframe_add(video_start_frame)
					video_key.value[e_value.SCA_X] = video_aspect
					tl_update_values()
					log("VIDEO_DIAG 12 before video_track_attach_audio")
					video_track_attach_audio(id)
					log("VIDEO_DIAG 13 before tl_update_length")
					tl_update_length()
				}
			}
			
			// Add templates connected to the bench
			with (obj_template)
			{
				if (creator != app.bench_settings)
					continue
				
				sortlist_add(app.lib_list, id)
				creator = app
				
				if (model_tex != null)
					model_tex.count++
				
				if (model_tex_material != null)
					model_tex_material.count++
				
				if (model_tex_normal != null)
					model_tex_normal.count++
				
				if (item_tex != null)
					item_tex.count++
				
				if (item_tex_material != null)
					item_tex_material.count++
				
				if (item_tex_normal != null)
					item_tex_normal.count++
				
				if (block_tex != null)
					block_tex.count++
				
				if (block_tex_material != null)
					block_tex_material.count++
				
				if (block_tex_normal != null)
					block_tex_normal.count++
				
				if (scenery > 0)
					scenery.count++
				
				if (shape_tex != null && shape_tex.type != e_tl_type.CAMERA)
					shape_tex.count++
				
				if (shape_tex_material != null)
					shape_tex_material.count++
				
				if (shape_tex_normal != null)
					shape_tex_normal.count++
				
				if (text_font != null)
					text_font.count++
				
				with (hobj)
				{
					spawn_save_id[spawn_amount] = other.save_id
					spawn_amount++
				}
			}
		}
		
		if (history_redo)
		{
			with (bench_settings)
				temp_particles_type_clear()
			
			with (tl)
			{
				value_default[e_value.POS_X] = history_data.value_default[e_value.POS_X]
				value_default[e_value.POS_Y] = history_data.value_default[e_value.POS_Y]
				value_default[e_value.POS_Z] = history_data.value_default[e_value.POS_Z]
				value_default[e_value.ROT_X] = history_data.value_default[e_value.ROT_X]
				value_default[e_value.ROT_Y] = history_data.value_default[e_value.ROT_Y]
				value_default[e_value.ROT_Z] = history_data.value_default[e_value.ROT_Z]
				value[e_value.POS_X] = value_default[e_value.POS_X]
				value[e_value.POS_Y] = value_default[e_value.POS_Y]
				value[e_value.POS_Z] = value_default[e_value.POS_Z]
				value[e_value.ROT_X] = value_default[e_value.ROT_X]
				value[e_value.ROT_Y] = value_default[e_value.ROT_Y]
				value[e_value.ROT_Z] = value_default[e_value.ROT_Z]
				tl_set_parent(history_data.parent)
			}
		}
		else
		{
			with (hobj)
			{
				value_default[e_value.POS_X] = tl.value_default[e_value.POS_X]
				value_default[e_value.POS_Y] = tl.value_default[e_value.POS_Y]
				value_default[e_value.POS_Z] = tl.value_default[e_value.POS_Z]
				value_default[e_value.ROT_X] = tl.value_default[e_value.ROT_X]
				value_default[e_value.ROT_Y] = tl.value_default[e_value.ROT_Y]
				value_default[e_value.ROT_Z] = tl.value_default[e_value.ROT_Z]
				parent = app
			}
			
			// Start placing
			if (setting_place_new && !keyboard_check(vk_shift) &&
				tl.type != e_tl_type.FOLDER &&
				tl.type != e_tl_type.CAMERA &&
				(tl.type != e_tl_type.SCENERY || tl.temp.scenery != null) &&
				(tl.type != e_tl_type.MODEL || tl.temp.model != null) &&
				tl.value_type[e_value_type.TRANSFORM_POS])
				app_start_place(tl, true)
			
			log("Created", tl_type_name_list[|bench_settings.type])
		}
	}
	
	if (!history_redo && edit)
		tab_template_editor_update_ptype_list()
	
	if (bench_settings.type = e_temp_type.PARTICLE_SPAWNER)
	{
		if (!history_undo && !history_redo)
			bench_update_particles_list()
		
		particle_spawner_clear()
		preview_reset_view()
		update = true
	}
	
	log("VIDEO_NATIVE 20 before timeline list")
	tl_update_list()
	log("VIDEO_NATIVE 21 before timeline matrix")
	tl_update_matrix()
	log("VIDEO_NATIVE 22 after timeline matrix")
	lib_preview.update = true
}
