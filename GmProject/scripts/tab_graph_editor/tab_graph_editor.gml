/// Synchronized time, independent objects/groups/value scales.
function tab_graph_editor()
{
 if (graph_preferences_open) { graph_preferences_panel(); return 0 }
 if (ga_tools) { ga_tools_panel(); return 0 }
	var ox = content_x, oy = content_y, ow = content_width, oh = content_height;
	graph_focus_x = ox
	graph_focus_y = oy
	graph_focus_w = ow
	graph_focus_h = oh
	var blocked = popup_mouseon || toast_mouseon || context_menu_mouseon;
	content_mouseon = !blocked && app_mouse_box(ox, oy, ow, oh, "place")
	draw_box(ox, oy, ow, 32, false, c_level_middle, 1)
	if (!mouse_left && !mouse_left_released) graph_pressed_control = ""
	var bx = ox + 8;
	for (var n = 1; n <= 3; n++)
	{
		if (graph_button(string(n) + (n == 1 ? " view" : " views"), bx, oy + 4, 64, graph_layout_count == n, false, "Independent graph views; enlarge panel vertically for more views")) graph_layout_count = n
		bx += 68
	}
	// Settings is always reachable; secondary controls collapse on narrow panels.
	if (ow >= 320 && graph_button(ow < 600 ? "Prefs" : "Settings", ox + ow - 80, oy + 4, 72, graph_preferences_open, false, "Appearance, interaction and display preferences")) { graph_options_capture(); graph_preferences_open = true }
	if (ow >= 720)
	{
		if (graph_button("Undo", ox + ow - 368, oy + 4, 52, false, history_pos >= history_amount, "Shared project history")) action_toolbar_undo()
		if (graph_button("Redo", ox + ow - 312, oy + 4, 52, false, history_pos <= 0, "Shared project history")) action_toolbar_redo()
		if (graph_button("Grid", ox + ow - 252, oy + 4, 48, graph_show_grid, false, "Toggle grid")) graph_show_grid = !graph_show_grid
		if (graph_button("Snap", ox + ow - 200, oy + 4, 48, graph_snap_value, false, "Value delta snapping; interval in Settings")) graph_snap_value = !graph_snap_value
		if (graph_button("Ghost", ox + ow - 148, oy + 4, 60, graph_show_ghost, false, "Show captured reference; capture in Tools")) graph_show_ghost = !graph_show_ghost
	}
	var actual = min(graph_layout_count, max(1, floor((oh - 32)/220)));
	if (actual < graph_layout_count && ow > 500 && ow < 720)
		draw_label("Need " + string(graph_layout_count*220 + 32) + " px high", bx + 8, oy + 16, fa_left, fa_middle, c_text_secondary, 1, font_label)
	// Resizing must not strand a captured drag in a hidden viewport.
	if (window_busy == "graphdrag" && (graph_active_view >= actual || oh < 252))
	{
		graph_view_load(graph_active_view)
		graph_target = graph_timeline
		graph_drag_release()
		graph_view_save(graph_active_view)
	}
	var laneh = floor((oh - 32)/actual);
	for (var view = 0; view < actual; view++)
	{
		graph_view_index = view
		graph_view_load(view)
		content_x = ox
		content_y = oy + 32 + view*laneh
		content_width = ow
		content_height = view == actual - 1 ? oh - 32 - view*laneh : laneh
		graph_panel_editor()
		graph_view_save(view)
		if (!graph_mode) break
	}
	content_x = ox
	content_y = oy
	content_width = ow
	content_height = oh
}

/// Native timeline replacement view. No webview or external dependencies.
function graph_panel_editor()
{
	var xx = content_x, yy = content_y, wid = content_width, hei = content_height;
	var side = clamp(graph_sidebar_width, 168, max(168, min(420, wid - 200)));
	var gx = xx + side + 64, gy = yy + 76, gw = wid - side - 76, gh = hei - 104;
	// The divider captures its drag independently from plot gestures.
	if (!popup_mouseon && !context_menu_mouseon && !toast_mouseon && window_busy == "" && app_mouse_box(xx + side - 4, yy + 32, 8, hei - 32))
	{
		mouse_cursor = cr_size_we
		if (mouse_left_pressed) { graph_sidebar_resizing = true; graph_active_view = graph_view_index; window_busy = "graphresize" }
	}
	if (graph_sidebar_resizing && graph_active_view == graph_view_index)
	{
		graph_sidebar_width = clamp(mouse_x - xx, 168, max(168, min(420, wid - 200)))
		mouse_cursor = cr_size_we
		if (!mouse_left) { graph_sidebar_resizing = false; window_busy = ""; app_mouse_clear() }
	}
	var blocked = popup_mouseon || toast_mouseon || context_menu_mouseon;
	content_mouseon = !blocked && app_mouse_box(xx, yy, wid, hei, "place")
	draw_box(xx, yy, wid, hei, false, c_level_middle, 1)
	draw_box(xx + side, yy + 32, wid - side, hei - 32, false, c_level_bottom, 1)
	draw_divide(xx, yy + 32, wid)
	draw_divide_vertical(xx + side, yy, hei)
	draw_box(xx, yy + hei - 28, wid, 28, false, c_level_middle, 1)
	draw_divide(xx, yy + hei - 28, wid)
	if (graph_active_view == graph_view_index) draw_box(xx, yy, 3, 32, false, c_accent, 0.8)
	if (content_mouseon && window_busy == "") graph_active_view = graph_view_index

	// A stable switch at the same position as in the normal timeline.
	if (graph_toggle_button(xx + 8, yy + 36)) { graph_close(); return 0 }
	if (graph_button("Tools", xx + side - 60, yy + 4, 56, false, false, "Independent channel keys, Auto tangents, modifiers and editing")) ga_tools_open()
	draw_label("GRAPH " + string(graph_view_index + 1), xx + 8, yy + 16, fa_left, fa_middle, graph_active_view == graph_view_index ? c_accent : c_text_main, 1, font_label)
	if (wid < 380 || hei < 220 || gw < 80)
	{
		draw_label("Enlarge this panel to edit curves", xx + 40, yy + 48, fa_left, fa_middle, c_text_secondary, 1, font_label)
		return 0
	}

	var tl = instance_exists(graph_pin_tl) ? graph_pin_tl : tl_edit;
	if (!instance_exists(tl) || tl.object_index != obj_timeline) tl = null
	graph_target = tl
	if (graph_timeline != tl)
	{
		// Target changed mid-drag: restore touched values instead of leaving them without Undo.
		if (window_busy == "graphdrag" && graph_active_view == graph_view_index)
		{
			if (graph_drag >= 1 && graph_drag <= 3) graph_changes_cancel()
			window_busy = ""
		}
		graph_timeline = tl
		graph_selected = null
		graph_selection_clear()
		graph_fit_pending = true
		graph_drag = 0
		ga_time_delta = 0
	}
	if (instance_exists(graph_selected))
	{
		if (graph_selected.timeline != tl || !ga_has(graph_selected, e_value.POS_X + graph_group*3 + graph_axis)) graph_selected = null
	}
	else graph_selected = null
	graph_selection_clean(tl)
	var available = [false, false, false];
	if (instance_exists(tl))
	{
		available = [tl.value_type[e_value_type.TRANSFORM_POS], tl.value_type[e_value_type.TRANSFORM_ROT], tl.value_type[e_value_type.TRANSFORM_SCA]]
		if (!available[graph_group])
		{
			for (var g = 0; g < 3; g++)
				if (available[g]) { graph_group = g; graph_fit_pending = true; break }
		}
	}
	var count = instance_exists(tl) ? ds_list_size(tl.keyframe_list) : 0;
	var selectedindex = graph_selected != null ? ds_list_find_index(tl.keyframe_list, graph_selected) : -1;
	var vid = e_value.POS_X + graph_group * 3 + graph_axis;
	var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X) * 5;
	var canedit = instance_exists(tl) && !tl.lock && available[graph_group];
	var hassegment = canedit && array_length(graph_sel_keys) > 0;
	var currentmode = graph_selected != null && selectedindex < count - 1 ? graph_selected.value[base] : -1;

	// Header actions adapt to panel width instead of overlapping the sidebar.
	var bx = xx + side + 8;
	var compact = wid - side < 500;
	if (draw_button_icon("graphplay", bx, yy + 4, 24, 24, timeline_playing, timeline_playing ? icons.PAUSE : icons.PLAY, null, false))
	{
		action_tl_play()
	}
	bx += 28
	draw_button_icon("graphstop", bx, yy + 4, 24, 24, false, icons.STOP, action_tl_play_stop, false)
	bx += 32
	if (graph_button("Fit", bx, yy + 4, compact ? 32 : 40, false, false, "Frame the visible channels")) graph_fit_pending = true
	bx += compact ? 36 : 44
	if (graph_button(compact ? "MI" : "Legacy", bx, yy + 4, compact ? 24 : 58, currentmode == 0, !hassegment, "Use the original shared Mine-imator transition")) graph_set_mode(0)
	bx += compact ? 28 : 62
	if (bx + (compact ? 28 : 52) < xx + wid && graph_button(compact ? "Lin" : "Linear", bx, yy + 4, compact ? 28 : 52, currentmode == 1, !hassegment, "Linear outgoing segment for this axis only")) graph_set_mode(1)
	bx += compact ? 32 : 56
	if (bx + (compact ? 30 : 56) < xx + wid && graph_button(compact ? "Bez" : "Bezier", bx, yy + 4, compact ? 30 : 56, currentmode == 2, !hassegment, "Editable independent time/value handles")) graph_set_mode(2)
	bx += compact ? 34 : 60
	if (bx + (compact ? 32 : 44) < xx + wid && graph_button("Step", bx, yy + 4, compact ? 32 : 44, currentmode == 3, !hassegment, "Hold value until the next key")) graph_set_mode(3)
	bx += compact ? 36 : 48
	if (bx + (compact ? 30 : 44) < xx + wid && graph_button("Flat", bx, yy + 4, compact ? 30 : 44, false, !hassegment, "Set the outgoing segment's two tangents horizontal")) graph_flat_handles()

	// Scrollable sidebar keeps the inspector reachable in low multi-view layouts.
	var sidebarmax = setting_graph_options[25] != 0 ? max(0, 452 - hei) : max(0, 288 - hei);
	graph_sidebar_scroll[graph_view_index] = clamp(graph_sidebar_scroll[graph_view_index], 0, sidebarmax)
	if (content_mouseon && !textbox_isediting && window_busy == "" && app_mouse_box(xx, yy + 64, side - 4, hei - 92) && mouse_wheel != 0)
		graph_sidebar_scroll[graph_view_index] = clamp(graph_sidebar_scroll[graph_view_index] + (mouse_wheel == 1 ? -32 : 32), 0, sidebarmax)
	// Channel picker: labels as well as RGB make axes identifiable.
	clip_begin(xx, yy + 32, side, hei - 60)
	var targetname = instance_exists(tl) ? tl.display_name : "Select an object";
	var titlechars = max(2, floor((side - 104)/7));
	draw_label(string_copy(targetname, 1, titlechars), xx + 40, yy + 48, fa_left, fa_middle, c_text_secondary, 1, font_label)
	if (graph_button(instance_exists(graph_pin_tl) ? "Pinned" : "Pin", xx + side - 60, yy + 36, 56, instance_exists(graph_pin_tl), !instance_exists(tl), "Pin this view to its object; other views can follow another selected object"))
		graph_pin_tl = instance_exists(graph_pin_tl) ? null : tl
	clip_end()
	clip_begin(xx, yy + 64, side, hei - 92)
	var labels = ["Position", "Rotation", "Scale"];
	// Accordion tree: all transform groups are discoverable; active group expands.
	var tree_y = yy + 72 - graph_sidebar_scroll[graph_view_index];
	if (hei < 390)
	{
		var compactw = floor((side - 24)/3);
		for (var group = 0; group < 3; group++)
			if (graph_button(group == 0 ? "Pos" : (group == 1 ? "Rot" : "Scale"), xx + 8 + group*(compactw + 4), tree_y, compactw, graph_group == group, !available[group], "Active transform group"))
			{
				graph_group = group
				graph_selection_clear()
				graph_axis = 0
				graph_fit_pending = true
			}
		tree_y += 28
	}

	for (var group = 0; group < 3; group++)
	{
		if (hei < 390 && graph_group != group) continue
		if (hei >= 390 && graph_button((graph_group == group ? "-  " : "+  ") + labels[group], xx + 8, tree_y, side - 16, graph_group == group, !available[group], "Expand transform channels"))
		{
			graph_group = group
			graph_selection_clear()
			graph_axis = 0
			graph_fit_pending = true
		}
		if (hei >= 390) tree_y += 28
		if (graph_group != group) continue
		for (var axis = 0; axis < 3; axis++)
		{
			var axiscolor = graph_axis_color(axis);
			if (graph_button(graph_visible[axis] ? "o" : "-", xx + 16, tree_y, 28, graph_visible[axis], false, "Visibility; Shift+click: show only this axis"))
			{
				if (keyboard_check(vk_shift)) { graph_visible = [false, false, false]; graph_visible[axis] = true }
				else graph_visible[axis] = !graph_visible[axis]
			}
			var channelcount = instance_exists(tl) ? array_length(ga_keys(tl, e_value.POS_X + group*3 + axis)) : 0;
			if (graph_button(labels[group] + " " + chr(ord("X") + axis) + "  (" + string(channelcount) + ")", xx + 48, tree_y, side - 56, graph_axis == axis, !available[group], "Active channel; editing never overwrites other channels"))
			{
				graph_axis = axis
				graph_visible[axis] = true
			}
			draw_box(xx + 48, tree_y + 4, 3, 16, false, axiscolor, 1)
			tree_y += 28
		}
	}
	// Inspector is clipped above the footer, with a compact fallback for short views.
	var inspector_y = tree_y + 8;
	if (true)
	{
		var half = floor((side - 20)/2);
		if (graph_button("+ Key", xx + 8, inspector_y, half, false, !canedit, "Add channel key at playhead")) graph_add_key()
		if (graph_button("Delete", xx + 12 + half, inspector_y, half, false, !canedit || array_length(graph_sel_keys) == 0, "Delete selected channel points only")) graph_remove_key()
		inspector_y += 32
	}
	if (setting_graph_options[25] != 0 && instance_exists(graph_selected) && canedit)
	{
		draw_label("KEYFRAME / " + string(array_length(graph_sel_keys)) + " selected", xx + 12, inspector_y + 10, fa_left, fa_middle, c_text_secondary, 1, font_label)
		inspector_y += 24
		// Keep numeric targets stable while native text/drag fields own input.
		if (content_mouseon && app_mouse_box(xx + 8, max(yy + 64, inspector_y), side - 16, max(0, min(yy + hei - 28, inspector_y + 56) - max(yy + 64, inspector_y))) && mouse_left_pressed)
		{
			graph_numeric_tl = tl
			graph_numeric_kf = graph_selected
			graph_numeric_vid = e_value.POS_X + graph_group*3 + graph_axis
		}
		draw_label("Frame", xx + 12, inspector_y + 12, fa_left, fa_middle, c_text_secondary, 1, font_label)
		// Frame is text-entry only: numeric dragging would create many structural Undo records.
		if (inspector_y >= yy + 64 && inspector_y + 24 <= yy + hei - 28) graph_frame_field(xx + 72, inspector_y, side - 80, graph_selected.position)
		inspector_y += 28
		draw_label("Value", xx + 12, inspector_y + 12, fa_left, fa_middle, c_text_secondary, 1, font_label)
		if (inspector_y >= yy + 64 && inspector_y + 24 <= yy + hei - 28) draw_dragger("graphpointvalue", xx + 72, inspector_y, side - 80, graph_selected.value[e_value.POS_X + graph_group*3 + graph_axis], 1, -no_limit, no_limit, tl_value_default(e_value.POS_X + graph_group*3 + graph_axis), 0.1, graph_tbx_value, action_graph_point_value, 0, false, false, "Exact channel value")
		inspector_y += 32
		if (inspector_y + 24 < yy + hei - 28)
		{
			var third = floor((side - 24)/3);
			if (graph_button("Auto", xx + 8, inspector_y, third, currentmode == 5, false, "Monotone automatic tangents, limited at extrema")) ga_auto(5)
			if (graph_button("Free", xx + 12 + third, inspector_y, third, !graph_handle_link, false, "Manual handle drags affect one tangent")) graph_handle_link = false
			if (graph_button("Linked", xx + 16 + third*2, inspector_y, third, graph_handle_link, false, "Manual handle drags align the opposite tangent; Alt temporarily breaks it")) graph_handle_link = true
		}
	}
	clip_end()
	if (sidebarmax > 0)
	{
		var trackh = hei - 100, thumbh = max(18, trackh*trackh/(trackh + sidebarmax));
		draw_box(xx + side - 4, yy + 68, 2, trackh, false, c_border, 0.3)
		draw_box(xx + side - 4, yy + 68 + (trackh - thumbh)*graph_sidebar_scroll[graph_view_index]/sidebarmax, 2, thumbh, false, c_text_secondary, 0.65)
	}

	// Buttons may create/delete keys; refresh references before drawing the plot.
	count = instance_exists(tl) ? ds_list_size(tl.keyframe_list) : 0
	if (!instance_exists(graph_selected)) graph_selected = null
	selectedindex = graph_selected != null ? ds_list_find_index(tl.keyframe_list, graph_selected) : -1

	// Read the picker again; mode/group changes apply immediately to the drawing.
	canedit = instance_exists(tl) && !tl.lock && available[graph_group]
	vid = e_value.POS_X + graph_group * 3 + graph_axis
	base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X) * 5
	ga_plot_width = gw
	if (graph_fit_pending) graph_fit(tl, gw, gh)
	if (timeline_playing && setting_timeline_autoscroll && timeline_marker > graph_x_start + gw / graph_x_scale)
		graph_x_start = max(0, timeline_marker - gw * 0.25 / graph_x_scale)
	var inside = content_mouseon && app_mouse_box(gx, gy, gw, gh, "place");
	// The ENTIRE 44px numbered band is interactive, including text and gaps.
	var rulerinside = content_mouseon && app_mouse_box(gx, yy + 32, gw, 44, "place");
	draw_box(gx, yy + 32, gw, 44, false, c_level_middle, 1)
	if (rulerinside && window_busy == "")
	{
		mouse_cursor = cr_size_we
		draw_box(gx, yy + 32, gw, 44, false, c_accent_overlay, a_accent_overlay)
		tip_set("Click or drag anywhere on this ruler to scrub frames", gx, yy + 32, gw, 44)
		if (mouse_left_pressed)
		{
			action_tl_play_break()
			graph_active_view = graph_view_index
			graph_scrub_start = timeline_marker
			graph_drag = 6
			timeline_marker = max(0, round(graph_x_start + (mouse_x - gx)/graph_x_scale))
			window_busy = "graphdrag"
		}
	}

	// Zoom at the pointer; Shift restricts zoom to the value axis.
	if ((inside || rulerinside) && window_busy == "" && mouse_wheel != 0)
	{
		var factor = mouse_wheel == 1 ? 1/setting_graph_options[9] : setting_graph_options[9];
		var anchorframe = graph_x_start + (mouse_x - gx) / graph_x_scale;
		var anchorvalue = graph_y_center + (gy + gh/2 - mouse_y) / graph_y_scale;
		if (rulerinside || (!keyboard_check(vk_shift) && setting_graph_options[10] != 2))
		{
			graph_x_scale = clamp(graph_x_scale * factor, 0.25, 128)
			graph_x_start = max(0, anchorframe - (mouse_x - gx) / graph_x_scale)
		}
		if (!rulerinside && !keyboard_check(vk_control) && setting_graph_options[10] != 1)
		{
			graph_y_scale = clamp(graph_y_scale * factor, 0.0001, 100000)
			graph_y_center = anchorvalue - (gy + gh/2 - mouse_y) / graph_y_scale
		}
	}

	// Adaptive 1/2/5 spacing; integer frame labels and meaningful value precision.
	var fstep = max(1, graph_grid_step(setting_graph_options[6]/graph_x_scale));
	var firstframe = max(0, ceil(graph_x_start/fstep)*fstep);
	clip_begin(gx - 58, yy + 32, gw + 58, gh + 44)
	for (var frame = firstframe; frame <= graph_x_start + gw/graph_x_scale; frame += fstep)
	{
		var fx = graph_screen_x(frame, gx);
		if (graph_show_grid) draw_line_ext(fx, gy, fx, gy + gh, c_text_secondary, setting_graph_options[5]*(setting_graph_options[19] != 0 ? 1.5 : 1))
		if (graph_show_grid && setting_graph_options[18] != 0 && fstep*graph_x_scale >= 56)
			for (var minor = 1; minor < 5; minor++)
			{
				var mx = fx + fstep*graph_x_scale*minor/5;
				if (mx <= gx + gw) draw_line_ext(mx, gy, mx, gy + gh, c_text_secondary, setting_graph_options[5]*0.35)
			}
		draw_line_ext(fx, gy - 6, fx, gy, c_text_secondary, 0.45)
		draw_label(string(frame), clamp(fx, gx + 16, gx + gw - 16), gy - 23, fa_center, fa_middle, c_text_main, 1, font_label)
	}
	var vstep = graph_grid_step(max(32, setting_graph_options[6]*0.7)/graph_y_scale);
	var vmin = graph_y_center - gh/(2*graph_y_scale), vmax = graph_y_center + gh/(2*graph_y_scale);
	var decimals = clamp(ceil(-log2(vstep)/log2(10)), 0, 6);
	for (var value = ceil(vmin/vstep)*vstep; value <= vmax; value += vstep)
	{
		var fy = graph_screen_y(value, gy, gh);
		if (graph_show_grid) draw_line_ext(gx, fy, gx + gw, fy, c_text_secondary, setting_graph_options[5]*(setting_graph_options[19] != 0 ? 1.5 : 1))
		draw_label(string_format(abs(value) < vstep*0.001 ? 0 : value, 0, decimals), gx - 8, fy, fa_right, fa_middle, c_text_secondary, 1, font_label)
	}
	clip_end()

	var hitkf = null, hitaxis = graph_axis, hitkind = 0, hitdistance = 100, hitx = 0, hity = 0;
	var dragging = window_busy == "graphdrag" && graph_active_view == graph_view_index;
	clip_begin(gx, gy, gw, gh)
	var zeroy = graph_screen_y(0, gy, gh);
	if (setting_graph_options[15] != 0) draw_line_ext(gx, zeroy, gx + gw, zeroy, c_text_secondary, setting_graph_options[19] != 0 ? 0.65 : 0.4)
	if (count > 0)
	{
		for (var axis = 0; axis < 3; axis++)
		{
			if (!graph_visible[axis]) continue
			var channel = e_value.POS_X + graph_group * 3 + axis, color = graph_axis_color(axis);
			var channelkeys = ga_keys(tl, channel);
			var px = gx, py = graph_screen_y(ga_curve_keys(tl, channel, graph_x_start, channelkeys), gy, gh);
			var samples = max(1, ceil(gw/3));
			for (var sample = 1; sample <= samples; sample++)
			{
				var nx = gx + gw*sample/samples, frame = graph_x_start + (nx - gx)/graph_x_scale;
				var ny = graph_screen_y(ga_curve_keys(tl, channel, frame, channelkeys), gy, gh);
				graph_curve_line(px, py, nx, ny, color, axis == graph_axis ? 1 : setting_graph_options[4])
				px = nx
				py = ny
			}
			if (graph_show_ghost && ga_ghost_owner == save_id_get(tl) && ga_ghost_group == graph_group)
			{
				for (var g = 1; g < array_length(ga_ghost); g++)
					draw_line_ext(graph_screen_x(ga_ghost[g - 1][0], gx), graph_screen_y(ga_ghost[g - 1][1][axis], gy, gh), graph_screen_x(ga_ghost[g][0], gx), graph_screen_y(ga_ghost[g][1][axis], gy, gh), c_text_secondary, 0.3)
			}
			for (var k = 0; k < count; k++)
			{
				var kf = tl.keyframe_list[|k];
				if (!ga_has(kf, channel)) continue
				var sx = graph_screen_x(kf.position, gx), sy = graph_screen_y(kf.value[channel], gy, gh);
				graph_node(sx, sy, color, graph_selection_find(kf, channel) >= 0)
				if (inside && !dragging)
				{
					// 10 px grab radius; the active axis wins ties.
					var distance = point_distance(mouse_x, mouse_y, sx, sy) + (axis == graph_axis ? -1 : 0);
					if (distance < setting_graph_options[1] && distance < hitdistance)
					{
						hitdistance = distance
						hitkf = kf
						hitaxis = axis
						hitkind = 1
						hitx = sx
						hity = sy
					}
				}
			}
		}
		// Tangents on incoming and outgoing segments of the selected node.
		if (setting_graph_options[17] != 0 && selectedindex >= 0 && graph_visible[graph_axis])
		{
			var selx = graph_screen_x(graph_selected.position, gx), sely = graph_screen_y(graph_selected.value[vid], gy, gh);
			for (var hand = 0; hand < 2; hand++)
			{
				var leftindex = hand == 0 ? ga_next(tl, selectedindex, vid, -1) : selectedindex;
				var rightindex = leftindex < 0 ? -1 : ga_next(tl, leftindex, vid, 1);
				if (leftindex < 0 || rightindex < 0) continue
				var leftkf = tl.keyframe_list[|leftindex], rightkf = tl.keyframe_list[|rightindex];
				if (round(leftkf.value[base]) != 2) continue
				var xfield = hand == 0 ? 3 : 1, yfield = xfield + 1;
				var hframe = lerp(leftkf.position, rightkf.position, leftkf.value[base + xfield]);
				var hvalue = (hand == 0 ? rightkf.value[vid] : leftkf.value[vid]) + leftkf.value[base + yfield];
				var hx = graph_screen_x(hframe, gx), hy = graph_screen_y(hvalue, gy, gh);
				var handlehot = dragging && graph_drag == (hand == 0 ? 3 : 2) && graph_drag_kf == leftkf;
				draw_line_ext(selx, sely, hx, hy, c_accent, 0.7)
				draw_circle_ext(hx, hy, setting_graph_options[2] + (handlehot ? 1 : 0), !handlehot, 24, c_accent, 1)
				if (inside && !dragging)
				{
					// Handles win over a node lying under them.
					var hd = point_distance(mouse_x, mouse_y, hx, hy) - 4;
					if (hd < setting_graph_options[1] - 4 && hd < hitdistance)
					{
						hitdistance = hd
						hitkf = leftkf
						hitaxis = graph_axis
						hitkind = hand == 0 ? 3 : 2
						hitx = hx
						hity = hy
					}
				}
			}
		}
	}
	// Hover feedback: you see what will be grabbed before pressing.
	if (setting_graph_options[13] != 0 && hitkf != null && canedit && window_busy == "")
	{
		draw_circle_ext(hitx, hity, hitkind == 1 ? setting_graph_options[0] + 4 : setting_graph_options[2] + 4, true, 24, c_accent, 0.9)
		tip_set("Frame " + string(hitkf.position) + " / " + labels[graph_group] + " " + chr(ord("X") + hitaxis), hitx - 12, hity - 12, 24, 24)
	}
	var markerx = graph_screen_x(timeline_marker, gx);
	draw_line_ext(markerx, gy, markerx, gy + gh, c_accent, 1)
	clip_end()
	clip_begin(gx, yy + 32, gw, 44)
	if (markerx >= gx && markerx <= gx + gw)
	{
		draw_line_ext(markerx, yy + 32, markerx, gy, c_accent, 1)
		draw_box(markerx - 5, gy - 6, 10, 6, false, c_accent, 1)
		if (setting_graph_options[16] != 0)
		{
			var badge = string(round(timeline_marker));
			draw_set_font(font_digits)
			var badgew = max(36, string_width(badge) + 16);
			var badgex = clamp(markerx - badgew/2, gx + 2, gx + gw - badgew - 2);
			draw_box(badgex, gy - 20, badgew, 18, false, c_level_top, 1)
			draw_box(badgex, gy - 20, badgew, 18, true, c_accent, 0.85)
			draw_label(badge, badgex + badgew/2, gy - 11, fa_center, fa_middle, c_accent, 1, font_digits)
		}
	}
	clip_end()

	if (!graph_visible[0] && !graph_visible[1] && !graph_visible[2])
		draw_label("No visible curves - enable an axis on the left", gx + gw/2, gy + gh/2, fa_center, fa_middle, c_text_secondary, 1, font_label)
	else if (count == 0)
		draw_label(gw < 360 ? "Select a keyed object" : "Select an object with keyframes in the timeline", gx + gw/2, gy + gh/2, fa_center, fa_middle, c_text_secondary, 1, font_label)
	if (hitkf != null && canedit && window_busy == "") mouse_cursor = cr_handpoint
	if (inside && window_busy == "" && !textbox_isediting)
	{
		if (keyboard_check_pressed(ord("F"))) ga_fit(tl, gw, gh, true)
		if (keyboard_check_pressed(ord("L")) && canedit)
		{
			var linkedvid = e_value.POS_X + graph_group*3 + hitaxis, keys = ga_keys(tl, linkedvid);
			graph_selection_clear()
			for (var i = 0; i < array_length(keys); i++) graph_selection_add(keys[i], linkedvid)
			if (array_length(keys) > 0) { graph_selected = keys[0]; graph_axis = hitaxis }
		}
	}

	if (inside && window_busy == "" && mouse_middle_pressed)
	{
		graph_active_view = graph_view_index
		graph_drag = 5
		graph_pan_x = graph_x_start
		graph_pan_y = graph_y_center
		graph_mouse_x = mouse_x
		graph_mouse_y = mouse_y
		window_busy = "graphdrag"
	}
	if (inside && window_busy == "" && mouse_left_double_pressed && hitkf == null && canedit)
	{
		action_tl_play_break()
		graph_add_at(graph_x_start + (mouse_x - gx)/graph_x_scale, graph_y_center + (gy + gh/2 - mouse_y)/graph_y_scale)
		app_mouse_clear()
	}
	if (inside && window_busy == "" && mouse_left_pressed)
	{
		graph_active_view = graph_view_index
		if (hitkf != null && canedit)
		{
			action_tl_play_break()
			graph_axis = hitaxis
			var hitvid = e_value.POS_X + graph_group*3 + hitaxis;
			if (hitkind == 1 && (keyboard_check(vk_control) || keyboard_check(vk_shift)))
			{
				graph_selection_toggle(hitkf, hitvid)
				if (graph_selection_find(hitkf, hitvid) >= 0) graph_selected = hitkf
				else if (array_length(graph_sel_keys) > 0)
				{
					graph_selected = graph_sel_keys[0]
					graph_axis = (graph_sel_vids[0] - e_value.POS_X) mod 3
				}
				else graph_selected = null
				graph_drag = 0
			}
			else
			{
				graph_drag_kf = hitkf
				graph_drag_first = true
				graph_drag_started = false
				graph_drag_vid = hitvid
				graph_drag_base = e_value.GRAPH_POS_X_MODE + (hitvid - e_value.POS_X)*5
				graph_drag_lock = 0
				graph_drag_last_value = 0
				graph_drag_last_time = 0
				graph_drag_conflict = false
				graph_drag_opp_kf = null
				graph_drag_opp_right = null
				graph_mouse_x = mouse_x
				graph_mouse_y = mouse_y
				graph_drag_prev_x = mouse_x
				graph_drag_prev_y = mouse_y
				graph_drag_accum_x = 0
				graph_drag_accum_y = 0
				// Keep the grab offset: the point never jumps under the cursor.
				graph_grab_dx = mouse_x - hitx
				graph_grab_dy = mouse_y - hity
				ga_time_delta = 0
				for (var field = 0; field < 5; field++) graph_drag_values[field] = hitkf.value[graph_drag_base + field]
				if (hitkind == 1)
				{
					if (graph_selection_find(hitkf, hitvid) < 0)
					{
						graph_selection_clear()
						graph_selection_add(hitkf, hitvid)
					}
					graph_selected = hitkf
					graph_drag = 1
					timeline_marker = hitkf.position
					graph_select_key(tl, hitkf)
					graph_changes_begin_drag()
				}
				else
				{
					graph_drag = hitkind
					graph_handle_begin(tl, hitkf, hitvid, hitkind, hitx, hity, gx, gy, gh)
				}
				window_busy = "graphdrag"
			}
		}
		else
		{
			action_tl_play_break()
			graph_drag = 7
			graph_mouse_x = mouse_x
			graph_mouse_y = mouse_y
			graph_box_add = keyboard_check(vk_control) || keyboard_check(vk_shift)
			window_busy = "graphdrag"
		}
	}

	if (window_busy == "graphdrag" && graph_active_view == graph_view_index)
	{
		if (keyboard_check_pressed(vk_escape) || mouse_right_pressed)
		{
			// Esc / right click: restore everything touched by this gesture, no Undo record.
			if (graph_drag >= 1 && graph_drag <= 3) graph_changes_cancel()
			if (graph_drag == 5) { graph_x_start = graph_pan_x; graph_y_center = graph_pan_y }
			if (graph_drag == 6) timeline_marker = graph_scrub_start
			ga_time_delta = 0
			graph_drag = 0
			window_busy = ""
			app_mouse_clear()
			graph_status_set("Drag cancelled")
		}
		else if (graph_drag == 5)
		{
			graph_x_start = max(0, graph_pan_x - (mouse_x - graph_mouse_x)/graph_x_scale)
			graph_y_center = graph_pan_y + (mouse_y - graph_mouse_y)/graph_y_scale
		}
		else if (graph_drag == 6) timeline_marker = max(0, round(graph_x_start + (mouse_x - gx)/graph_x_scale))
		else if (graph_drag == 7)
		{
			var rx = clamp(mouse_x, gx, gx + gw), ry = clamp(mouse_y, gy, gy + gh);
			clip_begin(gx, gy, gw, gh)
			draw_box(min(graph_mouse_x, rx), min(graph_mouse_y, ry), abs(rx - graph_mouse_x), abs(ry - graph_mouse_y), false, c_accent, 0.12)
			draw_box(min(graph_mouse_x, rx), min(graph_mouse_y, ry), abs(rx - graph_mouse_x), abs(ry - graph_mouse_y), true, c_accent, 0.8)
			clip_end()
		}
		else if (instance_exists(graph_drag_kf) && instance_exists(tl))
		{
			var movex = mouse_x - graph_mouse_x, movey = mouse_y - graph_mouse_y;
			var fine = keyboard_check(vk_alt) ? setting_graph_options[7] : 1;
			graph_drag_accum_x += (mouse_x - graph_drag_prev_x)*fine
			graph_drag_accum_y += (mouse_y - graph_drag_prev_y)*fine
			graph_drag_prev_x = mouse_x
			graph_drag_prev_y = mouse_y
			// 3 px dead zone: a click never nudges a value by accident.
			if (!graph_drag_started && max(abs(movex), abs(movey)) >= setting_graph_options[8]) graph_drag_started = true
			if (graph_drag == 1 && graph_drag_started)
			{
				mouse_cursor = cr_size_all
				if (keyboard_check_pressed(ord("X"))) graph_drag_lock = graph_drag_lock == 1 ? 0 : 1
				if (keyboard_check_pressed(ord("Y"))) graph_drag_lock = graph_drag_lock == 2 ? 0 : 2
				var delta = graph_drag_lock == 1 ? 0 : -graph_drag_accum_y/graph_y_scale;
				if (graph_snap_value || keyboard_check(vk_control)) delta = round(delta/setting_graph_options[12])*setting_graph_options[12]
				ga_time_delta = graph_drag_lock == 2 ? 0 : round(graph_drag_accum_x/graph_x_scale/setting_graph_options[11])*setting_graph_options[11]
				if (graph_drag_first || delta != graph_drag_last_value) graph_changes_offset(delta)
				graph_drag_last_value = delta
				graph_drag_first = false
				graph_drag_preview(gx, gy, gw, gh)
			}
			else if ((graph_drag == 2 || graph_drag == 3) && graph_drag_started)
			{
				if (graph_drag_first || mouse_still == 0 || keyboard_check_pressed(vk_alt) || keyboard_check_released(vk_alt))
					graph_handle_update(tl, graph_mouse_x + graph_drag_accum_x - graph_grab_dx, graph_mouse_y + graph_drag_accum_y - graph_grab_dy, gx, gy, gh)
				graph_drag_first = false
			}
		}
		if (window_busy == "graphdrag" && ((graph_drag == 5 && !mouse_middle) || (graph_drag != 5 && !mouse_left)))
		{
			if (graph_drag == 7)
			{
				if (point_distance(graph_mouse_x, graph_mouse_y, mouse_x, mouse_y) < 4)
				{
					if (!graph_box_add) graph_selection_clear()
					timeline_marker = max(0, round(graph_x_start + (mouse_x - gx)/graph_x_scale))
				}
				else graph_box_select(tl, graph_mouse_x, graph_mouse_y, clamp(mouse_x, gx, gx + gw), clamp(mouse_y, gy, gy + gh), gx, gy, gh, graph_box_add)
			}
			graph_drag_release()
		}
	}

	// Status stays readable and does not cover the plot.
	clip_begin(xx + side, yy + hei - 22, wid - side, 22)
	var status = string(array_length(graph_sel_keys)) + " selected";
	if (setting_graph_options[14] != 0) status += rulerinside ? "  |  Drag ruler: scrub  |  Wheel: time zoom" : "  |  X/Y: lock  |  Alt: precise  |  Esc: cancel";
	if (window_busy == "graphdrag" && graph_drag == 1 && graph_active_view == graph_view_index)
		status = (graph_drag_lock == 1 ? "TIME ONLY (X)" : (graph_drag_lock == 2 ? "VALUE ONLY (Y)" : "FREE")) + "  |  " + (ga_time_delta > 0 ? "+" : "") + string(ga_time_delta) + " fr  |  Esc/RMB: cancel"
	if (!canedit && instance_exists(tl)) status = "Object is locked - curves are read-only"
	if (graph_status_time > current_time) status = graph_status_msg
	else if (graph_selected != null) status = "Key " + string(graph_selected.position) + "  /  " + labels[graph_group] + " " + chr(ord("X") + graph_axis) + " = " + string_format(graph_selected.value[vid], 0, 3) + "   |   " + status
	draw_set_font(font_label)
	draw_label(string_limit(status, wid - side - 16), xx + side + 8, yy + hei - 11, fa_left, fa_middle, c_text_secondary, 1, font_label)
	clip_end()
}
