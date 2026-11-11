/// Initialize world import variables

function app_startup_interface_world_import()
{
	world_import_settings_popup = new_popup("worldsettings", popup_worldsettings_draw, 600, 532, true, false, false, true, world_import_apply_settings)
	world_import_surface = null
	world_import_add_tl = false
	world_import_temp = null
	
	world_import_settings_block_select = null
	world_import_settings_block_list = new_obj(obj_sortlist)
	world_import_settings_block_list.can_deselect = true
	world_import_settings_block_list.script = action_world_import_settings_block_select
	sortlist_column_add(world_import_settings_block_list, "blockfilter", 0)
	for (var b = 0; b < ds_list_size(mc_assets.block_list); b++)
		if (ds_list_find_index(setting_world_import_filter_list, b) < 0) // Not filtered
			sortlist_add(world_import_settings_block_list, b)
		
	world_import_settings_filter_select = null
	world_import_settings_filter_list = new_obj(obj_sortlist)
	world_import_settings_filter_list.can_deselect = true
	world_import_settings_filter_list.script = action_world_import_settings_filter_select
	sortlist_column_add(world_import_settings_filter_list, "blockfilter", 0)
	for (var i = 0; i < ds_list_size(setting_world_import_filter_list); i++) // Add indices from filter list
		sortlist_add(world_import_settings_filter_list, setting_world_import_filter_list[|i])
	
	world_import_set_instance(setting_world_import_instance)
	world_import_startup()
}

/// CppSeparate void world_import_startup()
function world_import_startup()
{
}

/// Start "Import from world" mode.
function world_import_begin(addtl = true, temp = null)
{
	window_state = "world_import"
	popup_ani_type = ""
	world_import_world_root = ""
	world_import_world_name = text_get("worldimportnoworld")
	world_import_dimension = "overworld"
	world_import_add_tl = addtl
	world_import_temp = temp
}

/// CppSeparate void world_import_select_world(ScopeAny, StringType, StringType dim = "", BoolType update = false)
/// Selects a world from the given folder. If no dimension is selected the current of the player is selected.
function world_import_select_world(root, dimension = "", update = false)
{
	app.world_import_world_root = root
	app.world_import_world_name = root
	app.world_import_dimension = dimension = "" ? "overworld" : dimension
}

/// Selects a dimension from the given name.
function world_import_select_dimension(name)
{
	world_import_select_world(app.world_import_world_root, name)
}

/// CppSeparate BoolType world_import_has_dimension(StringType)
/// Returns whether a dimension is available in the current world.
function world_import_has_dimension(name)
{
	return true
}

/// CppSeparate StringType world_import_choose_instance(StringType)
/// Asks for a Minecraft instance folder. Returns the folder, "<default>" for the default .minecraft, or "" when cancelled.
function world_import_choose_instance(loadedinstance)
{
	return ""
}

/// CppSeparate void world_import_set_instance(StringType)
/// Sets the Minecraft instance folder used for the world list ("" = default .minecraft).
function world_import_set_instance(folder)
{
}

/// CppSeparate void world_import_dimension_menu_add_mods()
/// Adds the mod dimensions of the current world to the menu list.
function world_import_dimension_menu_add_mods()
{
}

/// Chooses the Minecraft instance folder and stores it in the settings.
function world_import_choose_instance_folder()
{
	var folder = world_import_choose_instance(mod_assets_instance);
	if (folder = "")
		return 0

	app.setting_world_import_instance = (folder = "<default>" ? "" : folder)
	world_import_set_instance(app.setting_world_import_instance)
	settings_save()
}

/// Returns the display name of a dimension. Mod dimensions show their ID.
function world_import_dimension_name(dim)
{
	if (string_pos(":", dim) > 0)
		return dim

	return text_get("worldimport" + string_replace_all(dim, "_", ""))
}

/// CppSeparate StringType modworld_prepare_assets(StringType, StringType)
/// Builds or re-uses the cache of mod block assets, returns the manifest file.
function modworld_prepare_assets(instance, cachedir)
{
	return ""
}

/// CppSeparate void modworld_set_cache_dir(StringType)
function modworld_set_cache_dir(dir)
{
}

/// CppSeparate void modworld_set_sheet_info(IntType, IntType, IntType)
/// Block sheet size, used to validate cached scenery meshes.
function modworld_set_sheet_info(width, height, baseheight)
{
}

/// Loads the mod block manifest of the chosen Minecraft instance (on startup).
function modworld_load_manifest()
{
	var instance = app.setting_world_import_instance;
	if (!is_string(instance) || instance = "")
		return false

	if (!directory_exists_lib(instance))
	{
		log("Minecraft instance folder not found, mod blocks not loaded", instance)
		return false
	}

	var cachedir, manifest;
	cachedir = file_directory + "Minecraft_mods/"
	manifest = modworld_prepare_assets(instance, cachedir)
	if (manifest = "" || !file_exists_lib(manifest))
	{
		log("Could not prepare mod blocks", instance)
		return false
	}

	mod_assets_type_map = ds_int_map_create()
	mod_assets_map = json_load(manifest, mod_assets_type_map)
	if (!ds_map_valid(mod_assets_map) || !ds_list_valid(mod_assets_map[?"blocks"]))
	{
		log("Could not parse mod block manifest", manifest)
		if (ds_map_valid(mod_assets_map))
			ds_map_destroy(mod_assets_map)
		ds_map_destroy(mod_assets_type_map)
		mod_assets_map = null
		mod_assets_type_map = null
		return false
	}

	mod_assets_dir = cachedir
	mod_assets_instance = instance
	mod_assets_block_amount = ds_list_size(mod_assets_map[?"blocks"])
	modworld_set_cache_dir(cachedir)
	log("Mod blocks to load", mod_assets_block_amount)

	return true
}

/// Adds the textures used by mod blocks to the block sheet and grows the sheet.
function modworld_add_textures()
{
	mod_assets_base_sheet_height = minecraft_block_sheet_size[1]

	if (mod_assets_map = null)
		return 0

	var texlist, maxslots, added;
	texlist = mod_assets_map[?"block_textures"]
	// Limit default-pack atlas height to 8192 pixels at its largest mod texture size.
	var modtexsize = value_get_real(mod_assets_map[?"texture_size"], 16);
	maxslots = minecraft_block_sheet_size[0] * floor(8192 / max(16, modtexsize))
	added = 0

	if (ds_list_valid(texlist))
	{
		for (var i = 0; i < ds_list_size(texlist); i++)
		{
			var name = texlist[|i];
			if (!is_string(name))
				continue

			if (ds_list_find_index(mc_assets.block_texture_list, name) > -1 || ds_list_find_index(mc_assets.block_texture_ani_list, name) > -1)
				continue

			if (!file_exists_lib(modworld_texture_file_path(name, "")))
				continue

			if (ds_list_size(mc_assets.block_texture_list) >= maxslots)
			{
				log("Too many mod block textures, skipped", ds_list_size(texlist) - i)
				break
			}

			ds_list_add(mc_assets.block_texture_list, name)
			added++
		}
	}

	// Biome tinted mod textures
	var colormap = mod_assets_map[?"block_textures_color"];
	if (ds_map_valid(colormap))
	{
		var key = ds_map_find_first(colormap);
		while (!is_undefined(key))
		{
			if (is_undefined(mc_assets.block_texture_color_map[?key]))
				mc_assets.block_texture_color_map[?key] = colormap[?key]
			key = ds_map_find_next(colormap, key)
		}
	}

	minecraft_block_sheet_size[1] = max(minecraft_block_sheet_size[1], ceil(ds_list_size(mc_assets.block_texture_list) / minecraft_block_sheet_size[0]))
	log("Mod block textures added", added, "block sheet rows", minecraft_block_sheet_size[1])

	return added
}

/// Loads mod block number "index" from the manifest.
function modworld_load_block(index)
{
	var blocks, blockmap, block;
	blocks = mod_assets_map[?"blocks"]
	blockmap = blocks[|index]
	if (!ds_map_valid(blockmap))
		return null

	block = block_load(blockmap, mod_assets_type_map)
	if (!block)
	{
		log("Could not load mod block", blockmap[?"name"])
		return null
	}

	return block
}

/// Called when all blocks are loaded.
function modworld_load_done()
{
	var baseheight = (mod_assets_base_sheet_height > 0 ? mod_assets_base_sheet_height : minecraft_block_sheet_size[1]);
	modworld_set_sheet_info(minecraft_block_sheet_size[0], minecraft_block_sheet_size[1], baseheight)

	if (mod_assets_map != null)
	{
		ds_map_destroy(mod_assets_map)
		mod_assets_map = null
	}

	if (mod_assets_type_map != null)
	{
		ds_map_destroy(mod_assets_type_map)
		mod_assets_type_map = null
	}

	if (mod_assets_block_amount > 0)
		log("Loaded mod blocks", mod_assets_block_amount)
}

/// Returns whether a file is in the mod asset cache.
function modworld_is_mod_file(fname)
{
	return (mod_assets_dir != "" && string_pos(mod_assets_dir, fname) = 1)
}

/// Key for loaded model files. Minecraft models use the file name, mod models the full path.
function modworld_model_cache_key(fname)
{
	if (modworld_is_mod_file(fname))
		return fname

	return filename_name(fname)
}

/// Returns the file of a block state ("stone.json" or "namespace:name").
function modworld_state_file_path(filename)
{
	var c = string_pos(":", filename);
	if (c = 0 || mod_assets_dir = "")
		return load_assets_dir + mc_blockstates_directory + filename

	return mod_assets_dir + "assets/" + string_copy(filename, 1, c - 1) + "/blockstates/" + string_delete(filename, 1, c) + ".json"
}

/// Returns the file of a block model ("block/stone" or "namespace:block/name").
function modworld_model_file_path(name)
{
	var c = string_pos(":", name);
	if (c = 0 || mod_assets_dir = "")
		return load_assets_dir + mc_models_directory + name + ".json"

	name = string_lower(name)
	return mod_assets_dir + "assets/" + string_copy(name, 1, c - 1) + "/models/" + string_delete(name, 1, c) + ".json"
}

/// Returns the file of a block texture ("block/stone" or "namespace:block/name").
/// Resource packs may contain mod textures, the default pack uses the mod asset cache.
function modworld_texture_file_path(name, suffix)
{
	var c = string_pos(":", name);
	if (c = 0)
		return load_assets_dir + mc_textures_directory + name + suffix + ".png"

	var ns, path, fn;
	ns = string_copy(name, 1, c - 1)
	path = string_delete(name, 1, c)
	fn = load_assets_dir + "assets/" + ns + "/textures/" + path + suffix + ".png"

	if (!file_exists_lib(fn) && mod_assets_dir != "" && load_assets_dir = load_assets_startup_dir)
		fn = mod_assets_dir + "assets/" + ns + "/textures/" + path + suffix + ".png"

	return fn
}

/// CppSeparate BoolType world_import_has_selection()
/// Returns whether a selection has been created.
function world_import_has_selection()
{
	return true
}

/// CppSeparate VecType world_import_get_selection_size()
/// Returns the selection size.
function world_import_get_selection_size()
{
	return vec3(1, 2, 3)
}

/// CppSeparate void world_import_apply_settings(ScopeAny)
/// Applies the world settings after closing the popup.
function world_import_apply_settings()
{
	show_debug_message("Apply settings")
}

/// CppSeparate void world_import_go_to_player()
/// Jumps to the player or start position.
function world_import_go_to_player()
{
	show_debug_message("Go to player")
}

/// CppSeparate void world_import_set_selection(StringType size)
function world_import_set_selection(size)
{
	show_debug_message("Set selection to " + size)
}

/// CppSeparate StringType world_import_get_saves_dir()
function world_import_get_saves_dir()
{
	return "";
}

/// CppSeparate void world_import_confirm()
/// Confirms the selection in the world.
function world_import_confirm()
{
	show_debug_message("confirm")
}

function world_import_confirm_done()
{
	window_state = ""
}

/// CppSeparate void world_import_cancel()
/// Cancels the world import operation.
function world_import_cancel()
{
	show_debug_message("cancel")
	window_state = ""
}

/// CppSeparate void world_import_update_surface(IntType, IntType, IntType, IntType, IntType, IntType, IntType, IntType)
/// Update the world preview surface drawn at position x,y.
function world_import_update_surface(xx, yy, width, height, confirmx, confirmy, confirmwidth, confirmheight)
{
	surface_set_target(world_import_surface)
	draw_clear(c_level_middle)
	surface_reset_target()
}

/// CppSeparate void world_import_world_menu_init()
/// Add the worlds available on the system to the menu list.
function world_import_world_menu_init()
{
	menu_add_item("Some World 1", "Some World 1")
	menu_add_item("Some World 2", "Some World 2")
	menu_add_item("Some World 3", "Some World 3")
	menu_add_item("Some World 4", "Some World 4")
}

/// Add the dimensions available on the system to the menu list.
function world_import_dimension_menu_init()
{
	menu_add_item("overworld", text_get("worldimportoverworld"))
	if (world_import_has_dimension("the_nether"))
		menu_add_item("the_nether", text_get("worldimportthenether"))
	if (world_import_has_dimension("the_end"))
		menu_add_item("the_end", text_get("worldimporttheend"))
	world_import_dimension_menu_add_mods()
}

/// Draw the world import interface.
function window_draw_world_import()
{
	var spacing, capwid, hasselection, surfacey, surfaceh;
	spacing = 12
	content_x = 0
	content_y = toolbar_size
	content_width = window_width
	content_height = 40
	
	dx = content_x + 12
	dy = content_y + 8
	dw = content_width
	dh = content_height
	
	draw_clear(c_level_bottom)
	
	// Get selection status
	var confirmx, confirmy, confirmw, confirmh;
	hasselection = world_import_has_selection()
	confirmh = 64
	
	if (hasselection)
	{
		draw_set_font(font_heading_big)
		confirmw = string_width(text_get("worldimportconfirm")) + 70
	}
	else
		confirmw = 0
	
	confirmx = window_width / 2 - confirmw / 2
	confirmy = window_height - 60 - confirmh - (setting_show_shortcuts_bar * 28)
	
	// Draw surface
	surfacey = content_y + content_height
	surfaceh = window_height - surfacey
	if (setting_show_shortcuts_bar)
		surfaceh -= 28
	world_import_surface = surface_require(world_import_surface, window_width, surfaceh)
	world_import_update_surface(0, surfacey, window_width, surfaceh, confirmx, confirmy, confirmw, confirmh)
	draw_set_color(c_ltgray);
	draw_rectangle(0, surfacey, window_width, surfacey + surfaceh, false);
	draw_surface(world_import_surface, 0, surfacey)
	
	// Draw world import toolbar
	draw_box(content_x, content_y, content_width, content_height, false, c_level_middle, 1)
	draw_divide(content_x, content_y + content_height, content_width)
	draw_gradient(content_x, content_y + content_height, content_width, shadow_size, c_black, shadow_alpha, shadow_alpha, 0, 0)
	
	content_mouseon = true
	
	// Cancel
	draw_set_font(font_button)
	dw = string_width(text_get("worldimportcancel")) + 24
	if (draw_button_label("worldimportcancel", dx, content_y + 4, null, null, e_button.SECONDARY, null, e_anchor.LEFT))
		world_import_cancel()
	
	dx += dw
	
	dx += 12
	draw_divide_vertical(dx, content_y + 6, content_height - 12)
	dx += 12
	
	// World
	dw = 256
	capwid = 50
	draw_button_menu("worldimportworld", e_menu.LIST, dx, dy, dw, 24, world_import_world_root, world_import_world_name, world_import_select_world, false, null, null, "", null, null, capwid)
	
	// Dimension
	dx += dw + spacing
	dw = 208
	capwid = 80
	draw_button_menu("worldimportdimension", e_menu.LIST, dx, dy, dw, 24, world_import_dimension, world_import_dimension_name(world_import_dimension), world_import_select_dimension, false, null, null, "", null, null, capwid)
	
	dx += dw + 12
	
	// Buttons
	dw = 24
	spacing = 4
	
	if (draw_button_icon("worldimportbrowse", dx, dy, dw, dw, false, icons.FOLDER, null, false, "worldimportbrowsetip"))
	{
		var leveldat = file_dialog_open(text_get("worldimportbrowseworlds") + " (level.dat)|level.dat;", "", world_import_get_saves_dir(), text_get("worldimportbrowsecaption"))
		if (file_exists_lib(leveldat))
			world_import_select_world(filename_dir(leveldat))
	}
	
	dx += dw + spacing
	if (draw_button_icon("worldimportinstance", dx, dy, dw, dw, false, icons.FOLDER, null, false, "worldimportinstancetip"))
		world_import_choose_instance_folder()

	dx += dw + spacing
	var worldpicked = world_import_world_root != "";
	if (draw_button_icon("worldimportreload", dx, dy, dw, dw, false, icons.REFRESH, null, !worldpicked, "worldimportreloadtip"))
		world_import_select_world(world_import_world_root, world_import_dimension)
	
	dx += dw + 12
	draw_divide_vertical(dx, content_y + 6, content_height - 12)
	
	dx += 12
	if (draw_button_icon("worldimportgotoplayer", dx, dy, dw, dw, false, icons.PATH_POINT, null, !worldpicked, "worldimportgotoplayertip"))
		world_import_go_to_player()
	
	dx += dw + spacing
	if (draw_button_icon("worldimportsettings", dx, dy, dw, dw, false, icons.SETTINGS, null, false, "worldimportsettingstip"))
		popup_show(world_import_settings_popup)
	
	dx += dw
	
	dx += 12
	draw_divide_vertical(dx, content_y + 6, content_height - 12)
	dx += 12
	
	// Selection
	dy = content_y + 4
	draw_set_font(font_label)
	spacing = 12
	dw = string_width(text_get("worldimportselection") + ":")
	draw_label(text_get("worldimportselection") + ":", dx, content_y + content_height / 2, fa_left, fa_middle, c_text_secondary, a_text_secondary)
	
	dx += dw + spacing
	draw_set_font(font_button)
	dw = string_width(text_get("worldimportselectionsmall")) + 24
	if (draw_button_label("worldimportselectionsmall", dx, dy, null, null, e_button.SECONDARY, null, e_anchor.LEFT, !worldpicked))
		world_import_set_selection("small")
	
	dx += dw + spacing
	dw = string_width(text_get("worldimportselectionmedium")) + 24
	if (draw_button_label("worldimportselectionmedium", dx, dy, null, null, e_button.SECONDARY, null, e_anchor.LEFT, !worldpicked))
		world_import_set_selection("medium")
	
	dx += dw + spacing
	dw = string_width(text_get("worldimportselectionlarge")) + 24
	if (draw_button_label("worldimportselectionlarge", dx, dy, null, null, e_button.SECONDARY, null, e_anchor.LEFT, !worldpicked))
		world_import_set_selection("large")
	
	if (world_import_has_selection())
	{
		dx += dw + 20
		var size = world_import_get_selection_size();
		draw_label(text_get("worldimportblockstotal", size[X] * size[Y] * size[Z]), dx, (content_y + content_height / 2) - 8, fa_left, fa_middle, c_text_main, a_text_main, font_value)
		draw_label(text_get("worldimportblockssizes", size[X], size[Y], size[Z]), dx, (content_y + content_height / 2) + 8, fa_left, fa_middle, c_text_main, a_text_main, font_value)
	}
	
	// Draw confirm button
	content_x = 0
	content_y = 0
	content_width = window_width
	content_height = window_height
	if (hasselection)
		if (draw_button_label("worldimportconfirm", confirmx, confirmy, confirmw, null, e_button.BIG, null, e_anchor.LEFT))
			world_import_confirm()
	
	// Filter enabled
	if (worldpicked && setting_world_import_filter_enabled && ds_list_size(setting_world_import_filter_list) > 0)
	{
		var filtertext = text_get("worldimportfilteractive");
		draw_label(filtertext, confirmx + confirmw / 2 + 1, confirmy + 80 + 1, fa_center, fa_top, c_black, 1, font_heading_big)
		draw_label(filtertext, confirmx + confirmw / 2, confirmy + 80, fa_center, fa_top, c_warning, 1, font_heading_big)
	}
}

function action_world_import_settings_filter_enabled(value)
{
	setting_world_import_filter_enabled = value
}

function action_world_import_settings_filter_mode(value)
{
	setting_world_import_filter_mode = value
}

function action_world_import_settings_block_select(value)
{
	world_import_settings_block_select = value
}

function action_world_import_settings_filter_select(value)
{
	world_import_settings_filter_select = value
}

function action_world_import_settings_unload_regions(value)
{
	setting_world_import_unload_regions = value
}

function popup_worldsettings_draw()
{
	content_mouseon = true
	dw = dw/2
	
	// Unload far away regions
	tab_control_switch()
	draw_switch("worldsettingsunloadregions", dx, dy, setting_world_import_unload_regions, action_world_import_settings_unload_regions, "worldsettingsunloadregionstip")
	tab_next()
	
	// Filter settings
	tab_control_switch()
	draw_switch("worldsettingsfilterenabled", dx, dy, setting_world_import_filter_enabled, action_world_import_settings_filter_enabled)
	tab_next()
	
	if (setting_world_import_filter_enabled)
	{
		tab_control_togglebutton()
		togglebutton_add("worldsettingsfilterremove", null, 0, setting_world_import_filter_mode = 0, action_world_import_settings_filter_mode)
		togglebutton_add("worldsettingsfilterkeep", null, 1, setting_world_import_filter_mode = 1, action_world_import_settings_filter_mode)
		draw_togglebutton("worldsettingsfiltermode", dx, dy)
		tab_next()
		
		var listdw = content_width - 24;
		
		dy += 8
		
		tab_control_sortlist(10)
		sortlist_draw(world_import_settings_block_list, dx, dy, listdw / 2 - 20, tab_control_h, world_import_settings_block_select, false, text_get("worldsettingsfilterblocks"))
		sortlist_draw(world_import_settings_filter_list, dx + listdw / 2 + 20, dy, listdw / 2 - 20, tab_control_h, world_import_settings_filter_select, false, text_get("worldsettingsfilterfiltered"))
		
		if (draw_button_icon("worldsettingsfilterright", dx + listdw / 2 - 12, dy + (tab_control_h/2) - 16, 24, 24, false, icons.CHEVRON_RIGHT_TINY, null, world_import_settings_block_select = null))
		{
			sortlist_add(world_import_settings_filter_list, world_import_settings_block_select)
			sortlist_remove(world_import_settings_block_list, world_import_settings_block_select)
			sortlist_update(world_import_settings_filter_list)
			sortlist_update(world_import_settings_block_list)
			ds_list_add(setting_world_import_filter_list, world_import_settings_block_select)
			world_import_settings_block_select = null
		}
		
		if (draw_button_icon("worldsettingsfilterleft", dx + listdw / 2 - 12, dy + (tab_control_h/2) + 16, 24, 24, false, icons.CHEVRON_LEFT_TINY, null, world_import_settings_filter_select = null))
		{
			var blocklist = world_import_settings_block_list.list;
			var index;
			for (index = 0; index < ds_list_size(blocklist); index++)
				if (blocklist[|index] > world_import_settings_filter_select)
					break;
			
			sortlist_add(world_import_settings_block_list, world_import_settings_filter_select, index)
			sortlist_remove(world_import_settings_filter_list, world_import_settings_filter_select)
			sortlist_update(world_import_settings_block_list)
			sortlist_update(world_import_settings_filter_list)
			ds_list_delete_value(setting_world_import_filter_list, world_import_settings_filter_select)
			world_import_settings_filter_select = null
		}
		
		tab_next()
		
		dw = content_width - 24
		dy += 8
		draw_tooltip_label("worldsettingsfilterhelp", icons.INFO, e_toast.INFO)
	}
}