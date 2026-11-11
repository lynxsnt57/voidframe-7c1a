/// tl_value_clamp(valueid, value)
/// @arg valueid
/// @arg value

function tl_value_clamp(vid, val)
{
 if (vid >= e_value.GRAPH_KEY_MASK && vid <= e_value.GRAPH_SCA_Z_LIMIT_MAX)
 {
  if (!is_real(val) || val != val || abs(val) > 1000000000) return tl_value_default(vid)
  if (vid == e_value.GRAPH_KEY_MASK) return clamp(round(val), 0, 511)
  if (vid == e_value.GRAPH_ONLY || vid == e_value.GRAPH_CHANNELS_USED) return clamp(round(val), 0, 1)
  var field = (vid - e_value.GRAPH_POS_X_CYCLE) mod 14;
  if (field == 0) return clamp(round(val), 0, 3)
  if (field == 1 || field == 6 || field == 11) return clamp(round(val), 0, 1)
  if (field == 2) return clamp(val, 0, 2)
  if (field == 4) return clamp(val, 0.0001, 100)
  if (field == 5) return clamp(round(val), -1000000, 1000000)
  if (field == 7 || field == 8) return clamp(round(val), 0, 1000000)
  if (field == 10) return clamp(val, 0.001, 1000000)
  return clamp(val, -1000000, 1000000)
 }

	if (vid >= e_value.RB_ENABLED && vid <= e_value.RB_MASK)
	{
		if (vid == e_value.RB_PARENT) return val
		if (!is_real(val) || val != val || abs(val) > 1000000) return tl_value_default(vid)
		if (vid == e_value.RB_ENABLED) return val >= 0.5
		if (vid == e_value.RB_MODE) return clamp(round(val), 0, 2)
		if (vid == e_value.RB_SHAPE) return clamp(round(val), 0, 1)
		if (vid == e_value.RB_JOINT) return clamp(round(val), 0, 3)
		if (vid == e_value.RB_MASS) return clamp(val, 0.001, 1000000)
		if (vid == e_value.RB_FRICTION) return clamp(val, 0, 2)
		if (vid == e_value.RB_BOUNCE) return clamp(val, 0, 1)
		if (vid == e_value.RB_LINEAR_DRAG || vid == e_value.RB_ANGULAR_DRAG) return clamp(val, 0, 10)
		if (vid >= e_value.RB_SIZE_X && vid <= e_value.RB_SIZE_Z) return clamp(val, 0.01, 100000)
		if (vid >= e_value.RB_LIMIT_X && vid <= e_value.RB_LIMIT_Z) return clamp(val, 0, 175)
		if (vid == e_value.RB_LAYER) return clamp(round(val), 0, 7)
		if (vid == e_value.RB_MASK) return clamp(round(val), 0, 255)
		return clamp(val, -100000, 100000)
	}
	if (vid >= e_value.SECONDARY_MODE && vid <= e_value.SECONDARY_TRAVEL_Z)
	{
		if (!is_real(val) || val != val || abs(val) > 1000000) return tl_value_default(vid)
		if (vid == e_value.SECONDARY_MODE) return clamp(round(val), 0, 1)
		if (vid >= e_value.SECONDARY_TRAVEL_X) return clamp(val, 0.001, 100)
		return clamp(val, 0.1, 4)
	}
	if (vid >= e_value.SECONDARY_ENABLED && vid <= e_value.SECONDARY_WIND_Z)
	{
		switch (vid)
		{
			case e_value.SECONDARY_ENABLED: return clamp(val, 0, 1)
			case e_value.SECONDARY_BLEND: return clamp(val, 0, 1)
			case e_value.SECONDARY_FREQUENCY: return clamp(val, 0.1, 12)
			case e_value.SECONDARY_DAMPING: return clamp(val, 0, 3)
			case e_value.SECONDARY_INERTIA: return clamp(val, 0, 2)
			case e_value.SECONDARY_GRAVITY: return clamp(val, -1000, 1000)
			case e_value.SECONDARY_LIMIT: return clamp(val, 0, 175)
			case e_value.SECONDARY_ANCHOR_X: return clamp(val, -10000, 10000)
			case e_value.SECONDARY_ANCHOR_Y: return clamp(val, -10000, 10000)
			case e_value.SECONDARY_ANCHOR_Z: return clamp(val, -10000, 10000)
			case e_value.SECONDARY_TIP_X: return clamp(val, -10000, 10000)
			case e_value.SECONDARY_TIP_Y: return clamp(val, -10000, 10000)
			case e_value.SECONDARY_TIP_Z: return clamp(val, -10000, 10000)
			case e_value.SECONDARY_WIND_X: return clamp(val, -1000, 1000)
			case e_value.SECONDARY_WIND_Y: return clamp(val, -1000, 1000)
			case e_value.SECONDARY_WIND_Z: return clamp(val, -1000, 1000)
		}
	}

	if (vid >= e_value.GRAPH_POS_X_MODE && vid <= e_value.GRAPH_SCA_Z_IN_Y)
	{
		var field = (vid - e_value.GRAPH_POS_X_MODE) mod 5;
		if (!is_real(val) || val != val || abs(val) > 1000000000) return tl_value_default(vid)
		if (field = 0) return clamp(round(val), 0, 5)
		if (field = 1 || field = 3) return clamp(val, 0, 1)
		return clamp(val, -1000000, 1000000)
	}

	if (app.setting_unlimited_values)
	{
		if (tl_value_is_string(vid) || tl_value_is_bool(vid) || tl_value_is_obj(vid))
			return val
		else if (tl_value_is_color(vid))
			return clamp(val, c_black, c_white)
		else if ((vid = e_value.SCA_X || vid = e_value.SCA_Y || vid = e_value.SCA_Z) && val = 0)
			return 0.0001
		else
			return clamp(val, -no_limit, no_limit)
	}
	
	switch (vid)
	{
		case e_value.LIGHT_GOBO_INTENSITY:
		case e_value.LIGHT_GOBO_SOFTNESS:
		case e_value.LIGHT_GOBO_FEATHER: return clamp(val, 0, 1)
		case e_value.LIGHT_GOBO_REPEAT_X:
		case e_value.LIGHT_GOBO_REPEAT_Y: return max(val, 0)
		case e_value.POS_X:
		case e_value.POS_Y:
		case e_value.POS_Z: return clamp(val, -app.project_render_distance, app.project_render_distance)
		case e_value.SCA_X:
		case e_value.SCA_Y:
		case e_value.SCA_Z: return max(val, 0.0001)
		case e_value.ALPHA:
		case e_value.MIX_PERCENT:
		case e_value.METALLIC:
		case e_value.ROUGHNESS:
		case e_value.SUBSURFACE_RADIUS_RED:
		case e_value.SUBSURFACE_RADIUS_GREEN:
		case e_value.SUBSURFACE_RADIUS_BLUE:
		case e_value.WIND_INFLUENCE:
		case e_value.CAM_BLOOM_THRESHOLD:
		case e_value.CAM_VIGNETTE_RADIUS:
		case e_value.CAM_VIGNETTE_SOFTNESS:
		case e_value.CAM_VIGNETTE_STRENGTH: return clamp(val, 0, 1)
		case e_value.RGB_ADD:
		case e_value.RGB_SUB:
		case e_value.RGB_MUL:
		case e_value.HSB_ADD:
		case e_value.HSB_SUB:
		case e_value.HSB_MUL:
		case e_value.MIX_COLOR:
		case e_value.SUBSURFACE_COLOR:
		case e_value.GLOW_COLOR:
		case e_value.LIGHT_COLOR:
		case e_value.CAM_BLOOM_BLEND:
		case e_value.CAM_COLOR_BURN:
		case e_value.CAM_VIGNETTE_COLOR:
		case e_value.BG_SKY_COLOR: 
		case e_value.BG_SKY_CLOUDS_COLOR:
		case e_value.BG_SUNLIGHT_COLOR:
		case e_value.BG_AMBIENT_COLOR:
		case e_value.BG_NIGHT_SKY_COLOR:
		case e_value.BG_NIGHT_SKY_CLOUDS_COLOR:
		case e_value.BG_NIGHT_SKY_STARS_COLOR:
		case e_value.BG_NIGHT_COLOR:
		case e_value.BG_GRASS_COLOR:
		case e_value.BG_FOLIAGE_COLOR:
		case e_value.BG_DRY_FOLIAGE_COLOR:
		case e_value.BG_WATER_COLOR:
		case e_value.BG_LEAVES_OAK_COLOR:
		case e_value.BG_LEAVES_SPRUCE_COLOR:
		case e_value.BG_LEAVES_BIRCH_COLOR:
		case e_value.BG_LEAVES_JUNGLE_COLOR:
		case e_value.BG_LEAVES_ACACIA_COLOR:
		case e_value.BG_LEAVES_DARK_OAK_COLOR:
		case e_value.BG_LEAVES_MANGROVE_COLOR:
		case e_value.BG_FOG_COLOR: return clamp(val, c_black, c_white)
		case e_value.BEND_ANGLE_X:
		case e_value.BEND_ANGLE_Y:
		case e_value.BEND_ANGLE_Z: return clamp(val, -180, 180)
		case e_value.CAM_FOV: return clamp(val, 1, 170)
		case e_value.CAM_BLADE_AMOUNT: return clamp(val, 0, 32)
		case e_value.CAM_ROTATE_DISTANCE: return max(1, val)
		case e_value.CAM_EXPOSURE:
		case e_value.CAM_GAMMA:
		case e_value.BG_SUNLIGHT_STRENGTH:
		case e_value.LIGHT_STRENGTH:
		case e_value.EMISSIVE:
		case e_value.SUBSURFACE:
		case e_value.CAM_SHAKE_STRENGTH_X:
		case e_value.CAM_SHAKE_STRENGTH_Y:
		case e_value.CAM_SHAKE_STRENGTH_Z:
		case e_value.CAM_SHAKE_SPEED_X:
		case e_value.CAM_SHAKE_SPEED_Y:
		case e_value.CAM_SHAKE_SPEED_Z:
		case e_value.CAM_CA_RED_OFFSET:
		case e_value.CAM_CA_GREEN_OFFSET:
		case e_value.CAM_CA_BLUE_OFFSET: return clamp(val, 0, no_limit)
		case e_value.CAM_WIDTH:
		case e_value.CAM_HEIGHT: return max(1, val)
		case e_value.BG_SKY_MOON_PHASE: return clamp(val, 0, 7)
		case e_value.BG_FOG_DISTANCE:
		case e_value.BG_FOG_SIZE: return clamp(val, 10, app.project_render_distance)
		case e_value.BG_FOG_HEIGHT: return clamp(val, 10, 2000)
		case e_value.BG_WIND_SPEED: return clamp(val, 0, 1)
		case e_value.BG_WIND_STRENGTH: return clamp(val, 0, 8)
		case e_value.BG_TEXTURE_ANI_SPEED: return max(val, 0)
		case e_value.SOUND_VOLUME: return clamp(val, 0, 1)
		case e_value.SOUND_PITCH: return clamp(val, 0.5, 2)
		case e_value.SOUND_START: return max(val, 0)
		case e_value.TEXT:
		case e_value.TEXT_HALIGN:
		case e_value.TEXT_VALIGN:
		case e_value.CAM_OUTLINE_BLEND_MODE:
		case e_value.CAM_BLOOM_BLEND_MODE:
		case e_value.BG_BIOME:
		case e_value.TRANSITION: return val
	}
	
	return clamp(val, -no_limit, no_limit)
}
