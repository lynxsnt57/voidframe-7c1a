// Mod asset cache for importing modded Java worlds (Minecraft Java 1.20.1 - 26.2).
//
// Reads block assets from the JAR files in "<instance>/mods" and writes a
// normalized copy to Mine-imator's own cache folder:
//   <cache>/assets/<namespace>/blockstates/*.json
//   <cache>/assets/<namespace>/models/**.json
//   <cache>/assets/<namespace>/textures/**.png   (square atlas textures, up to 128x128)
//   <cache>/modworld.json                         (manifest read on startup)
//   <cache>/report.txt                            (human readable report)
//
// The instance, its worlds and its mods are only read. No code from a mod is
// executed; only JSON and PNG files are parsed.

#include "Generated/Scripts.hpp"
#include "FurnitureCache.hpp"

// The project links libzip statically (as in Library/FileLib.cpp).
#define ZIP_STATIC
#include <zip.h>

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QHash>
#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

#include <functional>

#define MODWORLD_CACHE_FORMAT 5
#define MODWORLD_MAX_JSON_SIZE (4 * 1024 * 1024)
#define MODWORLD_MAX_PNG_SIZE (16 * 1024 * 1024)
#define MODWORLD_MAX_NESTED_JAR_SIZE (64 * 1024 * 1024)
#define MODWORLD_MAX_ENTRIES 200000
#define MODWORLD_MAX_STATE_IDS 16384
#define MODWORLD_TEXTURE_SIZE 16
#define MODWORLD_TEXTURE_MAX_SIZE 128
#define MODWORLD_PLACEHOLDER_NS "mineimator_missing"
#define MODWORLD_PLACEHOLDER_ID "mineimator_missing:unknown_block"

namespace CppProject
{
	namespace
	{
		struct ArchiveRef
		{
			QString jarPath;	// Top-level JAR on disk
			QString nestedName;	// Entry name of a nested JAR, or empty
			QString modFile;	// File name used in the report
		};

		struct TextureRef
		{
			int archive = -1;
			QString entry;
		};

		struct ModBlock
		{
			QString id;
			QStringList stateOrder;
			QHash<QString, QStringList> states;
			QStringList models;
			QJsonObject json;
		};

		const QRegularExpression& NamespaceRegex()
		{
			static const QRegularExpression re("^[a-z0-9_.\\-]+$");
			return re;
		}

		const QRegularExpression& PathRegex()
		{
			static const QRegularExpression re("^[a-z0-9_.\\-/]+$");
			return re;
		}

		BoolType ValidPath(const QString& path)
		{
			if (path.isEmpty() || path.startsWith('/') || path.endsWith('/') || !PathRegex().match(path).hasMatch())
				return false;
			for (const QString& part : path.split('/'))
				if (part.isEmpty() || part == "." || part == "..")
					return false;
			return true;
		}

		// Removes // and /* */ comments and trailing commas outside strings.
		// Several mods ship slightly lenient JSON that Minecraft accepts.
		QByteArray SanitizeJson(const QByteArray& in)
		{
			QByteArray out;
			out.reserve(in.size());
			BoolType inString = false, escape = false;
			for (int i = 0; i < in.size(); i++)
			{
				char c = in[i];
				if (inString)
				{
					out += c;
					if (escape) escape = false;
					else if (c == '\\') escape = true;
					else if (c == '"') inString = false;
					continue;
				}
				if (c == '"') { inString = true; out += c; continue; }
				if (c == '/' && i + 1 < in.size() && in[i + 1] == '/')
				{
					while (i < in.size() && in[i] != '\n') i++;
					out += '\n';
					continue;
				}
				if (c == '/' && i + 1 < in.size() && in[i + 1] == '*')
				{
					i += 2;
					while (i + 1 < in.size() && !(in[i] == '*' && in[i + 1] == '/')) i++;
					i++;
					continue;
				}
				if (c == ']' || c == '}')
				{
					int j = out.size() - 1;
					while (j >= 0 && (out[j] == ' ' || out[j] == '\t' || out[j] == '\r' || out[j] == '\n')) j--;
					if (j >= 0 && out[j] == ',')
						out.remove(j, 1);
				}
				out += c;
			}
			return out;
		}

		BoolType ParseJsonObject(const QByteArray& data, QJsonObject& out)
		{
			QByteArray bytes = data;
			if (bytes.startsWith("\xEF\xBB\xBF")) // UTF-8 BOM
				bytes.remove(0, 3);
			QJsonParseError err;
			QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
			if (err.error != QJsonParseError::NoError)
				doc = QJsonDocument::fromJson(SanitizeJson(bytes), &err);
			if (err.error != QJsonParseError::NoError || !doc.isObject())
				return false;
			out = doc.object();
			return true;
		}

		BoolType ReadEntry(zip_t* archive, zip_uint64_t index, zip_uint64_t maxSize, QByteArray& out)
		{
			zip_stat_t st;
			zip_stat_init(&st);
			if (zip_stat_index(archive, index, 0, &st) != 0 || !(st.valid & ZIP_STAT_SIZE) || st.size > maxSize)
				return false;
			zip_file_t* file = zip_fopen_index(archive, index, 0);
			if (!file)
				return false;
			out.resize(static_cast<int>(st.size));
			zip_uint64_t pos = 0;
			while (pos < st.size)
			{
				zip_int64_t n = zip_fread(file, out.data() + pos, st.size - pos);
				if (n <= 0)
				{
					zip_fclose(file);
					return false;
				}
				pos += static_cast<zip_uint64_t>(n);
			}
			zip_fclose(file);
			return true;
		}

		// Opens a top-level JAR or a JAR nested inside it. "holder" keeps the
		// nested archive data alive while the returned archive is open.
		zip_t* OpenArchive(const ArchiveRef& ref, QByteArray& holder)
		{
			int err = 0;
			zip_t* outer = zip_open(ref.jarPath.toUtf8().constData(), ZIP_RDONLY, &err);
			if (!outer || ref.nestedName.isEmpty())
				return outer;

			zip_int64_t index = zip_name_locate(outer, ref.nestedName.toUtf8().constData(), 0);
			BoolType ok = (index >= 0 && ReadEntry(outer, static_cast<zip_uint64_t>(index), MODWORLD_MAX_NESTED_JAR_SIZE, holder));
			zip_close(outer);
			if (!ok)
				return nullptr;

			zip_error_t zerr;
			zip_error_init(&zerr);
			zip_source_t* src = zip_source_buffer_create(holder.constData(), holder.size(), 0, &zerr);
			if (!src)
			{
				zip_error_fini(&zerr);
				return nullptr;
			}
			zip_t* nested = zip_open_from_source(src, ZIP_RDONLY, &zerr);
			if (!nested)
				zip_source_free(src);
			zip_error_fini(&zerr);
			return nested;
		}

		QString JsonValueToState(const QJsonValue& value)
		{
			if (value.isBool())
				return value.toBool() ? "true" : "false";
			if (value.isDouble())
			{
				double d = value.toDouble();
				if (d == (double)(qint64)d)
					return QString::number((qint64)d);
				return QString::number(d);
			}
			return value.toString();
		}

		void AddStateValue(ModBlock& block, const QString& name, const QString& value)
		{
			QString n = name.trimmed(), v = value.trimmed();
			if (n.isEmpty() || v.isEmpty())
				return;
			if (!block.states.contains(n))
				block.stateOrder.append(n);
			QStringList& values = block.states[n];
			if (!values.contains(v))
				values.append(v);
		}

		void CollectWhen(ModBlock& block, const QJsonObject& when)
		{
			for (auto it = when.begin(); it != when.end(); ++it)
			{
				if ((it.key() == "OR" || it.key() == "AND") && it.value().isArray())
				{
					for (const QJsonValue& sub : it.value().toArray())
						if (sub.isObject())
							CollectWhen(block, sub.toObject());
					continue;
				}
				for (const QString& v : JsonValueToState(it.value()).split('|'))
					AddStateValue(block, it.key(), v);
			}
		}

		// Multipart conditions may use JSON numbers/booleans, Mine-imator expects strings
		QJsonObject NormalizeWhen(const QJsonObject& when)
		{
			QJsonObject out;
			for (auto it = when.begin(); it != when.end(); ++it)
			{
				if ((it.key() == "OR" || it.key() == "AND") && it.value().isArray())
				{
					QJsonArray list;
					for (const QJsonValue& sub : it.value().toArray())
						if (sub.isObject())
							list.append(NormalizeWhen(sub.toObject()));
					out[it.key()] = list;
				}
				else
					out[it.key()] = JsonValueToState(it.value());
			}
			return out;
		}

		void CollectModels(ModBlock& block, const QJsonValue& value)
		{
			if (value.isArray())
			{
				for (const QJsonValue& v : value.toArray())
					CollectModels(block, v);
				return;
			}
			if (value.isObject())
			{
				QString model = value.toObject().value("model").toString();
				if (!model.isEmpty() && !block.models.contains(model))
					block.models.append(model);
			}
		}

		// "stone" -> "minecraft:stone", lower case
		QString NormalizeId(QString id)
		{
			id = id.trimmed().toLower();
			if (!id.contains(':'))
				id = "minecraft:" + id;
			return id;
		}

		// Texture name as used by Mine-imator: vanilla names have no namespace
		QString TextureListName(const QString& id)
		{
			QString n = NormalizeId(id);
			if (n.startsWith("minecraft:"))
				return n.mid(10);
			return n;
		}

		QString TextureValue(const QJsonValue& value)
		{
			if (value.isObject())
				return value.toObject().value("sprite").toString();
			return value.toString();
		}

		BoolType WriteFile(const QString& path, const QByteArray& data)
		{
			QDir().mkpath(QFileInfo(path).absolutePath());
			QFile file(path);
			if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
				return false;
			BoolType ok = (file.write(data) == data.size());
			file.close();
			return ok;
		}

		// Static Minecraft sprites can be rectangular: normalize the whole image
		// to a square atlas slot, preserving normalized UVs. Only animated sprites
		// use their first frame; do not crop ordinary rectangular textures.
		QImage NormalizeTexture(const QImage& source, const QJsonObject& metadata)
		{
			QImage img = source.convertToFormat(QImage::Format_ARGB32);
			if (img.isNull())
				return QImage();

			if (metadata.value("animation").isObject())
			{
				QJsonObject animation = metadata.value("animation").toObject();
				int fw = animation.value("width").toInt(0);
				int fh = animation.value("height").toInt(0);
				if (fw <= 0 && fh <= 0)
					fw = fh = qMin(img.width(), img.height());
				else
				{
					if (fw <= 0) fw = img.width();
					if (fh <= 0) fh = img.height();
				}
				if (fw <= 0 || fh <= 0 || fw > img.width() || fh > img.height() || img.width() % fw || img.height() % fh)
					return QImage();
				int frame = 0;
				QJsonArray frames = animation.value("frames").toArray();
				if (!frames.isEmpty())
				{
					QJsonValue first = frames.at(0);
					frame = first.isObject() ? first.toObject().value("index").toInt(0) : first.toInt(0);
				}
				int columns = img.width() / fw, rows = img.height() / fh;
				if (frame < 0 || frame >= columns * rows) frame = 0;
				img = img.copy((frame % columns) * fw, (frame / columns) * fh, fw, fh);
			}

			const int side = qMin(MODWORLD_TEXTURE_MAX_SIZE, qMax(img.width(), img.height()));
			if (side <= 0)
				return QImage();
			if (img.width() != side || img.height() != side)
				img = img.scaled(side, side, Qt::IgnoreAspectRatio, Qt::FastTransformation);
			return img;
		}

		QString TintCategory(const QString& texture)
		{
			QString t = texture.toLower();
			if (t.contains("leaves") || t.contains("leaf") || t.contains("foliage") || t.contains("vine"))
				return "foliage";
			if (t.contains("grass") || t.contains("fern"))
				return "grass";
			return "";
		}
	}

	// Prepares (or re-uses) the mod asset cache for the given instance folder.
	// Returns the path of the manifest, or "" when nothing could be prepared.
	StringType modworld_prepare_assets(StringType instanceDir, StringType cacheDir)
	{
		const QString instancePath = QDir(instanceDir.QStr()).absolutePath();
		const QDir modsDir(instancePath + "/mods");
		const QString cachePath = QDir(cacheDir.QStr()).absolutePath();
		const QString manifestPath = cachePath + "/modworld.json";
		const QString signaturePath = cachePath + "/signature.txt";
		const QString reportPath = cachePath + "/report.txt";
		QDir().mkpath(cachePath);

		if (instanceDir.IsEmpty() || !QDir(instancePath).exists())
			return "";

		// Signature of the installed mods, used to re-use the cache
		QFileInfoList jars = modsDir.exists()
			? modsDir.entryInfoList(QStringList() << "*.jar", QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase)
			: QFileInfoList();
		QByteArray signature = "modworld " + QByteArray::number(MODWORLD_CACHE_FORMAT) + "\n" + instancePath.toUtf8() + "\n";
		for (const QFileInfo& jar : jars)
			signature += jar.fileName().toUtf8() + "|" + QByteArray::number(jar.size()) + "|" +
				QByteArray::number(jar.lastModified().toMSecsSinceEpoch()) + "\n";
		signature += FurnitureSignature(instancePath);
		const QByteArray signatureHash = QCryptographicHash::hash(signature, QCryptographicHash::Sha1).toHex();

		{
			QFile sigFile(signaturePath);
			if (QFileInfo::exists(manifestPath) && sigFile.open(QIODevice::ReadOnly) && sigFile.readAll().trimmed() == signatureHash)
			{
				DEBUG("Mod asset cache is up to date: " + cachePath);
				return manifestPath;
			}
		}

		DEBUG("Building mod asset cache for " + instancePath + " (" + NumStr(jars.size()) + " JAR files)");
		Timer timer;

		// Remove only files created by this cache
		QDir(cachePath + "/assets").removeRecursively();
		QFile::remove(manifestPath);
		QFile::remove(signaturePath);
		QFile::remove(reportPath);

		QStringList report;
		QStringList archiveErrors;
		QVector<ArchiveRef> archives;
		QHash<QString, QByteArray> blockstateData, modelData; // "ns:path" -> JSON
		QHash<QString, TextureRef> textureRefs; // "ns:path" -> entry
		QSet<QString> namespaces;

		// Pass 1: index every archive and read the JSON files
		auto scanArchive = [&](const ArchiveRef& ref, auto& scanSelf, int depth) -> void
		{
			QByteArray holder;
			zip_t* archive = OpenArchive(ref, holder);
			if (!archive)
			{
				archiveErrors.append(ref.modFile + (ref.nestedName.isEmpty() ? "" : " -> " + ref.nestedName) + ": cannot open");
				return;
			}
			const int archiveIndex = archives.size();
			archives.append(ref);

			zip_int64_t count = zip_get_num_entries(archive, 0);
			if (count < 0 || count > MODWORLD_MAX_ENTRIES)
			{
				archiveErrors.append(ref.modFile + ": invalid or excessive number of entries");
				zip_close(archive);
				return;
			}

			QStringList nestedJars;
			for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(count); i++)
			{
				const char* rawName = zip_get_name(archive, i, 0);
				if (!rawName)
					continue;
				const QString name = QString::fromUtf8(rawName);

				// Jar-in-jar (Fabric: META-INF/jars, Forge/NeoForge: META-INF/jarjar)
				if (depth == 0 && (name.startsWith("META-INF/jars/") || name.startsWith("META-INF/jarjar/")) && name.endsWith(".jar", Qt::CaseInsensitive))
				{
					nestedJars.append(name);
					continue;
				}

				if (!name.startsWith("assets/") || name.endsWith('/'))
					continue;

				const QStringList parts = name.split('/');
				if (parts.size() < 4)
					continue;
				const QString ns = parts[1];
				const QString type = parts[2];
				if (ns == "minecraft" || !NamespaceRegex().match(ns).hasMatch())
					continue;
				const QString rest = QStringList(parts.mid(3)).join('/');

				if (type == "blockstates" && rest.endsWith(".json"))
				{
					QString path = rest.left(rest.size() - 5);
					if (!ValidPath(path) || path.contains('/'))
						continue;
					QString id = ns + ":" + path;
					if (blockstateData.contains(id))
						continue;
					QByteArray data;
					if (ReadEntry(archive, i, MODWORLD_MAX_JSON_SIZE, data))
					{
						blockstateData[id] = data;
						namespaces.insert(ns);
					}
				}
				else if (type == "models" && rest.endsWith(".json"))
				{
					QString path = rest.left(rest.size() - 5);
					if (!ValidPath(path))
						continue;
					QString id = ns + ":" + path;
					if (modelData.contains(id))
						continue;
					QByteArray data;
					if (ReadEntry(archive, i, MODWORLD_MAX_JSON_SIZE, data))
						modelData[id] = data;
				}
				else if (type == "textures" && rest.endsWith(".png"))
				{
					QString path = rest.left(rest.size() - 4);
					if (!ValidPath(path))
						continue;
					QString id = ns + ":" + path;
					if (!textureRefs.contains(id))
					{
						TextureRef ref;
						ref.archive = archiveIndex;
						ref.entry = name;
						textureRefs[id] = ref;
					}
				}
			}
			zip_close(archive);

			for (const QString& nested : nestedJars)
			{
				ArchiveRef nestedRef;
				nestedRef.jarPath = ref.jarPath;
				nestedRef.nestedName = nested;
				nestedRef.modFile = ref.modFile;
				scanSelf(nestedRef, scanSelf, depth + 1);
			}
		};

		for (const QFileInfo& jar : jars)
		{
			ArchiveRef ref;
			ref.jarPath = jar.absoluteFilePath();
			ref.modFile = jar.fileName();
			scanArchive(ref, scanArchive, 0);
		}

		// Parse block states
		QList<ModBlock> blocks;
		QStringList skippedBlocks;
		QStringList blockIds = blockstateData.keys();
		blockIds.sort();
		for (const QString& id : blockIds)
		{
			ModBlock block;
			block.id = id;
			if (!ParseJsonObject(blockstateData[id], block.json))
			{
				skippedBlocks.append(id + ": invalid JSON");
				continue;
			}

			QJsonValue variants = block.json.value("variants");
			QJsonValue multipart = block.json.value("multipart");
			if (variants.isObject())
			{
				QJsonObject obj = variants.toObject();
				for (auto it = obj.begin(); it != obj.end(); ++it)
				{
					if (it.key() != "" && it.key() != "normal")
						for (const QString& pair : it.key().split(','))
						{
							int eq = pair.indexOf('=');
							if (eq > 0)
								AddStateValue(block, pair.left(eq), pair.mid(eq + 1));
						}
					CollectModels(block, it.value());
				}
			}
			else if (multipart.isArray())
			{
				QJsonArray cases;
				for (const QJsonValue& c : multipart.toArray())
				{
					QJsonObject mcase = c.toObject();
					if (mcase.value("when").isObject())
					{
						mcase["when"] = NormalizeWhen(mcase.value("when").toObject());
						CollectWhen(block, mcase.value("when").toObject());
					}
					CollectModels(block, mcase.value("apply"));
					cases.append(mcase);
				}
				block.json["multipart"] = cases;
			}
			else
			{
				skippedBlocks.append(id + ": no variants or multipart (custom loader)");
				continue;
			}

			if (block.models.isEmpty())
			{
				skippedBlocks.append(id + ": no models");
				continue;
			}

			// Multipart often mentions only the true value of a boolean property.
			// Include false as well so absent connections are not treated as true.
			for (const QString& s : block.stateOrder)
			{
				QStringList& values = block.states[s];
				if (values.contains("true") && !values.contains("false"))
					values.prepend("false");
				else if (values.contains("false") && !values.contains("true"))
					values.append("true");
			}

			qint64 stateIds = 1;
			for (const QString& s : block.stateOrder)
			{
				stateIds *= qMax(1, block.states[s].size());
				if (stateIds > MODWORLD_MAX_STATE_IDS)
					break;
			}
			if (stateIds > MODWORLD_MAX_STATE_IDS)
			{
				skippedBlocks.append(id + ": too many block state combinations (" + QString::number(stateIds) + ")");
				continue;
			}

			blocks.append(block);
		}

		// Resolve models and the textures they use
		QHash<QString, QJsonObject> parsedModels;
		QSet<QString> visitedModels, missingModels, invalidModels;
		QSet<QString> usedTextures; // Mine-imator texture list names
		std::function<void(const QString&, int)> resolveModel = [&](const QString& rawId, int depth)
		{
			QString id = NormalizeId(rawId);
			if (depth > 32 || id.startsWith("minecraft:") || visitedModels.contains(id))
				return;
			visitedModels.insert(id);
			if (!modelData.contains(id))
			{
				missingModels.insert(id);
				return;
			}
			QJsonObject model;
			if (!ParseJsonObject(modelData[id], model))
			{
				invalidModels.insert(id);
				return;
			}
			parsedModels[id] = model;

			QJsonValue textures = model.value("textures");
			auto addTexture = [&](const QJsonValue& v)
			{
				QString t = TextureValue(v);
				if (!t.isEmpty() && !t.startsWith('#'))
					usedTextures.insert(TextureListName(t));
			};
			if (textures.isObject())
			{
				QJsonObject obj = textures.toObject();
				for (auto it = obj.begin(); it != obj.end(); ++it)
					addTexture(it.value());
			}
			else if (textures.isArray())
				for (const QJsonValue& v : textures.toArray())
					addTexture(v);

			if (model.value("parent").isString())
				resolveModel(model.value("parent").toString(), depth + 1);
		};

		for (const ModBlock& block : blocks)
			for (const QString& m : block.models)
				resolveModel(m, 0);

		// Biome tint guess for faces with a tint index
		QJsonObject textureColors;
		auto tintForModel = [&](const QString& rawId)
		{
			QString id = NormalizeId(rawId);
			QJsonObject merged;
			QJsonArray elements;
			BoolType hasElements = false;
			QString cur = id;
			for (int d = 0; d < 32 && parsedModels.contains(cur); d++)
			{
				const QJsonObject& m = parsedModels[cur];
				QJsonObject tex = m.value("textures").toObject();
				for (auto it = tex.begin(); it != tex.end(); ++it)
					if (!merged.contains(it.key()))
						merged[it.key()] = it.value();
				if (!hasElements && m.value("elements").isArray())
				{
					elements = m.value("elements").toArray();
					hasElements = true;
				}
				if (!m.value("parent").isString())
					break;
				cur = NormalizeId(m.value("parent").toString());
			}
			for (const QJsonValue& e : elements)
			{
				QJsonObject faces = e.toObject().value("faces").toObject();
				for (auto it = faces.begin(); it != faces.end(); ++it)
				{
					QJsonObject face = it.value().toObject();
					if (!face.contains("tintindex"))
						continue;
					QString ref = face.value("texture").toString();
					for (int i = 0; i < 8 && ref.startsWith('#'); i++)
						ref = TextureValue(merged.value(ref.mid(1)));
					if (ref.isEmpty() || ref.startsWith('#'))
						continue;
					QString name = TextureListName(ref);
					if (name.contains(':'))
					{
						QString category = TintCategory(name);
						if (!category.isEmpty())
							textureColors[name] = category;
					}
				}
			}
		};
		for (const ModBlock& block : blocks)
			for (const QString& m : block.models)
				tintForModel(m);

		QJsonArray furnitureBlocks;
		QSet<QString> furnitureTextures, furnitureSprites;
		QStringList furnitureReport;
		PrepareFurniture(instancePath, cachePath, furnitureBlocks, furnitureTextures, furnitureSprites, furnitureReport);
		usedTextures.unite(furnitureSprites);

		// Pass 2: extract and normalize the used mod textures
		QHash<int, QStringList> texturesByArchive;
		QStringList missingTextures;
		for (const QString& name : usedTextures)
		{
			if (!name.contains(':'))
				continue; // Vanilla texture
			if (!textureRefs.contains(name))
			{
				missingTextures.append(name);
				continue;
			}
			texturesByArchive[textureRefs[name].archive].append(name);
		}

		QSet<QString> writtenTextures = furnitureTextures;
		int maxTextureSize = furnitureTextures.isEmpty() ? MODWORLD_TEXTURE_SIZE : MODWORLD_TEXTURE_MAX_SIZE;
		for (auto it = texturesByArchive.begin(); it != texturesByArchive.end(); ++it)
		{
			QByteArray holder;
			zip_t* archive = OpenArchive(archives[it.key()], holder);
			if (!archive)
				continue;
			for (const QString& name : it.value())
			{
				const TextureRef& ref = textureRefs[name];
				zip_int64_t index = zip_name_locate(archive, ref.entry.toUtf8().constData(), 0);
				QByteArray data;
				QImage img;
				QJsonObject metadata;
				zip_int64_t metaIndex = zip_name_locate(archive, (ref.entry + ".mcmeta").toUtf8().constData(), 0);
				QByteArray metaBytes;
				if (metaIndex >= 0 && ReadEntry(archive, static_cast<zip_uint64_t>(metaIndex), MODWORLD_MAX_JSON_SIZE, metaBytes))
					ParseJsonObject(metaBytes, metadata);
				if (index < 0 || !ReadEntry(archive, static_cast<zip_uint64_t>(index), MODWORLD_MAX_PNG_SIZE, data) ||
					!img.loadFromData(data, "PNG") || (img = NormalizeTexture(img, metadata)).isNull())
				{
					missingTextures.append(name + " (unreadable)");
					continue;
				}
				int colon = name.indexOf(':');
				QString out = cachePath + "/assets/" + name.left(colon) + "/textures/" + name.mid(colon + 1) + ".png";
				QDir().mkpath(QFileInfo(out).absolutePath());
				if (img.save(out, "PNG"))
				{
					writtenTextures.insert(name);
					maxTextureSize = qMax(maxTextureSize, img.width());
				}
			}
			zip_close(archive);
		}

		// Write block states and models
		auto pathFor = [&](const QString& id, const QString& type)
		{
			int colon = id.indexOf(':');
			return cachePath + "/assets/" + id.left(colon) + "/" + type + "/" + id.mid(colon + 1) + ".json";
		};

		for (auto it = parsedModels.begin(); it != parsedModels.end(); ++it)
			WriteFile(pathFor(it.key(), "models"), QJsonDocument(it.value()).toJson(QJsonDocument::Compact));

		QJsonArray blockArray = furnitureBlocks;
		for (const ModBlock& block : blocks)
		{
			if (!WriteFile(pathFor(block.id, "blockstates"), QJsonDocument(block.json).toJson(QJsonDocument::Compact)))
			{
				skippedBlocks.append(block.id + ": cannot write cache file");
				continue;
			}
			QJsonObject entry;
			entry["name"] = block.id;
			entry["id"] = block.id;
			entry["file"] = block.id;
			if (!block.stateOrder.isEmpty())
			{
				QJsonObject states;
				for (const QString& s : block.stateOrder)
					states[s] = QJsonArray::fromStringList(block.states[s]);
				entry["states"] = states;
			}
			blockArray.append(entry);
		}

		// Placeholder for blocks without usable assets
		{
			QImage missing(MODWORLD_TEXTURE_SIZE, MODWORLD_TEXTURE_SIZE, QImage::Format_ARGB32);
			for (int y = 0; y < MODWORLD_TEXTURE_SIZE; y++)
				for (int x = 0; x < MODWORLD_TEXTURE_SIZE; x++)
					missing.setPixel(x, y, ((x / 8 + y / 8) % 2) ? qRgb(248, 0, 248) : qRgb(16, 16, 16));
			QString ns = MODWORLD_PLACEHOLDER_NS;
			QDir().mkpath(cachePath + "/assets/" + ns + "/textures/block");
			missing.save(cachePath + "/assets/" + ns + "/textures/block/unknown_block.png", "PNG");
			WriteFile(cachePath + "/assets/" + ns + "/models/block/unknown_block.json",
				"{\"parent\":\"minecraft:block/cube_all\",\"textures\":{\"all\":\"" MODWORLD_PLACEHOLDER_NS ":block/unknown_block\"}}");
			WriteFile(cachePath + "/assets/" + ns + "/blockstates/unknown_block.json",
				"{\"variants\":{\"\":{\"model\":\"" MODWORLD_PLACEHOLDER_NS ":block/unknown_block\"}}}");
			QJsonObject entry;
			entry["name"] = MODWORLD_PLACEHOLDER_ID;
			entry["id"] = MODWORLD_PLACEHOLDER_ID;
			entry["file"] = MODWORLD_PLACEHOLDER_ID;
			blockArray.append(entry);
			writtenTextures.insert(MODWORLD_PLACEHOLDER_NS ":block/unknown_block");
		}

		// Texture list: mod textures that exist + vanilla textures referenced by mod models
		QStringList textureList;
		for (const QString& name : usedTextures)
			if (!name.contains(':'))
				textureList.append(name);
		for (const QString& name : writtenTextures)
			textureList.append(name);
		textureList.sort();
		// Always reserve a slot for the missing-block placeholder.
		textureList.removeAll(MODWORLD_PLACEHOLDER_NS ":block/unknown_block");
		textureList.prepend(MODWORLD_PLACEHOLDER_NS ":block/unknown_block");

		QJsonObject manifest;
		manifest["format"] = MODWORLD_CACHE_FORMAT;
		manifest["instance"] = instancePath;
		manifest["mods"] = jars.size();
		manifest["texture_size"] = maxTextureSize;
		manifest["placeholder"] = MODWORLD_PLACEHOLDER_ID;
		manifest["blocks"] = blockArray;
		manifest["block_textures"] = QJsonArray::fromStringList(textureList);
		manifest["block_textures_color"] = textureColors;

		// Report
		report << "Mine-imator mod asset cache";
		report << "Instance: " + instancePath;
		report << "Mod JAR files: " + QString::number(jars.size()) + " (archives incl. nested: " + QString::number(archives.size()) + ")";
		report << "Namespaces with block states: " + QString::number(namespaces.size());
		report << "Blocks prepared: " + QString::number(blocks.size());
		report << "Blocks skipped: " + QString::number(skippedBlocks.size());
		report << "Models prepared: " + QString::number(parsedModels.size());
		report << "Mod textures prepared: " + QString::number(writtenTextures.size() - 1);
		report << "Maximum mod texture slot size: " + QString::number(maxTextureSize);
		report << "Missing models: " + QString::number(missingModels.size() + invalidModels.size());
		report << "Missing textures: " + QString::number(missingTextures.size());
		report << "Time: " + QString::number(timer.Elapsed()) + " s";
		auto section = [&](const QString& title, QStringList list)
		{
			if (list.isEmpty())
				return;
			list.sort();
			report << "" << title + ":";
			for (int i = 0; i < list.size() && i < 2000; i++)
				report << "  " + list[i];
			if (list.size() > 2000)
				report << "  ... " + QString::number(list.size() - 2000) + " more";
		};
		section("Immersive Furniture", furnitureReport);
		section("Archive errors", archiveErrors);
		section("Skipped blocks", skippedBlocks);
		section("Missing models", QStringList(missingModels.values()));
		section("Invalid model JSON", QStringList(invalidModels.values()));
		section("Missing textures", missingTextures);

		WriteFile(reportPath, report.join("\n").toUtf8() + "\n");
		if (!WriteFile(manifestPath, QJsonDocument(manifest).toJson(QJsonDocument::Compact)))
			return "";
		WriteFile(signaturePath, signatureHash + "\n");

		DEBUG("Mod asset cache ready: " + NumStr(blocks.size()) + " blocks, " + NumStr(writtenTextures.size()) + " textures");
		return manifestPath;
	}
}
