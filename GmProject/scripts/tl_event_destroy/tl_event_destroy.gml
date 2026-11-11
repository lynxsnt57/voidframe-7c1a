/// tl_event_destroy()
/// @desc Destroy event of timelines

function tl_event_destroy()
{
	if (app.rb_session != 0 || app.rb_job) with (app) action_rb_reset()
	secondary_motion_release(id)
	if (!delete_ready)
		tl_remove_clean()
}
