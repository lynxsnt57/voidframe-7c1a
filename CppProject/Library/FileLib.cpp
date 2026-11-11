#include "Generated/Scripts.hpp"
#include "World/GZIP.hpp"
#include "AppHandler.hpp"

#include <QBuffer>
#include <QDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QProcess>
#include <QPushButton>
#include <QUuid>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QTreeWidget>
#include <QComboBox>
#include <QCheckBox>
#include <functional>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPainter>
#include <QPainterPath>
#include <QPolygonF>
#include <QColor>
#include <QTimer>
#include <QPixmap>
#include <QVector3D>
#include <QTransform>
#include <algorithm>
#include <QSet>
#include <QMap>
#include <memory>
#include <cmath>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QDesktopServices>
#include <QImageReader>
#include <QJsonArray>
#include <QTemporaryDir>
#include <cstring>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>
#include <QUrl>

#define ZIP_STATIC
#include <zip.h>

namespace CppProject
{
	RealType lib_open_url(StringType url)
	{
		if (!url.StartsWith("http"))
			url = "file:///" + url;
		return QDesktopServices::openUrl((QString)url);
	}

	RealType lib_execute(StringType file, StringType param, RealType wait)
	{
		return 0.0;
	}

	// Extract only ordinary quoted fields in [[mods]] sections. This is not a
	// full TOML parser; unknown/dynamic fields are shown as Unknown.
	struct ForgeModInfo { QString id; QString name; QString version; };
	static QList<ForgeModInfo> mod_forge_metadata(const QByteArray& data)
	{
		QList<ForgeModInfo> result;
		QString text=QString::fromUtf8(data); bool inMod=false;
		const QRegularExpression section("^\\s*\\[\\[mods\\]\\]\\s*(?:#.*)?$");
		const QRegularExpression otherSection("^\\s*\\[");
		const QRegularExpression field("^\\s*(modId|displayName|version)\\s*=\\s*(?:\"([^\"\\\\]*)\"|'([^']*)')\\s*(?:#.*)?$");
		for (const QString& raw:text.split('\n')) {
			QString line=raw.trimmed();
			if(section.match(line).hasMatch()) { result.append(ForgeModInfo{}); inMod=true; continue; }
			if(otherSection.match(line).hasMatch()) { inMod=false; continue; }
			if(!inMod || result.isEmpty()) continue;
			auto match=field.match(line); if(!match.hasMatch()) continue;
			QString key=match.captured(1), val=match.capturedStart(2)>=0?match.captured(2):match.captured(3);
			val.replace(QRegularExpression("[\\x00-\\x1F\\x7F]"), " "); val=val.trimmed().left(100);
			if(key=="modId") result.last().id=val;
			else if(key=="displayName") result.last().name=val;
			else result.last().version=val;
		}
		return result;
	}

	// Inspect Fabric or Forge metadata, never extract files or run code.
	StringType mod_inspect_fabric(StringType path)
	{
		int err = 0;
		std::string archivePath = path.ToStdString();
		zip_t* archive = zip_open(archivePath.c_str(), ZIP_RDONLY, &err);
		if (!archive)
			return "Cannot open the selected JAR.";
		auto fail = [archive](const char* message) -> StringType {
			zip_close(archive);
			return message;
		};
		const zip_int64_t count = zip_get_num_entries(archive, 0);
		if (count < 0 || count > 100000)
			return fail("Archive has an invalid or excessive number of entries.");
		zip_int64_t metadataIndex = -1, forgeIndex = -1;
		int blocks = 0, models = 0, textures = 0;
		for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(count); ++i)
		{
			const char* rawName = zip_get_name(archive, i, 0);
			if (!rawName) return fail("Cannot read archive entries.");
			QString name = QString::fromUtf8(rawName);
			if (name == "fabric.mod.json") {
				if (metadataIndex >= 0) return fail("Duplicate Fabric metadata entries.");
				metadataIndex = static_cast<zip_int64_t>(i);
			}
			if (name == "META-INF/mods.toml") {
				if (forgeIndex >= 0) return fail("Duplicate Forge metadata entries.");
				forgeIndex = static_cast<zip_int64_t>(i);
			}
			if (!name.startsWith("assets/") || name.endsWith('/')) continue;
			if (name.contains("/blockstates/") && name.endsWith(".json")) ++blocks;
			if (name.contains("/models/block/") && name.endsWith(".json")) ++models;
			if (name.contains("/textures/") && name.endsWith(".png")) ++textures;
		}
		bool forge=metadataIndex<0 && forgeIndex>=0;
		if (metadataIndex < 0 && forgeIndex < 0) return fail("No Fabric or Forge metadata found. Older Forge or NeoForge formats may not be supported.");
		if(forge) metadataIndex=forgeIndex;
		zip_stat_t stat;
		zip_stat_init(&stat);
		if (zip_stat_index(archive, metadataIndex, 0, &stat) != 0 || !(stat.valid & ZIP_STAT_SIZE) || stat.size > 262144)
			return fail("Mod metadata is missing or too large.");
		zip_file_t* file = zip_fopen_index(archive, metadataIndex, 0);
		if (!file) return fail("Cannot open mod metadata.");
		QByteArray data;
		data.resize(static_cast<int>(stat.size));
		zip_uint64_t pos = 0;
		while (pos < stat.size) {
			zip_int64_t n = zip_fread(file, data.data() + pos, stat.size - pos);
			if (n <= 0) { zip_fclose(file); return fail("Cannot read mod metadata."); }
			pos += static_cast<zip_uint64_t>(n);
		}
		if (zip_fclose(file) != 0) return fail("Invalid or corrupt mod metadata.");
		zip_close(archive);
		if(forge) {
			QList<ForgeModInfo> mods=mod_forge_metadata(data);
			if(mods.isEmpty() || mods.first().id.isEmpty()) return "Forge metadata present, but a simple mod ID could not be read.";
			const ForgeModInfo& mod=mods.first();
			QString out="Loader: Forge\nName: "+(mod.name.isEmpty()?mod.id:mod.name)+"\nID: "+mod.id+"\nVersion: "+(mod.version.isEmpty()?QString("Unknown"):mod.version);
			if(mods.size()>1) out+=QString("\nMods in archive: %1 (showing the first)").arg(mods.size());
			out+=QString("\nBlockstates: %1\nBlock model JSON files: %2\nTexture PNG files: %3").arg(blocks).arg(models).arg(textures);
			return out+"\nInspection only. No assets imported.";
		}
		QJsonParseError parseError;
		QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
		if (parseError.error != QJsonParseError::NoError || !doc.isObject())
			return "Fabric metadata is not valid JSON.";
		QJsonObject meta = doc.object();
		auto clean = [](QString text) -> QString {
			text.replace(QRegularExpression("[\\x00-\\x1F\\x7F]"), " ");
			text = text.trimmed().left(100);
			return text.isEmpty() ? QString("Unknown") : text;
		};
		QString id = clean(meta.value("id").toString());
		if (id == "Unknown") return "Fabric metadata has no mod ID.";
		QString name = clean(meta.value("name").toString(id));
		QString version = clean(meta.value("version").toString());
		QString out = "Loader: Fabric\nName: " + name + "\nID: " + id + "\nVersion: " + version;
		out += QString("\nBlockstates: %1\nBlock model JSON files: %2\nTexture PNG files: %3").arg(blocks).arg(models).arg(textures);
		out += "\nInspection only. No assets imported.";
		return out;
	}

	// Prepare exactly the static acacia chair from Refurbished, without running code.
	// This is deliberately not a generic mod importer or a skeletal animation converter.
	StringType mod_export_demo_chair(StringType jarPath)
	{
		int errorCode = 0;
		std::string jarName = jarPath.ToStdString();
		zip_t* archive = zip_open(jarName.c_str(), ZIP_RDONLY, &errorCode);
		if (!archive) return "ERROR: Cannot open the mod JAR.";
		auto fail = [archive](const char* message) -> StringType {
			zip_close(archive);
			return message;
		};
		const zip_int64_t count = zip_get_num_entries(archive, 0);
		if (count < 0 || count > 100000)
			return fail("ERROR: Invalid or excessively large archive.");
		// Match exact entry names and reject ambiguous duplicates; never extract paths.
		auto readEntry = [archive, count](const char* name, zip_uint64_t maxSize, QByteArray& out) -> bool {
			zip_int64_t found = -1;
			for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(count); ++i) {
				const char* entry = zip_get_name(archive, i, 0);
				if (!entry) return false;
				if (std::strcmp(entry, name) == 0) {
					if (found >= 0) return false;
					found = static_cast<zip_int64_t>(i);
				}
			}
			if (found < 0) return false;
			zip_stat_t st;
			zip_stat_init(&st);
			if (zip_stat_index(archive, found, 0, &st) != 0 || !(st.valid & ZIP_STAT_SIZE) || st.size > maxSize) return false;
			zip_file_t* f = zip_fopen_index(archive, found, 0);
			if (!f) return false;
			out.resize(static_cast<int>(st.size));
			zip_uint64_t pos = 0;
			while (pos < st.size) {
				zip_int64_t n = zip_fread(f, out.data() + pos, st.size - pos);
				if (n <= 0) { zip_fclose(f); return false; }
				pos += static_cast<zip_uint64_t>(n);
			}
			return zip_fclose(f) == 0;
		};
		QByteArray metaBytes, childBytes, parentBytes, textureBytes, particleBytes;
		if (!readEntry("fabric.mod.json", 262144, metaBytes) ||
			!readEntry("assets/refurbished_furniture/models/block/acacia_chair.json", 1048576, childBytes) ||
			!readEntry("assets/refurbished_furniture/models/block/chair.json", 1048576, parentBytes) ||
			!readEntry("assets/refurbished_furniture/textures/block/acacia_chair.png", 8388608, textureBytes) ||
			!readEntry("assets/refurbished_furniture/textures/block/acacia_particle.png", 8388608, particleBytes))
			return fail("ERROR: Required chair files are missing, invalid, duplicated, or too large.");
		zip_close(archive);
		QJsonParseError jsonError;
		auto readObject = [&jsonError](const QByteArray& bytes, QJsonObject& object) -> bool {
			QJsonDocument doc = QJsonDocument::fromJson(bytes, &jsonError);
			if (jsonError.error != QJsonParseError::NoError || !doc.isObject()) return false;
			object = doc.object();
			return true;
		};
		QJsonObject meta, child, parent;
		if (!readObject(metaBytes, meta) || !readObject(childBytes, child) || !readObject(parentBytes, parent))
			return "ERROR: Chair model JSON is invalid.";
		if (meta.value("id").toString() != "refurbished_furniture" ||
			child.value("parent").toString() != "refurbished_furniture:block/chair" ||
			child.contains("elements") || child.contains("loader") || parent.contains("loader") || parent.contains("parent"))
			return "ERROR: Unsupported mod or chair model version.";
		QJsonObject textures = child.value("textures").toObject();
		if (textures.value("texture").toString() != "refurbished_furniture:block/acacia_chair" ||
			textures.value("particle").toString() != "refurbished_furniture:block/acacia_particle" ||
			!parent.value("elements").isArray() || parent.value("elements").toArray().size() != 6)
			return "ERROR: This chair layout is not supported.";
		// Reject unsupported geometry rather than silently importing it incorrectly.
		auto numericArray = [](const QJsonValue& value, int n) -> bool {
			if (!value.isArray()) return false;
			QJsonArray arr = value.toArray();
			if (arr.size() != n) return false;
			for (const QJsonValue& item : arr) if (!item.isDouble()) return false;
			return true;
		};
		for (const QJsonValue& item : parent.value("elements").toArray()) {
			if (!item.isObject()) return "ERROR: Unsupported chair geometry.";
			QJsonObject element = item.toObject();
			if (!numericArray(element.value("from"), 3) || !numericArray(element.value("to"), 3) ||
				!element.value("faces").isObject() || element.contains("rotation"))
				return "ERROR: Unsupported chair geometry.";
			QJsonObject faces = element.value("faces").toObject();
			for (auto it = faces.begin(); it != faces.end(); ++it) {
				if (it.key() != "north" && it.key() != "south" && it.key() != "east" &&
					it.key() != "west" && it.key() != "up" && it.key() != "down")
					return "ERROR: Unsupported chair face.";
				if (!it.value().isObject()) return "ERROR: Invalid chair face.";
				QJsonObject face = it.value().toObject();
				if (face.value("texture").toString() != "#texture" || !numericArray(face.value("uv"), 4) || face.contains("rotation"))
					return "ERROR: Unsupported chair UV or texture.";
			}
		}
		auto validPng = [](const QByteArray& bytes) -> bool {
			QBuffer buffer;
			buffer.setData(bytes);
			if (!buffer.open(QIODevice::ReadOnly)) return false;
			QImageReader reader(&buffer, "png");
			QSize size = reader.size();
			if (!size.isValid() || size.width() > 4096 || size.height() > 4096 ||
				static_cast<qint64>(size.width()) * size.height() > 16777216) return false;
			return !reader.read().isNull();
		};
		if (!validPng(textureBytes) || !validPng(particleBytes))
			return "ERROR: Chair texture is invalid or too large.";
		// Flatten known inheritance for Mine-imator's standalone JSON block loader.
		QJsonObject model = parent;
		QJsonObject localTextures;
		localTextures.insert("texture", "chair_texture");
		localTextures.insert("particle", "chair_particle");
		model.insert("textures", localTextures);
		QTemporaryDir temp(QDir::tempPath() + "/MineimatorModChair-XXXXXX");
		if (!temp.isValid()) return "ERROR: Cannot create temporary chair directory.";
		auto write = [&temp](const QString& name, const QByteArray& data) -> bool {
			QFile file(temp.filePath(name));
			if (!file.open(QIODevice::WriteOnly | QIODevice::NewOnly)) return false;
			if (file.write(data) != data.size()) return false;
			return file.flush();
		};
		QString outputName = QFileInfo(temp.path()).fileName() + ".json";
		if (!write("chair_texture.png", textureBytes) || !write("chair_particle.png", particleBytes) ||
			!write(outputName, QJsonDocument(model).toJson(QJsonDocument::Indented)))
			return "ERROR: Cannot save temporary chair files.";
		QString outputPath = temp.filePath(outputName);
		temp.setAutoRemove(false); // Kept while the project resource is imported and saved.
		return outputPath;
	}

	// Lightweight orthographic thumbnail for the model picker; it is not the
	// application renderer. The actual imported model uses Mine-imator's renderer.
	static QImage mod_render_preview(const QString& modelFile)
	{
		QFile file(modelFile);
		if (!file.open(QIODevice::ReadOnly)) return QImage();
		QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
		if (!doc.isObject()) return QImage();
		QJsonObject model = doc.object();
		QJsonArray elements = model.value("elements").toArray();
		QJsonObject textures = model.value("textures").toObject();
		if (elements.isEmpty() || elements.size() > 1024) return QImage();
		QMap<QString, QImage> tex;
		QDir dir = QFileInfo(modelFile).absoluteDir();
		qint64 totalPixels = 0;
		for (auto it = textures.begin(); it != textures.end(); ++it) {
			QString name = it.value().toString();
			if (!QRegularExpression("^mod_texture_[0-9]+$").match(name).hasMatch()) return QImage();
			QImageReader previewReader(dir.filePath(name + ".png"), "png");
			QSize size = previewReader.size();
			if (!size.isValid() || size.width() > 2048 || size.height() > 2048) return QImage();
			totalPixels += qint64(size.width()) * size.height();
			if (totalPixels > 8388608) return QImage();
			QImage image = previewReader.read();
			if (image.isNull()) return QImage();
			tex.insert(it.key(), image);
		}
		struct Face { QVector<QVector3D> points; QPolygonF uv; QImage image; double depth; };
		QVector<Face> faces;
		const double yaw = 40.0 * 3.141592653589793 / 180.0, elevation = 25.0 * 3.141592653589793 / 180.0;
		const QVector3D right(std::cos(yaw), 0, -std::sin(yaw));
		const QVector3D view(std::sin(yaw)*std::cos(elevation), std::sin(elevation), std::cos(yaw)*std::cos(elevation));
		const QVector3D up = QVector3D::crossProduct(view, right);
		auto vector = [](const QJsonValue& v, const QVector3D& fallback) -> QVector3D {
			if (!v.isArray() || v.toArray().size() != 3) return fallback;
			QJsonArray a = v.toArray(); return QVector3D(a[0].toDouble(), a[1].toDouble(), a[2].toDouble());
		};
		for (const QJsonValue& v : elements) {
			QJsonObject e = v.toObject();
			QVector3D a = vector(e.value("from"), {}), b = vector(e.value("to"), {});
			float x=a.x(), y=a.y(), z=a.z(), X=b.x(), Y=b.y(), Z=b.z();
			QMap<QString, QVector<QVector3D>> geometry;
			geometry["north"] = {{X,Y,z},{x,Y,z},{x,y,z},{X,y,z}};
			geometry["south"] = {{x,Y,Z},{X,Y,Z},{X,y,Z},{x,y,Z}};
			geometry["east"]  = {{X,Y,Z},{X,Y,z},{X,y,z},{X,y,Z}};
			geometry["west"]  = {{x,Y,z},{x,Y,Z},{x,y,Z},{x,y,z}};
			geometry["up"]    = {{x,Y,z},{X,Y,z},{X,Y,Z},{x,Y,Z}};
			geometry["down"]  = {{x,y,Z},{X,y,Z},{X,y,z},{x,y,z}};
			QMap<QString, QVector3D> normals = {{"north",{0,0,-1}},{"south",{0,0,1}},{"east",{1,0,0}},{"west",{-1,0,0}},{"up",{0,1,0}},{"down",{0,-1,0}}};
			QJsonObject rotation = e.value("rotation").toObject();
			QString axis = rotation.value("axis").toString();
			double angle = rotation.value("angle").toDouble() * 3.141592653589793 / 180.0;
			double c = std::cos(angle), s = std::sin(angle);
			QVector3D origin = vector(rotation.value("origin"), {8,8,8});
			auto rotate = [axis,c,s](QVector3D p) -> QVector3D {
				if (axis == "x") return {p.x(), float(c*p.y()-s*p.z()), float(s*p.y()+c*p.z())};
				if (axis == "y") return {float(c*p.x()+s*p.z()), p.y(), float(-s*p.x()+c*p.z())};
				if (axis == "z") return {float(c*p.x()-s*p.y()), float(s*p.x()+c*p.y()), p.z()};
				return p;
			};
			QJsonObject namedFaces = e.value("faces").toObject();
			for (auto it = namedFaces.begin(); it != namedFaces.end(); ++it) {
				QString name = it.key();
				if (!geometry.contains(name)) continue;
				QVector3D normal = rotate(normals.value(name));
				if (QVector3D::dotProduct(normal, view) <= 0.0001) continue;
				QJsonObject f = it.value().toObject();
				QString ref = f.value("texture").toString();
				if (!ref.startsWith('#') || !tex.contains(ref.mid(1))) return QImage();
				QImage image = tex.value(ref.mid(1));
				QJsonArray uv = f.value("uv").toArray();
				if (uv.size() != 4) return QImage(); // Don't guess implicit UV in the thumbnail.
				double u=uv[0].toDouble()*image.width()/16., w=uv[2].toDouble()*image.width()/16.;
				double v0=uv[1].toDouble()*image.height()/16., v1=uv[3].toDouble()*image.height()/16.;
				QPolygonF source; source << QPointF(u,v0) << QPointF(w,v0) << QPointF(w,v1) << QPointF(u,v1);
				int turns = int(f.value("rotation").toDouble()) / 90;
				for (int i = 0; i < turns; ++i) source.prepend(source.takeLast());
				QVector<QVector3D> points = geometry.value(name);
				double depth = 0;
				for (QVector3D& p : points) {
					p -= origin;
					if (rotation.value("rescale").toBool() && std::abs(c) > 0.0001) {
						if (axis != "x") p.setX(p.x()/c);
						if (axis != "y") p.setY(p.y()/c);
						if (axis != "z") p.setZ(p.z()/c);
					}
					p = rotate(p) + origin;
					depth += QVector3D::dotProduct(p, view);
				}
				faces.append({points, source, image, depth / 4.});
			}
		}
		if (faces.isEmpty()) return QImage();
		double minX=1e30,maxX=-1e30,minY=1e30,maxY=-1e30;
		for (const Face& f : faces) for (const QVector3D& p : f.points) {
			double x=QVector3D::dotProduct(p,right), y=QVector3D::dotProduct(p,up);
			minX=std::min(minX,x); maxX=std::max(maxX,x); minY=std::min(minY,y); maxY=std::max(maxY,y);
		}
		if (maxX-minX < .0001 || maxY-minY < .0001) return QImage();
		const int width=400, height=360;
		double scale = std::min(310.0/(maxX-minX), 280.0/(maxY-minY));
		std::sort(faces.begin(), faces.end(), [](const Face& a,const Face& b){ return a.depth < b.depth; });
		QImage result(width, height, QImage::Format_ARGB32_Premultiplied);
		result.fill(QColor("#20242b"));
		QPainter painter(&result);
		painter.setRenderHint(QPainter::SmoothPixmapTransform, false);
		int drawnFaces = 0;
		for (const Face& f : faces) {
			QPolygonF target;
			for (const QVector3D& p : f.points) {
				double x=QVector3D::dotProduct(p,right), y=QVector3D::dotProduct(p,up);
				target << QPointF(width/2.0+(x-(minX+maxX)/2)*scale, height/2.0-(y-(minY+maxY)/2)*scale);
			}
			QTransform transform;
			if (!QTransform::quadToQuad(f.uv, target, transform)) continue;
			QPainterPath clip; clip.addPolygon(target); clip.closeSubpath();
			painter.save(); painter.setClipPath(clip); painter.setWorldTransform(transform); painter.drawImage(QPointF(0,0), f.image); painter.restore();
			++drawnFaces;
		}
		painter.end();
		return drawnFaces ? result : QImage();
	}

	// Export a standalone Blockbench Java-block project with embedded PNG copies.
	static QString mod_make_blockbench_project(const QString& modelFile)
	{
		QFile input(modelFile); if (!input.open(QIODevice::ReadOnly)) return "";
		QJsonDocument parsed = QJsonDocument::fromJson(input.readAll());
		if (!parsed.isObject()) return "";
		QJsonObject model = parsed.object(), refs = model.value("textures").toObject();
		QJsonArray inputElements = model.value("elements").toArray();
		if (inputElements.isEmpty() || inputElements.size() > 1024) return "";
		QDir dir = QFileInfo(modelFile).absoluteDir();
		QJsonArray textures, elements, outliner; QMap<QString,int> index;
		for (auto it=refs.begin(); it!=refs.end(); ++it) {
			QString name=it.value().toString();
			if (!QRegularExpression("^mod_texture_[0-9]+$").match(name).hasMatch()) return "";
			QFile image(dir.filePath(name+".png"));
			if (!image.open(QIODevice::ReadOnly) || image.size() > 8388608) return "";
			QByteArray bytes=image.readAll();
			QJsonObject texture;
			texture.insert("name", name+".png"); texture.insert("id", QString::number(textures.size()));
			texture.insert("uuid", QUuid::createUuid().toString(QUuid::WithoutBraces));
			texture.insert("source", "data:image/png;base64,"+QString::fromLatin1(bytes.toBase64()));
			texture.insert("internal", true); index.insert(it.key(), textures.size()); textures.append(texture);
		}
		const QStringList sides={"north","south","east","west","up","down"};
		for (const QJsonValue& val : inputElements) {
			QJsonObject e=val.toObject(), inFaces=e.value("faces").toObject(), faces;
			for (const QString& side : sides) {
				QJsonObject face;
				if (!inFaces.contains(side)) { face.insert("texture", QJsonValue(QJsonValue::Null)); }
				else {
					QJsonObject in=inFaces.value(side).toObject(); QString ref=in.value("texture").toString();
					if (!ref.startsWith('#') || !index.contains(ref.mid(1)) || in.value("uv").toArray().size()!=4) return "";
					face.insert("texture", index.value(ref.mid(1))); face.insert("uv", in.value("uv"));
					if (in.contains("rotation")) face.insert("rotation", in.value("rotation"));
				}
				faces.insert(side, face);
			}
			QString uuid=QUuid::createUuid().toString(QUuid::WithoutBraces);
			QJsonObject cube; cube.insert("name",QString("part_%1").arg(elements.size()+1)); cube.insert("type","cube"); cube.insert("uuid",uuid);
			cube.insert("from",e.value("from")); cube.insert("to",e.value("to")); cube.insert("faces",faces); cube.insert("box_uv",false);
			if (e.contains("rotation")) {
				QJsonObject r=e.value("rotation").toObject(); QString axis=r.value("axis").toString(); double a=r.value("angle").toDouble();
				QJsonArray angles; angles.append(axis=="x"?a:0); angles.append(axis=="y"?a:0); angles.append(axis=="z"?a:0);
				cube.insert("rotation",angles); cube.insert("rotation_axis",axis);
				cube.insert("origin",r.contains("origin")?r.value("origin"):QJsonValue(QJsonArray{8,8,8}));
				cube.insert("rescale",r.value("rescale").toBool());
			}
			elements.append(cube); outliner.append(uuid);
		}
		QJsonObject meta; meta.insert("format_version","4.10"); meta.insert("model_format","java_block"); meta.insert("box_uv",false);
		QJsonObject resolution; resolution.insert("width",16); resolution.insert("height",16);
		QJsonObject result; result.insert("meta",meta); result.insert("resolution",resolution); result.insert("elements",elements); result.insert("outliner",outliner); result.insert("textures",textures);
		QString path=dir.filePath(QFileInfo(modelFile).completeBaseName()+".bbmodel");
		QFile output(path); QByteArray data=QJsonDocument(result).toJson(QJsonDocument::Indented);
		if (!output.open(QIODevice::WriteOnly | QIODevice::NewOnly) || output.write(data)!=data.size() || !output.flush()) return "";
		return path;
	}

	// Convert a conservative subset of Bedrock geometry into native .mimodel.
	// Unsupported features fail explicitly; this never executes code from the mod.
	static QString mod_bedrock_to_mimodel(const QByteArray& data, const QByteArray& png, bool editable, QString* error)
	{
		auto fail=[error](const QString& why)->QString { *error=why; return ""; };
		QJsonParseError err; QJsonDocument doc=QJsonDocument::fromJson(data,&err);
		if (err.error!=QJsonParseError::NoError || !doc.isObject()) return fail("Invalid Bedrock geometry JSON.");
		QJsonArray geos=doc.object().value("minecraft:geometry").toArray();
		if (geos.size()!=1 || !geos[0].isObject()) return fail("Exactly one Bedrock geometry is required.");
		QJsonObject geo=geos[0].toObject(), desc=geo.value("description").toObject();
		QJsonArray bones=geo.value("bones").toArray();
		if (bones.isEmpty() || bones.size()>512) return fail("Invalid or excessive bone count (maximum 512).");
		int tw=desc.value("texture_width").toInt(), th=desc.value("texture_height").toInt();
		if (tw<1 || th<1 || tw>2048 || th>2048) return fail("Invalid model texture dimensions.");
		QByteArray pngCopy=png; QBuffer imgBuffer(&pngCopy); imgBuffer.open(QIODevice::ReadOnly);
		QImageReader reader(&imgBuffer,"png");
		if (reader.size()!=QSize(tw,th) || reader.read().isNull()) return fail("Texture size does not match the geometry description.");
		auto vector=[](const QJsonValue& value,int count,QJsonArray& out)->bool {
			if (!value.isArray() || value.toArray().size()!=count) return false;
			out=value.toArray(); for (const QJsonValue& n:out) if(!n.isDouble() || !std::isfinite(n.toDouble()) || std::abs(n.toDouble())>4096) return false;
			return true;
		};
		auto pos=[](const QJsonArray& a)->QVector3D { return QVector3D(-a[0].toDouble(),a[1].toDouble(),a[2].toDouble()); };
		auto arr=[](const QVector3D& p)->QJsonArray { return QJsonArray{p.x(),p.y(),p.z()}; };
		auto angle=[&vector](const QJsonValue& val, QJsonArray& out)->bool {
			if (val.isUndefined()) { out=QJsonArray{0,0,0}; return true; }
			QJsonArray a; if(!vector(val,3,a)) return false;
			// Reflection of Bedrock's X axis, following Blockbench's Bedrock importer.
			out=QJsonArray{-a[0].toDouble(),-a[1].toDouble(),a[2].toDouble()}; return true;
		};
		QMap<QString,QJsonObject> byName; QMap<QString,QStringList> children; QMap<QString,QVector3D> pivots; QStringList roots;
		int countCubes=0;
		for (const QJsonValue& value:bones) {
			if(!value.isObject()) return fail("Invalid bone."); QJsonObject b=value.toObject();
			QString name=b.value("name").toString(), parent=b.value("parent").toString(); QJsonArray pivot,rot;
			if(name.isEmpty() || name.size()>120 || byName.contains(name)) return fail("Missing, duplicate or excessively long bone name.");
			if(!vector(b.value("pivot"),3,pivot) || !angle(b.value("rotation"),rot)) return fail("Invalid bone pivot or rotation: "+name);
			if(b.contains("poly_mesh") || b.contains("texture_meshes") || b.contains("binding")) return fail("Unsupported bone geometry or binding: "+name);
			if(b.contains("cubes") && !b.value("cubes").isArray()) return fail("Invalid cubes: "+name);
			countCubes+=b.value("cubes").toArray().size(); if(countCubes>4096) return fail("Too many cubes.");
			byName.insert(name,b); pivots.insert(name,pos(pivot));
			if(parent.isEmpty()) roots.append(name); else children[parent].append(name);
		}
		for (auto it=children.cbegin();it!=children.cend();++it) if(!byName.contains(it.key())) return fail("Missing parent bone: "+it.key());
		if(roots.isEmpty()) return fail("Bone hierarchy has no root.");
		// Omit geometry-free branches (mostly locators). Empty leaf parts leave
		// infinite bounds in Mine-imator, which can break the model preview.
		QMap<QString,int> state; QMap<QString,bool> keep; QString graphError;
		std::function<bool(const QString&,int)> hasGeometry;
		hasGeometry=[&](const QString& name,int depth)->bool {
			if(depth>80 || state.value(name)==1) { graphError="Bone hierarchy has a cycle or is too deep."; return false; }
			if(state.value(name)==2) return keep.value(name);
			state[name]=1; bool found=!byName.value(name).value("cubes").toArray().isEmpty();
			for(const QString& child:children.value(name)) { bool childHas=hasGeometry(child,depth+1); found=found || childHas; if(!graphError.isEmpty()) return false; }
			state[name]=2; keep[name]=found; return found;
		};
		for(auto it=byName.cbegin();it!=byName.cend();++it) { hasGeometry(it.key(),0); if(!graphError.isEmpty()) return fail(graphError); }
		if(countCubes==0) return fail("The geometry has no cubes to import.");
		QSet<QString> visited; bool bad=false; QString why;
		std::function<QJsonObject(const QString&,const QVector3D&,int)> build;
		build=[&](const QString& name,const QVector3D& parentPivot,int depth)->QJsonObject {
			if (bad) return {};
			if(depth>80 || visited.contains(name)) { bad=true; why="Bone hierarchy has a cycle or is too deep."; return {}; }
			visited.insert(name); QJsonObject b=byName.value(name), part; QVector3D bonePivot=pivots.value(name); QJsonArray rotation;
			if(!angle(b.value("rotation"),rotation)) { bad=true;why="Invalid bone rotation.";return {}; }
			part.insert("name",name); part.insert("position",arr(bonePivot-parentPivot)); part.insert("rotation",rotation);
			if(!editable) part.insert("locked",true);
			part.insert("backfaces",true);
			QJsonArray shapes,subparts; QJsonArray cubes=b.value("cubes").toArray();
			for(int i=0;i<cubes.size();++i) {
				if(!cubes[i].isObject()) { bad=true;why="Invalid cube: "+name;return {}; }
				QJsonObject c=cubes[i].toObject(); QJsonArray origin,size,uv,crot,pivot;
				if(!vector(c.value("origin"),3,origin) || !vector(c.value("size"),3,size) || !vector(c.value("uv"),2,uv) || !angle(c.value("rotation"),crot)) { bad=true;why="Unsupported cube or per-face UV: "+name;return {}; }
				for(const QJsonValue& n:size) if(n.toDouble()<0) {bad=true;why="Negative cube size: "+name;return {};}
				if(c.contains("inflate") && !c.value("inflate").isDouble()) { bad=true;why="Invalid inflation: "+name;return {}; }
				double inf=c.contains("inflate")?c.value("inflate").toDouble():b.value("inflate").toDouble();
				if(!std::isfinite(inf) || std::abs(inf)>128) { bad=true;why="Unsupported inflation: "+name;return {}; }
				bool rotated=crot[0].toDouble()!=0 || crot[1].toDouble()!=0 || crot[2].toDouble()!=0;
				QVector3D cubePivot=bonePivot;
				if(rotated) { if(!vector(c.value("pivot"),3,pivot)) {bad=true;why="Rotated cube is missing its pivot: "+name;return {};} cubePivot=pos(pivot); }
				QVector3D from(-origin[0].toDouble()-size[0].toDouble(),origin[1].toDouble(),origin[2].toDouble());
				QVector3D to=from+QVector3D(size[0].toDouble(),size[1].toDouble(),size[2].toDouble());
				QJsonObject shape; shape.insert("type","block"); shape.insert("from",arr(from-cubePivot));shape.insert("to",arr(to-cubePivot));
				// MI's box-UV origin is the front face, offset by depth from Bedrock UV.
				double d=std::floor(size[2].toDouble()+0.000001);
				shape.insert("uv",QJsonArray{uv[0].toDouble()+d,uv[1].toDouble()+d});
				if(inf!=0) shape.insert("inflate",inf);
				// Bedrock/Blockbench and Mine-imator use opposite box face labels.
				// A 180-degree UV remap exchanges both face pairs; invert the X
				// mirror flag so Bedrock's own mirrored UVs are preserved.
				bool bedrockMirror=c.value("mirror").isBool()?c.value("mirror").toBool():b.value("mirror").toBool();
				shape.insert("texture_mirror",!bedrockMirror);
				shape.insert("texture_mirror_y",true);
				if(rotated) {
					QString synthetic="__mi_cube_"+QString::number(visited.size())+"_"+QString::number(i);
					while(byName.contains(synthetic) || visited.contains(synthetic)) synthetic+="_";
					visited.insert(synthetic); QJsonObject sub; sub.insert("name",synthetic);sub.insert("position",arr(cubePivot-bonePivot));sub.insert("rotation",crot);sub.insert("locked",true);sub.insert("backfaces",true);sub.insert("shapes",QJsonArray{shape});subparts.append(sub);
				} else shapes.append(shape);
			}
			if(!shapes.isEmpty()) part.insert("shapes",shapes);
			for(const QString& child:children.value(name)) if(keep.value(child)) subparts.append(build(child,bonePivot,depth+1));
			if(!subparts.isEmpty()) part.insert("parts",subparts); return part;
		};
		QJsonArray parts; for(const QString& root:roots) if(keep.value(root)) parts.append(build(root,QVector3D(),0));
		if(bad) return fail(why);
		// visited contains synthetic cube nodes too; validate all actual bones were reached.
		for(auto it=byName.cbegin();it!=byName.cend();++it) if(keep.value(it.key()) && !visited.contains(it.key())) return fail("Disconnected or cyclic bone hierarchy.");
		QTemporaryDir temp(QDir::tempPath()+"/MineimatorBedrock-XXXXXX"); if(!temp.isValid()) return fail("Cannot create a temporary model directory.");
		QString suffix=QFileInfo(temp.path()).fileName().right(6);
		QString tex="bedrock_texture_"+suffix+".png";
		QJsonObject model; model.insert("name","Bedrock model"); model.insert("texture",tex);model.insert("texture_size",QJsonArray{tw,th});model.insert("floor_box_uvs",true);model.insert("parts",parts);
		auto write=[&temp](const QString& name,const QByteArray& bytes)->bool { QFile f(temp.filePath(name));return f.open(QIODevice::WriteOnly|QIODevice::NewOnly)&&f.write(bytes)==bytes.size()&&f.flush(); };
		QString fileName="BedrockModel-"+suffix+".mimodel";
		if(!write(tex,png) || !write(fileName,QJsonDocument(model).toJson(QJsonDocument::Compact))) return fail("Cannot save temporary model or texture.");
		QString result=temp.filePath(fileName); temp.setAutoRemove(false); return result;
	}


	// Cobblemon uses Bedrock geometry. This is a browser/exporter for Blockbench,
	// not a Mine-imator skeletal importer; it reads archive data and runs no mod code.
	template<class ReadEntry>
	static QString mod_browse_cobblemon(const QMap<QString, zip_uint64_t>& entries, ReadEntry& readEntry)
	{
		const QString modelRoot = "assets/cobblemon/bedrock/pokemon/models/";
		const QString textureRoot = "assets/cobblemon/textures/pokemon/";
		QStringList models;
		for (auto it=entries.cbegin(); it!=entries.cend(); ++it) {
			const QString path=it.key();
			if (path.startsWith(modelRoot) && path.endsWith(".geo.json")) {
				QString rest=path.mid(modelRoot.size());
				if (QRegularExpression("^[a-z0-9_.-]+/[a-z0-9_.-]+\\.geo\\.json$").match(rest).hasMatch()) models.append(path);
			}
		}
		if (models.isEmpty() || models.size()>10000) {
			QMessageBox::information(nullptr, "Cobblemon", "No supported Pokemon geometry files found in this JAR."); return QString();
		}
		models.sort(Qt::CaseInsensitive);
		QDialog dialog; dialog.setWindowTitle("Cobblemon models and bones"); dialog.resize(1000,600);
		dialog.setStyleSheet("QDialog { background:#17191d; color:#e5e8ef; font-size:13px; } QLabel {color:#b8c1d0; padding:4px;} QLineEdit,QListWidget,QTreeWidget,QComboBox {background:#20242b; color:#f1f4f8; border:1px solid #414954; padding:5px;} QListWidget::item {padding:4px;} QListWidget::item:selected {background:#476da9;} QPushButton {background:#344c72; color:white; border:1px solid #6286bc; padding:7px;}");
		QVBoxLayout layout(&dialog);
		QLabel notice("Experimental Bedrock import. Verify the texture, then press Create in the workbench. Animations and geometry-free locator branches are not imported.");
		notice.setWordWrap(true); layout.addWidget(&notice);
		QHBoxLayout body; layout.addLayout(&body);
		QVBoxLayout left; body.addLayout(&left, 2); QVBoxLayout right; body.addLayout(&right, 3);
		QLineEdit search; search.setPlaceholderText("Search Pokemon models..."); left.addWidget(&search);
		QListWidget list;
		for (const QString& path:models) { QListWidgetItem* item=new QListWidgetItem(path.mid(modelRoot.size())); item->setData(Qt::UserRole,path); list.addItem(item); }
		left.addWidget(&list);
		QLabel details("Select a model"); details.setWordWrap(true); right.addWidget(&details);
		QTreeWidget bones; bones.setColumnCount(3); bones.setHeaderLabels(QStringList{"Bone", "Parent", "Cubes"}); right.addWidget(&bones,1);
		QLabel textureLabel("Texture copy (choose manually if needed)"); right.addWidget(&textureLabel);
		QComboBox textures; right.addWidget(&textures);
		QCheckBox editable("Enable individual bone controls (unchecked: lock bones, not merge)"); editable.setChecked(true); right.addWidget(&editable);
		QPushButton useModel("Use model in Mine-imator"); right.addWidget(&useModel);
		QPushButton open("Open geometry copy in Blockbench..."); right.addWidget(&open);
		QString importedPath;
		QDialogButtonBox buttons(QDialogButtonBox::Close); layout.addWidget(&buttons);
		QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
		QObject::connect(&search, &QLineEdit::textChanged, &list, [&list](const QString& text) {
			QListWidgetItem* first=nullptr;
			for (int i=0;i<list.count();++i) { QListWidgetItem* item=list.item(i); bool visible=item->text().contains(text,Qt::CaseInsensitive); item->setHidden(!visible); if(visible && !first) first=item; }
			list.setCurrentItem(first);
		});
		QObject::connect(&list, &QListWidget::currentItemChanged, &dialog, [&readEntry, &entries, &modelRoot, &textureRoot, &details, &bones, &textures, &open](QListWidgetItem* item, QListWidgetItem*) {
			bones.clear(); textures.clear(); open.setEnabled(false);
			if (!item || item->isHidden()) { details.setText("Select a model"); return; }
			QString path=item->data(Qt::UserRole).toString(); QByteArray data;
			if (!readEntry(path, 8388608, data)) { details.setText("Cannot read geometry (missing or too large)."); return; }
			QJsonParseError err; QJsonDocument doc=QJsonDocument::fromJson(data, &err);
			if (err.error!=QJsonParseError::NoError || !doc.isObject()) { details.setText("Invalid geometry JSON."); return; }
			QJsonArray geos=doc.object().value("minecraft:geometry").toArray();
			if (geos.size()!=1 || !geos[0].isObject()) { details.setText("Only single-geometry Bedrock models are supported here."); return; }
			QJsonObject geo=geos[0].toObject(); QJsonArray array=geo.value("bones").toArray();
			if (array.isEmpty() || array.size()>2048) { details.setText("Invalid or excessive number of bones."); return; }
			int cubes=0;
			for (const QJsonValue& val:array) {
				QJsonObject bone=val.toObject(); QString name=bone.value("name").toString().left(120); QString parent=bone.value("parent").toString().left(120);
				if (name.isEmpty()) { bones.clear(); details.setText("Invalid bone name."); return; }
				int n=bone.value("cubes").toArray().size(); cubes+=n;
				new QTreeWidgetItem(&bones, QStringList{name,parent,QString::number(n)});
			}
			QString group=path.mid(modelRoot.size()).section('/',0,0);
			QString prefix=textureRoot+group+"/";
			for (auto it=entries.cbegin(); it!=entries.cend(); ++it) {
				QString p=it.key(); if (!p.startsWith(prefix) || !p.endsWith(".png")) continue;
				QString name=p.mid(prefix.size());
				if (!QRegularExpression("^[a-z0-9_.-]+\\.png$").match(name).hasMatch()) continue;
				textures.addItem(name,p);
			}
			// Prefer the base species texture, not a shiny/emissive variant.
			QString species=group; species.remove(QRegularExpression("^[0-9]+_"));
			int defaultTexture=textures.findText(species+".png"); if (defaultTexture>=0) textures.setCurrentIndex(defaultTexture);
			details.setText(QString("%1 bones, %2 cubes. Texture is a suggested choice; verify the variant.").arg(array.size()).arg(cubes));
			open.setEnabled(true);
		});
		QObject::connect(&useModel, &QPushButton::clicked, &dialog, [&]() {
			QListWidgetItem* item=list.currentItem(); if (!item || item->isHidden()) return;
			QByteArray geometry, png;
			if (!readEntry(item->data(Qt::UserRole).toString(),8388608,geometry) || textures.currentIndex()<0 || !readEntry(textures.currentData().toString(),8388608,png)) {
				QMessageBox::warning(&dialog,"Bedrock import","Cannot read geometry or selected texture."); return;
			}
			QString error; QString result=mod_bedrock_to_mimodel(geometry,png,editable.isChecked(),&error);
			if(result.isEmpty()) { QMessageBox::warning(&dialog,"Bedrock import",error); return; }
			importedPath=result; dialog.accept();
		});
		QObject::connect(&open, &QPushButton::clicked, &dialog, [&dialog, &list, &textures, &readEntry]() {
			QListWidgetItem* item=list.currentItem(); if (!item || item->isHidden()) return;
			QString exe=QFileDialog::getOpenFileName(&dialog,"Select installed Blockbench executable","","Applications (*.exe)"); if(exe.isEmpty()) return;
			QByteArray geometry, png; QString path=item->data(Qt::UserRole).toString();
			if (!readEntry(path,8388608,geometry)) { QMessageBox::warning(&dialog,"Cobblemon","Cannot read geometry."); return; }
			bool hasTexture=textures.currentIndex()>=0;
			if (hasTexture) {
				if (!readEntry(textures.currentData().toString(),8388608,png)) { QMessageBox::warning(&dialog,"Cobblemon","Cannot read selected texture."); return; }
				QBuffer buffer(&png); buffer.open(QIODevice::ReadOnly); QImageReader reader(&buffer,"png"); QSize size=reader.size();
				if (!size.isValid() || size.width()>4096 || size.height()>4096 || reader.read().isNull()) { QMessageBox::warning(&dialog,"Cobblemon","Invalid or excessively large texture."); return; }
			}
			QTemporaryDir temp(QDir::tempPath()+"/MineimatorCobblemon-XXXXXX");
			if (!temp.isValid()) { QMessageBox::warning(&dialog,"Cobblemon","Cannot create a temporary folder."); return; }
			auto write=[&temp](const QString& name,const QByteArray& bytes) { QFile f(temp.filePath(name)); return f.open(QIODevice::WriteOnly|QIODevice::NewOnly) && f.write(bytes)==bytes.size() && f.flush(); };
			if (!write("model.geo.json",geometry) || (hasTexture && !write("texture.png",png))) { QMessageBox::warning(&dialog,"Cobblemon","Cannot save temporary copies."); return; }
			if (!QProcess::startDetached(exe,QStringList{temp.filePath("model.geo.json")},temp.path())) { QMessageBox::warning(&dialog,"Cobblemon","Could not start Blockbench."); return; }
			QString folder=temp.path(); temp.setAutoRemove(false);
			QString message="A geometry copy was sent to Blockbench. Bones remain in the Bedrock file. Save As to keep your changes.\n\n";
			message += hasTexture ? "Import texture.png manually in Blockbench. It is in this temporary folder:\n" : "No matching texture was found. The geometry copy is in:\n";
			message += QDir::toNativeSeparators(folder) + "\n\nThis does not import the model into Mine-imator or include animations.";
			QMessageBox::information(&dialog,"Cobblemon",message);
			QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
		});
		list.setCurrentRow(0);
		App->ExecDialog(&dialog);
		return importedPath;
	}

	// Import external Bedrock data only; never executes a mod or opens an archive.
	StringType mod_import_bedrock_file()
	{
		QString geometryPath=QFileDialog::getOpenFileName(nullptr,"Select Bedrock geometry",QString(),"Bedrock geometry (*.geo.json *.json)");
		if(geometryPath.isEmpty()) return "";
		QString texturePath=QFileDialog::getOpenFileName(nullptr,"Select matching PNG texture",QFileInfo(geometryPath).absolutePath(),"PNG image (*.png)");
		if(texturePath.isEmpty()) return "";
		auto read=[](const QString& path,QByteArray& out)->bool { QFile f(path); if(!f.open(QIODevice::ReadOnly) || f.size()>8388608) return false; out=f.readAll(); return f.error()==QFile::NoError; };
		QByteArray geometry,png; if(!read(geometryPath,geometry) || !read(texturePath,png)) return "ERROR: Cannot read geometry or texture (maximum 8 MB each).";
		QDialog dialog; dialog.setWindowTitle("Bedrock import options"); QVBoxLayout layout(&dialog);
		QLabel note("Experimental import; animations and non-cube geometry are not supported.\nUnchecked locks the bones; it does not merge the geometry."); note.setWordWrap(true); layout.addWidget(&note);
		QCheckBox editable("Enable individual bone controls"); editable.setChecked(true); layout.addWidget(&editable);
		QDialogButtonBox buttons(QDialogButtonBox::Ok|QDialogButtonBox::Cancel); layout.addWidget(&buttons);
		QObject::connect(&buttons,&QDialogButtonBox::accepted,&dialog,&QDialog::accept);
		QObject::connect(&buttons,&QDialogButtonBox::rejected,&dialog,&QDialog::reject);
		App->ExecDialog(&dialog); if(dialog.result()!=QDialog::Accepted) return "";
		QString error, result=mod_bedrock_to_mimodel(geometry,png,editable.isChecked(),&error);
		if(result.isEmpty()) return QString("ERROR: ")+error; return result;
	}

	// Experimental static Minecraft block model importer. Reads data only, never mod code.
	StringType mod_import_static_model(StringType jarPath, StringType minecraftZipPath)
	{
		int zipError = 0;
		std::string archiveName = jarPath.ToStdString();
		std::unique_ptr<zip_t, decltype(&zip_discard)> archive(zip_open(archiveName.c_str(), ZIP_RDONLY, &zipError), zip_discard);
		if (!archive) return "ERROR: Cannot open the selected JAR.";
		const zip_int64_t count = zip_get_num_entries(archive.get(), 0);
		if (count < 0 || count > 100000) return "ERROR: Invalid or excessive number of archive entries.";
		QMap<QString, zip_uint64_t> entries;
		for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(count); ++i) {
			const char* raw = zip_get_name(archive.get(), i, 0);
			if (!raw) return "ERROR: Cannot read archive index.";
			QString name = QString::fromUtf8(raw);
			if (entries.contains(name)) return "ERROR: Duplicate archive entries are not supported.";
			entries.insert(name, i);
		}
		auto readEntry = [&archive, &entries](const QString& name, zip_uint64_t limit, QByteArray& out) -> bool {
			if (!entries.contains(name)) return false;
			zip_uint64_t index = entries.value(name);
			zip_stat_t st; zip_stat_init(&st);
			if (zip_stat_index(archive.get(), index, 0, &st) != 0 || !(st.valid & ZIP_STAT_SIZE) || st.size > limit) return false;
			zip_file_t* f = zip_fopen_index(archive.get(), index, 0);
			if (!f) return false;
			out.resize(static_cast<int>(st.size));
			zip_uint64_t pos = 0;
			while (pos < st.size) {
				zip_int64_t n = zip_fread(f, out.data() + pos, st.size - pos);
				if (n <= 0) { zip_fclose(f); return false; }
				pos += static_cast<zip_uint64_t>(n);
			}
			return zip_fclose(f) == 0;
		};
		// Read the currently selected Minecraft asset archive without extracting it.
		std::string mcName = minecraftZipPath.ToStdString();
		std::unique_ptr<zip_t, decltype(&zip_discard)> mcArchive(zip_open(mcName.c_str(), ZIP_RDONLY, &zipError), zip_discard);
		QMap<QString, zip_uint64_t> mcEntries;
		if (mcArchive) {
			zip_int64_t n = zip_get_num_entries(mcArchive.get(), 0);
			if (n < 0 || n > 100000) return "ERROR: Invalid Minecraft asset archive.";
			for (zip_uint64_t i = 0; i < static_cast<zip_uint64_t>(n); ++i) {
				const char* raw = zip_get_name(mcArchive.get(), i, 0);
				if (!raw) return "ERROR: Invalid Minecraft asset index.";
				QString name = QString::fromUtf8(raw);
				if (mcEntries.contains(name)) return "ERROR: Duplicate Minecraft asset entries.";
				mcEntries.insert(name, i);
			}
		}
		auto readMc = [&mcArchive, &mcEntries](const QString& name, zip_uint64_t limit, QByteArray& out) -> bool {
			if (!mcArchive || !mcEntries.contains(name)) return false;
			zip_uint64_t index = mcEntries.value(name); zip_stat_t st; zip_stat_init(&st);
			if (zip_stat_index(mcArchive.get(), index, 0, &st) != 0 || !(st.valid & ZIP_STAT_SIZE) || st.size > limit) return false;
			zip_file_t* f = zip_fopen_index(mcArchive.get(), index, 0); if (!f) return false;
			out.resize(static_cast<int>(st.size)); zip_uint64_t pos = 0;
			while (pos < st.size) { zip_int64_t n = zip_fread(f, out.data()+pos, st.size-pos); if (n <= 0) { zip_fclose(f); return false; } pos += static_cast<zip_uint64_t>(n); }
			return zip_fclose(f) == 0;
		};
		auto readJson = [&readEntry](const QString& path, QJsonObject& out) -> bool {
			QByteArray bytes;
			if (!readEntry(path, 1048576, bytes)) return false;
			QJsonParseError error;
			QJsonDocument doc = QJsonDocument::fromJson(bytes, &error);
			if (error.error != QJsonParseError::NoError || !doc.isObject()) return false;
			out = doc.object(); return true;
		};
		QJsonObject metadata;
		if (entries.contains("fabric.mod.json")) {
			if (!readJson("fabric.mod.json", metadata) || !metadata.value("id").isString()) return "ERROR: Valid Fabric metadata not found.";
			if (metadata.value("id").toString() == "cobblemon") return mod_browse_cobblemon(entries, readEntry);
		} else {
			QByteArray forgeData;
			if (!readEntry("META-INF/mods.toml",262144,forgeData)) return "ERROR: Valid Fabric or Forge metadata not found.";
			QList<ForgeModInfo> forgeMods=mod_forge_metadata(forgeData);
			if (forgeMods.isEmpty() || forgeMods.first().id.isEmpty()) return "ERROR: Forge mod ID could not be read.";
			for(const ForgeModInfo& mod:forgeMods) if(mod.id=="cobblemon") return mod_browse_cobblemon(entries, readEntry);
		}
		const QRegularExpression idPattern("^([a-z0-9_.-]+):([a-z0-9_./-]+)$");
		auto resourcePath = [&idPattern](const QString& id, const QString& folder, const QString& suffix) -> QString {
			if (id.size() > 240) return QString();
			QString full = id.contains(':') ? id : "minecraft:" + id;
			auto match = idPattern.match(full);
			if (!match.hasMatch()) return QString();
			QString path = match.captured(2);
			for (const QString& segment : path.split('/'))
				if (segment.isEmpty() || segment == "." || segment == "..") return QString();
			return "assets/" + match.captured(1) + "/" + folder + "/" + path + suffix;
		};
		QStringList models;
		const QRegularExpression modelPattern("^assets/([a-z0-9_.-]+)/models/(block/[a-z0-9_./-]+)\\.json$");
		for (auto it = entries.cbegin(); it != entries.cend(); ++it) {
			auto match = modelPattern.match(it.key());
			if (match.hasMatch()) {
				QString id = match.captured(1) + ":" + match.captured(2);
				if (!resourcePath(id, "models", ".json").isEmpty()) models.append(id);
			}
		}
		if (models.isEmpty()) return "ERROR: No block model JSON files found in this JAR.";
		if (models.size() > 10000) return "ERROR: Too many models in this JAR.";
		models.sort(Qt::CaseInsensitive);
		auto prepare = [&resourcePath, &readJson, &readEntry, &entries, &readMc, &mcEntries](const QString& selected) -> StringType {
			QString current = selected;
			QSet<QString> visited;
			QList<QJsonObject> layers;
			for (int depth = 0; ; ++depth) {
				if (depth >= 32 || visited.contains(current)) return "ERROR: Model parent cycle or depth limit.";
				visited.insert(current);
				QString path = resourcePath(current, "models", ".json");
				QJsonObject layer;
				if (path.isEmpty() || !readJson(path, layer)) return "ERROR: Model parent missing from JAR or invalid (Minecraft/dependency assets are not supported yet).";
				// Framework open_model stores static cuboids in "components".
				// Other loaders may implement arbitrary semantics and are not executed.
				if (layer.contains("loader") || layer.contains("fabric:type")) {
					QString loader = layer.value("loader").toString();
					QString fabricType = layer.value("fabric:type").toString();
					if ((!loader.isEmpty() && loader != "framework:open_model") ||
					    (!fabricType.isEmpty() && fabricType != "framework:open_model") ||
					    (loader.isEmpty() && fabricType.isEmpty()) ||
					    !layer.value("components").isArray() || layer.contains("elements"))
						return "ERROR: This custom model loader is not supported.";
					layer.insert("elements", layer.value("components"));
					layer.remove("components"); layer.remove("loader"); layer.remove("fabric:type");
				}
				layers.append(layer);
				if (!layer.contains("parent")) break;
				if (!layer.value("parent").isString()) return "ERROR: Invalid model parent.";
				current = layer.value("parent").toString();
			}
			QJsonObject merged, textureRefs;
			for (auto it = layers.crbegin(); it != layers.crend(); ++it) {
				QJsonObject obj = *it;
				if (obj.contains("textures") && !obj.value("textures").isObject()) return "ERROR: Unsupported texture map.";
				QJsonObject textures = obj.value("textures").toObject();
				for (auto t = textures.begin(); t != textures.end(); ++t) textureRefs.insert(t.key(), t.value());
				for (auto p = obj.begin(); p != obj.end(); ++p)
					if (p.key() != "parent" && p.key() != "textures") merged.insert(p.key(), p.value());
			}
			if (!merged.value("elements").isArray()) return "ERROR: This model has no static geometry.";
			QJsonArray elements = merged.value("elements").toArray();
			if (elements.isEmpty() || elements.size() > 1024) return "ERROR: Empty or excessively complex geometry.";
			auto numbers = [](const QJsonValue& v, int n) -> bool {
				if (!v.isArray()) return false;
				QJsonArray a = v.toArray(); if (a.size() != n) return false;
				for (const auto& x : a) if (!x.isDouble() || !std::isfinite(x.toDouble()) || std::abs(x.toDouble()) > 4096) return false;
				return true;
			};
			QSet<QString> needed, tinted, plain;
			for (const QJsonValue& value : elements) {
				if (!value.isObject()) return "ERROR: Invalid geometry element.";
				QJsonObject element = value.toObject();
				if (!numbers(element.value("from"), 3) || !numbers(element.value("to"), 3) || !element.value("faces").isObject()) return "ERROR: Unsupported geometry.";
				// Mine-imator's block loader supports the standard Minecraft single-axis
				// rotation with an origin and optional perpendicular rescaling.
				if (element.contains("rotation")) {
					if (!element.value("rotation").isObject()) return "ERROR: Invalid element rotation.";
					QJsonObject rotation = element.value("rotation").toObject();
					QString axis = rotation.value("axis").toString();
					if (axis != "x" && axis != "y" && axis != "z") return "ERROR: Unsupported rotation axis.";
					if (!rotation.value("angle").isDouble()) return "ERROR: Invalid rotation angle.";
					double angle = rotation.value("angle").toDouble();
					// Native loader accepts arbitrary single-axis angles; guard rescale near 90 degrees.
					if (!std::isfinite(angle) || std::abs(angle) > 360 || (rotation.value("rescale").toBool() && std::abs(std::cos(angle * 3.141592653589793 / 180.0)) < 0.01)) return "ERROR: Unsupported rotation angle.";
					if (rotation.contains("origin") && !numbers(rotation.value("origin"), 3)) return "ERROR: Invalid rotation origin.";
					if (rotation.contains("rescale") && !rotation.value("rescale").isBool()) return "ERROR: Invalid rotation rescale flag.";
				}
				if (element.contains("shade") && element.value("shade").isBool() && !element.value("shade").toBool()) return "ERROR: Unshaded elements are not supported yet.";
				QJsonObject faces = element.value("faces").toObject();
				if (faces.isEmpty()) return "ERROR: Empty model faces.";
				for (auto f = faces.begin(); f != faces.end(); ++f) {
					if (f.key() != "north" && f.key() != "south" && f.key() != "east" && f.key() != "west" && f.key() != "up" && f.key() != "down") return "ERROR: Unsupported face direction.";
					if (!f.value().isObject()) return "ERROR: Invalid face.";
					QJsonObject face = f.value().toObject();
					if (!face.value("texture").isString() || !face.value("texture").toString().startsWith('#')) return "ERROR: Unsupported face texture reference.";
					if (face.contains("uv")) {
						if (!numbers(face.value("uv"), 4)) return "ERROR: Unsupported face UV.";
						for (const QJsonValue& coord : face.value("uv").toArray())
							if (coord.toDouble() < 0 || coord.toDouble() > 16) return "ERROR: Out-of-range UV is not supported yet.";
					}
					QString textureKey = face.value("texture").toString().mid(1);
					if (face.contains("tintindex")) {
						if (!face.value("tintindex").isDouble() || face.value("tintindex").toDouble() != 0) return "ERROR: This tint index is not supported yet.";
						tinted.insert(textureKey);
					} else plain.insert(textureKey);
					if (face.contains("rotation") && (!face.value("rotation").isDouble() || (face.value("rotation").toDouble() != 0 && face.value("rotation").toDouble() != 90 && face.value("rotation").toDouble() != 180 && face.value("rotation").toDouble() != 270))) return "ERROR: Unsupported UV rotation.";
					needed.insert(face.value("texture").toString().mid(1));
				}
			}
			if (needed.size() > 32) return "ERROR: Too many textures.";
			// Give tinted faces their own texture slot if the same slot is also used
			// without tint. Never modify the source archive or the original image.
			const QSet<QString> originalTinted = tinted;
			for (const QString& key : originalTinted) if (plain.contains(key)) {
				QString alias = "__mi_tint_" + QString::number(textureRefs.size());
				while (textureRefs.contains(alias) || needed.contains(alias)) alias += "_";
				textureRefs.insert(alias, "#" + key); needed.insert(alias);
				for (int i=0; i<elements.size(); ++i) {
					QJsonObject element = elements[i].toObject(); QJsonObject faces = element.value("faces").toObject();
					for (auto it=faces.begin(); it!=faces.end(); ++it) {
						QJsonObject face=it.value().toObject();
						if (face.contains("tintindex") && face.value("texture").toString() == "#" + key) {
							face.insert("texture", "#" + alias); it.value()=face;
						}
					}
					element.insert("faces", faces); elements[i]=element;
				}
				tinted.remove(key); tinted.insert(alias);
			}
			if (needed.size() > 32) return "ERROR: Too many texture slots after tint separation.";
			QJsonObject localTextures;
			QMap<QString, QByteArray> images;
			int imageNo = 0;
			qint64 totalImageBytes = 0;
			for (const QString& key : needed) {
				QString ref = key; QSet<QString> seen;
				while (true) {
					if (seen.contains(ref)) return "ERROR: Cyclic texture reference: #" + ref;
					if (!textureRefs.value(ref).isString()) return "ERROR: Missing texture #" + ref + ". This may be a template; try a named variant instead.";
					seen.insert(ref);
					QString value = textureRefs.value(ref).toString();
					if (value.startsWith('#')) { ref = value.mid(1); continue; }
					QString path = resourcePath(value, "textures", ".png"); QByteArray bytes;
					if (path.isEmpty()) return "ERROR: Invalid texture identifier.";
					bool fromMinecraft = false;
					if (entries.contains(path)) {
						if (!readEntry(path, 8388608, bytes)) return "ERROR: Texture is invalid or too large.";
					} else {
						if (!path.startsWith("assets/minecraft/") || !readMc(path, 8388608, bytes)) return "ERROR: Texture missing from mod and current Minecraft assets.";
						fromMinecraft = true;
					}
					totalImageBytes += bytes.size();
					if (totalImageBytes > 33554432) return "ERROR: Total textures are too large.";
					QString metaPath = path + ".mcmeta"; QByteArray metaBytes;
					bool hasMeta = fromMinecraft ? mcEntries.contains(metaPath) : entries.contains(metaPath);
					if (hasMeta) {
						bool ok = fromMinecraft ? readMc(metaPath, 262144, metaBytes) : readEntry(metaPath, 262144, metaBytes);
						QJsonParseError err; QJsonDocument meta = QJsonDocument::fromJson(metaBytes, &err);
						if (!ok || err.error != QJsonParseError::NoError || !meta.isObject()) return "ERROR: Invalid texture metadata.";
						if (meta.object().contains("animation")) return "ERROR: Animated textures are not supported yet.";
					}
					QBuffer buffer(&bytes); if (!buffer.open(QIODevice::ReadOnly)) return "ERROR: Invalid texture.";
					QImageReader reader(&buffer, "png"); QSize size = reader.size();
					if (!size.isValid() || size.width() <= 0 || size.height() <= 0 || size.width() > 2048 || size.height() > 2048) return "ERROR: Unsupported PNG dimensions (maximum 2048 per side).";
					QImage image = reader.read(); if (image.isNull()) return "ERROR: Invalid PNG texture.";
					bool changedImage = false;
					// Mine-imator pads rectangular textures to a square. Instead, scale the
					// copy to a square so normalised Minecraft UVs retain their appearance.
					if (size.width() != size.height()) {
						int side = std::max(size.width(), size.height());
						image = image.scaled(side, side, Qt::IgnoreAspectRatio, Qt::FastTransformation);
						if (image.isNull()) return "ERROR: Cannot resize texture.";
						changedImage = true;
					}
					// Other tint sources depend on game/mod logic. Leave them untinted
					// rather than invent a colour (the dialog warns about this fallback).
					if (tinted.contains(key) && path == "assets/minecraft/textures/block/acacia_leaves.png") {
						QByteArray colorBytes; if (!readMc("assets/minecraft/textures/colormap/foliage.png", 1048576, colorBytes)) return "ERROR: Minecraft foliage colormap is missing.";
						QBuffer colorBuffer(&colorBytes); colorBuffer.open(QIODevice::ReadOnly); QImageReader colorReader(&colorBuffer, "png");
						if (colorReader.size() != QSize(256,256)) return "ERROR: Invalid foliage colormap.";
						QImage colors = colorReader.read(); if (colors.isNull()) return "ERROR: Invalid foliage colormap.";
						QColor tint = colors.pixelColor(50,173); // 26.2 plains foliage; an approximation for standalone models.
						image = image.convertToFormat(QImage::Format_ARGB32);
						for (int y=0; y<image.height(); ++y) for (int x=0; x<image.width(); ++x) {
							QColor px = image.pixelColor(x,y);
							px.setRgb(px.red()*tint.red()/255, px.green()*tint.green()/255, px.blue()*tint.blue()/255, px.alpha()); image.setPixelColor(x,y,px);
						}
						changedImage = true;
					}
					if (changedImage) {
						bytes.clear(); QBuffer out(&bytes); if (!out.open(QIODevice::WriteOnly) || !image.save(&out,"PNG") || bytes.size() > 8388608) return "ERROR: Cannot encode converted texture.";
					}
					QString local = QString("mod_texture_%1").arg(imageNo++);
					images.insert(local + ".png", bytes); localTextures.insert(key, local);
					break;
				}
			}
			// Tint is baked into a copied texture; keep the original JAR untouched.
			if (!tinted.isEmpty()) {
				for (int i=0; i<elements.size(); ++i) {
					QJsonObject element = elements[i].toObject(); QJsonObject faces = element.value("faces").toObject();
					for (auto it=faces.begin(); it!=faces.end(); ++it) { QJsonObject f=it.value().toObject(); f.remove("tintindex"); it.value()=f; }
					element.insert("faces", faces); elements[i]=element;
				}
				merged.insert("elements", elements);
			}
			merged.insert("textures", localTextures);
			QTemporaryDir temp(QDir::tempPath() + "/MineimatorModModel-XXXXXX");
			if (!temp.isValid()) return "ERROR: Cannot create temporary model directory.";
			auto write = [&temp](const QString& name, const QByteArray& data) -> bool {
				QFile file(temp.filePath(name));
				return file.open(QIODevice::WriteOnly | QIODevice::NewOnly) && file.write(data) == data.size() && file.flush();
			};
			for (auto it = images.cbegin(); it != images.cend(); ++it)
				if (!write(it.key(), it.value())) return "ERROR: Cannot save temporary texture.";
			QString shortName = selected.section('/', -1).left(80);
			QString outputName = shortName + "-" + QFileInfo(temp.path()).fileName().right(6) + ".json";
			if (!write(outputName, QJsonDocument(merged).toJson(QJsonDocument::Indented))) return "ERROR: Cannot save temporary model.";
			QString outputPath = temp.filePath(outputName);
			temp.setAutoRemove(false);
			return outputPath;
		};
		QDialog dialog;
		dialog.setWindowTitle("Select a static mod model");
		dialog.resize(1000, 570);
		dialog.setStyleSheet(
			"QDialog { background: #17191d; color: #e5e8ef; font-size: 13px; }"
			"QLabel { color: #b8c1d0; padding: 5px; }"
			"QLineEdit, QListWidget { background: #20242b; color: #f1f4f8; border: 1px solid #414954; border-radius: 4px; padding: 7px; }"
			"QListWidget::item { padding: 5px; }"
			"QListWidget::item:selected { background: #476da9; color: white; }"
			"QPushButton { background: #344c72; color: white; border: 1px solid #6286bc; border-radius: 4px; padding: 7px 18px; }"
			"QPushButton:hover { background: #486a9c; }");
		QVBoxLayout layout(&dialog);
		QLabel caption("Select a model to see an experimental preview. Some entries are parts, variants, or unsupported models.");
		caption.setWordWrap(true); layout.addWidget(&caption);
		QHBoxLayout* body = new QHBoxLayout(); layout.addLayout(body);
		QVBoxLayout* left = new QVBoxLayout(); body->addLayout(left, 3);
		QLineEdit search; search.setPlaceholderText("Search models..."); left->addWidget(&search);
		QListWidget list; list.addItems(models); left->addWidget(&list);
		QVBoxLayout* right = new QVBoxLayout(); body->addLayout(right, 2);
		QLabel previewTitle("Preview"); right->addWidget(&previewTitle);
		QLabel preview("Select a model from the list");
		preview.setAlignment(Qt::AlignCenter); preview.setWordWrap(true);
		preview.setMinimumSize(300, 300);
		preview.setStyleSheet("QLabel { background: #20242b; color: #b8c1d0; border: 1px solid #414954; border-radius: 4px; }");
		right->addWidget(&preview, 1);
		QLabel previewNote("Approximate preview. Acacia leaves use plains tint; unknown tints are shown without colour."); previewNote.setWordWrap(true); right->addWidget(&previewNote);
		QPushButton openBlockbench("Open copy in Blockbench..."); right->addWidget(&openBlockbench);
		QDialogButtonBox buttons(QDialogButtonBox::Ok | QDialogButtonBox::Cancel); layout.addWidget(&buttons);
		QTimer previewTimer; previewTimer.setSingleShot(true); previewTimer.setInterval(200);
		QObject::connect(&list, &QListWidget::currentItemChanged, &previewTimer, [&preview, &previewTimer](QListWidgetItem* item, QListWidgetItem*) {
			previewTimer.stop(); preview.clear();
			if (!item || item->isHidden()) { preview.setText("Select a model from the list"); return; }
			preview.setText("Loading preview..."); previewTimer.start();
		});
		QObject::connect(&previewTimer, &QTimer::timeout, &dialog, [&list, &preview, &prepare]() {
			QListWidgetItem* item = list.currentItem();
			if (!item || item->isHidden()) { preview.setText("Select a model from the list"); return; }
			QString path = prepare(item->text()).QStr();
			if (path.startsWith("ERROR:")) { preview.setText(path); return; }
			QImage image = mod_render_preview(path);
			// Remove only the unique temporary folder created for this preview.
			QFileInfo info(path); QDir parent = info.absoluteDir();
			if (parent.dirName().startsWith("MineimatorModModel-")) parent.removeRecursively();
			if (image.isNull()) { preview.setText("Preview unavailable for this model. You may still try importing it."); return; }
			preview.setPixmap(QPixmap::fromImage(image));
		});
		QObject::connect(&search, &QLineEdit::textChanged, &list, [&list](const QString& text) {
			QListWidgetItem* first = nullptr;
			for (int i = 0; i < list.count(); ++i) {
				QListWidgetItem* item = list.item(i);
				bool visible = item->text().contains(text, Qt::CaseInsensitive);
				item->setHidden(!visible);
				if (visible && !first) first = item;
			}
			list.setCurrentItem(first);
		});
		QObject::connect(&openBlockbench, &QPushButton::clicked, &dialog, [&dialog, &list, &prepare]() {
			QListWidgetItem* item = list.currentItem();
			if (!item || item->isHidden()) { QMessageBox::information(&dialog, "Blockbench", "Select a model first."); return; }
			QString exe = QFileDialog::getOpenFileName(&dialog, "Select installed Blockbench executable", "", "Applications (*.exe)");
			if (exe.isEmpty()) return;
			QString modelPath = prepare(item->text()).QStr();
			if (modelPath.startsWith("ERROR:")) { QMessageBox::warning(&dialog, "Blockbench", modelPath); return; }
			QFileInfo info(modelPath); QDir folder=info.absoluteDir();
			QString bbPath=mod_make_blockbench_project(modelPath);
			if (bbPath.isEmpty()) {
				if (folder.dirName().startsWith("MineimatorModModel-")) folder.removeRecursively();
				QMessageBox::warning(&dialog, "Blockbench", "Cannot export this model to Blockbench. Explicit face UVs are required."); return;
			}
			if (!QProcess::startDetached(exe, QStringList{bbPath}, folder.absolutePath())) {
				if (folder.dirName().startsWith("MineimatorModModel-")) folder.removeRecursively();
				QMessageBox::warning(&dialog, "Blockbench", "Could not start Blockbench. Check the selected program."); return;
			}
			QMessageBox::information(&dialog, "Blockbench", "A temporary copy was opened. In Blockbench use Save As to keep your changes; they are not automatically imported back.");
		});
		QObject::connect(&buttons, &QDialogButtonBox::accepted, &dialog, [&dialog, &list]() {
			if (list.currentItem() && !list.currentItem()->isHidden()) dialog.accept();
		});
		QObject::connect(&buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
		QObject::connect(&list, &QListWidget::itemDoubleClicked, &dialog, [&dialog](QListWidgetItem*) { dialog.accept(); });
		if (App->ExecDialog(&dialog) != QDialog::Accepted || !list.currentItem()) return "";
		return prepare(list.currentItem()->text());
	}

	RealType lib_unzip(StringType src, StringType dst)
	{
		int err;
		std::string srcStd = src.ToStdString();
		struct zip* za = zip_open(srcStd.c_str(), 0, &err);
		if (!za)
		{
			WARNING("Could not unzip " + src);
			return -1;
		}

		IntType numEntries = zip_get_num_entries(za, 0);
		IntType files = 0, numFiles = numEntries;
		for (int i = 0; i < numEntries; i++)
		{
			struct zip_stat sb;
			if (zip_stat_index(za, i, 0, &sb))
			{
				WARNING("Could not extract file " + StringType(sb.name));
				continue;
			}

			QString fileName = dst + sb.name;
			QFileInfo info(fileName);
			if (fileName.endsWith("/")) // Skip directories
			{
				numFiles--;
				continue;
			}

			// Create path of directories to file if needed
			if (!QDir(info.path()).exists())
				if (!QDir().mkpath(info.path()))
					WARNING("Could not create path " + info.path());

			// Open destination file for writing
			struct zip_file* zf = zip_fopen_index(za, i, 0);
			QFile file(fileName);
			AddPerms(file);

			if (!zf || !file.open(QFile::WriteOnly))
			{
				WARNING("Could not extract file " + QString(sb.name) + ": " + file.errorString());
				continue;
			}

			int sum = 0;
			bool readErr = false;
			while (sum != sb.size)
			{
				char buf[1024];
				int readNum = zip_fread(zf, buf, sizeof(buf));
				if (readNum < 0)
				{
					WARNING("Could not extract file " + StringType(sb.name));
					readErr = true;
					break;
				}
				if (file.write(buf, readNum) < 0)
				{
					WARNING("Could not extract file " + StringType(sb.name));
					readErr = true;
					break;
				}
				sum += readNum;
			}

			zip_fclose(zf);
			if (!readErr)
				files++;
		}

		if (zip_close(za) < 0)
			return -1;
		
		DEBUG("Extracted " + NumStr(files) + "/" + NumStr(numFiles) + " files");
		return files == numFiles;
	}

	RealType lib_gzunzip(StringType src, StringType dst)
	{
		Gzip::Decompress(src, dst);
		return 0;
	}

	RealType lib_file_rename(StringType src, StringType dst)
	{
		QDir srcDir(src);
		if (srcDir.exists()) // Directory rename
		{
			QDir dstDir(dst);
			if (dstDir.exists() && !dstDir.removeRecursively())
				WARNING("Could not remove directory " + dst);

			BoolType ok = srcDir.rename(src, dst);
			if (!ok)
				WARNING("Could not rename directory " + src);

			return ok;
		}

		// File rename
		QFile srcFile(src);
		if (!srcFile.exists())
			return false;

		QFile dstFile(dst);
		if (dstFile.exists())
		{
			AddPerms(dstFile);
			if (!dstFile.remove())
				WARNING("Could not delete file " + dst.QStr() + ": " + dstFile.errorString());
		}

		AddPerms(srcFile);
		BoolType ok = srcFile.rename(dst);
		if (!ok)
			WARNING("Could not rename file " + src.QStr() + ": " + srcFile.errorString());
		return ok;
	}

	RealType lib_file_copy(StringType src, StringType dst)
	{
		QFile srcFile(src);
		if (!srcFile.exists())
			return false;

		QFile dstFile(dst);
		if (dstFile.exists())
		{
			AddPerms(dstFile);
			if (!dstFile.remove(dst))
				WARNING("Could not delete file " + dst.QStr() + ": " + srcFile.errorString());
		}

		AddPerms(srcFile);
		BoolType ok = srcFile.copy(dst);
		if (!ok)
			WARNING("Could not copy file " + src.QStr() + ": " + srcFile.errorString());
		return ok;
	}

	RealType lib_file_delete(StringType fn)
	{
		QFile file(fn);
		if (!file.exists())
			return false;

		AddPerms(file);
		BoolType ok = file.remove();
		if (!ok)
			WARNING("Could not delete file " + fn.QStr() + ": " + file.errorString());
		return ok;
	}

	RealType lib_file_exists(StringType file)
	{
		return QFile::exists(file);
	}

	RealType lib_directory_create(StringType dir)
	{
		BoolType ok = QDir().mkpath(dir);
		if (!ok)
			WARNING("Could not create directory " + dir);
		return ok;
	}

	RealType lib_directory_delete(StringType dir)
	{
		BoolType ok = QDir(dir).removeRecursively();
		if (!ok)
			WARNING("Could not delete directory " + dir);
		return ok;
	}

	RealType lib_directory_exists(StringType dir)
	{
		return QDir(dir).exists();
	}

	RealType lib_json_file_convert_unicode(StringType src, StringType dst)
	{
		return lib_file_copy(src, dst);
	}
}