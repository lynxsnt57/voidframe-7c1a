///convert_fov_to_focal_length(fov, res_width, res_height)
///@arg fov
///@arg res_width
///@arg res_height

function convert_fov_to_focal_length(fov_deg, res_width, res_height)
{
    var fov_rad = fov_deg * pi / 180;
    var aspect = res_width / res_height;
	var result
    
    // Assume FOV is horizontal if aspect > 1 (landscape)
    if (aspect > 1)
        result = (res_width / (2 * tan(fov_rad / 2)));  // horizontal FOV
    else
        result = (res_height / (2 * tan(fov_rad / 2))); // vertical FOV

	return round(result / (((aspect > 1) ? res_height : res_width) / 1920)) / 100;
}