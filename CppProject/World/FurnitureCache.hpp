#pragma once
#include <QByteArray>
#include <QJsonArray>
#include <QSet>
#include <QStringList>

namespace CppProject {
// Fingerprints furniture definitions from instance saves so the mod cache is rebuilt.
QByteArray FurnitureSignature(const QString& instance);
// Produce model JSON and texture pages from saved furniture definitions.
void PrepareFurniture(const QString& instance,const QString& cache,QJsonArray& blocks,
                      QSet<QString>& textures,QSet<QString>& spriteTextures,QStringList& report);
}
