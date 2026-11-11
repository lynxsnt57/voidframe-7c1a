/// Secondary motion controls: local anchor and tip offset, not a mesh editor.
function tab_frame_editor_secondary()
{
	if (tl_edit.type != e_tl_type.BODYPART) return 0
	tab_control_switch()
	draw_button_collapse("secondarymotion", collapse_map[?"secondarymotion"], action_tl_secondary_enabled, tl_edit.value[e_value.SECONDARY_ENABLED], "secondaryenabled", "secondarytip")
	tab_next()
	if (!collapse_map[?"secondarymotion"] || !tl_edit.value[e_value.SECONDARY_ENABLED]) return 0
	tab_collapse_start()
        if (!ds_map_exists(collapse_map, "secondaryaxes")) collapse_map[?"secondaryaxes"] = false
        if (!ds_map_exists(collapse_map, "secondaryfiles")) collapse_map[?"secondaryfiles"] = false
        var preset_ids = secondary_preset_ids(), preset_values = array_create(26, 0);
        for (var preset_field = 0; preset_field < 26; preset_field++) preset_values[preset_field] = tl_edit.value[preset_ids[preset_field]]
        var matched_preset = secondary_preset_match(preset_values);
        var preset_names = secondary_preset_names();
        var preset_text = matched_preset < 0 ? "Custom settings" : preset_names[matched_preset];
        tab_control_menu()
        draw_button_menu("secondarypresetlist", e_menu.LIST, dx, dy, dw, 24, matched_preset, preset_text, action_secondary_preset_choose, tl_edit.lock)
        tab_next()
        tab_control_menu()
        draw_button_menu("secondarymodelist", e_menu.LIST, dx, dy, dw, 24, tl_edit.value[e_value.SECONDARY_MODE], tl_edit.value[e_value.SECONDARY_MODE] == 0 ? "Swing - hair and accessories" : "Soft - spring translation", action_secondary_mode_choose, tl_edit.lock)
        tab_next()
        tab_control(32)
        var test_name = sm_preview ? "secondaryteststop" : "secondaryteststart";
        if (draw_button_label(test_name, dx, dy, min(dw, 200), null, e_button.SECONDARY, null, e_anchor.LEFT, tl_edit.lock))
        {
            action_secondary_preview_toggle()
            app_mouse_clear()
        }
        if (content_mouseon && app_mouse_box(dx, dy, min(dw, 200), 32))
            tip_set("Live test runs while the timeline is stopped. Ctrl+Alt+drag the round tip in the main viewport, then release. Esc cancels the grab. Test dragging does not create keyframes. Enable viewport Gizmos to see the handle.", dx, dy, min(dw, 200), 32)
        tab_next()

		tab_control_dragger()
		draw_dragger("secondaryblend", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_BLEND] * 100 * 100) / 100, 1, 0, 100, 100, 1, tab.constraints.tbx_secondary_blend, action_tl_secondary_blend, null, true, false, "Mix physics over the animated pose. Lower Influence for a subtler result.")
		tab_next()
		tab_control_dragger()
		draw_dragger("secondaryfrequency", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_FREQUENCY] * 1 * 100) / 100, 0.1, 0.1, 12, 2.5, 0.1, tab.constraints.tbx_secondary_frequency, action_tl_secondary_frequency, null, true, false, "Lower frequency is slower and softer; higher frequency is firmer. Start with a preset.")
		tab_next()
		tab_control_dragger()
		draw_dragger("secondarydamping", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_DAMPING] * 1 * 100) / 100, 0.01, 0, 3, 0.7, 0.01, tab.constraints.tbx_secondary_damping, action_tl_secondary_damping, null, true, false, "Increase Damping if the part keeps bouncing for too long.")
		tab_next()
		tab_control_dragger()
		draw_dragger("secondaryinertia", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_INERTIA] * 1 * 100) / 100, 0.01, 0, 2, 1, 0.01, tab.constraints.tbx_secondary_inertia, action_tl_secondary_inertia, null, true, false, "Response to parent movement. Start near 1; lower it to reduce lag.")
		tab_next()
		tab_control_dragger()
		draw_dragger("secondarygravity", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_GRAVITY] * 1 * 100) / 100, 1, -1000, 1000, 32, 1, tab.constraints.tbx_secondary_gravity, action_tl_secondary_gravity, null, true, false, "Downward acceleration along world Z. For less sag, reduce Gravity or raise Stiffness.")
		tab_next()
		tab_control_dragger()
		draw_dragger("secondarylimit", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_LIMIT] * 1 * 100) / 100, 1, 0, 175, 60, 1, tab.constraints.tbx_secondary_limit, action_tl_secondary_limit, null, true, false, "Swing angular limit in degrees. Soft mode uses the XYZ travel limits instead.")
		tab_next()
        if (tl_edit.value[e_value.SECONDARY_MODE] == 1)
        {
            tab_control_switch()
            draw_button_collapse("secondaryaxes", collapse_map[?"secondaryaxes"], null, true, "secondaryaxes", "Local-axis stiffness, damping and travel. Start with a preset before tuning these.")
            tab_next()
            if (collapse_map[?"secondaryaxes"])
            {
                tab_collapse_start()

			tab_control_dragger()
			draw_dragger("secondaryv2_1", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_STIFF_X], 0, 0.1, 4, 1, 0.01, sm_boxes[1], action_secondary_v2_1, null, true, false, "Local-axis stiffness multiplier. Higher values are firmer. Click the number to type.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_2", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_STIFF_Y], 0, 0.1, 4, 1, 0.01, sm_boxes[2], action_secondary_v2_2, null, true, false, "Local-axis stiffness multiplier. Higher values are firmer. Click the number to type.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_3", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_STIFF_Z], 0, 0.1, 4, 1, 0.01, sm_boxes[3], action_secondary_v2_3, null, true, false, "Local-axis stiffness multiplier. Higher values are firmer. Click the number to type.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_4", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_DAMP_X], 0, 0.1, 4, 1, 0.01, sm_boxes[4], action_secondary_v2_4, null, true, false, "Local-axis damping multiplier. Higher values settle faster.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_5", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_DAMP_Y], 0, 0.1, 4, 1, 0.01, sm_boxes[5], action_secondary_v2_5, null, true, false, "Local-axis damping multiplier. Higher values settle faster.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_6", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_DAMP_Z], 0, 0.1, 4, 1, 0.01, sm_boxes[6], action_secondary_v2_6, null, true, false, "Local-axis damping multiplier. Higher values settle faster.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_7", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_TRAVEL_X], 0, 0.001, 100, 0.5, 0.01, sm_boxes[7], action_secondary_v2_7, null, true, false, "Maximum local-axis travel, in world length units. Adjust for your model size.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_8", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_TRAVEL_Y], 0, 0.001, 100, 0.5, 0.01, sm_boxes[8], action_secondary_v2_8, null, true, false, "Maximum local-axis travel, in world length units. Adjust for your model size.")
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryv2_9", dx, dy, dragger_width, tl_edit.value[e_value.SECONDARY_TRAVEL_Z], 0, 0.001, 100, 0.5, 0.01, sm_boxes[9], action_secondary_v2_9, null, true, false, "Maximum local-axis travel, in world length units. Adjust for your model size.")
			tab_next()
		
                tab_collapse_end()
            }
        }
        tab_control_switch()
        draw_button_collapse("secondaryfiles", collapse_map[?"secondaryfiles"], null, true, "secondaryfiles", "Save or load JSON presets. Anchor and tip coordinates are preserved unless explicitly enabled below.")
        tab_next()
        if (collapse_map[?"secondaryfiles"])
        {
            tab_collapse_start()
            tab_control(32)
            var file_width = floor((dw - 8)/2);
            if (draw_button_label("secondarysave", dx, dy, file_width, null, e_button.SECONDARY, null))
            {
                action_secondary_preset_export()
                app_mouse_clear()
            }
            if (draw_button_label("secondaryload", dx + file_width + 8, dy, file_width, null, e_button.SECONDARY, null, e_anchor.LEFT, tl_edit.lock))
            {
                action_secondary_preset_import()
                app_mouse_clear()
            }
            tab_next()
            tab_control_switch()
            draw_switch("secondarygeometry", dx, dy, sm_preset_geometry, action_secondary_preset_geometry, "Off by default. Only enable this when the source and destination rig use compatible anchor/tip coordinates.")
            tab_next()
            tab_collapse_end()
        }

		tab_control_switch()
		draw_button_collapse("secondaryanchor", collapse_map[?"secondaryanchor"], null, true, "secondaryanchor", "secondaryanchortip")
		tab_next()
		if (collapse_map[?"secondaryanchor"])
		{
			tab_collapse_start()
			tab_control_dragger()
			draw_dragger("secondaryanchor_x", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_ANCHOR_X] * 1 * 100) / 100, 0.1, -10000, 10000, 0, 0.1, tab.constraints.tbx_secondary_anchor_x, action_tl_secondary_anchor_x)
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryanchor_y", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_ANCHOR_Y] * 1 * 100) / 100, 0.1, -10000, 10000, 0, 0.1, tab.constraints.tbx_secondary_anchor_y, action_tl_secondary_anchor_y)
			tab_next()
			tab_control_dragger()
			draw_dragger("secondaryanchor_z", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_ANCHOR_Z] * 1 * 100) / 100, 0.1, -10000, 10000, 0, 0.1, tab.constraints.tbx_secondary_anchor_z, action_tl_secondary_anchor_z)
			tab_next()
			tab_control_dragger()
			draw_dragger("secondarytip_x", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_TIP_X] * 1 * 100) / 100, 0.1, -10000, 10000, 0, 0.1, tab.constraints.tbx_secondary_tip_x, action_tl_secondary_tip_x)
			tab_next()
			tab_control_dragger()
			draw_dragger("secondarytip_y", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_TIP_Y] * 1 * 100) / 100, 0.1, -10000, 10000, 0, 0.1, tab.constraints.tbx_secondary_tip_y, action_tl_secondary_tip_y)
			tab_next()
			tab_control_dragger()
			draw_dragger("secondarytip_z", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_TIP_Z] * 1 * 100) / 100, 0.1, -10000, 10000, -8, 0.1, tab.constraints.tbx_secondary_tip_z, action_tl_secondary_tip_z)
			tab_next()
			tab_collapse_end()
		}
		tab_control_switch()
		draw_button_collapse("secondarywind", collapse_map[?"secondarywind"], null, true, "secondarywind")
		tab_next()
		if (collapse_map[?"secondarywind"])
		{
			tab_collapse_start()
			tab_control_dragger()
			draw_dragger("secondarywind_x", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_WIND_X] * 1 * 100) / 100, 1, -1000, 1000, 0, 1, tab.constraints.tbx_secondary_wind_x, action_tl_secondary_wind_x)
			tab_next()
			tab_control_dragger()
			draw_dragger("secondarywind_y", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_WIND_Y] * 1 * 100) / 100, 1, -1000, 1000, 0, 1, tab.constraints.tbx_secondary_wind_y, action_tl_secondary_wind_y)
			tab_next()
			tab_control_dragger()
			draw_dragger("secondarywind_z", dx, dy, dragger_width, round(tl_edit.value[e_value.SECONDARY_WIND_Z] * 1 * 100) / 100, 1, -1000, 1000, 0, 1, tab.constraints.tbx_secondary_wind_z, action_tl_secondary_wind_z)
			tab_next()
			tab_collapse_end()
		}
		tab_control_switch()
		secondary_ui_button(text_get("secondaryreset"), action_tl_secondary_reset)
		tab_next()
        tab_control_switch()
        secondary_ui_hint(sm_notice, sm_notice)
        tab_next()
	tab_collapse_end()
}

function rb_ui_button(name, action, disabled = false)
{
 tab_control(32)
 if (draw_button_label(name,dx,dy,min(dw,240),null,e_button.SECONDARY,null,e_anchor.LEFT,disabled || rb_job))
 { script_execute(action); app_mouse_clear() }
 tab_next()
}
function rb_ui_value(index, low, high, def, help)
{
 var ids = rb_ids();
 tab_control_dragger()
 draw_dragger("rigidvalue"+string(index),dx,dy,dragger_width,tl_edit.value[ids[index]],index >= 15 && index <= 17 || index >= 21 && index <= 23 || index >= 25 ? 1 : (index >= 8 ? 0.1 : 0.01),low,high,def,0.01,rb_boxes[index],rb_value_action(index),null,true,rb_job || tl_edit.lock,help)
 tab_next()
}
function rb_value_action(index)
{
 switch(index)
 {
  case 3: return action_rb_value_3
  case 4: return action_rb_value_4
  case 5: return action_rb_value_5
  case 6: return action_rb_value_6
  case 7: return action_rb_value_7
  case 8: return action_rb_value_8
  case 9: return action_rb_value_9
  case 10: return action_rb_value_10
  case 11: return action_rb_value_11
  case 12: return action_rb_value_12
  case 13: return action_rb_value_13
  case 15: return action_rb_value_15
  case 16: return action_rb_value_16
  case 17: return action_rb_value_17
  case 18: return action_rb_value_18
  case 19: return action_rb_value_19
  case 20: return action_rb_value_20
  case 21: return action_rb_value_21
  case 22: return action_rb_value_22
  case 23: return action_rb_value_23
  case 25: return action_rb_value_25
  case 26: return action_rb_value_26
 }
 return null
}
function tab_frame_editor_rigidbody()
{
 if (!rb_supported(tl_edit)) return 0
 var groups = ["rigidbody","rigidsetup","rigidcollider","rigidmaterial","rigidlimits","rigidvelocity","rigidcache"];
 for (var group = 0; group < array_length(groups); group++)
  if (!ds_map_exists(collapse_map,groups[group])) collapse_map[?groups[group]] = false
 tab_control_switch()
 draw_button_collapse("rigidbody",collapse_map[?"rigidbody"],action_rb_enabled,tl_edit.value[e_value.RB_ENABLED],"rigidbody","Enable controls simulation, not visibility of settings. All manual settings remain available while disabled. Optional Auto Setup commands are in the separate collapsed Automatic setup section below. Preview does not edit keys.")
 tab_next()
 if (!collapse_map[?"rigidbody"]) return 0
 tab_collapse_start()
 tab_control_menu()
 var modes = ["Dynamic","Static environment","Kinematic / animated"];
 draw_button_menu("rigidmotion",e_menu.LIST,dx,dy,dw,24,tl_edit.value[e_value.RB_MODE],modes[tl_edit.value[e_value.RB_MODE]],action_rb_mode,rb_job || tl_edit.lock)
 tab_next()
 rb_ui_button(rb_session == 1 ? "rigidstoppreview" : "rigidpreview",action_rb_preview,tl_edit.lock)
 rb_ui_button("rigidreset",action_rb_reset)
 tab_control_switch()
 draw_switch("rigidshow",dx,dy,rb_show_colliders,action_rb_colliders,"Outlines show collision proxies, not the visual mesh. Sphere proxies are shown as enclosing boxes.")
 tab_next()
 tab_control_switch()
 secondary_ui_hint(rb_notice,rb_notice)
 tab_next()
 tab_control_switch()
 draw_button_collapse("rigidsetup",collapse_map[?"rigidsetup"],null,true,"rigidsetup","Applies to all selected objects. Select parent/child parts for a ragdoll; the active object becomes the compound root for Weld Selection.")
 tab_next()
 if (collapse_map[?"rigidsetup"])
 {
  tab_collapse_start()
  rb_ui_button("rigiddynamic",action_rb_selected_dynamic,tl_edit.lock)
  rb_ui_button("rigidstatic",action_rb_selected_static,tl_edit.lock)
  rb_ui_button("rigidragdoll",action_rb_selected_ragdoll,tl_edit.lock)
  rb_ui_button("rigidweld",action_rb_selected_weld,tl_edit.lock)
  tab_collapse_end()
 }
 tab_control_switch()
 draw_button_collapse("rigidcollider",collapse_map[?"rigidcollider"],null,true,"rigidcollider","Fit sizes and offsets in local model units. There is no automatic mesh/voxel collision extraction. Use multiple Static boxes for stairs and uneven ground.")
 tab_next()
 if (collapse_map[?"rigidcollider"])
 {
  tab_collapse_start()
  tab_control_menu()
  draw_button_menu("rigidshape",e_menu.LIST,dx,dy,dw,24,tl_edit.value[e_value.RB_SHAPE],tl_edit.value[e_value.RB_SHAPE]==0?"Box":"Sphere",action_rb_shape,rb_job || tl_edit.lock)
  tab_next()
  rb_ui_value(8,0.01,100000,16,"Full local collider size. Start with the object dimensions; Reset and Preview to test.")
  rb_ui_value(9,0.01,100000,16,"Full local collider size. Start with the object dimensions; Reset and Preview to test.")
  rb_ui_value(10,0.01,100000,16,"Full local collider size. Start with the object dimensions; Reset and Preview to test.")
  rb_ui_value(11,-100000,100000,0,"Local offset from the timeline pivot to the collider center.")
  rb_ui_value(12,-100000,100000,0,"Local offset from the timeline pivot to the collider center.")
  rb_ui_value(13,-100000,100000,0,"Local offset from the timeline pivot to the collider center.")
  rb_ui_value(25,0,7,0,"Collision layer 0-7. Bodies interact only if both masks include the other body layer.")
  rb_ui_value(26,0,255,255,"Bit mask: 255 enables all 8 layers; 0 disables collision response.")
  tab_collapse_end()
 }
 tab_control_switch()
 draw_button_collapse("rigidmaterial",collapse_map[?"rigidmaterial"],null,true,"rigidmaterial")
 tab_next()
 if(collapse_map[?"rigidmaterial"])
 {
  tab_collapse_start()
  rb_ui_value(3,0.001,1000000,1,"Body mass. Avoid extreme mass ratios between linked parts.")
  rb_ui_value(4,0,2,0.6,"Surface friction. Increase to reduce sliding.")
  rb_ui_value(5,0,1,0.05,"Restitution: 0 is inelastic, 1 is very bouncy.")
  rb_ui_value(6,0,10,0.05,"Linear velocity damping.")
  rb_ui_value(7,0,10,0.1,"Angular velocity damping.")
  tab_collapse_end()
 }
 tab_control_switch()
 draw_button_collapse("rigidlimits",collapse_map[?"rigidlimits"],null,true,"rigidlimits","Child pivots are joint anchors. Blank target uses the enabled timeline parent. Weld merges linked shapes into a compound body; ball/hinge joints keep bodies separate.")
 tab_next()
 if(collapse_map[?"rigidlimits"])
 {
  tab_collapse_start()
  tab_control_menu()
  var connections = ["None / independent","Limited ball joint","Hinge (local X)","Weld / compound body"];
  draw_button_menu("rigidjoint",e_menu.LIST,dx,dy,dw,24,tl_edit.value[e_value.RB_JOINT],connections[tl_edit.value[e_value.RB_JOINT]],action_rb_joint,rb_job || tl_edit.lock)
  tab_next()
  tab_control_menu()
  var target = tl_edit.value[e_value.RB_PARENT];
  draw_button_menu("rigidtarget",e_menu.TIMELINE,dx,dy,dw,24,target,instance_exists(target)?"Assigned target":"Timeline parent",action_rb_parent,rb_job || tl_edit.lock)
  tab_next()
  rb_ui_value(15,0,175,60,"Symmetric angular limit relative to the starting pose. Hinge uses local X; Y/Z are locked. Rotation-vector limits are approximate, not anatomical joint motors.")
  rb_ui_value(16,0,175,60,"Symmetric angular limit relative to the starting pose. Hinge uses local X; Y/Z are locked. Rotation-vector limits are approximate, not anatomical joint motors.")
  rb_ui_value(17,0,175,45,"Symmetric angular limit relative to the starting pose. Hinge uses local X; Y/Z are locked. Rotation-vector limits are approximate, not anatomical joint motors.")
  tab_collapse_end()
 }
 tab_control_switch()
 draw_button_collapse("rigidvelocity",collapse_map[?"rigidvelocity"],null,true,"rigidvelocity","Used when starting a fresh simulation. It does not alter animation keys.")
 tab_next()
 if(collapse_map[?"rigidvelocity"])
 {
  tab_collapse_start()
  rb_ui_value(18,-100000,100000,0,"Initial world-space linear velocity.")
  rb_ui_value(19,-100000,100000,0,"Initial world-space linear velocity.")
  rb_ui_value(20,-100000,100000,0,"Initial world-space linear velocity.")
  rb_ui_value(21,-100000,100000,0,"Initial world-space angular velocity in degrees per second.")
  rb_ui_value(22,-100000,100000,0,"Initial world-space angular velocity in degrees per second.")
  rb_ui_value(23,-100000,100000,0,"Initial world-space angular velocity in degrees per second.")
  tab_collapse_end()
 }
 tab_control_switch()
 draw_button_collapse("rigidcache",collapse_map[?"rigidcache"],null,true,"rigidcache","Simulate all enabled/configured bodies, including Static environment proxies. Esc cancels. Cache is transient and invalidated by edits. Bake affects Dynamic bodies only, preserves unrelated channels, and adds boundary keys.")
 tab_next()
 if(collapse_map[?"rigidcache"])
 {
  tab_collapse_start()
  tab_control_dragger()
  draw_dragger("rigidstart",dx,dy,dragger_width,rb_frame_start,1,0,100000,0,1,rb_boxes[27],action_rb_start_frame,null,true,rb_job)
  tab_next()
  tab_control_dragger()
  draw_dragger("rigidend",dx,dy,dragger_width,rb_frame_end,1,0,100000,120,1,rb_boxes[28],action_rb_end_frame,null,true,rb_job)
  tab_next()
  rb_ui_button("rigidsimulate",action_rb_simulate)
  rb_ui_button("rigidbake",action_rb_bake,!rb_cache_valid || rb_session!=2)
  tab_collapse_end()
 }
 tab_collapse_end()
}

// Optional automation never replaces or gates the manual modifier controls.
function tab_frame_editor_rig_auto_tools()
{
 if (!instance_exists(tl_edit)) return 0
 if (!rb_supported(tl_edit) && tl_edit.type != e_tl_type.MODEL && tl_edit.type != e_tl_type.CHARACTER) return 0
 if (!ds_map_exists(collapse_map,"rigidautotools")) collapse_map[?"rigidautotools"] = false
 tab_control_switch()
 draw_button_collapse("rigidautotools",collapse_map[?"rigidautotools"],null,true,"rigidautotools","Optional whole-rig presets. Expanding this section changes no settings. Auto Setup only runs when its button is clicked and its confirmation dialog is accepted.")
 tab_next()
 if (!collapse_map[?"rigidautotools"]) return 0
 tab_collapse_start()
 rb_ui_button("rigidautosetup",action_rb_auto_setup,tl_edit.lock)
 rb_ui_button("rigidautofall",action_rb_auto_fall,tl_edit.lock)
 tab_control_switch()
 draw_switch("rigidautoground",dx,dy,rb_use_ground,action_rb_ground,"Collide with the visible background ground at Z=-0.01. Flat ground only; imported scenery still needs colliders.",rb_job)
 tab_next()
 tab_collapse_end()
}
