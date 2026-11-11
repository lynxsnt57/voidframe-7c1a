/// render_world_start_light(from, to, sampleoffset, timeline]
/// @arg from
/// @arg to
/// @arg sampleoffset
/// @arg timeline
/// @desc Render the scene from the light's point of view.

function render_world_start_light(from, to, offset, tl)
{
	render_light_from = from
	render_light_to = to
	render_light_offset = offset
	render_light_near = 1
	render_light_far = tl.value[e_value.LIGHT_RANGE]
	render_light_color = tl.value[e_value.LIGHT_COLOR]
	render_light_strength = tl.value[e_value.LIGHT_STRENGTH]
	render_light_specular_strength = tl.value[e_value.LIGHT_SPECULAR_STRENGTH]
	render_light_size = tl.value[e_value.LIGHT_SIZE]
	render_light_shadow_blur = tl.value[e_value.LIGHT_SHADOW_BLUR]
	render_light_fade_size = tl.value[e_value.LIGHT_FADE_SIZE]
	render_light_tl = tl.object_tag
	
	if (tl.type = e_tl_type.POINT_LIGHT)
	{
		if (!proj_depth_paraboloid)
			render_light_fov = 90
		else
			render_light_fov = 180
	}
	else if (tl.type = e_tl_type.SPOT_LIGHT)
	{
		render_light_fov = tl.value[e_value.LIGHT_SPOT_RADIUS]
		render_light_spot_sharpness = tl.value[e_value.LIGHT_SPOT_SHARPNESS]
		render_gobo_sprite = spr_shape
		render_gobo_mode = 0
		render_gobo_offset[X] = tl.value[e_value.LIGHT_GOBO_OFFSET_X]
		render_gobo_offset[Y] = tl.value[e_value.LIGHT_GOBO_OFFSET_Y]
		render_gobo_repeat[X] = tl.value[e_value.LIGHT_GOBO_REPEAT_X]
		render_gobo_repeat[Y] = tl.value[e_value.LIGHT_GOBO_REPEAT_Y]
		render_gobo_tiling = tl.value[e_value.LIGHT_GOBO_TILING]
		render_gobo_invert = tl.value[e_value.LIGHT_GOBO_INVERT]
		render_gobo_intensity = clamp(tl.value[e_value.LIGHT_GOBO_INTENSITY], 0, 1)
		render_gobo_softness = clamp(tl.value[e_value.LIGHT_GOBO_SOFTNESS], 0, 1)
		render_gobo_feather = clamp(tl.value[e_value.LIGHT_GOBO_FEATHER], 0, 1)
		var gobo_time = 0;
		if (app.project_tempo > 0) gobo_time = app.timeline_marker / app.project_tempo
		render_gobo_offset[X] += tl.value[e_value.LIGHT_GOBO_SCROLL_X] * gobo_time * 2 / max(render_gobo_repeat[X], 0.0001)
		render_gobo_offset[Y] += tl.value[e_value.LIGHT_GOBO_SCROLL_Y] * gobo_time * 2 / max(render_gobo_repeat[Y], 0.0001)
		render_gobo_rotation = (tl.value[e_value.LIGHT_GOBO_ROTATION] + tl.value[e_value.LIGHT_GOBO_ROT_SPEED] * gobo_time) * pi / 180
		var gobo = tl.value[e_value.TEXTURE_OBJ];
		if (tl.value[e_value.LIGHT_GOBO_ENABLED] && gobo != null && instance_exists(gobo))
		{
			if (gobo.object_index = obj_resource && sprite_exists(gobo.texture))
			{
				render_gobo_sprite = gobo.texture
				render_gobo_mode = 1
			}
			else if (gobo.object_index = obj_timeline && gobo.type = e_tl_type.VIDEO)
			{
				render_gobo_mode = 2
				if (gobo.video_path != "" && gobo.video_speed > 0 && app.project_tempo > 0)
				{
					var duration = (gobo.video_source_out - gobo.video_source_in) / gobo.video_speed;
					var elapsed = (app.timeline_marker - gobo.video_start_frame) / app.project_tempo;
					if (duration > 0)
					{
						if (elapsed < 0 && gobo.video_hold_outside) elapsed = 0
						if (elapsed > duration && gobo.video_loop) elapsed = elapsed mod duration
						else if (elapsed > duration && gobo.video_hold_outside) elapsed = duration
						if (elapsed >= 0 && elapsed <= duration)
						{
							var frame = video_frame_sprite(gobo.video_path, gobo.video_source_in + elapsed * gobo.video_speed);
							if (frame >= 0 && sprite_exists(frame))
							{
								render_gobo_sprite = frame
								render_gobo_mode = 1
							}
						}
					}
				}
			}
		}

	}
	
	gpu_set_ztestenable(true)
	
	// Get origin matrix for spotlight
	if (tl.type = e_tl_type.SPOT_LIGHT)
	{
		render_set_projection(render_light_from, render_light_to, vec3(0, 0, 1), render_light_fov, 1, 1, render_light_far)
		spot_proj_matrix = matrix_get(matrix_projection)
		spot_view_matrix = matrix_get(matrix_view)
		spot_view_proj_matrix = matrix_multiply(spot_view_matrix, spot_proj_matrix)
		
		render_spot_matrix = spot_view_proj_matrix
	}
	
	render_set_projection(point3D_add(render_light_from, render_light_offset), point3D_add(render_light_to, render_light_offset), vec3(0, 0, 1), render_light_fov, 1, 1, render_light_far)
	
	render_proj_from = point3D_add(render_light_from, render_light_offset)
	render_shadow_from = render_proj_from
	light_proj_matrix = matrix_get(matrix_projection)
	light_view_matrix = matrix_get(matrix_view)
	light_view_proj_matrix = matrix_multiply(light_view_matrix, light_proj_matrix)
	
	proj_depth_near = render_light_near
	proj_depth_far = render_light_far
	
	render_shadow_matrix = light_view_proj_matrix
}