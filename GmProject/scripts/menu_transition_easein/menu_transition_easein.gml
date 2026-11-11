/// menu_transition_easein(x, y, width, height)
/// @arg x
/// @arg y
/// @arg width
/// @arg height

function menu_transition_easein(xx, yy, wid, hei)
{
	var transition = null;
	dx_start = xx + 12
	dx = dx_start
	dy_start = yy
	dy = dy_start + 12
	
	// Ease in
	dy += 14
	draw_label(text_get("transitionmenueasein"), dx, dy, fa_left, fa_bottom, c_text_tertiary, a_text_tertiary, font_subheading)
	dy += 12
	
	for (var i = 0; i < ds_list_size(transition_list); i++)
	{
		if (string_contains(transition_list[|i], "easein") &&
			!string_contains(transition_list[|i], "easeinout"))
		{
			if (dx + 46 > xx + wid)
			{
				dx = dx_start
				dy += 46
			}
			
			if (draw_button_transition(dx, dy, transition_list[|i]))
				transition = transition_list[|i]
			
			dx += 46
		}
	}
	dx = dx_start
	dy += 46
	
	if (settings_menu_name != "" && transition != null) // Settings menu
	{
		action_tl_frame_transition(transition)
		
		// Save this transition as the new default for the Ease In button
		timeline.transition_easein = transition
		
		settings_menu_ani_type = "hide"
		settings_menu_ani = (settings_menu_quick ? 0.01 : 1)
		window_busy = ""
		
		return true
	}
	
	if (context_menu_name != "" && transition != null) // Context menu
	{
		action_tl_frame_transition(transition)
		
		// Save this transition as the new default for the Ease In button
		timeline.transition_easein = transition
		
		context_menu_close()
		
		app_mouse_clear()
		
		return true
	}
	else if (transition != null) // Drop down menu
	{
		action_tl_frame_transition(transition)
		
		// Save this transition as the new default for the Ease In button
		timeline.transition_easein = transition
		
		menu_current.menu_ani = 1
		menu_current.menu_ani_type = "hide"
		window_busy = ""
		
		return true
	}
	
	return transition
}
