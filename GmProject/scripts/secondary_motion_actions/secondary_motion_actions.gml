/// Secondary motion: all settings use ordinary keyframes and Undo.

function action_tl_secondary_enabled(enable)
{
	tl_value_set_start(action_tl_secondary_enabled, false)
	tl_value_set(e_value.SECONDARY_ENABLED, enable, false)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_blend(val, add)
{
	tl_value_set_start(action_tl_secondary_blend, true)
	tl_value_set(e_value.SECONDARY_BLEND, val / 100, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_frequency(val, add)
{
	tl_value_set_start(action_tl_secondary_frequency, true)
	tl_value_set(e_value.SECONDARY_FREQUENCY, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_damping(val, add)
{
	tl_value_set_start(action_tl_secondary_damping, true)
	tl_value_set(e_value.SECONDARY_DAMPING, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_inertia(val, add)
{
	tl_value_set_start(action_tl_secondary_inertia, true)
	tl_value_set(e_value.SECONDARY_INERTIA, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_gravity(val, add)
{
	tl_value_set_start(action_tl_secondary_gravity, true)
	tl_value_set(e_value.SECONDARY_GRAVITY, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_limit(val, add)
{
	tl_value_set_start(action_tl_secondary_limit, true)
	tl_value_set(e_value.SECONDARY_LIMIT, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_anchor_x(val, add)
{
	tl_value_set_start(action_tl_secondary_anchor_x, true)
	tl_value_set(e_value.SECONDARY_ANCHOR_X, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_anchor_y(val, add)
{
	tl_value_set_start(action_tl_secondary_anchor_y, true)
	tl_value_set(e_value.SECONDARY_ANCHOR_Y, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_anchor_z(val, add)
{
	tl_value_set_start(action_tl_secondary_anchor_z, true)
	tl_value_set(e_value.SECONDARY_ANCHOR_Z, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_tip_x(val, add)
{
	tl_value_set_start(action_tl_secondary_tip_x, true)
	tl_value_set(e_value.SECONDARY_TIP_X, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_tip_y(val, add)
{
	tl_value_set_start(action_tl_secondary_tip_y, true)
	tl_value_set(e_value.SECONDARY_TIP_Y, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_tip_z(val, add)
{
	tl_value_set_start(action_tl_secondary_tip_z, true)
	tl_value_set(e_value.SECONDARY_TIP_Z, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_wind_x(val, add)
{
	tl_value_set_start(action_tl_secondary_wind_x, true)
	tl_value_set(e_value.SECONDARY_WIND_X, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_wind_y(val, add)
{
	tl_value_set_start(action_tl_secondary_wind_y, true)
	tl_value_set(e_value.SECONDARY_WIND_Y, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_wind_z(val, add)
{
	tl_value_set_start(action_tl_secondary_wind_z, true)
	tl_value_set(e_value.SECONDARY_WIND_Z, val / 1, add)
	with (obj_timeline)
		if (selected) secondary_motion_release(id)
	tl_value_set_done()
}

function action_tl_secondary_reset()
{
	with (obj_timeline)
	{
		if (selected)
		{
			secondary_motion_release(id)
			update_matrix = true
		}
	}
	with (app)
	{
		tl_update_matrix()
		render_samples = -1
	}
}

// Secondary Motion v2: preset transactions and a non-destructive preview.
function secondary_preset_keys() { return ["enabled", "blend", "frequency", "damping", "inertia", "gravity", "swing_limit", "anchor_x", "anchor_y", "anchor_z", "tip_x", "tip_y", "tip_z", "wind_x", "wind_y", "wind_z", "mode", "stiff_x", "stiff_y", "stiff_z", "damp_x", "damp_y", "damp_z", "travel_x", "travel_y", "travel_z"] }
function secondary_preset_ids() { return [e_value.SECONDARY_ENABLED, e_value.SECONDARY_BLEND, e_value.SECONDARY_FREQUENCY, e_value.SECONDARY_DAMPING, e_value.SECONDARY_INERTIA, e_value.SECONDARY_GRAVITY, e_value.SECONDARY_LIMIT, e_value.SECONDARY_ANCHOR_X, e_value.SECONDARY_ANCHOR_Y, e_value.SECONDARY_ANCHOR_Z, e_value.SECONDARY_TIP_X, e_value.SECONDARY_TIP_Y, e_value.SECONDARY_TIP_Z, e_value.SECONDARY_WIND_X, e_value.SECONDARY_WIND_Y, e_value.SECONDARY_WIND_Z, e_value.SECONDARY_MODE, e_value.SECONDARY_STIFF_X, e_value.SECONDARY_STIFF_Y, e_value.SECONDARY_STIFF_Z, e_value.SECONDARY_DAMP_X, e_value.SECONDARY_DAMP_Y, e_value.SECONDARY_DAMP_Z, e_value.SECONDARY_TRAVEL_X, e_value.SECONDARY_TRAVEL_Y, e_value.SECONDARY_TRAVEL_Z] }
function secondary_preset_names() { return ["Hair - soft", "Hair - short", "Hair - heavy", "Tail - flexible", "Tail - restrained", "Pendant", "Soft body - subtle", "Soft body - water light", "Soft body - water heavy", "Soft body - supported", "Ears - soft", "Accessory - firm"] }
function secondary_builtin(index)
{
 var presets = [[1, 1, 2.2, 0.45, 1, 32, 50, 0, 0, 0, 0, 0, -8, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5], [1, 1, 4, 0.8, 1, 32, 25, 0, 0, 0, 0, 0, -8, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5], [1, 1, 1.6, 0.65, 1, 45, 40, 0, 0, 0, 0, 0, -8, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5], [1, 1, 1.8, 0.5, 1, 32, 55, 0, 0, 0, 0, 0, -12, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5], [1, 1, 3.2, 0.85, 1, 32, 30, 0, 0, 0, 0, 0, -8, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5], [1, 1, 2.8, 0.35, 1, 32, 35, 0, 0, 0, 0, 0, -4, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5], [1, 1, 3.4, 0.65, 1, 12, 60, 0, 0, 0, 0, 0, -8, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0.2, 0.25, 0.3], [1, 1, 2.1, 0.28, 1.05, 22, 60, 0, 0, 0, 0, 0, -8, 0, 0, 0, 1, 1.4, 1, 0.75, 1.2, 1, 0.9, 0.3, 0.5, 0.65], [1, 1, 1.7, 0.36, 1.15, 28, 60, 0, 0, 0, 0, 0, -8, 0, 0, 0, 1, 1.5, 1.1, 0.8, 1.3, 1, 1, 0.35, 0.65, 0.9], [1, 1, 4.2, 0.8, 1, 10, 60, 0, 0, 0, 0, 0, -8, 0, 0, 0, 1, 1.5, 1.2, 1.4, 1, 1, 1, 0.12, 0.18, 0.2], [1, 1, 3.5, 0.5, 1, 32, 22, 0, 0, 0, 0, 0, -3, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5], [1, 1, 5, 0.9, 1, 10, 15, 0, 0, 0, 0, 0, -8, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0.5, 0.5, 0.5]];
 return presets[clamp(round(index), 0, array_length(presets) - 1)]
}
function secondary_preset_apply(values)
{
 if (!instance_exists(tl_edit) || tl_edit.type != e_tl_type.BODYPART || tl_edit.lock || array_length(values) != 26) return 0
 var valid_selection = true;
 with (obj_timeline) if (selected && (type != e_tl_type.BODYPART || lock)) valid_selection = false
 if (!valid_selection) { sm_notice = "Select only unlocked body parts; nothing changed."; return 0 }
 var ids = secondary_preset_ids();
 // Fully validate BEFORE starting a history transaction. Files never execute code.
 for (var i = 0; i < 26; i++)
  if (!is_real(values[i]) || values[i] != values[i] || abs(values[i]) > 1000000) { sm_notice = "Invalid preset value; nothing changed."; return 0 }
 action_secondary_preview_stop()
 tl_value_set_start(secondary_preset_apply, false)
 for (var i = 0; i < 26; i++)
 {
  if (!sm_preset_geometry && i >= 7 && i <= 12) continue
  tl_value_set(ids[i], tl_value_clamp(ids[i], values[i]), false)
 }
 with (obj_timeline) if (selected) secondary_motion_release(id)
 tl_value_set_done()
 sm_notice = "Preset applied. Start Preview, then Ctrl+Alt+drag the round tip in the viewport."
}
function action_secondary_preset_apply()
{
 secondary_preset_apply(secondary_builtin(sm_preset_index))
}
function action_secondary_preset_next()
{
 sm_preset_index = (sm_preset_index + 1) mod array_length(secondary_preset_names())
}
function action_secondary_preset_previous()
{
 sm_preset_index = (sm_preset_index + array_length(secondary_preset_names()) - 1) mod array_length(secondary_preset_names())
}
function action_secondary_preset_geometry()
{
 sm_preset_geometry = !sm_preset_geometry
 sm_notice = sm_preset_geometry ? "Import includes anchor/tip coordinates. Check your rig scale and orientation." : "Your anchor/tip setup is preserved when applying a preset."
}
function action_secondary_preset_export()
{
 if (!instance_exists(tl_edit) || tl_edit.type != e_tl_type.BODYPART) return 0
 var fn = file_dialog_save("Physics preset (*.json)|*.json", "secondary-preset.json", "", "Save selected part physics preset");
 if (fn == "") return 0
 var ids = secondary_preset_ids(), keys = secondary_preset_keys();
 json_save_start(fn)
 json_save_object_start()
 json_save_var("format", "mineimator-secondary-preset")
 json_save_var("version", 2)
 json_save_var("name", "Custom preset")
 for (var i = 0; i < 26; i++) json_save_var(keys[i], i == 0 ? (tl_edit.value[ids[i]] ? 1 : 0) : tl_edit.value[ids[i]])
 json_save_object_done()
 json_save_done()
 sm_notice = "Preset exported. Share the JSON file; Load preset imports it on another rig."
}
function action_secondary_preset_import()
{
 var fn = file_dialog_open("Physics preset (*.json)|*.json", "", "", "Load physics preset");
 if (fn == "") return 0
 var data = json_load(fn);
 if (!ds_map_valid(data)) { sm_notice = "Cannot read preset JSON."; return 0 }
 if (data[?"format"] != "mineimator-secondary-preset" || data[?"version"] != 2)
 { ds_map_destroy(data); sm_notice = "Unsupported preset format/version."; return 0 }
 var keys = secondary_preset_keys(), values = array_create(26, 0);
 var valid = true;
 for (var i = 0; i < 26; i++)
 {
  var val = data[?keys[i]];
  if (!is_real(val) || val != val || abs(val) > 1000000) valid = false
  else values[i] = val
 }
 ds_map_destroy(data)
 if (!valid) { sm_notice = "Missing or non-numeric preset fields; nothing changed."; return 0 }
 secondary_preset_apply(values)
}
function action_secondary_preview_stop()
{
 sm_preview = false
 if (instance_exists(sm_grab_tl)) secondary_motion_grab(sm_grab_tl, [], 0)
 sm_grab_tl = null
 sm_grab_view = null
 if (window_busy == "secondarygrab") { window_busy = ""; app_mouse_clear() }
 with (obj_timeline)
 {
  if (secondary_motion_active || value[e_value.SECONDARY_ENABLED]) { secondary_motion_release(id); update_matrix = true }
 }
}
function action_secondary_preview_toggle()
{
 if (sm_preview) { action_secondary_preview_stop(); sm_notice = "Preview stopped. No keyframes were created by dragging."; return 0 }
 if (!instance_exists(tl_edit) || tl_edit.lock || !tl_edit.value[e_value.SECONDARY_ENABLED]) { sm_notice = "Enable physics on an unlocked body part first."; return 0 }
 action_tl_play_break()
 action_secondary_preview_stop()
 sm_preview_start = current_time
 sm_preview = true
 sm_notice = "Preview uses real time. Ctrl+Alt+drag the round tip. Release to bounce; Esc cancels."
}
function secondary_v2_set(index, val, add)
{
 if (!instance_exists(tl_edit) || tl_edit.lock) return 0
 var ids = secondary_preset_ids();
 tl_value_set_start(secondary_v2_set, false)
 tl_value_set(ids[16 + index], val, add)
 with (obj_timeline) if (selected) { secondary_motion_release(id); update_matrix = true }
 tl_value_set_done()
}
function action_secondary_swing() { secondary_v2_set(0, 0, false) }
function action_secondary_soft() { secondary_v2_set(0, 1, false) }
function action_secondary_v2_1(val, add) { secondary_v2_set(1, val, add) }
function action_secondary_v2_2(val, add) { secondary_v2_set(2, val, add) }
function action_secondary_v2_3(val, add) { secondary_v2_set(3, val, add) }
function action_secondary_v2_4(val, add) { secondary_v2_set(4, val, add) }
function action_secondary_v2_5(val, add) { secondary_v2_set(5, val, add) }
function action_secondary_v2_6(val, add) { secondary_v2_set(6, val, add) }
function action_secondary_v2_7(val, add) { secondary_v2_set(7, val, add) }
function action_secondary_v2_8(val, add) { secondary_v2_set(8, val, add) }
function action_secondary_v2_9(val, add) { secondary_v2_set(9, val, add) }

// Test handle only exists while Preview is enabled, and never writes animation values.
function secondary_preview_gizmo(view)
{
 if (!instance_exists(tl_edit) || tl_edit.type != e_tl_type.BODYPART || tl_edit.lock || !tl_edit.value[e_value.SECONDARY_ENABLED]) return 0
 var points = secondary_motion_tip(tl_edit);
 if (array_length(points) != 6) return 0
 var anchor = vec3(points[0], points[1], points[2]), tip = vec3(points[3], points[4], points[5]);
 var screen = view_shape_project(tip);
 if (point3D_project_error) return 0
 draw_set_color(c_accent)
 view_shape_line(anchor, tip)
 draw_circle_ext(screen[X], screen[Y], 7, true, 24, c_accent, 1)
 var scaling = render_quality == e_view_mode.RENDER && render_view_scaling ? setting_view_scaling_value : 1;
 var mx = (mouse_x - content_x)*scaling, my = (mouse_y - content_y)*scaling;
 if (content_mouseon && window_busy == "" && !popup_mouseon && !context_menu_mouseon && !toast_mouseon && keyboard_check(vk_control) && keyboard_check(vk_alt) && point_distance(mx, my, screen[X], screen[Y]) < 16 && mouse_left_pressed)
 {
  sm_grab_tl = tl_edit
  sm_grab_view = view
  sm_grab_plane = tip
  sm_grab_normal = vec3_normalize(vec3_sub(tip, cam_from))
  window_busy = "secondarygrab"
  mouse_left_pressed = false
  mouse_left_released = false
 }
 if (window_busy == "secondarygrab" && sm_grab_view == view)
 {
  if (!mouse_left || keyboard_check_pressed(vk_escape) || mouse_right_pressed || sm_grab_tl != tl_edit)
  {
   if (instance_exists(sm_grab_tl)) secondary_motion_grab(sm_grab_tl, [], 0)
   if (keyboard_check_pressed(vk_escape) && instance_exists(sm_grab_tl)) secondary_motion_release(sm_grab_tl)
   sm_grab_tl = null
   sm_grab_view = null
   window_busy = ""
   app_mouse_clear()
  }
  else
  {
   var ray = vec3_project_ray([mx/content_width, my/content_height]);
   var denom = vec3_dot(sm_grab_normal, ray);
   if (abs(denom) > 0.00001)
   {
    var distance = vec3_dot(vec3_sub(sm_grab_plane, cam_from), sm_grab_normal)/denom;
    if (distance > 0 && distance < 100000) secondary_motion_grab(sm_grab_tl, vec3_add(cam_from, vec3_mul(ray, distance)), 1)
   }
  }
 }
}

// Release independently of visibility/projection, so a hidden tip cannot trap mouse capture.
function secondary_preview_tick()
{
 if (window_busy != "secondarygrab") return 0
 if (!sm_preview || !instance_exists(sm_grab_tl) || sm_grab_tl != tl_edit || !mouse_left || keyboard_check_pressed(vk_escape) || mouse_right_pressed)
 {
  if (instance_exists(sm_grab_tl))
  {
   secondary_motion_grab(sm_grab_tl, [], 0)
   if (keyboard_check_pressed(vk_escape)) secondary_motion_release(sm_grab_tl)
  }
  sm_grab_tl = null
  sm_grab_view = null
  window_busy = ""
  app_mouse_clear()
 }
}
function secondary_ui_button(label, action, help = "")
{
 draw_set_font(font_value)
 draw_button_text(string_limit(label, max(40, dw - 8)), dx, dy + tab_control_h, action, null, help)
}
function secondary_ui_hint(label, help)
{
 draw_set_font(font_label)
 draw_label(string_limit(label, max(40, dw - 8)), dx, dy + tab_control_h/2, fa_left, fa_middle, c_text_secondary, 1, font_label)
 if (content_mouseon && app_mouse_box(dx, dy, dw, tab_control_h)) tip_set(help, dx, dy, dw, tab_control_h)
}

// This label is computed from actual values, not from a browsed preset index.
function secondary_preset_match(values)
{
 if (array_length(values) != 26) return -1
 var names = secondary_preset_names();
 for (var preset = 0; preset < array_length(names); preset++)
 {
  var expected = secondary_builtin(preset), matches = true;
  for (var field = 0; field < 26; field++)
  {
   if (field >= 7 && field <= 12) continue // rig-specific anchor and tip are not a preset identity
   if (!is_real(values[field]) || values[field] != values[field] || abs(values[field] - expected[field]) > 0.00001) { matches = false; break }
  }
  if (matches) return preset
 }
 return -1
}
function action_secondary_preset_choose(index)
{
 if (!is_real(index) || index < 0 || index >= array_length(secondary_preset_names()) || index != round(index)) return 0
 sm_preset_index = index
 secondary_preset_apply(secondary_builtin(index))
}
function action_secondary_mode_choose(mode)
{
 if (mode != 0 && mode != 1) return 0
 secondary_v2_set(0, mode, false)
}

// Rigid Body / Ragdoll v1 - native solver integration, transient cache and transactional bake.
function rb_ids()
{
 return [e_value.RB_ENABLED,e_value.RB_MODE,e_value.RB_SHAPE,e_value.RB_MASS,e_value.RB_FRICTION,e_value.RB_BOUNCE,e_value.RB_LINEAR_DRAG,e_value.RB_ANGULAR_DRAG,e_value.RB_SIZE_X,e_value.RB_SIZE_Y,e_value.RB_SIZE_Z,e_value.RB_OFFSET_X,e_value.RB_OFFSET_Y,e_value.RB_OFFSET_Z,e_value.RB_JOINT,e_value.RB_LIMIT_X,e_value.RB_LIMIT_Y,e_value.RB_LIMIT_Z,e_value.RB_VEL_X,e_value.RB_VEL_Y,e_value.RB_VEL_Z,e_value.RB_ANG_X,e_value.RB_ANG_Y,e_value.RB_ANG_Z,e_value.RB_PARENT,e_value.RB_LAYER,e_value.RB_MASK]
}
function rb_supported(tl)
{
 return instance_exists(tl) && tl.value_type[e_value_type.TRANSFORM_POS] && tl.value_type[e_value_type.TRANSFORM_ROT] && tl.type != e_tl_type.CAMERA
}
function rb_configured(tl)
{
 if (!rb_supported(tl)) return false
 if (tl.value[e_value.RB_ENABLED]) return true
 for (var k = 0; k < ds_list_size(tl.keyframe_list); k++) if (tl.keyframe_list[|k].value[e_value.RB_ENABLED]) return true
 return false
}
function rb_history_mark()
{
 rb_history_amount = history_amount
 rb_history_pos = history_pos
 rb_history_token = history_amount > 0 ? history[0] : null
}
function rb_evaluate(frame)
{
 rb_capture = true
 timeline_marker = frame
 with (obj_timeline) { tl_update_values(true); update_matrix = true }
 app_update_animate(true)
 rb_capture = false
}
function action_rb_reset()
{
 var restore = rb_job;
 rb_session = 0
 rb_job = false
 rb_baking = false
 rb_capture = false
 rb_cache_valid = false
 if (window_busy == "rigidjob") window_busy = ""
 rigid_clear()
 if (restore) timeline_marker = rb_marker_restore
 rb_targets = []
 rb_dynamic = []
 with (obj_timeline) { tl_update_values(true); update_matrix = true }
 tl_update_matrix()
 render_samples = -1
 rb_notice = "Reset. Source animation has not been changed."
}
function rb_fail(message)
{
 action_rb_reset()
 rb_notice = message
}
function rb_prepare()
{
 action_tl_play_break()
 action_secondary_preview_stop()
 action_rb_reset()
 rb_evaluate(timeline_marker)
 with (obj_timeline)
 {
  if (rb_configured(id))
  {
   array_add(app.rb_targets, id, false)
   if (value[e_value.RB_MODE] == 0) array_add(app.rb_dynamic, id, false)
  }
 }
 if (array_length(rb_targets) == 0 || array_length(rb_targets) > 128) { rb_fail("Enable 1-128 rigid bodies, including environment colliders."); return false }
 var ids = rb_ids();
 for (var t = 0; t < array_length(rb_targets); t++)
 {
  var tl = rb_targets[t], params = array_create(26, 0);
  for (var f = 1; f < 27; f++) params[f-1] = tl.value[ids[f]]
  var target = tl.value[e_value.RB_PARENT];
  if (!instance_exists(target)) target = tl.parent
  params[23] = instance_exists(target) && target != app ? target : -1
  if (!rigid_add(tl, tl.rb_base_matrix, params)) { var err = rigid_error(); rb_fail(err); return false }
 }
 if (!rigid_build()) { var err = rigid_error(); rb_fail(err); return false }
 if (!rigid_ground(rb_use_ground && background_ground_show,-0.01,0.8)) { var err = rigid_error(); rb_fail(err); return false }
 for (var t = 0; t < array_length(rb_targets); t++)
 {
  var tl = rb_targets[t];
  rigid_target(tl, tl.rb_base_matrix, tl.value[e_value.RB_ENABLED])
 }
 rb_history_mark()
 return true
}
function action_rb_preview()
{
 if (rb_session == 1) { action_rb_reset(); return 0 }
 if (!rb_prepare()) return 0
 rb_session = 1
 rb_clock = current_time
 rb_notice = "Live preview. Reset restores source animation. Preview is not recorded; use Simulate range before Bake."
}
function rb_live_tick()
{
 if (rb_session != 1 || rb_job) return 0
 var dt = clamp((current_time-rb_clock)/1000, 0, 0.1);
 rb_clock = current_time
 for (var t = 0; t < array_length(rb_targets); t++)
 {
  var tl = rb_targets[t];
  if (!instance_exists(tl)) { rb_fail("Object removed. Simulation reset."); return 0 }
  rigid_target(tl, tl.rb_base_matrix, tl.value[e_value.RB_ENABLED])
 }
 if (!rigid_ground(rb_use_ground && background_ground_show,-0.01,0.8)) { var err = rigid_error(); rb_fail(err); return 0 }
 if (rigid_error() != "" || !rigid_step(dt)) { var err = rigid_error(); rb_fail(err) }
 render_samples = -1
}
function rb_session_guard()
{
 if (rb_baking) return 0
 var token = history_amount > 0 ? history[0] : null;
 if (history_amount != rb_history_amount || history_pos != rb_history_pos || token != rb_history_token)
 { rb_fail("Animation edited. Cached physics was invalidated; reset and simulate again."); return 0 }
 if (rb_session == 1 && (timeline_playing || window_state != "")) rb_fail("Live preview stopped. Simulate a range or Bake before playback/export.")
}
function action_rb_start_frame(value, add)
{
 if (rb_job) return 0
 rb_frame_start = max(0, round((add ? rb_frame_start : 0)+value))
 if (rb_cache_valid || rb_session != 0) action_rb_reset()
}
function action_rb_end_frame(value, add)
{
 if (rb_job) return 0
 rb_frame_end = max(0, round((add ? rb_frame_end : 0)+value))
 if (rb_cache_valid || rb_session != 0) action_rb_reset()
}
function action_rb_simulate()
{
 if (rb_job) return 0
 if (project_tempo < 1 || project_tempo > 240) { rb_notice = "Simulation supports project FPS from 1 to 240."; return 0 }
 if (rb_frame_end < rb_frame_start || rb_frame_end-rb_frame_start > 5000) { rb_notice = "Choose a range of at most 5,001 frames with End >= Start."; return 0 }
 var marker = timeline_marker;
 timeline_marker = rb_frame_start
 if (!rb_prepare()) { timeline_marker = marker; return 0 }
 if ((rb_frame_end-rb_frame_start+1)*array_length(rb_targets) > 100000) { rb_fail("Cache exceeds 100,000 object/frame samples. Shorten the range."); timeline_marker = marker; return 0 }
 rb_marker_restore = marker
 rb_frame_next = rb_frame_start
 rb_job = true
 rb_baking = false
 window_busy = "rigidjob"
 rb_notice = "Simulating range... Press Esc to cancel. No keyframes are being changed."
}
function rb_job_tick()
{
 if (!rb_job) return 0
 if (keyboard_check_pressed(vk_escape) || timeline_playing || window_state != "") { rb_fail("Cancelled. Source keyframes were not changed."); return 0 }
 var token = history_amount > 0 ? history[0] : null;
 if (history_amount != rb_history_amount || history_pos != rb_history_pos || token != rb_history_token) { rb_fail("Scene edited during the job. Cancelled without writing keyframes."); return 0 }
 var frame = rb_frame_next;
 rb_evaluate(frame)
 if (!rb_job) return 0
 if (rb_baking)
 {
  if (!rb_bake_collect(frame)) { rb_fail("Bake stopped: local transform could not be reproduced. No keyframes were changed."); return 0 }
 }
 else
 {
  for (var t = 0; t < array_length(rb_targets); t++)
  {
   var tl = rb_targets[t];
   if (!instance_exists(tl)) { rb_fail("Object removed. Range cancelled."); return 0 }
   rigid_target(tl, tl.rb_base_matrix, tl.value[e_value.RB_ENABLED])
  }
  if (rigid_error() != "") { var err = rigid_error(); rb_fail(err); return 0 }
  if (!rigid_ground(rb_use_ground && background_ground_show,-0.01,0.8)) { var err = rigid_error(); rb_fail(err); return 0 }
  if (frame > rb_frame_start && !rigid_step(1/max(1, project_tempo))) { var err = rigid_error(); rb_fail(err); return 0 }
  if (!rigid_store(frame)) { var err = rigid_error(); rb_fail(err); return 0 }
 }
 rb_frame_next++
 var last = rb_baking ? rb_frame_end+1 : rb_frame_end;
 rb_notice = (rb_baking ? "Preparing Bake: " : "Simulating: ") + string(frame) + " / " + string(last) + " (Esc to cancel)"
 if (rb_frame_next > last)
 {
  rb_job = false
  window_busy = ""
  if (rb_baking)
  {
   rb_baking = false
   action_rb_bake_commit()
  }
  else
  {
   rb_session = 2
   rb_cache_valid = true
   timeline_marker = rb_marker_restore
   rb_evaluate(timeline_marker)
   tl_update_matrix()
   rb_history_mark()
   rb_notice = "Range cached. Scrub within Start/End or Bake to editable keyframes. Reset discards only the cache."
  }
 }
}
function rb_snapshot()
{
 var state = [];
 for (var t = 0; t < array_length(rb_dynamic); t++)
 {
  var tl = rb_dynamic[t], frames = [];
  if (!instance_exists(tl)) continue
  for (var k = 0; k < ds_list_size(tl.keyframe_list); k++)
  {
   var kf = tl.keyframe_list[|k], values = array_create(e_value.amount, 0);
   for (var v = 0; v < e_value.amount; v++) values[v] = tl_value_get_save_id(v, kf.value[v])
   array_add(frames, [kf.position, values, kf.selected], false)
  }
  array_add(state, [save_id_get(tl), frames], false)
 }
 return state
}
function rb_restore(state)
{
 for (var t = 0; t < array_length(state); t++)
 {
  var tl = save_id_find(state[t][0]);
  if (!instance_exists(tl)) continue
  while (ds_list_size(tl.keyframe_list) > 0) with (tl.keyframe_list[|0]) instance_destroy()
  var frames = state[t][1];
  for (var k = 0; k < array_length(frames); k++)
  {
   var kf;
   with (tl) kf = tl_keyframe_add(frames[k][0])
   for (var v = 0; v < e_value.amount; v++) kf.value[v] = tl_value_find_save_id(v, kf.value[v], frames[k][1][v])
   kf.selected = frames[k][2]
  }
 }
 with (obj_timeline) { tl_update_values(true); update_matrix = true }
 tl_update_matrix()
 tl_update_length()
 app_update_tl_edit()
}
function action_rb_bake()
{
 if (rb_job || !rb_cache_valid || rb_session != 2) { rb_notice = "Simulate range successfully before Bake."; return 0 }
 rb_session_guard()
 if (!rb_cache_valid) return 0
 if ((rb_frame_end-rb_frame_start+3)*array_length(rb_dynamic) > 5000) { rb_notice = "Bake exceeds 5,000 body/frame samples. Shorten the range to limit keyframe memory."; return 0 }
 if (array_length(rb_dynamic) == 0) { rb_notice = "No Dynamic bodies to bake."; return 0 }
 for (var t = 0; t < array_length(rb_dynamic); t++)
 {
  var tl = rb_dynamic[t];
  if (!instance_exists(tl) || tl.lock || !tl.inherit_position || !tl.inherit_rotation || !tl.inherit_scale || tl.inherit_pose || tl.value[e_value.IK_TARGET] != null || tl.value[e_value.POS_TARGET] != null || tl.value[e_value.ROT_TARGET] != null || tl.value[e_value.SCALE_TARGET] != null || tl.value[e_value.LOOK_AT_TARGET] != null || tl.value[e_value.PATH_OBJ] != null)
  { rb_notice = "Bake requires unlocked FK transforms with normal inheritance. Remove IK/copy/look-at/path/inherit-pose constraints first."; return 0 }
  for (var k = 0; k < ds_list_size(tl.keyframe_list); k++)
   if (tl.keyframe_list[|k].position >= max(0,rb_frame_start-1) && tl.keyframe_list[|k].position <= rb_frame_end+1 && tl.keyframe_list[|k].value[e_value.GRAPH_ONLY] > 0)
   { rb_notice = "Graph-only keys overlap the Bake range. Convert/remove them first; nothing was changed."; return 0 }
 }
 var answer = show_message_ext("Bake rigid physics", "Write position/rotation keys for all Dynamic bodies in the cached range? Existing values on those channels will be replaced inside the range. Other channels are preserved. One Undo restores the original keys.", "Bake", "Cancel", "");
 if (answer != 0) return 0
 rb_bake_before = rb_snapshot()
 rb_bake_rows = []
 rb_bake_prev_rot = []
 for (var t = 0; t < array_length(rb_dynamic); t++) array_add(rb_bake_prev_rot,[rb_dynamic[t].value[e_value.ROT_X],rb_dynamic[t].value[e_value.ROT_Y],rb_dynamic[t].value[e_value.ROT_Z]], false)
 rb_marker_restore = timeline_marker
 rb_frame_next = max(0,rb_frame_start-1)
 rb_session = 0
 rb_job = true
 rb_baking = true
 window_busy = "rigidjob"
 rb_notice = "Preparing Bake... Esc cancels without editing keyframes."
}
function rb_bake_collect(frame)
{
 for (var t = 0; t < array_length(rb_dynamic); t++)
 {
  var tl = rb_dynamic[t];
  if (!instance_exists(tl)) return false
  var values = array_create(e_value.amount, 0);
  for (var v = 0; v < e_value.amount; v++) values[v] = tl_value_get_save_id(v, tl.value[v])
  if (frame >= rb_frame_start && frame <= rb_frame_end)
  {
   var desired = rigid_result(tl, tl.rb_base_matrix, frame), parent_matrix = array_copy_1d(tl.rb_channel_parent);
   if (instance_exists(tl.parent) && tl.parent != app && array_length(tl.parent.rb_base_matrix) > 0)
   {
    var desired_parent = rigid_result(tl.parent, tl.parent.rb_base_matrix, frame);
    parent_matrix = matrix_multiply(matrix_multiply(parent_matrix, matrix_inverse(tl.parent.rb_base_matrix)), desired_parent)
   }
   var position = point3D_mul_matrix([desired[MAT_X],desired[MAT_Y],desired[MAT_Z]],matrix_inverse(parent_matrix));
   var parent_rotation = array_copy_1d(parent_matrix), desired_rotation = array_copy_1d(desired);
   matrix_remove_scale(parent_rotation)
   matrix_remove_scale(desired_rotation)
   var local_rotation = matrix_multiply(desired_rotation,matrix_inverse(parent_rotation));
   var reference = rb_bake_prev_rot[t];
   var rotation = rigid_euler(local_rotation, reference);
   if (array_length(rotation) != 3) return false
   rb_bake_prev_rot[t] = rotation
   for (var axis = 0; axis < 3; axis++) { values[e_value.POS_X+axis] = position[axis]; values[e_value.ROT_X+axis] = rotation[axis] }
   values[e_value.RB_ENABLED] = false
   values[e_value.SECONDARY_ENABLED] = false
   values[e_value.MODIFIER_SHAKE] = false
   values[e_value.MODIFIER_FRAMESKIP] = false
   values[e_value.GRAPH_CHANNELS_USED] = 0
   values[e_value.GRAPH_ONLY] = 0
  }
  array_add(rb_bake_rows, [save_id_get(tl), frame, values], false)
 }
 return true
}
function action_rb_bake_commit()
{
 if (history_undo || history_redo)
 {
  action_rb_reset()
  rb_restore(history_undo ? history_data.rb_before : history_data.rb_after)
  return 0
 }
 var hist = history_set(action_rb_bake_commit);
 hist.rb_before = rb_bake_before
 for (var row = 0; row < array_length(rb_bake_rows); row++)
 {
  var record = rb_bake_rows[row], tl = save_id_find(record[0]);
  if (!instance_exists(tl)) continue
  var kf = null;
  for (var k = 0; k < ds_list_size(tl.keyframe_list); k++) if (tl.keyframe_list[|k].position == record[1]) { kf = tl.keyframe_list[|k]; break }
  if (!instance_exists(kf)) with (tl) kf = tl_keyframe_add(record[1])
  for (var v = 0; v < e_value.amount; v++) kf.value[v] = tl_value_find_save_id(v, kf.value[v], record[2][v])
 }
 hist.rb_after = rb_snapshot()
 timeline_marker = rb_marker_restore
 action_rb_reset()
 rb_notice = "Baked. Physics is disabled on baked frames; boundary keys preserve animation outside the range. Undo restores source keys."
}
function rb_edit(index, value, add)
{
 if (rb_job || !instance_exists(tl_edit) || tl_edit.lock || !rb_selection_editable()) return 0
 action_rb_reset()
 var ids = rb_ids();
 var action = rb_value_action(index);
 tl_value_set_start(action == null ? rb_edit : action, add)
 tl_value_set(ids[index], value, add)
 tl_value_set_done()
 rb_notice = "Settings changed. Reset and simulate again to refresh physics."
}
function action_rb_enabled(value) { rb_edit(0,value,false) }
function action_rb_mode(value) { rb_edit(1,value,false) }
function action_rb_shape(value) { rb_edit(2,value,false) }
function action_rb_joint(value) { rb_edit(14,value,false) }
function action_rb_parent(value) { rb_edit(24,value,false) }
function action_rb_colliders() { rb_show_colliders = !rb_show_colliders; render_low_drawing = 0 }
function rb_draw_colliders()
{
 if (rb_capture) return 0
 for (var t = 0; t < ds_list_size(project_timeline_list); t++)
 {
  var tl = project_timeline_list[|t];
  if (!instance_exists(tl) || !tl.value[e_value.RB_ENABLED] || !rb_supported(tl)) continue
  var size = [tl.value[e_value.RB_SIZE_X],tl.value[e_value.RB_SIZE_Y],tl.value[e_value.RB_SIZE_Z]], offset = [tl.value[e_value.RB_OFFSET_X],tl.value[e_value.RB_OFFSET_Y],tl.value[e_value.RB_OFFSET_Z]], points = [];
  if (tl.value[e_value.RB_SHAPE] == 1) { var diameter = max(size[0],size[1],size[2]); size = [diameter,diameter,diameter] }
  for (var corner = 0; corner < 8; corner++)
   array_add(points, point3D_mul_matrix([offset[0]+((corner div 4) mod 2-.5)*size[0],offset[1]+((corner div 2) mod 2-.5)*size[1],offset[2]+(corner mod 2-.5)*size[2]],tl.matrix), false)
  draw_set_color(tl.value[e_value.RB_MODE] == 1 ? c_text_secondary : c_accent)
  view_shape_draw(points)
 }
}

function action_rb_value_3(val, add) { rb_edit(3,val,add) }

function action_rb_value_4(val, add) { rb_edit(4,val,add) }

function action_rb_value_5(val, add) { rb_edit(5,val,add) }

function action_rb_value_6(val, add) { rb_edit(6,val,add) }

function action_rb_value_7(val, add) { rb_edit(7,val,add) }

function action_rb_value_8(val, add) { rb_edit(8,val,add) }

function action_rb_value_9(val, add) { rb_edit(9,val,add) }

function action_rb_value_10(val, add) { rb_edit(10,val,add) }

function action_rb_value_11(val, add) { rb_edit(11,val,add) }

function action_rb_value_12(val, add) { rb_edit(12,val,add) }

function action_rb_value_13(val, add) { rb_edit(13,val,add) }

function action_rb_value_15(val, add) { rb_edit(15,val,add) }

function action_rb_value_16(val, add) { rb_edit(16,val,add) }

function action_rb_value_17(val, add) { rb_edit(17,val,add) }

function action_rb_value_18(val, add) { rb_edit(18,val,add) }

function action_rb_value_19(val, add) { rb_edit(19,val,add) }

function action_rb_value_20(val, add) { rb_edit(20,val,add) }

function action_rb_value_21(val, add) { rb_edit(21,val,add) }

function action_rb_value_22(val, add) { rb_edit(22,val,add) }

function action_rb_value_23(val, add) { rb_edit(23,val,add) }

function action_rb_value_25(val, add) { rb_edit(25,val,add) }

function action_rb_value_26(val, add) { rb_edit(26,val,add) }

function action_rb_selected_dynamic()
{
 if (rb_job || !rb_selection_editable()) return 0
 action_rb_reset()
 tl_value_set_start(action_rb_selected_dynamic,false)
 tl_value_set(e_value.RB_ENABLED,true,false)
 tl_value_set(e_value.RB_MODE,0,false)
 tl_value_set(e_value.RB_JOINT,0,false)
 tl_value_set_done()
 rb_notice = "Selected objects are independent Dynamic bodies. Fit collider size/offset before Preview."
}
function action_rb_selected_static()
{
 if (rb_job || !rb_selection_editable()) return 0
 action_rb_reset()
 tl_value_set_start(action_rb_selected_static,false)
 tl_value_set(e_value.RB_ENABLED,true,false)
 tl_value_set(e_value.RB_MODE,1,false)
 tl_value_set(e_value.RB_JOINT,0,false)
 tl_value_set_done()
 rb_notice = "Selected objects are Static environment colliders. Model geometry is not automatically converted; fit box/sphere proxies."
}
function action_rb_selected_ragdoll()
{
 if (rb_job || !rb_selection_editable()) return 0
 action_rb_reset()
 tl_value_set_start(action_rb_selected_ragdoll,false)
 tl_value_set(e_value.RB_ENABLED,true,false)
 tl_value_set(e_value.RB_MODE,0,false)
 tl_value_set(e_value.RB_JOINT,1,false)
 tl_value_set(e_value.RB_PARENT,null,false)
 tl_value_set_done()
 rb_notice = "Ragdoll uses selected/enabled parent parts. Each child pivot is its joint anchor. Fit colliders, pivots and angle limits to your rig."
}
function action_rb_selected_weld()
{
 if (rb_job || !instance_exists(tl_edit) || !rb_selection_editable()) return 0
 var target = tl_edit;
 action_rb_reset()
 tl_value_set_start(action_rb_selected_weld,false)
 tl_value_set(e_value.RB_ENABLED,true,false)
 tl_value_set(e_value.RB_MODE,0,false)
 tl_value_set(e_value.RB_JOINT,3,false)
 tl_value_set(e_value.RB_PARENT,target,false)
 tl_value_set_done()
 rb_notice = "Selection welded to the active object as one compound body. Reset/rebuild after changing membership."
}

function rb_selection_editable()
{
 var valid = true, count = 0;
 with (obj_timeline) if (selected) { count++; if (lock || !rb_supported(id)) valid = false }
 if (!valid || count == 0) { rb_notice = "Select only unlocked objects with position/rotation transforms."; return false }
 return true
}

// Adaptive whole-rig setup. Geometry is measured in the same local space as the collider.
function rb_auto_role(tl)
{
 var n = string_lower(tl.model_part_name + " " + tl.name);
 if (string_pos("head",n)>0 || string_pos("skull",n)>0) return 1
 if (string_pos("arm",n)>0 || string_pos("hand",n)>0) return 2
 if (string_pos("leg",n)>0 || string_pos("foot",n)>0) return 3
 if (string_pos("body",n)>0 || string_pos("torso",n)>0 || string_pos("chest",n)>0 || string_pos("pelvis",n)>0) return 4
 return 0
}
function rb_auto_owner(tl)
{
 if (instance_exists(tl.part_of)) return tl.part_of
 var node = tl;
 for (var depth = 0; depth < 128; depth++)
 {
  if (!instance_exists(node.parent) || node.parent == app) return node
  node = node.parent
 }
 return null
}
function rb_auto_fit(tl)
{
 if (!instance_exists(tl.model_part) || tl.model_part.shape_list == null) return []
 var lo = [100000000,100000000,100000000], hi = [-100000000,-100000000,-100000000], count = 0;
 for (var s = 0; s < ds_list_size(tl.model_part.shape_list); s++)
 {
  var shape = tl.model_part.shape_list[|s];
  if (!instance_exists(shape)) continue
  // No invented bounds for external meshes. Decorative planes have no solid volume.
  if (shape.type == "mesh") return []
  if (shape.type != "block" || shape.color_alpha <= 0) continue
  if (shape.bend_vertex) return []
  var transform = matrix_create(shape.position,shape.rotation,vec3(1));
  for (var corner = 0; corner < 8; corner++)
  {
   var p = [((corner div 4) mod 2)==0?shape.from[0]:shape.to[0],((corner div 2) mod 2)==0?shape.from[1]:shape.to[1],(corner mod 2)==0?shape.from[2]:shape.to[2]];
   p = point3D_mul_matrix(p,transform)
   for (var axis = 0; axis < 3; axis++)
   {
    p[axis] -= tl.rot_point_render[axis]
    if (!(abs(p[axis]) < 100000000)) return []
    lo[axis] = min(lo[axis],p[axis])
    hi[axis] = max(hi[axis],p[axis])
   }
   count++
  }
 }
 if (count == 0) return []
 var size = [hi[0]-lo[0],hi[1]-lo[1],hi[2]-lo[2]];
 if (min(size[0],size[1],size[2]) <= 0.001) return []
 var m = tl.rb_base_matrix;
 var determinant = m[0]*(m[5]*m[10]-m[6]*m[9])-m[4]*(m[1]*m[10]-m[2]*m[9])+m[8]*(m[1]*m[6]-m[2]*m[5]);
 if (!(determinant > 0.000001 && determinant < 100000000)) return []
 var volume = size[0]*size[1]*size[2]*determinant;
 return [max(0.01,size[0]*0.96),max(0.01,size[1]*0.96),max(0.01,size[2]*0.96),(lo[0]+hi[0])*0.5,(lo[1]+hi[1])*0.5,(lo[2]+hi[2])*0.5,volume]
}
function action_rb_ground()
{
 if (rb_job) return 0
 var enabled = !rb_use_ground;
 action_rb_reset()
 rb_use_ground = enabled
 rb_notice = enabled ? "Visible background ground now collides at Z=-0.01. Re-simulate to refresh the cache." : "Background ground collision disabled. Scene colliders still work."
}
function rb_auto_empty_values(targets)
{
 var saved = [];
 for (var i = 0; i < array_length(targets); i++)
 {
  var tl = targets[i];
  if (ds_list_size(tl.keyframe_list) != 0) continue
  var values = array_create(e_value.amount,0);
  for (var v = 0; v < e_value.amount; v++) values[v] = tl_value_get_save_id(v,tl.value[v])
  array_add(saved,[save_id_get(tl),values], false)
 }
 return saved
}
function action_rb_auto_setup(fall_test = false)
{
 if (history_undo || history_redo)
 {
  action_rb_reset()
  rb_restore(history_undo ? history_data.rb_before : history_data.rb_after)
  if (history_undo)
   for (var i = 0; i < array_length(history_data.rb_empty); i++)
   {
    var tl = save_id_find(history_data.rb_empty[i][0]);
    if (!instance_exists(tl)) continue
    for (var v = 0; v < e_value.amount; v++) tl.value[v] = tl_value_find_save_id(v,tl.value[v],history_data.rb_empty[i][1][v])
    tl.update_matrix = true
   }
  tl_update_matrix()
  return 0
 }
 if (rb_job || !instance_exists(tl_edit)) return 0
 action_tl_play_break()
 action_secondary_preview_stop()
 action_rb_reset()
 rb_evaluate(timeline_marker)
 var owners = [], parts = [], fits = [], masses = [], roles = [], skipped = 0;
 for (var i = 0; i < ds_list_size(project_timeline_list); i++)
 {
  var tl = project_timeline_list[|i];
  if (!(tl.selected || tl == tl_edit)) continue
  var owner = rb_auto_owner(tl);
  if (instance_exists(owner) && !array_contains(owners,owner)) array_add(owners,owner, false)
 }
 for (var i = 0; i < ds_list_size(project_timeline_list); i++)
 {
  var tl = project_timeline_list[|i];
  if (tl.type != e_tl_type.BODYPART || !array_contains(owners,rb_auto_owner(tl))) continue
  if (tl.lock) { rb_notice = "Auto Setup stopped: unlock all parts of the rig first. Nothing was changed."; return 0 }
  var fit = rb_auto_fit(tl);
  if (array_length(fit) != 7) { skipped++; continue }
  for (var k = 0; k < ds_list_size(tl.keyframe_list); k++)
   if (tl.keyframe_list[|k].value[e_value.GRAPH_ONLY] > 0) { rb_notice = "Convert graph-only keys on this rig before Auto Setup. Nothing was changed."; return 0 }
  var role = rb_auto_role(tl), factor = role == 4 ? 2 : (role == 1 ? 0.65 : 1);
  array_add(parts,tl, false)
  array_add(fits,fit, false)
  array_add(roles,role, false)
  array_add(masses,clamp(fit[6]*0.005*factor,0.01,10000), false)
 }
 if (array_length(parts) == 0 || array_length(parts) > 128) { rb_notice = "Auto Setup needs 1-128 solid model parts. Mesh/vertex-deformed/flat-only parts need manual colliders."; return 0 }
 // Resolve all parent links before any mutation. Use hierarchy first, same-rig mass root second.
 var targets = array_copy_1d(parts), parents = [];
 for (var i = 0; i < array_length(parts); i++)
 {
  var owner = rb_auto_owner(parts[i]), root = -1;
  for (var j = 0; j < array_length(parts); j++)
  {
   if (rb_auto_owner(parts[j]) != owner) continue
   var ancestor = parts[j].parent, has_ancestor = false;
   for (var depth = 0; depth < 128 && instance_exists(ancestor) && ancestor != app; depth++)
   {
    if (array_contains(parts,ancestor)) { has_ancestor = true; break }
    ancestor = ancestor.parent
   }
   if (!has_ancestor && (root == -1 || masses[j] > masses[root])) root = j
  }
  if (root == -1) { rb_notice = "Cyclic rig hierarchy. Nothing was changed."; return 0 }
  var parent = null, node = parts[i].parent;
  for (var depth = 0; depth < 128 && instance_exists(node) && node != app; depth++)
  {
   if (array_contains(parts,node)) { parent = node; break }
   node = node.parent
  }
  if (parent == null && root != i) parent = parts[root]
  if (parent == parts[i]) { rb_notice = "Invalid cyclic rig hierarchy. Nothing was changed."; return 0 }
  array_add(parents,parent, false)
 }
 // The actor/container is not a second collider surrounding all its child parts.
 for (var i = 0; i < array_length(owners); i++)
  if (!array_contains(targets,owners[i]) && rb_configured(owners[i]))
  {
   if (owners[i].lock) { rb_notice = "Unlock the model root before Auto Setup. Nothing was changed."; return 0 }
   array_add(targets,owners[i], false)
  }
 var answer = show_message_ext("Adaptive ragdoll setup", "Configure the entire rig from its solid model geometry? This replaces Rigid Body settings on all existing keys of the affected parts, clears old weld links, and disables overlapping root colliders. Pose keys are not moved. One Undo restores the previous settings. Unsupported parts will be skipped.","Auto Setup","Cancel","");
 if (answer != 0) return 0
 rb_dynamic = targets
 var before = rb_snapshot(), empty = rb_auto_empty_values(targets), ids = rb_ids();
 var hist = history_set(action_rb_auto_setup);
 hist.rb_before = before
 hist.rb_empty = empty
 for (var i = 0; i < array_length(targets); i++)
 {
  var tl = targets[i];
  if (ds_list_size(tl.keyframe_list) == 0) with (tl) tl_keyframe_add(0)
  for (var k = 0; k < ds_list_size(tl.keyframe_list); k++)
  {
   var kf = tl.keyframe_list[|k];
   if (i >= array_length(parts)) { kf.value[e_value.RB_ENABLED] = false; continue }
   var fit = fits[i], role = roles[i], parent = parents[i];
   var limits = role == 1 ? [35,50,40] : (role == 2 ? [100,85,90] : (role == 3 ? [60,35,35] : [35,40,35]));
   var velocity = [0,0,0], spin = [0,0,0];
   if (fall_test)
   {
    var owner = rb_auto_owner(tl), m = owner.rb_base_matrix;
    var length_xy = max(0.0001,sqrt(m[4]*m[4]+m[5]*m[5]));
    var forward = [m[4]/length_xy,m[5]/length_xy,0];
    var center = point3D_mul_matrix([fit[3],fit[4],fit[5]],tl.rb_base_matrix);
    var speed = clamp((center[2]+0.01)*0.6,0,100);
    velocity = [forward[0]*speed,forward[1]*speed,0]
    spin = [-forward[1]*35,forward[0]*35,0]
   }
   var config = [true,0,0,masses[i],0.75,0.02,0.12,0.45,fit[0],fit[1],fit[2],fit[3],fit[4],fit[5],parent == null ? 0 : 1,limits[0],limits[1],limits[2],velocity[0],velocity[1],velocity[2],spin[0],spin[1],spin[2],parent,0,255];
   for (var f = 0; f < 27; f++) kf.value[ids[f]] = config[f]
  }
 }
 hist.rb_after = rb_snapshot()
 action_rb_reset()
 rb_evaluate(timeline_marker)
 rb_notice = "Auto Setup: " + string(array_length(parts)) + " parts fitted from geometry; " + string(skipped) + " unsupported parts skipped. Visible background ground collides when enabled. Preview to test."
}

function action_rb_auto_fall() { action_rb_auto_setup(true) }
