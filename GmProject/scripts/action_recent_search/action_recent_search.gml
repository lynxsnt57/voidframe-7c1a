/// action_recent_search(search)
/// @arg search

function action_recent_search(search)
{
	recent_search = search
	recent_scrollbar.value = 0
	recent_scrollbar.value_goal = 0
	recent_update()
}
