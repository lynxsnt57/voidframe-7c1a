function graph_select_key(tl, kf)
{
	if (!instance_exists(tl) || !instance_exists(kf) || tl.lock) return false
	if (!kf.selected || tl.keyframe_select_amount != 1 || tl_edit_amount != 1)
		action_tl_keyframe_select_single(tl, kf)
	return true
}

function action_graph_values(values, combine)
{
	// Selected key is deliberately restricted to one by graph_select_key.
	// Native value editing supplies persistence, copy/paste and Undo/Redo.
	tl_value_set_start(action_graph_values, combine)
	for (var i = 0; i < array_length(values); i += 2)
		tl_value_set(values[i], values[i + 1], false)
	tl_value_set_done()
}

function action_graph_point_value(val, add)
{
	if (!instance_exists(graph_numeric_kf) || !graph_select_key(graph_numeric_tl, graph_numeric_kf)) return 0
	var vid = graph_numeric_vid;
	tl_value_set_start(action_graph_point_value, true)
	tl_value_set(vid, val, add)
	tl_value_set_done()
}

function graph_set_mode(mode)
{
	graph_batch_mode(mode, false)
}

function graph_flat_handles()
{
	graph_batch_mode(2, true)
}

function graph_close()
{
	if (instance_exists(graph_target) && array_length(graph_sel_keys) > 0) graph_prepare_native_keys(graph_target)
	graph_mode = false
	timeline_zoom = clamp(graph_x_scale, 0.25, 32)
	timeline_zoom_goal = timeline_zoom
	timeline_zoom_current = timeline_zoom
	timeline.hor_scroll.value = graph_x_start * timeline_zoom
	timeline.hor_scroll.value_goal = timeline.hor_scroll.value
}

function graph_fit(tl, wid, hei) { ga_fit(tl, wid, hei, false) }

function graph_add_key() { ga_add_active() }

function graph_remove_key() { ga_delete_selected() }
// Graph point selection is independent from complete object-key selection.
function graph_selection_find(kf, vid)
{
	for (var i = 0; i < array_length(graph_sel_keys); i++)
		if (graph_sel_keys[i] == kf && graph_sel_vids[i] == vid) return i
	return -1
}

function graph_selection_clear()
{
	graph_selected = null
	graph_sel_keys = array_create(0)
	graph_sel_vids = array_create(0)
}

function graph_selection_add(kf, vid)
{
	if (!instance_exists(kf) || graph_selection_find(kf, vid) >= 0) return 0
	var n = array_length(graph_sel_keys);
	graph_sel_keys[n] = kf
	graph_sel_vids[n] = vid
}

function graph_selection_clean(tl)
{
	var keys = array_create(0), vids = array_create(0);
	for (var i = 0; i < array_length(graph_sel_keys); i++)
	{
		var kf = graph_sel_keys[i];
		if (!instance_exists(kf) || kf.timeline != tl || !ga_has(kf, graph_sel_vids[i])) continue
		var n = array_length(keys);
		keys[n] = kf
		vids[n] = graph_sel_vids[i]
	}
	graph_sel_keys = keys
	graph_sel_vids = vids
}

function graph_selection_toggle(kf, vid)
{
	var found = graph_selection_find(kf, vid);
	if (found < 0) { graph_selection_add(kf, vid); return 0 }
	var keys = array_create(0), vids = array_create(0);
	for (var i = 0; i < array_length(graph_sel_keys); i++)
	{
		if (i == found) continue
		var n = array_length(keys);
		keys[n] = graph_sel_keys[i]
		vids[n] = graph_sel_vids[i]
	}
	graph_sel_keys = keys
	graph_sel_vids = vids
}

function graph_select_all_points(tl)
{
	graph_selection_clear()
	if (!instance_exists(tl)) return 0
	for (var k = 0; k < ds_list_size(tl.keyframe_list); k++)
		for (var axis = 0; axis < 3; axis++)
			if (graph_visible[axis] && ga_has(tl.keyframe_list[|k], e_value.POS_X + graph_group*3 + axis)) graph_selection_add(tl.keyframe_list[|k], e_value.POS_X + graph_group*3 + axis)
}

function graph_refresh_target(tl)
{
	if (!instance_exists(tl)) return 0
	with (tl) { update_matrix = true; tl_update_values() }
	with (app) { tl_update_matrix(); render_samples = -1 }
}

function graph_changes_clear()
{
	graph_edit_keys = array_create(0)
	graph_edit_vids = array_create(0)
	graph_edit_old = array_create(0)
	graph_edit_new = array_create(0)
}

function graph_change_push(kf, vid, val)
{
	var n = array_length(graph_edit_keys);
	graph_edit_keys[n] = kf
	graph_edit_vids[n] = vid
	graph_edit_old[n] = kf.value[vid]
	graph_edit_new[n] = tl_value_clamp(vid, val)
}

function graph_changes_apply()
{
	graph_changes_write(true)
	graph_refresh_target(graph_target)
}

function graph_changes_begin_drag()
{
	graph_changes_clear()
	for (var i = 0; i < array_length(graph_sel_keys); i++)
		graph_change_push(graph_sel_keys[i], graph_sel_vids[i], graph_sel_keys[i].value[graph_sel_vids[i]])
}

function graph_changes_offset(delta)
{
	for (var i = 0; i < array_length(graph_edit_keys); i++)
		graph_edit_new[i] = tl_value_clamp(graph_edit_vids[i], graph_edit_old[i] + delta)
	graph_changes_apply()
}

function graph_changes_finish()
{
	if (!instance_exists(graph_target) || array_length(graph_edit_keys) == 0) return 0
	var changed = false;
	for (var i = 0; i < array_length(graph_edit_keys); i++)
		if (graph_edit_old[i] != graph_edit_new[i]) changed = true
	if (!changed) return 0
	// A single history record for every point/axis in the entire gesture.
	var h = history_set(action_graph_batch);
	h.ge_tl_save_id = save_id_get(graph_target)
	h.ge_count = array_length(graph_edit_keys)
	for (var i = 0; i < h.ge_count; i++)
	{
		h.ge_index[i] = ds_list_find_index(graph_target.keyframe_list, graph_edit_keys[i])
		h.ge_vid[i] = graph_edit_vids[i]
		h.ge_old[i] = graph_edit_old[i]
		h.ge_new[i] = graph_edit_new[i]
	}
	graph_changes_clear()
}

function action_graph_batch()
{
	if (!history_undo && !history_redo) return 0
	var h = history_data, tl = save_id_find(h.ge_tl_save_id);
	if (!instance_exists(tl)) return 0
	for (var i = 0; i < h.ge_count; i++)
	{
		if (h.ge_index[i] < 0 || h.ge_index[i] >= ds_list_size(tl.keyframe_list)) continue
		var kf = tl.keyframe_list[|h.ge_index[i]];
		kf.value[h.ge_vid[i]] = history_undo ? h.ge_old[i] : h.ge_new[i]
	}
	graph_refresh_target(tl)
}

function graph_prepare_native_keys(tl)
{
	with (tl) tl_select_single()
	tl_keyframes_deselect_all()
	for (var i = 0; i < array_length(graph_sel_keys); i++)
		if (instance_exists(graph_sel_keys[i])) tl_keyframe_select(graph_sel_keys[i])
	app_update_tl_edit()
}

function graph_box_select(tl, x1, y1, x2, y2, gx, gy, gh, additive)
{
	if (!additive) graph_selection_clear()
	if (!instance_exists(tl) || tl.lock) return 0
	for (var k = 0; k < ds_list_size(tl.keyframe_list); k++)
	{
		var kf = tl.keyframe_list[|k], sx = graph_screen_x(kf.position, gx);
		if (sx < min(x1, x2) || sx > max(x1, x2)) continue
		for (var axis = 0; axis < 3; axis++)
		{
			if (!graph_visible[axis]) continue
			var vid = e_value.POS_X + graph_group*3 + axis;
			if (!ga_has(kf, vid)) continue
			var sy = graph_screen_y(kf.value[vid], gy, gh);
			if (sy >= min(y1, y2) && sy <= max(y1, y2)) graph_selection_add(kf, vid)
		}
	}
}

function graph_batch_mode(mode, flat)
{
	if (!instance_exists(graph_target) || graph_target.lock) return 0
	if (array_length(graph_sel_keys) == 0 && instance_exists(graph_selected))
		graph_selection_add(graph_selected, e_value.POS_X + graph_group*3 + graph_axis)
	graph_changes_clear()
	for (var i = 0; i < array_length(graph_sel_keys); i++)
	{
		var kf = graph_sel_keys[i], vid = graph_sel_vids[i], index = ds_list_find_index(graph_target.keyframe_list, kf);
		var nextindex = ga_next(graph_target, index, vid, 1);
		if (index < 0 || nextindex < 0) continue
		var nextkf = graph_target.keyframe_list[|nextindex], base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
		var delta = nextkf.value[vid] - kf.value[vid];
		graph_change_push(kf, base, mode)
		graph_change_push(kf, base + 1, 1/3)
		graph_change_push(kf, base + 2, flat ? 0 : delta/3)
		graph_change_push(kf, base + 3, 2/3)
		graph_change_push(kf, base + 4, flat ? 0 : -delta/3)
	}
	graph_changes_apply()
	graph_changes_finish()
}

function graph_view_new(group)
{
	return [group, 0, [true, true, true], null, null, 0, true, [0, 0, 0, 0, 0], 0, 16, 0, 8, true, new_textbox_ndecimals(), null, 0, 0, 0, 0, 0, 0, 0, array_create(0), array_create(0), array_create(0), array_create(0), array_create(0), array_create(0), null, false]
}

function graph_view_load(index)
{
	graph_group = graph_views[index][0]
	graph_axis = graph_views[index][1]
	graph_visible = graph_views[index][2]
	graph_selected = graph_views[index][3]
	graph_timeline = graph_views[index][4]
	graph_drag = graph_views[index][5]
	graph_drag_first = graph_views[index][6]
	graph_drag_values = graph_views[index][7]
	graph_x_start = graph_views[index][8]
	graph_x_scale = graph_views[index][9]
	graph_y_center = graph_views[index][10]
	graph_y_scale = graph_views[index][11]
	graph_fit_pending = graph_views[index][12]
	graph_tbx_value = graph_views[index][13]
	graph_drag_kf = graph_views[index][14]
	graph_drag_offset = graph_views[index][15]
	graph_drag_vid = graph_views[index][16]
	graph_drag_base = graph_views[index][17]
	graph_pan_x = graph_views[index][18]
	graph_pan_y = graph_views[index][19]
	graph_mouse_x = graph_views[index][20]
	graph_mouse_y = graph_views[index][21]
	graph_sel_keys = graph_views[index][22]
	graph_sel_vids = graph_views[index][23]
	graph_edit_keys = graph_views[index][24]
	graph_edit_vids = graph_views[index][25]
	graph_edit_old = graph_views[index][26]
	graph_edit_new = graph_views[index][27]
	graph_pin_tl = graph_views[index][28]
	graph_box_add = graph_views[index][29]
}

function graph_view_save(index)
{
	graph_views[index] = [graph_group, graph_axis, graph_visible, graph_selected, graph_timeline, graph_drag, graph_drag_first, graph_drag_values, graph_x_start, graph_x_scale, graph_y_center, graph_y_scale, graph_fit_pending, graph_tbx_value, graph_drag_kf, graph_drag_offset, graph_drag_vid, graph_drag_base, graph_pan_x, graph_pan_y, graph_mouse_x, graph_mouse_y, graph_sel_keys, graph_sel_vids, graph_edit_keys, graph_edit_vids, graph_edit_old, graph_edit_new, graph_pin_tl, graph_box_add]
}

// ---- GE-R1: drag rework (smooth points/handles, one Undo per gesture) ----

function graph_status_set(text)
{
	graph_status_msg = text
	graph_status_time = current_time + 2500
}

function graph_changes_write(usenew)
{
	for (var i = 0; i < array_length(graph_edit_keys); i++)
		if (instance_exists(graph_edit_keys[i])) graph_edit_keys[i].value[graph_edit_vids[i]] = usenew ? graph_edit_new[i] : graph_edit_old[i]
}

function graph_change_set(kf, vid, val)
{
	for (var i = 0; i < array_length(graph_edit_keys); i++)
	{
		if (graph_edit_keys[i] == kf && graph_edit_vids[i] == vid)
		{
			graph_edit_new[i] = tl_value_clamp(vid, val)
			return i
		}
	}
	graph_change_push(kf, vid, val)
	return array_length(graph_edit_keys) - 1
}

function graph_changes_reset_key(kf)
{
	for (var i = 0; i < array_length(graph_edit_keys); i++)
		if (graph_edit_keys[i] == kf) graph_edit_new[i] = graph_edit_old[i]
}

function graph_changes_cancel()
{
	var owner = graph_target;
	if (array_length(graph_edit_keys) > 0 && instance_exists(graph_edit_keys[0])) owner = graph_edit_keys[0].timeline
	graph_changes_write(false)
	graph_refresh_target(owner)
	graph_changes_clear()
}

// Pressing a handle: capture its segment and, for Aligned, the opposite handle of the same key.
function graph_handle_begin(tl, leftkf, vid, kind, hx, hy, gx, gy, gh)
{
	var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
	graph_changes_clear()
	for (var f = 0; f < 5; f++) graph_change_push(leftkf, base + f, leftkf.value[base + f])
	graph_drag_opp_kf = null
	graph_drag_opp_right = null
	graph_drag_opp_len = 0
	graph_drag_aligned = false
	var leftindex = ds_list_find_index(tl.keyframe_list, leftkf);
	var rightindex = leftindex < 0 ? -1 : ga_next(tl, leftindex, vid, 1);
	if (leftindex < 0 || rightindex < 0) return 0
	var rightkf = tl.keyframe_list[|rightindex];
	// Pivot key owns both tangents: kind 2 = its outgoing (pivot = left), kind 3 = its incoming (pivot = right).
	var pivot = kind == 2 ? leftkf : rightkf, pivotindex = kind == 2 ? leftindex : rightindex;
	var oppleft = kind == 2 ? ga_next(tl, pivotindex, vid, -1) : pivotindex;
	var oppright = kind == 2 ? pivotindex : ga_next(tl, pivotindex, vid, 1);
	if (oppleft < 0 || oppright < 0) return 0
	var ol = tl.keyframe_list[|oppleft], orr = tl.keyframe_list[|oppright];
	if (round(ol.value[base]) != 2) return 0
	var oframe = lerp(ol.position, orr.position, kind == 2 ? ol.value[base + 3] : ol.value[base + 1]);
	var ovalue = kind == 2 ? orr.value[vid] + ol.value[base + 4] : ol.value[vid] + ol.value[base + 2];
	var kx = graph_screen_x(pivot.position, gx), ky = graph_screen_y(pivot.value[vid], gy, gh);
	var ox = graph_screen_x(oframe, gx), oy = graph_screen_y(ovalue, gy, gh);
	graph_drag_opp_kf = ol
	graph_drag_opp_right = orr
	graph_drag_opp_len = point_distance(kx, ky, ox, oy)
	// Already collinear (within ~10 degrees) stays Aligned; Shift at press forces Aligned.
	var l1 = point_distance(kx, ky, hx, hy);
	if (l1 > 0.5 && graph_drag_opp_len > 0.5)
	{
		var dotp = ((hx - kx)*(ox - kx) + (hy - ky)*(oy - ky))/(l1*graph_drag_opp_len);
		graph_drag_aligned = dotp < -0.985
	}
	graph_drag_aligned = graph_handle_link || keyboard_check(vk_shift)
	for (var f = 1; f < 5; f++) graph_change_push(ol, base + f, ol.value[base + f])
}

// tx/ty: handle target in screen space (mouse minus grab offset).
function graph_handle_update(tl, tx, ty, gx, gy, gh)
{
	var left = graph_drag_kf, vid = graph_drag_vid, base = graph_drag_base;
	var leftindex = ds_list_find_index(tl.keyframe_list, left);
	var rightindex = leftindex < 0 ? -1 : ga_next(tl, leftindex, vid, 1);
	if (leftindex < 0 || rightindex < 0) return 0
	var right = tl.keyframe_list[|rightindex], duration = max(1, right.position - left.position);
	var frame = graph_x_start + (tx - gx)/graph_x_scale;
	var val = graph_y_center + (gy + gh/2 - ty)/graph_y_scale;
	var fraction = (frame - left.position)/duration;
	var outx = graph_drag_values[1], outy = graph_drag_values[2], inx = graph_drag_values[3], iny = graph_drag_values[4];
	var pivot = left;
	if (graph_drag == 2) { outx = clamp(fraction, 0, inx); outy = val - left.value[vid] }
	else { inx = clamp(fraction, outx, 1); iny = val - right.value[vid]; pivot = right }
	graph_change_set(left, base, 2)
	graph_change_set(left, base + 1, outx)
	graph_change_set(left, base + 2, outy)
	graph_change_set(left, base + 3, inx)
	graph_change_set(left, base + 4, iny)
	if (instance_exists(graph_drag_opp_kf))
	{
		var ol = graph_drag_opp_kf, orr = graph_drag_opp_right;
		graph_changes_reset_key(ol)
		if (graph_drag_aligned && !keyboard_check(vk_alt) && instance_exists(orr))
		{
			// Mirror in screen space and keep the opposite handle's length.
			var hx = graph_screen_x(lerp(left.position, right.position, graph_drag == 2 ? outx : inx), gx);
			var hy = graph_screen_y(graph_drag == 2 ? left.value[vid] + outy : right.value[vid] + iny, gy, gh);
			var kx = graph_screen_x(pivot.position, gx), ky = graph_screen_y(pivot.value[vid], gy, gh);
			var len = point_distance(kx, ky, hx, hy);
			if (len > 0.5)
			{
				var ox = kx - (hx - kx)/len*graph_drag_opp_len, oy = ky - (hy - ky)/len*graph_drag_opp_len;
				var oframe = graph_x_start + (ox - gx)/graph_x_scale, ovalue = graph_y_center + (gy + gh/2 - oy)/graph_y_scale;
				var ofraction = (oframe - ol.position)/max(1, orr.position - ol.position);
				if (graph_drag == 2)
				{
					graph_change_set(ol, base + 3, clamp(ofraction, ol.value[base + 1], 1))
					graph_change_set(ol, base + 4, ovalue - orr.value[vid])
				}
				else
				{
					graph_change_set(ol, base + 1, clamp(ofraction, 0, ol.value[base + 3]))
					graph_change_set(ol, base + 2, ovalue - ol.value[vid])
				}
			}
		}
	}
	graph_changes_apply()
}

// Live preview of a time move: ghost nodes at the new frame, red on conflict.
function graph_drag_preview(gx, gy, gw, gh)
{
	if (ga_time_delta == 0) return 0
	if (ga_time_delta != graph_drag_last_time)
	{
		graph_drag_last_time = ga_time_delta
		graph_drag_conflict = ga_move_conflict(ga_time_delta, false)
	}
	var color = graph_drag_conflict ? c_error : c_accent;
	clip_begin(gx, gy, gw, gh)
	for (var i = 0; i < array_length(graph_sel_keys); i++)
	{
		var kf = graph_sel_keys[i];
		if (!instance_exists(kf)) continue
		var sy = graph_screen_y(kf.value[graph_sel_vids[i]], gy, gh);
		var sx0 = graph_screen_x(kf.position, gx), sx1 = graph_screen_x(kf.position + ga_time_delta, gx);
		draw_line_ext(sx0, sy, sx1, sy, color, 0.5)
		draw_circle_ext(sx1, sy, 5, true, 24, color, 1)
	}
	clip_end()
	draw_label((ga_time_delta > 0 ? "+" : "") + string(ga_time_delta) + " fr" + (graph_drag_conflict ? "  conflict" : ""), mouse_x + 14, mouse_y - 14, fa_left, fa_middle, color, 1, font_label)
}

// Finishes any graph gesture. Value + time of one drag = one Undo record.
function graph_drag_release()
{
	if (graph_drag == 1 && !graph_drag_first)
	{
		if (ga_time_delta != 0 && instance_exists(graph_target))
		{
			if (ga_move_conflict(ga_time_delta, false))
			{
				graph_changes_cancel()
				graph_status_set("Time conflict: channel key already on that frame - move cancelled")
			}
			else
			{
				graph_changes_write(false)
				var before = ga_state(graph_target);
				graph_changes_write(true)
				ga_move_selected(ga_time_delta, false, before)
				graph_changes_clear()
			}
		}
		else graph_changes_finish()
	}
	else if ((graph_drag == 2 || graph_drag == 3) && !graph_drag_first) graph_changes_finish()
	else if (graph_drag >= 1 && graph_drag <= 3) graph_changes_clear()
	ga_time_delta = 0
	graph_drag = 0
	window_busy = ""
}

// Numeric Frame edits move the entire graph selection, preserving channel offsets.
function action_graph_point_frame(val, add)
{
 if (!instance_exists(graph_numeric_tl) || !instance_exists(graph_numeric_kf) || graph_numeric_tl.lock) return 0
 graph_target = graph_numeric_tl
 var delta = round(add ? val : val - graph_numeric_kf.position);
 if (delta == 0) return 0
 // Text entry is one atomic channel move; the frame field has no drag action.
 if (!ga_move_selected(delta, false)) graph_status_set("Frame occupied or before frame 0: nothing moved")
 else timeline_marker = max(0, round(add ? timeline_marker + val : val))
}

function graph_add_at(frame, value)
{
 if (!instance_exists(graph_target) || graph_target.lock) return 0
 var vid = e_value.POS_X + graph_group*3 + graph_axis;
 frame = max(0, round(frame))
 var keys = ga_keys(graph_target, vid);
 for (var i = 0; i < array_length(keys); i++)
  if (keys[i].position == frame)
  {
   graph_selection_clear()
   graph_selection_add(keys[i], vid)
   graph_selected = keys[i]
   graph_status_set("Selected existing channel key")
   return 0
  }
 var before = ga_state(graph_target);
 var kf = ga_put(graph_target, vid, frame, tl_value_clamp(vid, value));
 graph_selection_clear()
 graph_selection_add(kf, vid)
 graph_selected = kf
 timeline_marker = frame
 ga_commit(graph_target, before)
}

// Text-only field: dragging a frame number must not generate structural edits.
function graph_frame_field(xx, yy, wid, value)
{
 var tbx = graph_frame_boxes[graph_view_index];
 if (window_focus != string(tbx)) tbx.text = string(value)
 if (draw_inputbox("graphpointframe" + string(graph_view_index), xx, yy, wid, 24, "0", tbx, null, false, false, font_digits, e_inputbox.RIGHT))
  action_graph_point_frame(max(0, round(eval(tbx.text, value))), false)
}

function graph_keyboard_target()
{
 if (!graph_mode || ga_tools || graph_preferences_open || popup_mouseon || toast_mouseon || context_menu_mouseon) return false
 if (!app_mouse_box(graph_focus_x, graph_focus_y, graph_focus_w, graph_focus_h, "place")) return false
 graph_view_load(graph_active_view)
 graph_target = graph_timeline
 return true
}

function graph_duplicate_selection()
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(graph_sel_keys) == 0) return 0
 var first = graph_sel_keys[0].position, last = first;
 for (var i = 1; i < array_length(graph_sel_keys); i++)
 {
  first = min(first, graph_sel_keys[i].position)
  last = max(last, graph_sel_keys[i].position)
 }
 // Place a duplicate block after the current range; never overwrite occupied keys.
 if (!ga_move_selected(max(1, last - first + 1), true)) graph_status_set("Duplicate cancelled: destination channel keys already exist")
}

// Comfort v2: one validated numeric preference schema shared by UI and settings.
function graph_option_keys() { return ["key_size", "hit_radius", "handle_size", "curve_width", "inactive_alpha", "grid_alpha", "grid_spacing", "fine_factor", "drag_threshold", "zoom_step", "wheel_mode", "frame_step", "value_step", "hover_feedback", "help_bar", "zero_axis", "frame_badge", "tangent_lines", "minor_grid", "high_contrast", "sidebar_width", "show_grid", "value_snap", "show_ghost", "linked_handles", "show_inspector"] }
function graph_options_defaults() { return [5, 12, 4, 2, 0.55, 0.14, 64, 0.1, 3, 1.2, 0, 1, 0.1, 1, 1, 1, 1, 1, 1, 0, 256, 1, 0, 1, 0, 1] }
function graph_option_clamp(index, val)
{
 var low = [3, 8, 3, 1, 0.2, 0.04, 36, 0.02, 0, 1.05, 0, 1, 0.001, 0, 0, 0, 0, 0, 0, 0, 168, 0, 0, 0, 0, 0], high = [8, 24, 8, 3, 1, 0.35, 112, 0.5, 8, 1.5, 2, 24, 10, 1, 1, 1, 1, 1, 1, 1, 420, 1, 1, 1, 1, 1];
 var result = clamp(val, low[index], high[index]);
 if (index == 0 || index == 1 || index == 2 || index == 3 || index == 6 || index == 8 || index == 10 || index == 11 || index >= 13) result = round(result)
 return result
}
function graph_options_apply()
{
 graph_sidebar_width = setting_graph_options[20]
 graph_show_grid = setting_graph_options[21] != 0
 graph_snap_value = setting_graph_options[22] != 0
 graph_show_ghost = setting_graph_options[23] != 0
 graph_handle_link = setting_graph_options[24] != 0
}
function graph_options_capture()
{
 setting_graph_options[20] = round(graph_sidebar_width)
 setting_graph_options[21] = graph_show_grid ? 1 : 0
 setting_graph_options[22] = graph_snap_value ? 1 : 0
 setting_graph_options[23] = graph_show_ghost ? 1 : 0
 setting_graph_options[24] = graph_handle_link ? 1 : 0
}
function graph_preference_change(index, direction)
{
 var steps = [1, 1, 1, 1, 0.05, 0.02, 8, 0.02, 1, 0.05, 1, 1, 0.1, 1, 1, 1, 1, 1, 1, 1, 16, 1, 1, 1, 1, 1];
 var old = setting_graph_options[index];
 if ((index >= 13 && index <= 19) || index >= 21) setting_graph_options[index] = old == 0 ? 1 : 0
 else setting_graph_options[index] = graph_option_clamp(index, old + direction*steps[index])
 graph_options_apply()
}
function graph_preferences_panel()
{
 var xx = content_x, yy = content_y, ww = content_width, hh = content_height;
 content_mouseon = !popup_mouseon && !toast_mouseon && !context_menu_mouseon && app_mouse_box(xx, yy, ww, hh, "place")
 draw_box(xx, yy, ww, hh, false, c_level_middle, 1)
 if (graph_control("Back", xx + 8, yy + 8, 64, 32, false, false, "Return to curves") || (content_mouseon && !textbox_isediting && keyboard_check_pressed(vk_escape))) graph_preferences_open = false
 draw_label("GRAPH PREFERENCES", xx + 88, yy + 24, fa_left, fa_middle, c_text_main, 1, font_label)
 if (ww < 360 || hh < 220)
 {
  draw_label("Enlarge panel to 360 x 220", xx + 16, yy + 76, fa_left, fa_middle, c_text_secondary, 1, font_label)
  return 0
 }
 var tabs = ["Appearance", "Interaction", "Display"];
 var tw = floor((ww - 32)/3);
 for (var tab = 0; tab < 3; tab++)
  if (graph_control(tabs[tab], xx + 8 + tab*(tw + 8), yy + 48, tw, 32, graph_preferences_tab == tab, false, "Graph preferences category")) { graph_preferences_tab = tab; graph_preferences_page = 0 }
 var labels = ["Key size", "Grab radius", "Handle size", "Curve width", "Inactive curves", "Grid contrast", "Grid spacing", "Alt precision", "Drag dead zone", "Wheel zoom step", "Wheel axes", "Time snap step", "Value snap step", "Hover feedback", "Help bar", "Zero value axis", "Frame badge", "Handle lines", "Minor grid", "High contrast", "Sidebar width", "Grid", "Value snapping", "Ghost curves", "Linked handles", "Key inspector"];
 var hints = ["Visible key size in logical pixels", "Pick radius, independent from visible point size", "Visible tangent handle radius", "Curve thickness in logical pixels", "Opacity of inactive axes", "Grid strength; zero axis remains separate", "Major grid density in logical pixels", "Sensitivity multiplier while Alt is held", "Movement required before a click becomes a drag", "Wheel zoom factor per notch", "0 = both; 1 = time; 2 = value; ruler always time", "Whole-frame step for point dragging; ruler stays one-frame precise", "Value delta snap increment; Ctrl temporarily enables snapping", "Highlight the point or handle under the cursor", "Show contextual help in the footer", "Emphasize the zero-value reference line", "Show the current frame above the playhead", "Show tangent lines and draggable handles", "Intermediate grid lines between major divisions", "Stronger borders and grid for readability", "Preferred width; constrained automatically by available space", "Show graph grid", "Snap value deltas during point drags", "Show the captured reference curve", "Align opposite manual tangent; Alt temporarily breaks link", "Show Frame/Value inspector; scroll sidebar to reach it"];
 var ids = graph_preferences_tab == 0 ? [0, 2, 3, 4, 5, 6, 19, 20] : (graph_preferences_tab == 1 ? [1, 7, 8, 9, 10, 11, 12, 22, 24] : [13, 14, 15, 16, 17, 18, 21, 23, 25]);
 var rows = max(1, floor((hh - 148)/44));
 var pages = max(1, ceil(array_length(ids)/rows));
 graph_preferences_page = clamp(graph_preferences_page, 0, pages - 1)
 if (content_mouseon && window_busy == "" && mouse_wheel != 0) graph_preferences_page = clamp(graph_preferences_page + (mouse_wheel == 1 ? -1 : 1), 0, pages - 1)
 for (var row = 0; row < rows; row++)
 {
  var item = graph_preferences_page*rows + row;
  if (item >= array_length(ids)) break
  var index = ids[item], ry = yy + 92 + row*44;
  var val = setting_graph_options[index];
  var toggle = (index >= 13 && index <= 19) || index >= 21;
  draw_box(xx + 8, ry, ww - 16, 40, false, c_level_bottom, 1)
  draw_label(labels[index], xx + 20, ry + 20, fa_left, fa_middle, c_text_main, 1, font_label)
  tip_set(hints[index], xx + 8, ry, ww - 16, 40)
  if (toggle)
  {
   if (graph_control(val == 0 ? "Off" : "On", xx + ww - 96, ry + 4, 76, 32, val != 0, false, hints[index])) graph_preference_change(index, 1)
  }
  else
  {
   if (graph_control("-", xx + ww - 160, ry + 4, 32, 32, false, false, "Decrease")) graph_preference_change(index, -1)
   draw_label(string_decimals(val), xx + ww - 92, ry + 20, fa_center, fa_middle, c_accent, 1, font_digits)
   if (graph_control("+", xx + ww - 56, ry + 4, 32, 32, false, false, "Increase")) graph_preference_change(index, 1)
  }
 }
 if (graph_control("<", xx + 8, yy + hh - 44, 32, 32, false, graph_preferences_page == 0, "Previous page")) graph_preferences_page--
 draw_label(string(graph_preferences_page + 1) + " / " + string(pages), xx + 70, yy + hh - 28, fa_center, fa_middle, c_text_secondary, 1, font_label)
 if (graph_control(">", xx + 100, yy + hh - 44, 32, 32, false, graph_preferences_page >= pages - 1, "Next page")) graph_preferences_page++
 if (graph_control("Reset", xx + ww - 184, yy + hh - 44, 76, 32, false, false, "Reset graph preferences only; animation is untouched")) { setting_graph_options = graph_options_defaults(); graph_options_apply() }
 if (graph_control("Save", xx + ww - 100, yy + hh - 44, 84, 32, false, false, "Save application preferences to settings.midata")) { graph_options_capture(); settings_save() }
}
