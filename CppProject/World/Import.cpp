#include "World.hpp"

#include "Generated/Scripts.hpp"

#include <QDirIterator>
#include <QStandardPaths>
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QPushButton>

namespace CppProject
{
	// Instance folder whose "saves" are listed ("" = default .minecraft).
	// Saves and mods are only read, never modified.
	static QString selectedInstance;

	static int CountModJars(const QString& instance)
	{
		const QDir mods(QDir(instance).filePath("mods"));
		if (instance.isEmpty() || !mods.exists())
			return 0;
		return mods.entryList(QStringList() << "*.jar", QDir::Files | QDir::Readable | QDir::NoDotAndDotDot, QDir::IgnoreCase).size();
	}

	void world_import_set_instance(StringType folder)
	{
		selectedInstance = folder.QStr();
	}

	StringType world_import_choose_instance(StringType loadedInstance)
	{
		const QString loaded = loadedInstance.QStr();
		QMessageBox box;
		box.setWindowTitle("Minecraft instance");
		box.setIcon(QMessageBox::Question);
		box.setText("Choose the game/instance folder of a modded Minecraft Java installation (1.20.1 - 26.2). "
			"Its worlds are listed in the world importer and the blocks of its mods are loaded when Mine-imator starts.");
		box.setInformativeText(QString("Current folder: %1\nMod blocks loaded now: %2")
			.arg(selectedInstance.isEmpty() ? QString("default .minecraft") : selectedInstance)
			.arg(loaded.isEmpty() ? QString("none") : loaded));
		QPushButton* chooseButton = box.addButton("Choose folder...", QMessageBox::AcceptRole);
		QPushButton* defaultButton = selectedInstance.isEmpty() ? nullptr : box.addButton("Use default .minecraft", QMessageBox::ResetRole);
		box.addButton(QMessageBox::Cancel);
		box.exec();

		QString result;
		if (box.clickedButton() == chooseButton)
		{
			const QString folder = QFileDialog::getExistingDirectory(nullptr, "Select Minecraft instance folder",
				selectedInstance, QFileDialog::ShowDirsOnly);
			if (folder.isEmpty())
				return "";

			QDir instance(folder);
			if (!QDir(instance.filePath("saves")).exists())
			{
				// Accept the saves folder itself or a world folder selected by mistake
				BoolType found = false;
				if (QFileInfo::exists(instance.filePath("level.dat")))
					instance.cdUp();
				if (instance.dirName() == "saves" && instance.cdUp())
					found = QDir(instance.filePath("saves")).exists();

				if (!found)
				{
					QMessageBox::warning(nullptr, "Minecraft instance",
						"This folder has no \"saves\" subfolder. Select the game/instance folder that contains \"saves\" and \"mods\".");
					return "";
				}
			}
			result = QDir::cleanPath(instance.absolutePath());
		}
		else if (defaultButton && box.clickedButton() == defaultButton)
			result = "";
		else
			return "";

		selectedInstance = result;

		if (result != loaded)
		{
			const int jars = CountModJars(result);
			QString message = "Worlds from the selected folder are listed now.";
			if (jars > 0)
				message += QString("\n\nMod blocks are loaded when Mine-imator starts. Restart Mine-imator to load the blocks of %1 mod file(s). "
					"The first start after a change can take a while.").arg(jars);
			else if (!loaded.isEmpty())
				message += "\n\nRestart Mine-imator to unload the mod blocks of the previous folder.";
			QMessageBox::information(nullptr, "Minecraft instance", message);
		}

		return result.isEmpty() ? StringType("<default>") : StringType(result);
	}

	void modworld_set_cache_dir(StringType dir)
	{
		World::modCacheDir = dir.QStr();
	}

	void world_import_dimension_menu_add_mods()
	{
		if (!World::saves.contains(global::_app->world_import_world_root))
			return;

		QStringList dims = World::saves[global::_app->world_import_world_root].dimDir.keys();
		dims.sort();
		for (const QString& dim : dims)
			if (dim.contains(':'))
				menu_add_item({ StringType(dim), StringType(dim) });
	}

	void world_import_startup()
	{
		World::Init();
	}

	void world_import_world_menu_init()
	{
		// Get Minecraft saves
		World::saves.clear();

		// List all worlds
		QDirIterator it(world_import_get_saves_dir(), QDir::Dirs | QDir::NoDotAndDotDot);
		while (it.hasNext())
		{
			QDir saveDir = it.next();
			StringType saveRoot = saveDir.path();

			// Add save to menu if valid
			if (World::AddSave(saveRoot))
				menu_add_item({ saveRoot, StringType(World::saves[saveRoot].name) });
		}
	}

	void world_import_select_world(ScopeAny, StringType root, StringType dim, BoolType update)
	{
		if (root == "")
			return;

		if (!World::saves.contains(root) && !World::AddSave(root))
		{
			error("worldimporterror");
			return;
		}

		SaveInfo& save = World::saves[root];
		StringType saveDim = dim.IsEmpty() ? StringType(save.playerDim) : dim;
		if (!save.dimDir.contains(saveDim.QStr())) // Missing (mod) dimension
			saveDim = "overworld";
		global::_app->world_import_world_root = root;
		global::_app->world_import_world_name = save.name;
		global::_app->world_import_dimension = saveDim;

		// Open world once currently loaded regions are done
		World::preview->resetUpdate = update;
		Region::loader->active = false;
		QObject::connect(Region::loader, SIGNAL(UpdateDone()), World::preview, SLOT(Reset()));
	}

	BoolType world_import_has_dimension(StringType dim)
	{
		return World::saves[global::_app->world_import_world_root].dimDir.contains(dim);
	}

	BoolType world_import_has_selection()
	{
		return World::preview->selection.active;
	}

	VecType world_import_get_selection_size()
	{
		return World::preview->selection.GetSize();
	}

	void world_import_apply_settings(ScopeAny)
	{
		if (World::ApplyFilter()) // Reload world if needed (keep position)
			world_import_select_world(ScopeAny(0), global::_app->world_import_world_root, global::_app->world_import_dimension, true);
	}

	void world_import_go_to_player()
	{
		World::preview->GoToPlayer();
	}

	void world_import_set_selection(StringType size)
	{
		if (size == "small")
			World::preview->SetSelectionSize({ 50, 20, 50 });
		else if (size == "medium")
			World::preview->SetSelectionSize({ 200, 30, 200 });
		else
			World::preview->SetSelectionSize({ 400, 50, 400 });
	}

	StringType world_import_get_saves_dir()
	{
		if (!selectedInstance.isEmpty())
			return QDir(selectedInstance).filePath("saves");
	#if OS_WINDOWS
		return QFileInfo(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).path() + "/.minecraft/saves";
	#elif OS_MAC
		return QDir::homePath() + "/Library/Application Support/minecraft/saves";
	#else
		return QDir::homePath() + "/.minecraft/saves";
	#endif
	}

	void world_import_confirm()
	{
		// Confirm when region loading is done
		if (World::preview->selection.active && Region::loader->active)
		{
			Region::loader->active = false;
			QObject::connect(Region::loader, SIGNAL(UpdateDone()), World::preview, SLOT(Confirm()));
		}
	}

	void world_import_cancel()
	{
		// Unload world when region loading is done
		if (global::_app->world_import_world_root != "" && Region::loader->active)
		{
			Region::loader->active = false;
			QObject::connect(Region::loader, SIGNAL(UpdateDone()), World::preview, SLOT(Cancel()));
		}

		global::_app->window_state = "";
	}

	void world_import_update_surface(IntType x, IntType y, IntType width, IntType height, IntType confirmX, IntType confirmY, IntType confirmWidth, IntType confirmHeight)
	{
		World::preview->Update(FindSurface(global::_app->world_import_surface), QRect(x, y, width, height), QRect(confirmX, confirmY, confirmWidth, confirmHeight));
	}
}
