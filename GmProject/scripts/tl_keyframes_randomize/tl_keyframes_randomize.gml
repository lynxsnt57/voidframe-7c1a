/// tl_keyframes_randomize()
/// @desc Randomizes the positions of selected keyframes within their range.

function tl_keyframes_randomize()
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
		
		ds_list_delete_value(timeline.keyframe_list, id)
		position = irandom_range(minpos, maxpos)
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