/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#pragma once

#include <QPainter>

class CurveData;

class CurveImage
{
  public:
    CurveImage(QColor gridColor = Qt::darkGray, qreal gridWidth = 1);

    void drawCurve(const CurveData & curve, QColor color = Qt::black, qreal width = 2);
    const QImage & get() const { return image; }

  protected:
    int size;
    QImage image;
    QPainter painter;
};
