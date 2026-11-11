/// Shared evaluator: the graph and animation playback use the same function.
function graph_cubic(a, b, c, d, t)
{
	var u = 1 - t;
	return u*u*u*a + 3*u*u*t*b + 3*u*t*t*c + t*t*t*d
}

function graph_bezier_value(v0, v1, x1, dy1, x2, dy2, progress)
{
	// Ordered time handles make x(t) monotonic, including vertical handles.
	var p = clamp(progress, 0, 1);
	if (p <= 0) return v0
	if (p >= 1) return v1
	x1 = clamp(x1, 0, 1)
	x2 = clamp(x2, x1, 1)
	var lo = 0;
	var hi = 1;
	var t = 0.5;
	for (var i = 0; i < 28; i++)
	{
		t = (lo + hi) * 0.5
		if (graph_cubic(0, x1, x2, 1, t) < p) lo = t
		else hi = t
	}
	return graph_cubic(v0, v0 + dy1, v1 + dy2, v1, (lo + hi) * 0.5)
}

function graph_segment_value(left, right, vid, progress)
{
	var v0 = left[vid];
	var v1 = right[vid];
	var p = clamp(progress, 0, 1);
	var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X) * 5;
	var mode = left[base];
	var result = v0;
	if (mode == 0)
	{
		// Preserve the original segment transition until explicitly overridden.
		var transition = left[e_value.TRANSITION];
		var eased = 0;
		if (transition == "bezier")
			eased = ease_bezier_curve([0, 0], [left[e_value.EASE_IN_X], left[e_value.EASE_IN_Y]], [left[e_value.EASE_OUT_X], left[e_value.EASE_OUT_Y]], [1, 1], p)
		else eased = ease(transition, p)
		return tl_value_interpolate(vid, eased, v0, v1)
	}
	if (mode == 1) result = v0 + (v1 - v0) * p
	else if (mode == 3) result = (p >= 1 ? v1 : v0)
	else result = graph_bezier_value(v0, v1, left[base + 1], left[base + 2], left[base + 3], left[base + 4], p)
	return result
}

// Independent transform channels. Existing keyframes remain the union of times.
// MASK defaults to 511, so old projects have the exact former channel membership.
function ga_has(kf, vid)
{
 return (round(kf.value[e_value.GRAPH_KEY_MASK]) & (1 << (vid - e_value.POS_X))) != 0
}
function ga_keys(tl, vid)
{
 var result = array_create(0);
 for (var i = 0; i < ds_list_size(tl.keyframe_list); i++)
 {
  var kf = tl.keyframe_list[|i];
  if (ga_has(kf, vid)) result[array_length(result)] = kf
 }
 return result
}
function ga_next(tl, index, vid, direction)
{
 for (var i = index + direction; i >= 0 && i < ds_list_size(tl.keyframe_list); i += direction)
  if (ga_has(tl.keyframe_list[|i], vid)) return i
 return -1
}
function ga_config_id(vid, field)
{
 return e_value.GRAPH_POS_X_CYCLE + (vid - e_value.POS_X)*14 + field
}
function ga_slope(keys, index, vid, clamped)
{
 var n = array_length(keys);
 if (n < 2) return 0
 var k = keys[index];
 var a = max(0, index - 1);
 var b = min(n - 1, index + 1);
 var d = (keys[b].value[vid] - keys[a].value[vid])/max(0.0001, keys[b].position - keys[a].position);
 if (!clamped || index == 0 || index == n - 1) return d
 var h0 = k.position - keys[index - 1].position;
 var h1 = keys[index + 1].position - k.position;
 var d0 = (k.value[vid] - keys[index - 1].value[vid])/max(0.0001, h0);
 var d1 = (keys[index + 1].value[vid] - k.value[vid])/max(0.0001, h1);
 if (d0*d1 <= 0) return 0
 var w0 = 2*h1 + h0;
 var w1 = h1 + 2*h0;
 return (w0 + w1)/(w0/d0 + w1/d1)
}
function ga_segment(keys, index, vid, frame, tension)
{
 var a = keys[index];
 var b = keys[index + 1];
 var duration = max(0.0001, b.position - a.position);
 var p = clamp((frame - a.position)/duration, 0, 1);
 var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
 var mode = a.value[base];
 if (mode == 4 || mode == 5)
 {
  var strength = mode == 5 ? clamp(tension, 0, 1) : clamp(tension, 0, 2);
  var d0 = ga_slope(keys, index, vid, mode == 5)*strength;
  var d1 = ga_slope(keys, index + 1, vid, mode == 5)*strength;
  return graph_bezier_value(a.value[vid], b.value[vid], 1/3, d0*duration/3, 2/3, -d1*duration/3, p)
 }
 return graph_segment_value(a.value, b.value, vid, p)
}
function ga_noise_hash(index, seed)
{
 var n = sin(index*127.1 + seed*311.7)*43758.5453;
 return (n - floor(n))*2 - 1
}
function ga_noise(frame, frequency, seed)
{
 var x = frame*frequency;
 var i = floor(x);
 var p = x - i;
 p = p*p*(3 - 2*p)
 return lerp(ga_noise_hash(i, seed), ga_noise_hash(i + 1, seed), p)
}
function ga_curve(tl, vid, frame, modifiers = true)
{
 return ga_curve_keys(tl, vid, frame, ga_keys(tl, vid), modifiers)
}
function ga_curve_keys(tl, vid, frame, keys, modifiers = true)
{
 var n = array_length(keys);
 if (n == 0)
 {
  // Preserve the original parented-object position reset when there are no native keys.
  if (vid >= e_value.POS_X && vid <= e_value.POS_Z && tl.parent != app && tl.part_of == null && !instance_exists(tl.keyframe_current) && !instance_exists(tl.keyframe_next)) return 0
  return tl.value_default[vid]
 }
 var first = keys[0];
 var last = keys[n - 1];
 var span = last.position - first.position;
 var config = ga_config_id(vid, 0);
 var time = frame;
 var cycle = tl.value_default[config];
 var offset = 0;
 if (modifiers && cycle > 0 && span > 0 && (time < first.position || time > last.position))
 {
  var cycleindex = floor((time - first.position)/span);
  var local = time - first.position - cycleindex*span;
  if (cycle == 3 && abs(cycleindex) mod 2 == 1) local = span - local
  time = first.position + local
  if (cycle == 2) offset = cycleindex*(last.value[vid] - first.value[vid])
 }
 var result = first.value[vid];
 var tension = tl.value_default[config + 2];
 if (time >= last.position) result = last.value[vid]
 else if (time > first.position)
 {
  for (var i = 0; i < n - 1; i++)
   if (time <= keys[i + 1].position) { result = ga_segment(keys, i, vid, time, tension); break }
 }
 if (modifiers && tl.value_default[config + 1] > 0 && n > 1 && (time < first.position || time > last.position))
 {
  // Extrapolation follows the endpoint derivative, not the distant secant.
  var before = time < first.position;
  var seg = before ? 0 : n - 2;
  var endpoint = before ? first.position : last.position;
  var eps = max(0.0001, (keys[seg + 1].position - keys[seg].position)*0.0001);
  var slope = before ? (ga_segment(keys, seg, vid, endpoint + eps, tension) - first.value[vid])/eps : (last.value[vid] - ga_segment(keys, seg, vid, endpoint - eps, tension))/eps;
  result += (time - endpoint)*slope
 }
 result += offset
 if (!modifiers) return result
 result += tl.value_default[config + 3]*ga_noise(frame, tl.value_default[config + 4], tl.value_default[config + 5])
 if (tl.value_default[config + 6] > 0)
 {
  var start = tl.value_default[config + 7];
  var end = max(start, tl.value_default[config + 8]);
  var fade = max(0.0001, tl.value_default[config + 10]);
  var weight = clamp(min((frame - start)/fade, (end - frame)/fade), 0, 1);
  weight = weight*weight*(3 - 2*weight)
  result *= lerp(1, tl.value_default[config + 9], weight)
 }
 if (tl.value_default[config + 11] > 0)
 {
  var low = min(tl.value_default[config + 12], tl.value_default[config + 13]);
  var high = max(tl.value_default[config + 12], tl.value_default[config + 13]);
  result = clamp(result, low, high)
 }
 return tl_value_clamp(vid, result)
}
function ga_state(tl)
{
 var rows = array_create(0);
 var defaults = array_create(e_value.amount, 0);
 for (var v = 0; v < e_value.amount; v++) defaults[v] = tl_value_get_save_id(v, tl.value_default[v])
 for (var k = 0; k < ds_list_size(tl.keyframe_list); k++)
 {
  var kf = tl.keyframe_list[|k];
  var values = array_create(e_value.amount, 0);
  for (var v = 0; v < e_value.amount; v++) values[v] = tl_value_get_save_id(v, kf.value[v])
  rows[k] = [kf.position, values]
 }
 return [defaults, rows]
}
function ga_restore(tl, state)
{
 tl_keyframes_deselect_all()
 with (tl)
 {
  while (ds_list_size(keyframe_list) > 0)
   with (keyframe_list[|0]) instance_destroy()
  for (var v = 0; v < e_value.amount; v++) value_default[v] = tl_value_find_save_id(v, null, state[0][v])
  for (var k = 0; k < array_length(state[1]); k++)
  {
   var kf = new_obj(obj_keyframe);
   kf.timeline = id
   kf.position = state[1][k][0]
   kf.selected = false
   kf.sound_play_index = null
   for (var v = 0; v < e_value.amount; v++) kf.value[v] = tl_value_find_save_id(v, null, state[1][k][1][v])
   if (kf.value[e_value.SOUND_OBJ] != null) kf.value[e_value.SOUND_OBJ].count++
   ds_list_add(keyframe_list, kf)
  }
  keyframe_current = null
  keyframe_next = null
  keyframe_prev = null
  keyframe_current_values = null
  keyframe_next_values = null
  keyframe_animate = false
  keyframe_list_index_prev = 0
  keyframe_list_index_prev_position = 0
 }
 graph_selection_clear()
 graph_refresh_target(tl)
 tl_update_length()
 app_update_tl_edit()
}
// True when the next commit of this gesture merges into the newest Undo record.
function ga_can_merge(tl, gesture)
{
 if (gesture == "" || history_pos != 0 || !instance_exists(history[0]) || history[0].script != action_graph_channels) return false
 return history[0].ga_owner == save_id_get(tl) && history[0].ga_gesture == gesture && history[0].ga_token == ga_config_token
}
function ga_commit(tl, before, gesture = "")
{
 var h = null;
 if (ga_can_merge(tl, gesture)) h = history[0]
 if (h == null)
 {
  h = history_set(action_graph_channels)
  h.ga_before = before
 }
 h.ga_gesture = gesture
 h.ga_token = ga_config_token
 h.ga_owner = save_id_get(tl)
 // GE-R1: no full snapshot per slider tick. The "after" state is captured
 // when this record is undone - at that moment the project is exactly in it.
 h.ga_after = undefined
 graph_refresh_target(tl)
}
function action_graph_channels()
{
 if (!history_undo && !history_redo) return 0
 var tl = save_id_find(history_data.ga_owner);
 if (!instance_exists(tl)) return 0
 if (history_undo && is_undefined(history_data.ga_after)) history_data.ga_after = ga_state(tl)
 var state = history_undo ? history_data.ga_before : history_data.ga_after;
 if (!is_undefined(state)) ga_restore(tl, state)
}
function ga_enable(tl)
{
 tl.value_default[e_value.GRAPH_CHANNELS_USED] = 1
}
function ga_put(tl, vid, frame, val, source = null)
{
 var kf = null;
 frame = max(0, round(frame))
 for (var i = 0; i < ds_list_size(tl.keyframe_list); i++)
  if (tl.keyframe_list[|i].position == frame) { kf = tl.keyframe_list[|i]; break }
 if (kf == null)
 {
  with (tl) kf = tl_keyframe_add(frame)
  kf.value[e_value.GRAPH_KEY_MASK] = 0
  kf.value[e_value.GRAPH_ONLY] = 1
  // Graph-only keys must not introduce sound playback events.
  if (kf.value[e_value.SOUND_OBJ] != null) kf.value[e_value.SOUND_OBJ].count--
  kf.value[e_value.SOUND_OBJ] = null
 }
 var bit = 1 << (vid - e_value.POS_X);
 var mask = round(kf.value[e_value.GRAPH_KEY_MASK]);
 if ((mask & bit) == 0) kf.value[e_value.GRAPH_KEY_MASK] = mask + bit
 kf.value[vid] = tl_value_clamp(vid, val)
 if (instance_exists(source))
 {
  var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
  for (var f = 0; f < 5; f++) kf.value[base + f] = source.value[base + f]
 }
 ga_enable(tl)
 return kf
}
function ga_delete_selected()
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(graph_sel_keys) == 0) return 0
 var before = ga_state(graph_target);
 for (var i = 0; i < array_length(graph_sel_keys); i++)
 {
  var kf = graph_sel_keys[i];
  var vid = graph_sel_vids[i];
  if (instance_exists(kf) && ga_has(kf, vid)) kf.value[e_value.GRAPH_KEY_MASK] -= 1 << (vid - e_value.POS_X)
 }
 ga_enable(graph_target)
 graph_selection_clear()
 ga_commit(graph_target, before)
}
function ga_add_active()
{
 if (!instance_exists(graph_target) || graph_target.lock) return 0
 var vid = e_value.POS_X + graph_group*3 + graph_axis;
 var before = ga_state(graph_target);
 var val = ga_curve(graph_target, vid, timeline_marker);
 var kf = ga_put(graph_target, vid, timeline_marker, val);
 graph_selection_clear()
 graph_selection_add(kf, vid)
 graph_selected = kf
 ga_commit(graph_target, before)
}
// Reject collisions atomically. Never silently overwrite a channel key.
function ga_move_conflict(delta, duplicate)
{
 if (!instance_exists(graph_target)) return true
 delta = round(delta)
 for (var i = 0; i < array_length(graph_sel_keys); i++)
 {
  var kf = graph_sel_keys[i];
  if (!instance_exists(kf)) return true
  var vid = graph_sel_vids[i];
  var frame = kf.position + delta;
  if (frame < 0) return true
  var keys = ga_keys(graph_target, vid);
  for (var j = 0; j < array_length(keys); j++)
   if (keys[j].position == frame && (duplicate || graph_selection_find(keys[j], vid) < 0)) return true
 }
 return false
}
function ga_move_selected(delta, duplicate, before_state = undefined)
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(graph_sel_keys) == 0) return false
 delta = round(delta)
 if (delta == 0) return false
 if (ga_move_conflict(delta, duplicate)) return false
 var before = is_undefined(before_state) ? ga_state(graph_target) : before_state;
 var oldkeys = graph_sel_keys;
 var oldvids = graph_sel_vids;
 if (!duplicate)
  for (var i = 0; i < array_length(oldkeys); i++) oldkeys[i].value[e_value.GRAPH_KEY_MASK] -= 1 << (oldvids[i] - e_value.POS_X)
 // Copy source values before merging into shared destination keyframes.
 var data = array_create(0);
 for (var i = 0; i < array_length(oldkeys); i++)
 {
  var vid = oldvids[i];
  var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
  var fields = array_create(5, 0);
  for (var f = 0; f < 5; f++) fields[f] = oldkeys[i].value[base + f]
  data[i] = [oldkeys[i].position + delta, vid, oldkeys[i].value[vid], fields]
 }
 graph_selection_clear()
 for (var i = 0; i < array_length(data); i++)
 {
  var row = data[i];
  var kf = ga_put(graph_target, row[1], row[0], row[2]);
  var base = e_value.GRAPH_POS_X_MODE + (row[1] - e_value.POS_X)*5;
  for (var f = 0; f < 5; f++) kf.value[base + f] = row[3][f]
  graph_selection_add(kf, row[1])
  graph_selected = kf
 }
 ga_commit(graph_target, before)
 return true
}
function ga_auto(mode)
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(graph_sel_keys) == 0) return 0
 var before = ga_state(graph_target);
 for (var i = 0; i < array_length(graph_sel_keys); i++)
 {
  var vid = graph_sel_vids[i];
  var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
  graph_sel_keys[i].value[base] = mode
 }
 ga_enable(graph_target)
 ga_commit(graph_target, before)
}
function ga_filter_selected(kind)
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(graph_sel_keys) == 0) return 0
 var before = ga_state(graph_target);
 var output = array_create(0);
 for (var i = 0; i < array_length(graph_sel_keys); i++)
 {
  var kf = graph_sel_keys[i];
  var vid = graph_sel_vids[i];
  var result = kf.value[vid];
  var keys = ga_keys(graph_target, vid);
  var index = -1;
  for (var j = 0; j < array_length(keys); j++) if (keys[j] == kf) index = j
  if (kind == 0 && index > 0 && index < array_length(keys) - 1)
  {
   var a = keys[index - 1];
   var b = keys[index + 1];
   var mean = lerp(a.value[vid], b.value[vid], (kf.position - a.position)/max(1, b.position - a.position));
   result = lerp(result, mean, ga_strength)
  }
  if (kind == 1) result = ga_pivot_value*2 - result
   output[i] = result
 }
 for (var i = 0; i < array_length(output); i++) graph_sel_keys[i].value[graph_sel_vids[i]] = tl_value_clamp(graph_sel_vids[i], output[i])
 if (kind == 2)
 {
  for (var vid = e_value.ROT_X; vid <= e_value.ROT_Z; vid++)
  {
   var keys = ga_keys(graph_target, vid);
   for (var j = 1; j < array_length(keys); j++)
   {
    if (graph_selection_find(keys[j], vid) < 0) continue
    var previous = keys[j - 1].value[vid];
    var delta = keys[j].value[vid] - previous;
    keys[j].value[vid] = previous + delta - 360*floor((delta + 180)/360)
   }
  }
 }

 if (kind == 1)
 {
  for (var i = 0; i < array_length(graph_sel_keys); i++)
  {
   var vid = graph_sel_vids[i];
   var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
   graph_sel_keys[i].value[base + 2] *= -1
   graph_sel_keys[i].value[base + 4] *= -1
  }
 }
 ga_commit(graph_target, before)
}
function ga_fit(tl, wid, hei, selection)
{
 if (!instance_exists(tl)) { graph_fit_pending = false; return 0 }
 var first = 0;
 var last = max(10, timeline_marker);
 var low = 0;
 var high = 1;
 var found = false;
 if (selection && array_length(graph_sel_keys) > 0)
 {
  first = graph_sel_keys[0].position
  last = first
  for (var i = 0; i < array_length(graph_sel_keys); i++) { first = min(first, graph_sel_keys[i].position); last = max(last, graph_sel_keys[i].position) }
 }
 else if (ds_list_size(tl.keyframe_list) > 0) { first = tl.keyframe_list[|0].position; last = tl.keyframe_list[|ds_list_size(tl.keyframe_list) - 1].position }
 for (var axis = 0; axis < 3; axis++)
 {
  if (!graph_visible[axis]) continue
  var vid = e_value.POS_X + graph_group*3 + axis;
  var curvekeys = ga_keys(tl, vid);
  for (var s = 0; s <= 128; s++)
  {
   var val = ga_curve_keys(tl, vid, lerp(first, last, s/128), curvekeys);
   if (!found) { low = val; high = val; found = true }
   low = min(low, val)
   high = max(high, val)
  }
 }
 graph_x_scale = clamp((wid - 40)/max(10, last - first), 0.25, 128)
 graph_x_start = max(0, first - 20/graph_x_scale)
 graph_y_center = (low + high)/2
 graph_y_scale = clamp((hei - 32)/max(0.01, high - low), 0.0001, 100000)
 graph_fit_pending = false
}

// Native advanced tools: no browser runtime and no external UI dependencies.
function ga_copy()
{
 ga_clipboard = array_create(0)
 if (array_length(graph_sel_keys) == 0) return 0
 var first = graph_sel_keys[0].position;
 for (var i = 0; i < array_length(graph_sel_keys); i++) first = min(first, graph_sel_keys[i].position)
 for (var i = 0; i < array_length(graph_sel_keys); i++)
 {
  var kf = graph_sel_keys[i];
  var vid = graph_sel_vids[i];
  var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
  ga_clipboard[i] = [kf.position - first, vid, kf.value[vid], kf.value[base], kf.value[base + 1], kf.value[base + 2], kf.value[base + 3], kf.value[base + 4]]
 }
 ga_message = string(array_length(ga_clipboard)) + " channel points copied"
}
function ga_paste()
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(ga_clipboard) == 0) return 0
 var onechannel = true;
 var firstvid = ga_clipboard[0][1];
 var activevid = e_value.POS_X + graph_group*3 + graph_axis;
 for (var i = 0; i < array_length(ga_clipboard); i++) if (ga_clipboard[i][1] != firstvid) onechannel = false
 for (var i = 0; i < array_length(ga_clipboard); i++)
 {
  var row = ga_clipboard[i];
  var vid = onechannel ? activevid : row[1];
  var keys = ga_keys(graph_target, vid);
  if (!graph_target.value_type[floor((vid - e_value.POS_X)/3) == 0 ? e_value_type.TRANSFORM_POS : (floor((vid - e_value.POS_X)/3) == 1 ? e_value_type.TRANSFORM_ROT : e_value_type.TRANSFORM_SCA)]) { ga_message = "Target does not support the copied transform"; return 0 }
  for (var j = 0; j < array_length(keys); j++)
   if (keys[j].position == round(timeline_marker) + row[0]) { ga_message = "Paste cancelled: destination channel key exists"; return 0 }
 }
 var before = ga_state(graph_target);
 graph_selection_clear()
 for (var i = 0; i < array_length(ga_clipboard); i++)
 {
  var row = ga_clipboard[i];
  var vid = onechannel ? activevid : row[1];
  var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
  var kf = ga_put(graph_target, vid, round(timeline_marker) + row[0], row[2]);
  for (var f = 0; f < 5; f++) kf.value[base + f] = row[3 + f]
  graph_selection_add(kf, vid)
  graph_selected = kf
 }
 ga_commit(graph_target, before)
 ga_message = "Pasted channel points; other channels were not replaced"
}
function ga_scale_values()
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(graph_sel_keys) == 0) return 0
 var before = ga_state(graph_target);
 for (var i = 0; i < array_length(graph_sel_keys); i++)
 {
  var vid = graph_sel_vids[i];
  var kf = graph_sel_keys[i];
  var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
  kf.value[vid] = tl_value_clamp(vid, ga_pivot_value + (kf.value[vid] - ga_pivot_value)*ga_scale)
  kf.value[base + 2] *= ga_scale
  kf.value[base + 4] *= ga_scale
 }
 ga_commit(graph_target, before)
}
function ga_simplify()
{
 if (!instance_exists(graph_target) || graph_target.lock || array_length(graph_sel_keys) < 3) return 0
 var before = ga_state(graph_target);
 var removed = 0;
 // Only linear triples are eligible: their maximum error occurs at the removed key.
 // Nonlinear, modified and legacy segments are deliberately not approximated.
 for (var vid = e_value.POS_X; vid <= e_value.SCA_Z; vid++)
 {
  var config = ga_config_id(vid, 0);
  var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
  if (graph_target.value_default[config] != 0 || graph_target.value_default[config + 3] != 0 || graph_target.value_default[config + 6] != 0 || graph_target.value_default[config + 11] != 0) continue
  var original = ga_keys(graph_target, vid);
  var keys = ga_keys(graph_target, vid);
  var originalvalues = array_create(0);
  for (var i = 0; i < array_length(original); i++) originalvalues[i] = original[i].value[vid]
  for (var i = 1; i < array_length(keys) - 1; i++)
  {
   var a = keys[i - 1];
   var b = keys[i];
   var c = keys[i + 1];
   if (graph_selection_find(a, vid) < 0 || graph_selection_find(b, vid) < 0 || graph_selection_find(c, vid) < 0) continue
   if (a.value[base] != 1 || b.value[base] != 1) continue
   var safe = true;
   for (var j = 0; j < array_length(original); j++)
   {
    var frame = original[j].position;
    if (frame < a.position || frame > c.position) continue
    var val = lerp(a.value[vid], c.value[vid], (frame - a.position)/max(1, c.position - a.position));
    if (abs(val - originalvalues[j]) > ga_tolerance) safe = false
   }
   if (safe)
   {
    b.value[e_value.GRAPH_KEY_MASK] -= 1 << (vid - e_value.POS_X)
    removed++
    keys = ga_keys(graph_target, vid)
    i--
   }
  }
 }
 if (removed > 0) { ga_enable(graph_target); ga_commit(graph_target, before); graph_selection_clean(graph_target) }
 ga_message = string(removed) + " linear channel keys removed within tolerance"
}
function ga_set_config(field, val, add)
{
 if (!instance_exists(graph_target) || graph_target.lock) return 0
 var vid = e_value.POS_X + graph_group*3 + graph_axis;
 var target = ga_config_id(vid, field);
 var oldval = graph_target.value_default[target];
 var newval = tl_value_clamp(target, add ? oldval + val : val);
 if (oldval == newval) return 0
 var before = (add && ga_can_merge(graph_target, "config" + string(target))) ? undefined : ga_state(graph_target);
 graph_target.value_default[target] = newval
 ga_enable(graph_target)
 if (!add) ga_config_token++
 ga_commit(graph_target, before, "config" + string(target))
}
function ga_ghost_capture()
{
 if (!instance_exists(graph_target)) return 0
 ga_ghost_owner = save_id_get(graph_target)
 ga_ghost_group = graph_group
 ga_ghost = array_create(0)
 var span = max(10, ga_plot_width/graph_x_scale);
 for (var i = 0; i <= 200; i++)
 {
  var frame = graph_x_start + span*i/200;
  var values = array_create(3, 0);
  for (var axis = 0; axis < 3; axis++) values[axis] = ga_curve(graph_target, e_value.POS_X + graph_group*3 + axis, frame)
  ga_ghost[i] = [frame, values]
 }
 ga_message = "Reference captured for this object and transform group"
}
function ga_tools_open()
{
 ga_tools = true
 ga_tools_view = graph_view_index
 ga_message = "GC-A1 - use a copy of your project"
}
function ga_tools_panel()
{
 graph_view_index = ga_tools_view
 graph_view_load(ga_tools_view)
 graph_target = graph_timeline
 var xx = content_x;
 var yy = content_y;
 var ww = content_width;
 var hh = content_height;
 content_mouseon = !popup_mouseon && !toast_mouseon && !context_menu_mouseon && app_mouse_box(xx, yy, ww, hh, "place")
 if (content_mouseon && mouse_left_pressed) ga_config_token++
 draw_box(xx, yy, ww, hh, false, c_level_middle, 1)
 if (graph_button("Back", xx + 8, yy + 6, 60, false, false, "Back to curves")) ga_tools = false
 draw_label("CHANNEL TOOLS / GC-A1", xx + 84, yy + 18, fa_left, fa_middle, c_accent, 1, font_label)
 if (ww < 540 || hh < 380)
 {
  draw_label("Enlarge panel to at least 540 x 380 for advanced tools", xx + 16, yy + 64, fa_left, fa_middle, c_text_secondary, 1, font_label)
  return 0
 }
 if (!instance_exists(graph_target))
 {
  draw_label("Select an object with transform channels first", xx + 16, yy + 64, fa_left, fa_middle, c_text_secondary, 1, font_label)
  return 0
 }
 var disabled = graph_target.lock;
 var vid = e_value.POS_X + graph_group*3 + graph_axis;
 var groupnames = ["Position", "Rotation", "Scale"];
 var context = graph_target.display_name + " / " + groupnames[graph_group] + " " + chr(ord("X") + graph_axis);
 draw_label(string_copy(context, 1, max(4, floor((ww - 350)/7))), xx + 340, yy + 18, fa_left, fa_middle, c_text_secondary, 1, font_label)
 var tabs = ["Keys", "Channels", "Modifiers", "Handles"];
 for (var i = 0; i < 4; i++)
  if (graph_button(tabs[i], xx + 8 + i*112, yy + 40, 106, ga_tools_tab == i, false, "Settings stay with their channel")) ga_tools_tab = i
 clip_begin(xx + 8, yy + 74, ww - 16, hh - 104)
 var row = yy + 80;
 var fieldw = min(160, ww/4);
 if (ga_tools_tab == 0)
 {
  var names = ["Auto", "Clamped", "Copy", "Paste", "Ghost", "Smooth", "Mirror value", "Euler filter", "Simplify", "Scale values", "Move time", "Duplicate"];
  for (var i = 0; i < 12; i++)
  {
   var col = i mod 4;
   var line = floor(i/4);
   if (graph_button(names[i], xx + 8 + col*128, row + line*30, 122, false, disabled, names[i]))
   {
    if (i == 0) ga_auto(4)
    if (i == 1) ga_auto(5)
    if (i == 2) ga_copy()
    if (i == 3) ga_paste()
    if (i == 4) ga_ghost_capture()
    if (i == 5) ga_filter_selected(0)
    if (i == 6) ga_filter_selected(1)
    if (i == 7) ga_filter_selected(2)
    if (i == 8) ga_simplify()
    if (i == 9) ga_scale_values()
    if (i == 10 && !ga_move_selected(ga_time_offset, false)) ga_message = "Move cancelled: zero offset, collision or negative frame"
    if (i == 11 && !ga_move_selected(ga_time_offset, true)) ga_message = "Duplicate cancelled: zero offset or occupied channel key"
   }
  }
  row += 100
  var labels = ["Time offset / frames", "Value scale", "Value pivot", "Smooth strength 0..1", "Simplify tolerance"];
  var values = [ga_time_offset, ga_scale, ga_pivot_value, ga_strength, ga_tolerance];
  var scripts = [ga_setting_time, ga_setting_scale, ga_setting_pivot, ga_setting_strength, ga_setting_tolerance];
  for (var i = 0; i < 5; i++)
  {
   draw_label(labels[i], xx + 16, row + 12, fa_left, fa_middle, c_text_secondary, 1, font_label)
   draw_dragger("gasetting" + string(i), xx + 240, row, fieldw, values[i], 0.05, i == 3 || i == 4 ? 0 : -10000, i == 3 ? 1 : 10000, i == 1 ? 1 : 0, i == 0 ? 1 : 0.01, ga_setting_boxes[i], scripts[i], 0, false, disabled)
   row += 28
  }
 }
 if (ga_tools_tab == 1)
 {
  draw_label("Filter channels", xx + 16, row + 12, fa_left, fa_middle, c_text_secondary, 1, font_label)
  textbox_draw(ga_filter_box, xx + 160, row, 240, 24, true)
  row += 34
  var groups = ["Position", "Rotation", "Scale"];
  for (var g = 0; g < 3; g++)
  {
   var channel_type_id = g == 0 ? e_value_type.TRANSFORM_POS : (g == 1 ? e_value_type.TRANSFORM_ROT : e_value_type.TRANSFORM_SCA);
   if (!graph_target.value_type[channel_type_id]) continue
   for (var axis = 0; axis < 3; axis++)
   {
    var label = groups[g] + " " + chr(ord("X") + axis);
    if (ga_filter_box.text != "" && string_pos(string_lower(ga_filter_box.text), string_lower(label)) == 0) continue
    if (graph_button(label, xx + 16 + g*164, row + axis*30, 156, graph_group == g && graph_axis == axis, disabled, "Choose a channel; the object remains pinned"))
    {
     graph_group = g
     graph_axis = axis
     graph_visible[axis] = true
     graph_selection_clear()
     graph_fit_pending = true
    }
   }
  }
  row += 102
  draw_label("Search covers the selected object's transform channels.", xx + 16, row, fa_left, fa_middle, c_text_secondary, 1, font_label)
  row += 30
  if (graph_button("Select curve", xx + 16, row, 120, false, disabled, "Select every enabled key of the active channel"))
  {
   graph_selection_clear()
   var keys = ga_keys(graph_target, vid);
   for (var i = 0; i < array_length(keys); i++) graph_selection_add(keys[i], vid)
   if (array_length(keys) > 0) graph_selected = keys[0]
  }
  if (graph_button("Clear ghost", xx + 144, row, 120, false, false, "Discard reference without changing animation")) ga_ghost = array_create(0)
 }
 if (ga_tools_tab == 2)
 {
  var labels = ["Cycle: 0 off, 1 repeat, 2 offset, 3 ping-pong", "Extrapolate: 0 hold, 1 linear", "Auto tension (clamped caps at 1)", "Noise amplitude (0 disables)", "Noise frequency / frame", "Noise seed", "Envelope: 0 off, 1 on", "Envelope start", "Envelope end", "Envelope gain", "Envelope fade / frames", "Limits: 0 off, 1 on", "Limit minimum", "Limit maximum"];
  var scripts = [ga_config_0, ga_config_1, ga_config_2, ga_config_3, ga_config_4, ga_config_5, ga_config_6, ga_config_7, ga_config_8, ga_config_9, ga_config_10, ga_config_11, ga_config_12, ga_config_13];
  var pagecount = max(1, floor((hh - 130)/28));
  pagecount = min(14, pagecount)
  if (ga_config_page*pagecount >= 14) ga_config_page = 0
  for (var i = ga_config_page*pagecount; i < min(14, (ga_config_page + 1)*pagecount); i++)
  {
   var field = ga_config_id(vid, i);
   draw_label(string_copy(labels[i], 1, max(8, floor((min(400, ww - fieldw - 24) - 28)/7))), xx + 16, row + 12, fa_left, fa_middle, c_text_secondary, 1, font_label)
   tip_set(labels[i], xx + 16, row, min(384, ww - fieldw - 44), 24)
   draw_dragger("gaconfig" + string(i), xx + min(400, ww - fieldw - 24), row, fieldw, graph_target.value_default[field], 0.01, -100000, 100000, tl_value_default(field), 0.001, ga_config_boxes[i], scripts[i], 0, false, disabled)
   row += 28
  }
  if (14 > pagecount && graph_button("Next settings", xx + 16, row + 2, 140, false, false, "More modifier fields")) ga_config_page = (ga_config_page + 1) mod ceil(14/pagecount)
 }
 if (ga_tools_tab == 3)
 {
  var kf = graph_selected;
  var keys = ga_keys(graph_target, vid);
  var nextindex = -1;
  if (instance_exists(kf)) nextindex = ga_next(graph_target, ds_list_find_index(graph_target.keyframe_list, kf), vid, 1)
  if (nextindex < 0) draw_label("Select a channel key with a following key first.", xx + 16, row + 16, fa_left, fa_middle, c_text_secondary, 1, font_label)
  else
  {
   var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
   var labels = ["Outgoing X / segment fraction", "Outgoing Y / value offset", "Incoming X / segment fraction", "Incoming Y / value offset"];
   var scripts = [ga_handle_1, ga_handle_2, ga_handle_3, ga_handle_4];
   for (var i = 0; i < 4; i++)
   {
    draw_label(labels[i], xx + 16, row + 12, fa_left, fa_middle, c_text_secondary, 1, font_label)
    draw_dragger("gahandle" + string(i), xx + 330, row, fieldw, kf.value[base + i + 1], 0.01, -100000, 100000, i == 0 ? 1/3 : (i == 2 ? 2/3 : 0), 0.001, ga_handle_boxes[i], scripts[i], 0, false, disabled)
    row += 32
   }
   draw_label("Fields edit one outgoing segment. X is ordered 0..1.", xx + 16, row + 16, fa_left, fa_middle, c_text_secondary, 1, font_label)
  }
 }
 clip_end()
 clip_begin(xx, yy + hh - 26, ww, 26)
 draw_label(ga_message, xx + 12, yy + hh - 13, fa_left, fa_middle, c_text_secondary, 1, font_label)
 clip_end()
 graph_view_save(ga_tools_view)
}
function ga_set_handle(field, val, add)
{
 if (!instance_exists(graph_target) || graph_target.lock || !instance_exists(graph_selected)) return 0
 var vid = e_value.POS_X + graph_group*3 + graph_axis;
 var base = e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X)*5;
 var before = (add && ga_can_merge(graph_target, "handle" + string(vid) + ":" + string(field))) ? undefined : ga_state(graph_target);
 var value = add ? graph_selected.value[base + field] + val : val;
 if (field == 1) value = clamp(value, 0, graph_selected.value[base + 3])
 if (field == 3) value = clamp(value, graph_selected.value[base + 1], 1)
 if (field == 2 || field == 4) value = clamp(value, -1000000, 1000000)
 graph_selected.value[base] = 2
 graph_selected.value[base + field] = value
 ga_enable(graph_target)
 if (!add) ga_config_token++
 ga_commit(graph_target, before, "handle" + string(vid) + ":" + string(field))
}
function ga_setting_time(val, add) { ga_time_offset = clamp(round(add ? ga_time_offset + val : val), -10000, 10000) }
function ga_setting_scale(val, add) { ga_scale = clamp(add ? ga_scale + val : val, -100, 100) }
function ga_setting_pivot(val, add) { ga_pivot_value = clamp(add ? ga_pivot_value + val : val, -1000000, 1000000) }
function ga_setting_strength(val, add) { ga_strength = clamp(add ? ga_strength + val : val, 0, 1) }
function ga_setting_tolerance(val, add) { ga_tolerance = clamp(add ? ga_tolerance + val : val, 0, 100000) }

function ga_config_0(val, add) { ga_set_config(0, val, add) }
function ga_config_1(val, add) { ga_set_config(1, val, add) }
function ga_config_2(val, add) { ga_set_config(2, val, add) }
function ga_config_3(val, add) { ga_set_config(3, val, add) }
function ga_config_4(val, add) { ga_set_config(4, val, add) }
function ga_config_5(val, add) { ga_set_config(5, val, add) }
function ga_config_6(val, add) { ga_set_config(6, val, add) }
function ga_config_7(val, add) { ga_set_config(7, val, add) }
function ga_config_8(val, add) { ga_set_config(8, val, add) }
function ga_config_9(val, add) { ga_set_config(9, val, add) }
function ga_config_10(val, add) { ga_set_config(10, val, add) }
function ga_config_11(val, add) { ga_set_config(11, val, add) }
function ga_config_12(val, add) { ga_set_config(12, val, add) }
function ga_config_13(val, add) { ga_set_config(13, val, add) }
function ga_handle_1(val, add) { ga_set_handle(1, val, add) }
function ga_handle_2(val, add) { ga_set_handle(2, val, add) }
function ga_handle_3(val, add) { ga_set_handle(3, val, add) }
function ga_handle_4(val, add) { ga_set_handle(4, val, add) }
