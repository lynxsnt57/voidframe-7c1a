/// tl_update_values_ease(valueid)
/// @arg valueid

function tl_update_values_ease(vid)
{
 if (vid >= e_value.POS_X && vid <= e_value.SCA_Z && value_default[e_value.GRAPH_CHANNELS_USED] > 0)
 {
  var channelval = ga_curve(id, vid, app.timeline_marker);
  if (value[vid] != channelval) { value[vid] = channelval; update_matrix = true }
  return 0
 }

	var oldval, curval, nextval, val;
	oldval = value[vid]
	
	if (keyframe_animate)
	{
		curval = keyframe_current_values[vid]
		nextval = keyframe_next_values[vid]
		
		var customcurve = false;
		if (vid >= e_value.POS_X && vid <= e_value.SCA_Z)
			customcurve = keyframe_current_values[e_value.GRAPH_POS_X_MODE + (vid - e_value.POS_X) * 5] > 0

		// Equal endpoints can still have a Bezier overshoot.
		if ((oldval = curval) && (oldval = nextval) && !customcurve)
			return 0
		
		if (customcurve)
			val = graph_segment_value(keyframe_current_values, keyframe_next_values, vid, keyframe_progress)
		else
			val = tl_value_interpolate(vid, keyframe_progress_ease, curval, nextval)
	}
	else if (keyframe_next)
		val = keyframe_next_values[vid]
	else
		val = value_default[vid]
	
	if (oldval != val)
	{
		update_matrix = true
		value[vid] = val
	}
}
