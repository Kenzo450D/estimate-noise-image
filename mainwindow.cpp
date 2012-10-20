/* ============================================================
 *
 * This file is a part of digiKam project
 * http://www.digikam.org
 *
 * Date        : 2012-10-18
 * Description : Wavelets YCrCb Noise Reduction settings estimation.
 *
 * Copyright (C) 2012 by Sayantan Datta <kenzo dotzombie at gmail dot com
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

#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "nrestimate.h"

#include <highgui.h>

#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <QDebug>

MainWindow::MainWindow(QWidget* const parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_pushButton_clicked()    //for the "Open" button
{
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("*.*"));

    if (fileName == "")
    {
        QMessageBox::critical(this, tr("Error"), tr("Could not open File"));
        return;
    }

    ui->lineEdit->setText(fileName);

    IplImage* img = cvLoadImage(fileName.toStdString().c_str());

    if (img==0)
    {
        QMessageBox::critical(this, tr("Error"),tr("Could not load image"));
    }

    int width   = img->width;
    int height  = img->height;
    int step    = img->widthStep;
    int channel = img->nChannels;
    QString info;
    info.append("Width: ");
    info.append(QString::number(width));
    info.append("\nHeight: ");
    info.append(QString::number(height));
    info.append("\nChannels: ");
    info.append(QString::number(channel));
    info.append("\nWidth Step: ");
    info.append(QString::number(step));
    ui->textEdit->setText(info);
    cvReleaseImage(&img);
}

void MainWindow::on_pushButton_2_clicked()  //for the "View" Button
{
    QString file;
    file          = ui->lineEdit->text();
    IplImage* img = cvLoadImage(file.toStdString().c_str());
    cvNamedWindow("View Image", CV_WINDOW_AUTOSIZE);
    cvMoveWindow("View Image", 200, 200);
    cvShowImage("View Image", img);
    cvWaitKey(0);
    cvReleaseImage(&img);
}

void MainWindow::on_pushButton_4_clicked()  //for the "Estimate" Button
{
    NREstimate nre(this);

    connect(&nre, SIGNAL(signalProgress(int)),
            this, SLOT(slotProgress(int)));

    nre.setImagePath(ui->lineEdit->text());
    nre.estimateNoise();

    ui->textEdit->setText(nre.output());
    ui->textEdit->setTextBackgroundColor(QColor(1, 1, 0));
}

void MainWindow::slotProgress(int p)
{
    ui->progressBar->setValue(p);
}
