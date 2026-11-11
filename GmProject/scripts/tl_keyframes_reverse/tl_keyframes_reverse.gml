/// tl_keyframes_reverse()
/// @desc Reverses the selected keyframes.

function tl_keyframes_reverse()
{
	var minpos, maxpos;
	minpos = no_limit
	maxpos = -no_limit
	
	// Find range
	with (obj_keyframe)
	{
		if (!selected)
			continue
		
		minpos = min(minpos, position)
		maxpos = max(maxpos, position)
	}
	
	if (minpos = maxpos)
		return 0
	
	// Update positions
	with (obj_keyframe)
	{
		if (!selected)
			continue
		
		var newpos = minpos + (maxpos - position);
		
		// Remove from list and re-add at new position
		ds_list_delete_value(timeline.keyframe_list, id)
		position = newpos
	}
	
	// Re-add to lists
	with (obj_keyframe)
	{
		if (!selected)
			continue
		
		with (timeline)
		{
			tl_keyframe_add(other.position, other.id)
			update_matrix = true
		}
	}
}