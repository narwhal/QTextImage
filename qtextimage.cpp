/*
 * Copyright (c) 2015 Narwhal Software s.r.l.
 * www.narwhal.it
 * This software is licensed under an MIT-style license.
 * See the LICENCE file for details.
 */

#include "qtextimage.h"
#include <QMultiHash>
#include <QPainter>
#include <QPoint>
#include <QSharedData>
#include <QString>
#include <QStringList>

namespace {

static const QByteArray &glyphs() {
    static QByteArray g = QByteArray::fromRawData("123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz", 61);
    return g;
}

char next(char c) {
    int index = glyphs().indexOf(c);
    if (index < 0 || index >= glyphs().length()) {
        return '0';
    } else {
        return glyphs()[index+1];
    }
}

inline QVector<QPoint> rectPoints(const QList<QPoint> points) {
    Q_ASSERT(points.length() >= 3);
    int left = points.front().x(), right = points.front().x();
    int top = points.front().y(), bottom = points.front().y();
    foreach (const QPoint &p, points) {
        left = qMin(left, p.x());
        top = qMin(top, p.y());
        right = qMax(right, p.x());
        bottom = qMax(bottom, p.y());
    }
    QVector<QPoint> result;
    result << QPoint(left, top) << QPoint(right, bottom);
    return result;
}

struct Component {
    enum Type {
        Point,
        Line,
        Polygon,
        Ellipse
    };
    char glyph;
    Type type;
    QVector<QPoint> points;
};
}
class QTextImageData : public QSharedData
{
public:
    int rows;
    int columns;
    QVector<Component> components;
};

QTextImage::QTextImage() : data(new QTextImageData)
{
}

QTextImage::QTextImage(const QTextImage &rhs) : data(rhs.data)
{
}

QTextImage &QTextImage::operator=(const QTextImage &rhs)
{
    if (this != &rhs)
        data.operator=(rhs.data);
    return *this;
}

QTextImage::~QTextImage()
{
}

QTextImage QTextImage::parse(const QByteArray &text) {
    QTextImage result;
    int row = 0, columns = 0, column = 0;
    QMultiHash<char, QPoint> points;
    for (int i = 0, end = text.length(); i != end; ++i) {
        if (text[i] != '\n' && QChar(text[i]).isSpace()) continue;
        if (text[i] != '\n') {
            if (glyphs().indexOf(text[i]) >= 0) {
                points.insert(text[i], QPoint(column, row));
            }
            ++column;
        }
        if (text[i] == '\n' || (i == end - 1 && column != 0)){
            if (row == 0) {
                columns = column;
            } else if (column != columns) {
                return result;
            }
            column = 0;
            ++row;
        }
    }


    result.data->rows = row;
    result.data->columns = columns;
    Component polygon{'0', Component::Polygon, QVector<QPoint>()};
    for (int i = 0, end = glyphs().length(); i != end; ++i) {
        char glyph = glyphs()[i];
        QList<QPoint> glyphPoints = points.values(glyph);
        if (!polygon.points.isEmpty()) {
            Q_ASSERT(glyphPoints.size() == 1);
            polygon.points << glyphPoints.front();
            if (points.values(next(glyph)).length() != 1) {
                result.data->components << polygon;
                polygon.points.clear();
            }
            continue;
        }
        switch (glyphPoints.size()) {
        case 0:
            break;
        case 1:
            if (points.values(next(glyph)).length() == 1) {
                polygon = Component{glyph, Component::Polygon,  glyphPoints.toVector()};
            } else {
                result.data->components << Component{glyph, Component::Point, glyphPoints.toVector()};
            }
            break;
        case 2:
            result.data->components << Component{glyph, Component::Line, glyphPoints.toVector()};
            break;
        default:
            result.data->components << Component{glyph, Component::Ellipse, rectPoints(glyphPoints)};
            break;
        }
    }
    return result;
}

bool QTextImage::isValid() const {
    return data->rows && data->columns;
}

QImage QTextImage::render(int scale, const QPen &strokePen, const QBrush &fillBrush) const {
    return render(scale, [strokePen, fillBrush](char , QPainter &painter){
        painter.setPen(strokePen);
        painter.setBrush(fillBrush);
    });
}

QImage QTextImage::render(int scale, std::function<void(char, QPainter &)> lineConfig) const {
    if (!isValid()) return QImage();
    QImage img(data->columns*scale, data->rows*scale, QImage::Format_ARGB32);
    img.fill(Qt::transparent);
    foreach (const Component &component, data->components) {
        QPainter painter(&img);
        painter.scale(scale,scale);
        painter.translate(0.5,0.5);
        painter.setRenderHint(QPainter::Antialiasing );
        lineConfig(component.glyph, painter);
        switch (component.type) {
        case Component::Point:
            painter.drawPoint(component.points.front());
            break;
        case Component::Line:
            painter.drawLine(component.points.front(), component.points.back());
            break;
        case Component::Polygon:
            painter.drawPolygon(component.points.data(), component.points.size());
            break;
        case Component::Ellipse:
            painter.drawEllipse(QRect(component.points.front(), component.points.back())
                                .adjusted(0,0,-painter.pen().width(), -painter.pen().width()));
            break;
        }
    }
    return img;
}
