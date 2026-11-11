/// tab_frame_editor_light()

function tab_frame_editor_light()
{
	context_menu_group_temp = e_context_group.LIGHT
	
	tab_control_color()
	draw_button_color("frameeditorlightcolor", dx, dy, dw, tl_edit.value[e_value.LIGHT_COLOR], c_white, false, action_tl_frame_light_color)
	tab_next()
	
	tab_control_dragger()
	draw_dragger("frameeditorlightsize", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_SIZE], .05, 0, no_limit, 2, .05, tab.light.tbx_size, action_tl_frame_light_size)
	tab_next()
	
	if (project_render_engine)
	{
		tab_control_dragger()
		draw_dragger("frameeditorlightshadowblur", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_SHADOW_BLUR], .05, 0, no_limit, 1, .05, tab.light.tbx_shadow_blur, action_tl_frame_light_shadow_blur)
		tab_next()
	}
	
	tab_control_dragger()
	draw_dragger("frameeditorlightrange", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_RANGE], tl_edit.value[e_value.LIGHT_RANGE] / 100, 0, no_limit, 250, 0, tab.light.tbx_range, action_tl_frame_light_range)
	tab_next()
	
	tab_control_meter()
	draw_meter("frameeditorlightfadesize", dx, dy, dw, floor(tl_edit.value[e_value.LIGHT_FADE_SIZE] * 100), 0, 100, 50, 1, tab.light.tbx_fade_size, action_tl_frame_light_fade_size)
	tab_next()
	
	if (tab.light.has_spotlight)
	{
		tab_control_meter()
		draw_meter("frameeditorlightspotradius", dx, dy, dw, tl_edit.value[e_value.LIGHT_SPOT_RADIUS], 1, 150, 50, 1, tab.light.tbx_spot_radius, action_tl_frame_light_spot_radius)
		tab_next()
		
		tab_control_meter()
		draw_meter("frameeditorlightspotsharpness", dx, dy, dw, floor(tl_edit.value[e_value.LIGHT_SPOT_SHARPNESS] * 100), 0, 100, 50, 1, tab.light.tbx_spot_sharpness, action_tl_frame_light_spot_sharpness)
		tab_next()
	}
	
	tab_control_dragger()
	draw_dragger("frameeditorlightstrength", dx, dy, dragger_width, floor(tl_edit.value[e_value.LIGHT_STRENGTH] * 100), 0.1, 0, no_limit, 100, 1, tab.light.tbx_strength, action_tl_frame_light_strength)
	tab_next()
	
	tab_control_dragger()
	draw_dragger("frameeditorlightspecularstrength", dx, dy, dragger_width, floor(tl_edit.value[e_value.LIGHT_SPECULAR_STRENGTH] * 100), 0.1, 0, no_limit, 100, 1, tab.light.tbx_specular_strength, action_tl_frame_light_specular_strength)
	tab_next()
	
	if (tl_edit.type = e_tl_type.SPOT_LIGHT)
	{
		tab_control_switch()
		draw_button_collapse("light_gobo", collapse_map[?"light_gobo"], action_tl_frame_light_gobo_enabled, tl_edit.value[e_value.LIGHT_GOBO_ENABLED], "frameeditorlightgobo")
		tab_next()
		if (tl_edit.value[e_value.LIGHT_GOBO_ENABLED] && collapse_map[?"light_gobo"])
		{
		tab_collapse_start()
		var gobo = tl_edit.value[e_value.TEXTURE_OBJ];
		var gobo_text = text_get("listdefault", text_get("listnone"));
		var gobo_preview = null;
		if (gobo != null && instance_exists(gobo))
		{
			gobo_text = gobo.display_name
			if (gobo.object_index = obj_resource)
				gobo_preview = gobo.texture
		}
		tab_control_menu(ui_large_height)
		draw_button_menu("frameeditorlightgobotexture", e_menu.LIST, dx, dy, dw, ui_large_height, gobo, gobo_text, action_tl_frame_texture_obj, false, gobo_preview)
		tab_next()
		if (gobo != null && instance_exists(gobo))
		{
			tab_control_checkbox()
			draw_switch("frameeditorlightgobotiling", dx, dy, tl_edit.value[e_value.LIGHT_GOBO_TILING], action_tl_frame_light_gobo_tiling)
			tab_next()
			tab_control_checkbox()
			draw_switch("frameeditorlightgoboinvert", dx, dy, tl_edit.value[e_value.LIGHT_GOBO_INVERT], action_tl_frame_light_gobo_invert)
			tab_next()
			textfield_group_add("frameeditorlightgobohoffset", tl_edit.value[e_value.LIGHT_GOBO_OFFSET_X], 0, action_tl_frame_light_gobo_offset_x, X, tab.light.tbx_gobo_offset_x, null, 0.1, -no_limit, no_limit)
			textfield_group_add("frameeditorlightgobovoffset", tl_edit.value[e_value.LIGHT_GOBO_OFFSET_Y], 0, action_tl_frame_light_gobo_offset_y, Y, tab.light.tbx_gobo_offset_y, null, 0.1, -no_limit, no_limit)
			tab_control_textfield_group(true)
			draw_textfield_group("frameeditorlightgobooffset", dx, dy, dw, null, null, null, snap_min, true, 1, 3)
			tab_next()
			textfield_group_add("frameeditorlightgobohrepeat", tl_edit.value[e_value.LIGHT_GOBO_REPEAT_X], 1, action_tl_frame_light_gobo_repeat_x, X, tab.light.tbx_gobo_repeat_x, null, 0.1, 0, no_limit)
			textfield_group_add("frameeditorlightgobovrepeat", tl_edit.value[e_value.LIGHT_GOBO_REPEAT_Y], 1, action_tl_frame_light_gobo_repeat_y, Y, tab.light.tbx_gobo_repeat_y, null, 0.1, 0, no_limit)
			tab_control_textfield_group(true)
			draw_textfield_group("frameeditorlightgoborepeat", dx, dy, dw, null, null, null, snap_min, true, 1, 3)
			tab_next()
			tab_control_dragger()
			draw_dragger("frameeditorlightgoborotation", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_GOBO_ROTATION], 0.1, -no_limit, no_limit, 0, 0.1, tab.light.tbx_gobo_rotation, action_tl_frame_light_gobo_rotation)
			tab_next()
			tab_control_dragger()
			draw_dragger("frameeditorlightgobointensity", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_GOBO_INTENSITY] * 100, 1, 0, 100, 100, 1, tab.light.tbx_gobo_intensity, action_tl_frame_light_gobo_intensity)
			tab_next()
			tab_control_dragger()
			draw_dragger("frameeditorlightgobosoftness", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_GOBO_SOFTNESS] * 100, 1, 0, 100, 0, 1, tab.light.tbx_gobo_softness, action_tl_frame_light_gobo_softness)
			tab_next()
			tab_control_dragger()
			draw_dragger("frameeditorlightgobofeather", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_GOBO_FEATHER] * 100, 1, 0, 100, 0, 1, tab.light.tbx_gobo_feather, action_tl_frame_light_gobo_feather)
			tab_next()
			tab_control_dragger()
			draw_dragger("frameeditorlightgoboscrollx", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_GOBO_SCROLL_X], 0.01, -no_limit, no_limit, 0, 0.01, tab.light.tbx_gobo_scroll_x, action_tl_frame_light_gobo_scroll_x)
			tab_next()
			tab_control_dragger()
			draw_dragger("frameeditorlightgoboscrolly", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_GOBO_SCROLL_Y], 0.01, -no_limit, no_limit, 0, 0.01, tab.light.tbx_gobo_scroll_y, action_tl_frame_light_gobo_scroll_y)
			tab_next()
			tab_control_dragger()
			draw_dragger("frameeditorlightgoborotspeed", dx, dy, dragger_width, tl_edit.value[e_value.LIGHT_GOBO_ROT_SPEED], 0.1, -no_limit, no_limit, 0, 0.1, tab.light.tbx_gobo_rot_speed, action_tl_frame_light_gobo_rot_speed)
			tab_next()
		}
		tab_collapse_end()
		}
	}

	context_menu_group_temp = null
}
