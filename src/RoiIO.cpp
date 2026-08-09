#include "RoiIO.h"

#include <QFile>
#include <QTextStream>

namespace RoiIO
{
    bool savePolyTxt(const QString& path, const QPolygonF& poly)
    {
        if (poly.size() < 3)
            return false;

        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
            return false;

        QTextStream out(&f);
        out << poly.size() << "\n";
        for (const QPointF& p : poly)
            out << p.x() << " " << p.y() << "\n";

        f.close();
        return true;
    }
}
