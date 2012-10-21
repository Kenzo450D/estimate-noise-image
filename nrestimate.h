/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2012-10-18
 * Description : Wavelets YCrCb Noise Reduction settings estimation.
 *
 * Copyright (C) 2012 by Sayantan Datta <sayantan dot knz at gmail dot com>
 * Copyright (C) 2012 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#ifndef NRESTIMATE_H
#define NRESTIMATE_H

// Qt includes

#include <QObject>
#include <QString>

class NREstimate : public QObject
{
Q_OBJECT

public:

    explicit NREstimate(QObject* const parent);
    ~NREstimate();

    void setImagePath(const QString& path);

    void estimateNoise();

    QString output() const;

Q_SIGNALS:

    void signalProgress(int);

private:

    void postProgress(int, const QString&);

private:

    class Private;
    Private* const d;
};

#endif /* NRESTIMATE_H */
