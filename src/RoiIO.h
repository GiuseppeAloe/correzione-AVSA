#pragma once

#include <QString>
#include <QPolygonF>

namespace RoiIO
{
    bool savePolyTxt(const QString& path, const QPolygonF& poly);
}
