/// movie_set(width, height, bitrate, framerate, audio, colortype[0 = 420, 1 = 444], legacy)
/// @arg width
/// @arg height
/// @arg bitrate
/// @arg framerate
/// @arg audio

function movie_set(width, height, bitrate, framerate, audio, colortype, legacy)
{
	return external_call(lib_movie_set, width, height, bitrate, framerate, audio, colortype, legacy)
}
