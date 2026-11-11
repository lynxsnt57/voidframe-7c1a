function graph_button(label, xx, yy, wid, active, disabled, tip)
{
 return graph_control(label, xx, yy, wid, 24, active, disabled, tip)
}
function graph_control(label, xx, yy, wid, hei, active, disabled, tip)
{
 var clipped = shader_clip_active && !app_mouse_box(shader_clip_x, shader_clip_y, shader_clip_width, shader_clip_height);
 var hover = content_mouseon && !disabled && !clipped && window_busy == "" && app_mouse_box(xx, yy, wid, hei);
 var uid = string(xx) + ":" + string(yy) + ":" + string(wid) + ":" + string(hei);
 if (hover && mouse_left_pressed) graph_pressed_control = uid
 var pressed = graph_pressed_control == uid && mouse_left;
 draw_box(xx, yy, wid, hei, false, active ? c_accent_overlay : (hover ? c_level_top : c_level_middle), active ? max(0.35, a_accent_overlay) : 1)
 draw_box(xx, yy, wid, hei, true, active ? c_accent : c_border, active ? 0.65 : (hover ? 0.8 : (setting_graph_options[19] != 0 ? 0.55 : 0.3)))
 if (pressed) draw_box(xx + 1, yy + 1, wid - 2, hei - 2, false, c_accent_overlay, a_accent_overlay)
 draw_set_font(font_label)
 draw_label(string_limit(label, max(0, wid - 8)), xx + wid/2, yy + hei/2, fa_center, fa_middle, disabled ? c_text_tertiary : (active ? c_accent : c_text_main), 1, font_label)
 if (tip != "" && !clipped) tip_set(tip, xx, yy, wid, hei)
 if (hover) mouse_cursor = cr_handpoint
 if (hover && mouse_left_released && graph_pressed_control == uid)
 {
  graph_pressed_control = ""
  app_mouse_clear()
  return true
 }
 return false
}

function graph_icon(xx, yy, active)
{
	// Original glyph: an ascending ribbon curve, two square keys and a tangent.
	var color = active ? c_accent : c_text_secondary;
	draw_line_ext(xx + 3, yy + 20, xx + 3, yy + 4, color, 0.45)
	draw_line_ext(xx + 3, yy + 20, xx + 21, yy + 20, color, 0.45)
	var px = xx + 5, py = yy + 17;
	for (var s = 1; s <= 20; s++)
	{
		var t = s/20, nx = xx + 5 + 14*t, ny = yy + 17 - 11*(3*t*t - 2*t*t*t);
		draw_line_ext(px, py, nx, ny, color, 1)
		px = nx
		py = ny
	}
	draw_line_ext(xx + 13, yy + 6, xx + 21, yy + 6, color, 0.65)
	draw_box(xx + 3, yy + 15, 4, 4, false, color, 1)
	draw_box(xx + 17, yy + 4, 4, 4, false, color, 1)
}

function graph_toggle_button(xx, yy)
{
	var clicked = graph_button("", xx, yy, 24, graph_mode, false, graph_mode ? "Back to timeline" : "Graph editor - edit animation curves");
	graph_icon(xx, yy, graph_mode)
	return clicked
}

function graph_node(xx, yy, color, selected)
{
 var radius = setting_graph_options[0];
 if (selected)
 {
  draw_circle_ext(xx, yy, radius, false, 24, color, 1)
  draw_circle_ext(xx, yy, radius + 1.5, true, 24, c_text_main, 1)
  draw_circle_ext(xx, yy, 1.5, false, 16, c_text_main, 1)
 }
 else
 {
  draw_box(xx - radius + 1, yy - radius + 1, radius*2 - 2, radius*2 - 2, false, c_level_bottom, 1)
  draw_box(xx - radius + 1, yy - radius + 1, radius*2 - 2, radius*2 - 2, true, color, 0.85)
 }
}

function graph_axis_color(axis)
{
	if (axis == 0) return make_color_rgb(235, 108, 117)
	if (axis == 1) return make_color_rgb(126, 202, 133)
	return make_color_rgb(112, 177, 244)
}

function graph_screen_y(val, top, hei)
{
	return top + hei/2 - (val - graph_y_center) * graph_y_scale
}

function graph_screen_x(frame, left)
{
	return left + (frame - graph_x_start) * graph_x_scale
}

// 1 / 2 / 5 grid steps, stable at extreme zoom levels.
function graph_grid_step(raw)
{
 var magnitude = power(10, floor(log2(max(0.00000001, raw))/log2(10)));
 if (raw <= magnitude) return magnitude
 if (raw <= magnitude*2) return magnitude*2
 if (raw <= magnitude*5) return magnitude*5
 return magnitude*10
}

// Bounded pixel-offset strokes keep thickness independent from curve zoom.
function graph_curve_line(x1, y1, x2, y2, color, alpha)
{
 var width = setting_graph_options[3];
 for (var stroke = 0; stroke < width; stroke++)
 {
  var offset = stroke - (width - 1)/2;
  draw_line_ext(x1, y1 + offset, x2, y2 + offset, color, alpha)
 }
}
