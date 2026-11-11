/// tab_timeline_editor_video()

function tab_timeline_editor_video()
{
	if (tl_edit.type != e_tl_type.VIDEO)
		return 0

	var filename = filename_name(tl_edit.video_path)
	if (filename = "")
		filename = text_get("listnone")

	tab_control_button_label()
	if (draw_button_label("timelineeditorvideofile", dx, dy, dw, icons.MOVIE, e_button.SECONDARY))
		video_track_choose_file(tl_edit)
	tab_next()

	draw_label(filename, dx, dy, fa_left, fa_middle, c_text_secondary, a_text_secondary, font_caption)
	dy += 24

	if (tl_edit.video_duration > 0)
	{
		draw_label(text_get("timelineeditorvideoduration", string_decimals(tl_edit.video_duration)), dx, dy, fa_left, fa_middle, c_text_tertiary, a_text_tertiary, font_caption)
		dy += 24
	}

	tab_control_dragger()
	draw_dragger("timelineeditorvideospeed", dx, dy, dragger_width, tl_edit.video_speed * 100, 1, 1, 400, 100, 1, tab.video.tbx_speed, action_tl_video_speed)
	tab_next()

	tab_control_dragger()
	draw_dragger("timelineeditorvideosourcein", dx, dy, dragger_width, tl_edit.video_source_in, 0.1, 0, max(0, tl_edit.video_source_out - 0.01), 0, 0.01, tab.video.tbx_source_in, action_tl_video_source_in)
	tab_next()

	tab_control_dragger()
	draw_dragger("timelineeditorvideosourceout", dx, dy, dragger_width, tl_edit.video_source_out, 0.1, min(tl_edit.video_duration, tl_edit.video_source_in + 0.01), tl_edit.video_duration, tl_edit.video_duration, 0.01, tab.video.tbx_source_out, action_tl_video_source_out)
	tab_next()

	tab_control_switch()
	draw_switch("timelineeditorvideoloop", dx, dy, tl_edit.video_loop, action_tl_video_loop)
	tab_next()

	if (draw_button_label("timelineeditorvideoblankoutside", dx, dy, dw / 2 - 4, icons.BLOCK, tl_edit.video_hold_outside ? e_button.SECONDARY : e_button.PRIMARY))
		action_tl_video_hold_outside(false)
	if (draw_button_label("timelineeditorvideoholdoutsidebutton", dx + dw / 2 + 4, dy, dw / 2 - 4, icons.MOVIE, tl_edit.video_hold_outside ? e_button.PRIMARY : e_button.SECONDARY))
		action_tl_video_hold_outside(true)
	tab_next()

	tab_control_switch()
	draw_switch("timelineeditorvideovisible", dx, dy, !tl_edit.hide, action_tl_video_visible)
	tab_next()
}
