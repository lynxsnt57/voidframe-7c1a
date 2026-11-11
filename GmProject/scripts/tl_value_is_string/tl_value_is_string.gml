/// tl_value_is_string(valueid)
/// @arg valueid

function tl_value_is_string(vid)
{
	return (vid = e_value.TEXT ||
			vid = e_value.TEXT_HALIGN ||
			vid = e_value.TEXT_VALIGN ||
			vid = e_value.TRANSITION ||
			vid = e_value.CAM_OUTLINE_BLEND_MODE ||
			vid = e_value.CAM_BLOOM_BLEND_MODE ||
			vid = e_value.BG_BIOME)
}