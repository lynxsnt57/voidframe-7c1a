/// Native video decoding functions (implemented in CppProject/Library/MovieLib.cpp).
/// CppSeparate IntType video_frame_sprite(StringType, RealType)
function video_frame_sprite(filename, seconds) { return -1 }

/// CppSeparate RealType video_file_duration(StringType)
function video_file_duration(filename) { return 0 }

/// CppSeparate RealType video_file_aspect(StringType)
function video_file_aspect(filename) { return 1 }

/// CppSeparate BoolType video_file_has_audio(StringType)
function video_file_has_audio(filename) { return false }

/// CppSeparate void video_clip_retain(StringType)
function video_clip_retain(filename) { return 0 }

/// CppSeparate void video_clip_release(StringType)
function video_clip_release(filename) { return 0 }

/// CppSeparate void video_clip_discard(StringType)
function video_clip_discard(filename) { return 0 }

function video_track_choose_file(tl = null)
{
	var folder = "";
	if (tl != null && instance_exists(tl) && tl.video_path != "")
		folder = filename_dir(tl.video_path)
	else if (bench_video_path != "")
		folder = filename_dir(bench_video_path)

	var filename = file_dialog_open(
		"Video files (*.mp4;*.m4v;*.mov;*.avi;*.wmv;*.gif)|*.mp4;*.m4v;*.mov;*.avi;*.wmv;*.gif",
		"", folder, "Select video or animated GIF")
	if (!file_exists_lib(filename))
		return ""

	var duration = video_file_duration(filename)
	if (duration <= 0)
	{
		error("videocannotload")
		return ""
	}

	if (tl != null && instance_exists(tl))
		video_track_assign_file(tl, filename, duration)
	else
	{
		if (bench_video_path != "")
			video_clip_discard(bench_video_path)
		bench_video_path = filename
		bench_video_duration = duration
		bench_video_source_in = 0
		bench_video_source_out = duration
		bench_video_speed = 1
		bench_video_loop = false
		bench_video_hold_outside = true
	}
	return filename
}

function video_track_assign_file(tl, filename, duration = -1)
{
	if (tl = null || !instance_exists(tl) || !file_exists_lib(filename))
		return false
	if (duration <= 0)
		duration = video_file_duration(filename)
	if (duration <= 0)
	{
		error("videocannotload")
		return false
	}
	if (tl.video_path != "")
		video_clip_release(tl.video_path)

	// Remove a previously linked sound before replacing the source.
	with (tl)
	{
		for (var i = ds_list_size(tree_list) - 1; i >= 0; i--)
		{
			var child = tree_list[|i]
			if (child.type = e_tl_type.AUDIO && child.video_linked_audio)
				with (child)
				{
					tl_remove_clean()
					instance_destroy()
				}
		}

		video_path = filename
		video_clip_retain(filename)
		video_duration = duration
		video_source_in = 0
		video_source_out = duration
		video_speed = 1
		video_loop = false
		video_hold_outside = true
		video_aspect = video_file_aspect(filename)
		video_linked_audio = false
		
		if (temp != null)
		{
			with (temp)
			{
				type = e_temp_type.SURFACE
				shape_type = e_shape_type.SURFACE
				temp_update_shape()
			}
		}
		
		value[e_value.SCA_X] = video_aspect
		value_default[e_value.SCA_X] = video_aspect
		for (var k = 0; k < ds_list_size(keyframe_list); k++)
			keyframe_list[|k].value[e_value.SCA_X] = video_aspect
		tl_update_values()
	}

		video_track_attach_audio(tl)
	tl_update_length()
	project_reset_loaded()
	lib_preview.update = true
	return true
}

function video_track_attach_audio(video_tl)
{
	if (video_tl = null || !instance_exists(video_tl) || video_tl.video_path = "" || !video_file_has_audio(video_tl.video_path))
		return false

	// new_res uses app-owned resource lists and project paths.
	// This function can be called from a video timeline, so enter app scope.
	var sound = null
	log("VIDEO_AUDIO 01 before new_res")
	with (app)
		sound = new_res(video_tl.video_path, e_res_type.SOUND)
	log("VIDEO_AUDIO 02 after new_res")
	if (sound = null || !instance_exists(sound))
		return false
	sound.loaded = !sound.replaced
	with (sound)
		res_load()
	log("VIDEO_AUDIO 03 after res_load")

	var audio_tl = new_tl(e_tl_type.AUDIO)
	log("VIDEO_AUDIO 04 after new_tl")
	with (audio_tl)
	{
		name = video_tl.name + " Audio"
		tl_set_parent(video_tl)
		video_linked_audio = true
		var sound_key = tl_keyframe_add(video_tl.video_start_frame)
		sound_key.value[e_value.SOUND_OBJ] = sound
		sound.count++
		sound_key.value[e_value.SOUND_START] = video_tl.video_source_in
		sound_key.value[e_value.SOUND_PITCH] = video_tl.video_speed
		sound_key.value[e_value.SOUND_END] = ((video_tl.video_source_out - video_tl.video_source_in) / video_tl.video_speed) -
			(video_tl.video_duration / video_tl.video_speed) + video_tl.video_source_in
		tl_update_values()
		tl_update_display_name()
	}
	video_tl.video_linked_audio = true
	return true
}

function video_track_sync_audio(video_tl)
{
	if (video_tl = null || !instance_exists(video_tl))
		return 0
	with (video_tl)
	{
		for (var i = 0; i < ds_list_size(tree_list); i++)
		{
			var child = tree_list[|i]
			if (child.type != e_tl_type.AUDIO || !child.video_linked_audio)
				continue
			with (child)
			{
				for (var k = 0; k < ds_list_size(keyframe_list); k++)
				{
					var sound_key = keyframe_list[|k]
					sound_key.value[e_value.SOUND_START] = other.video_source_in
					sound_key.value[e_value.SOUND_PITCH] = other.video_speed
					sound_key.value[e_value.SOUND_END] = ((other.video_source_out - other.video_source_in) / other.video_speed) -
						(other.video_duration / other.video_speed) + other.video_source_in
				}
				tl_update_values()
			}
		}
	}
	return 0
}

// Keep linked audio aligned when the video's anchor keyframe is dragged or restored.
function video_track_sync_selected_audio_positions()
{
	with (obj_keyframe)
	{
		if (!selected || timeline.type != e_tl_type.VIDEO || position != timeline.video_start_frame)
			continue
		video_track_sync_audio_position(timeline)
	}
}

function video_track_sync_audio_position(video_tl)
{
	if (video_tl = null || !instance_exists(video_tl))
		return 0
	with (video_tl)
	{
		for (var i = 0; i < ds_list_size(tree_list); i++)
		{
			var child = tree_list[|i]
			if (child.type != e_tl_type.AUDIO || !child.video_linked_audio)
				continue
			with (child)
			{
				var count = ds_list_size(keyframe_list)
				if (count <= 0)
					continue
				var shift = other.video_start_frame - keyframe_list[|0].position
				if (shift = 0)
					continue
				var keys = array_create(count)
				var positions = array_create(count)
				for (var k = 0; k < count; k++)
				{
					keys[k] = keyframe_list[|k]
					positions[k] = keys[k].position + shift
				}
				ds_list_clear(keyframe_list)
				for (var k = 0; k < count; k++)
					tl_keyframe_add(positions[k], keys[k])
				tl_update_values()
			}
		}
	}
	return 0
}

function video_track_setting_changed()
{
	video_track_sync_audio(tl_edit)
	tl_update_length()
	project_reset_loaded()
	lib_preview.update = true
}

function action_tl_video_speed(value, add)
{
	var speed = (add ? tl_edit.video_speed * 100 + value : value) / 100
	tl_edit.video_speed = clamp(speed, 0.01, 4)
	video_track_setting_changed()
}

function action_tl_video_source_in(value, add)
{
	var source_in = add ? tl_edit.video_source_in + value : value
	tl_edit.video_source_in = clamp(source_in, 0, max(0, tl_edit.video_source_out - 0.01))
	video_track_setting_changed()
}

function action_tl_video_source_out(value, add)
{
	var source_out = add ? tl_edit.video_source_out + value : value
	tl_edit.video_source_out = clamp(source_out, min(tl_edit.video_duration, tl_edit.video_source_in + 0.01), tl_edit.video_duration)
	video_track_setting_changed()
}

function action_tl_video_loop(enabled)
{
	tl_edit.video_loop = enabled
	video_track_setting_changed()
}

function action_tl_video_hold_outside(enabled)
{
	tl_edit.video_hold_outside = enabled
	video_track_setting_changed()
}

function action_tl_video_visible(enabled)
{
	tl_edit.hide = !enabled
	project_reset_loaded()
}
