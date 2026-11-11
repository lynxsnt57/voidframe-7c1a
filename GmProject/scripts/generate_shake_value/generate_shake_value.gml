/// generate_shake_value(step, strength, offset,sinwavemode)
/// @arg step
/// @arg strength
/// @arg offset
/// @arg sinwavemode

function generate_shake_value(step, shakestrength, shakeoffset, sinwavemode){
	var shake
	
	if (!sinwavemode)
	{
		shake = vec3(
						simplex_lib(step + 0 + shakeoffset) * shakestrength,
						simplex_lib(step + 1000 + shakeoffset) * shakestrength,
						simplex_lib(step + 2000 + shakeoffset) * shakestrength,
					);
	} else {
		step = step + shakeoffset
		shake = vec3(sin(step) * shakestrength, cos(step) * shakestrength, sin(step + 1.5708) * shakestrength)
	}
	
	return shake;
}