/// action_project_video_flipaspect()

function action_project_video_flipaspect()
{
	var cache
	cache = project_video_width
	project_video_width = project_video_height
	project_video_height = cache
}
