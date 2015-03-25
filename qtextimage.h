/*
 * Copyright (c) 2015 Narwhal Software s.r.l.
 * www.narwhal.it
 * This software is licensed under an MIT-style license.
 * See the LICENCE file for details.
 */

#ifndef QTEXTIMAGE_H
#define QTEXTIMAGE_H

#include <QPainter>
#include <QPen>
#include <QSharedDataPointer>
#include <functional>

class QTextImageData;
/*!
 * \brief An ASCII-encoded image.
 *
 * To create a QTextImage use the #parse factory methods.
 *
 * See http://asciimage.org/ for the language reference and other information.
 *
 * QTextImage is implicitely shared (copy-on-write), so you can pass it by value with negligible
 * overhead.
 */
class QTextImage
{
public:
    /*!
     * \brief Constructs a null image.
     */
    QTextImage();
    QTextImage(const QTextImage &);
    QTextImage &operator=(const QTextImage &);
    ~QTextImage();

    /*!
     * \brief Parse a QTextImage from a byte array.
     *
     * Rows will be separated by newline charachters.
     */
    static QTextImage parse(const QByteArray &text);
    /*!
     * \brief Parse a QTextImage from a string.
     *
     * Rows will be separated by newline charachters.
     */
    static QTextImage parse(const QString &text);
    /*!
     * \brief Parse a QTextImage from a list of strings.
     *
     * Each string will be parsed as a different row.
     */
    static QTextImage parse(const QStringList &text);
    /*!
     * \brief Whether this image was parsed from a correct ASCIImage string.
     */
    bool isValid() const;
    /*!
     * \brief Render the image on a QImage
     * \param scale The scale of the rendering (1 = 1px per character)
     * \param strokePen The pen for stroking lines and points
     * \param fillBrush The brush for filling ellipses and polygons
     */
    QImage render(int scale, const QPen &strokePen, const QBrush &fillBrush = QBrush()) const;
    /*!
     * \brief Render the image on a QImage
     * \param scale The scale of the rendering (1 = 1px per character)
     * \param lineConfig A function that will configure the painter for each character
     *
     * For multi-character lines (polygons and ellypses), the first character will be passed to
     * \a lineConfig
     * If the painter is not configured for a character, QPainter default configuration will be used.
     *
     * Sample usage (A black circle with a transparent X in the middle):
       \code{.cpp}
    QTextImage textImage = QTextImage::parse(QStringLiteral(
        ". . . . 1 1 1 . . . .\n"
        ". . 1 . . . . . 1 . .\n"
        ". 1 . . . . . . . 1 .\n"
        "1 . . 2 . . . 3 . . 1\n"
        "1 . . . # . # . . . 1\n"
        "1 . . . . # . . . . 1\n"
        "1 . . . # . # . . . 1\n"
        "1 . . 3 . . . 2 . . 1\n"
        ". 1 . . . . . . . 1 .\n"
        ". . 1 . . . . . 1 . .\n"
        ". . . 1 1 1 1 1 . . .\n"));
    QImage img = textImage.render(2, [](char glyph, QPainter &painter){
        switch (glyph) {
        case '1':
            painter.setBrush(Qt::black);
            break;
        case '2':
        case '3':
            // This will cut a transparent hole in the image
            painter.setCompositionMode(QPainter::CompositionMode_Source);
            painter.setPen(Qt::transparent);
            break;
        }
    });
       \endcode
     */

    QImage render(int scale, std::function<void(char, QPainter &)> lineConfig) const;
private:
    QSharedDataPointer<QTextImageData> data;
};


inline QTextImage QTextImage::parse(const QString &text) {
    return parse(text.toLatin1());
}

inline QTextImage QTextImage::parse(const QStringList &text) {
    return parse(text.join('\n').toLatin1());
}

#endif // QTEXTIMAGE_H
