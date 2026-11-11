/// app_startup_interface_timeline()

function app_startup_interface_timeline()
{
	rb_use_ground = true
	rb_session = 0
	rb_capture = false
	rb_job = false
	rb_baking = false
	rb_show_colliders = true
	rb_frame_start = 0
	rb_frame_end = 120
	rb_frame_next = 0
	rb_marker_restore = 0
	rb_clock = 0
	rb_targets = []
	rb_dynamic = []
	rb_cache_valid = false
	rb_notice = "Assign Dynamic bodies and Static environment colliders, then Preview or Simulate range."
	rb_history_amount = 0
	rb_history_pos = 0
	rb_history_token = null
	rb_boxes = array_create(30, null)
	for (var rigid_box = 0; rigid_box < 30; rigid_box++) rb_boxes[rigid_box] = new_textbox_ndecimals()
	sm_preview = false
	sm_preview_start = 0
	sm_grab_tl = null
	sm_grab_view = null
	sm_grab_plane = [0, 0, 0]
	sm_grab_normal = [0, 0, 1]
	sm_preset_index = 0
	sm_preset_geometry = false
	sm_notice = "Choose a preset, then start the live test. Hover over controls for help."
	sm_boxes = array_create(10, null)
	for (var smi = 0; smi < 10; smi++) sm_boxes[smi] = new_textbox_ndecimals()
	timeline_playing = false
	timeline_playing_start_time = 0
	timeline_playing_start_marker = 0
	timeline_playing_last_marker = 0
	timeline_playing_start_hor_scroll = 0
	
	timeline_repeat = false
	timeline_seamless_repeat = false
	timeline_intervals_show = false
	timeline_interval_size = 24
	timeline_interval_offset = 0
	timeline_list_first = 0
	timeline_list_visible = 0
	timeline_mouse_pos = null
	timeline_marker = 0
	timeline_marker_move = 0
	timeline_marker_previous = 0
	timeline_region_start = null
	timeline_region_end = null
	timeline_region_pos = 0
	timeline_length = 0
	timeline_zoom = 16
	timeline_zoom_goal = 16
	timeline_zoom_current = 16
	timeline_zoom_target = 0
	timeline_camera = null
	timeline_insert_pos = 0
	timeline_show_frames = false
	timeline_marker_length = 0
	
	timeline_scroll_scalling = 0
	
	timeline_move_obj = null
	timeline_move_highlight_tl = null
	timeline_move_highlight_pos = null
	timeline_move_off_x = 0
	timeline_move_off_y = 0
	timeline_move_kf = null
	timeline_move_kf_mouse_pos = null
	timeline_sound_resize_mouse_pos = null
	timeline_sound_end_mousex = 0
	timeline_sound_end_value = 0
	
	timeline_settings = false
	timeline_settings_import_loop_tl = null
	timeline_settings_walk_fn = ""
	timeline_settings_run_fn = ""
	timeline_settings_keyframes = false
	timeline_settings_keyframes_export = false
	timeline_record_keyframes = false
	timeline_record_keyframes_balance = 0
	timeline_record_time_last = 0
	timeline_record_time_reset = true
	
	copy_kf_amount = 0
	copy_kf_pos[0] = 0
	copy_kf_value[0, 0] = 0
	copy_kf_tl_save_id[0] = ""
	copy_kf_tl_part_of_save_id[0] = ""
	copy_kf_tl_model_part_name[0] = null
	
	tree_list = ds_list_create()
	tree_list_filter = ds_list_create()
	tree_visible_list = ds_list_create()
	tree_update_parent_filter = app
	tree_update_extend = false
	tree_update_color = null
	
	project_model_list = ds_list_create()
	project_timeline_list = ds_list_create()
	
	timeline_settings_w = 0
	timeline_settings_right_w = 0
	
	timeline_marker_list = ds_list_create()
	timeline_marker_current = null
	timeline_marker_edit = null
	timeline_marker_edit_offset = 0
	

	// Native graph editor: UI-only state; actual curves live in e_value.
	graph_mode = false
	graph_group = 0
	graph_axis = 0
	graph_visible = [true, true, true]
	graph_selected = null
	graph_timeline = null
	graph_drag = 0
	graph_drag_first = true
	graph_drag_values = [0, 0, 0, 0, 0]
	graph_x_start = 0
	graph_x_scale = 16
	graph_y_center = 0
	graph_y_scale = 8
	graph_fit_pending = true
	graph_tbx_value = new_textbox_ndecimals()
	graph_layout_count = 1
	graph_view_index = 0
	graph_active_view = 0
	graph_target = null
	graph_views = [graph_view_new(0), graph_view_new(1), graph_view_new(2)]
	graph_numeric_tl = null
	graph_numeric_kf = null
	graph_numeric_vid = 0
	graph_drag_started = false
	graph_drag_lock = 0
	graph_drag_last_value = 0
	graph_drag_last_time = 0
	graph_drag_conflict = false
	graph_drag_opp_kf = null
	graph_drag_opp_right = null
	graph_drag_opp_len = 0
	graph_drag_aligned = false
	graph_grab_dx = 0
	graph_grab_dy = 0
	// Comfort UI state is not animation data.
	graph_sidebar_width = 256
	graph_sidebar_resizing = false
	graph_snap_value = false
	graph_show_grid = true
	graph_show_ghost = true
	graph_handle_link = false
	graph_frame_boxes = [new_textbox_ndecimals(), new_textbox_ndecimals(), new_textbox_ndecimals()]
	graph_focus_x = 0
	graph_focus_y = 0
	graph_focus_w = 0
	graph_focus_h = 0
	graph_drag_accum_x = 0
	graph_drag_accum_y = 0
	graph_drag_prev_x = 0
	graph_drag_prev_y = 0
	graph_scrub_start = 0
	graph_preferences_open = false
	graph_preferences_tab = 0
	graph_preferences_page = 0
	graph_pressed_control = ""
	graph_sidebar_scroll = [0, 0, 0]
	graph_options_apply()
	graph_status_msg = ""
	graph_status_time = 0
 ga_tools = false
 ga_tools_view = 0
 ga_tools_tab = 0
 ga_config_token = 0
 ga_config_page = 0
 ga_message = "Graph Channels alpha"
 ga_time_offset = 12
 ga_scale = 1
 ga_pivot_value = 0
 ga_strength = 0.5
 ga_tolerance = 0.01
 ga_time_delta = 0
 ga_plot_width = 800
 ga_clipboard = array_create(0)
 ga_ghost = array_create(0)
 ga_ghost_owner = ""
 ga_ghost_group = 0
 ga_config_boxes = array_create(14, null)
 ga_handle_boxes = array_create(4, null)
 ga_setting_boxes = array_create(5, null)
 ga_filter_box = new_textbox(true, 80, "")
 for (var g = 0; g < 14; g++) ga_config_boxes[g] = new_textbox_ndecimals()
 for (var g = 0; g < 4; g++) ga_handle_boxes[g] = new_textbox_ndecimals()
 for (var g = 0; g < 5; g++) ga_setting_boxes[g] = new_textbox_ndecimals()



	timeline_search = ""
	timeline_rename = null
	
	timeline_hide_color_tag = array_create(9, false)
	
	timeline_select_box_min = [no_limit, no_limit, no_limit]
	timeline_select_box_max = [-no_limit, -no_limit, -no_limit]
	
	// tl_update_list_indent scope fix
	with (obj_timeline)
		tl_update_list()
}
