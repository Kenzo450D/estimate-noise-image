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

#include "nrestimate.h"

// C++ includes

#include <cmath>
#include <cfloat>

// Qt includes.

#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <QApplication>

// OpenCV includes

#include <highgui.h>
#include <cvaux.h>
#include <opencv/cv.h>

class NREstimate::Private
{
public:

    Private():
       clusterCount(30)
    {
    }

    const int clusterCount;
    QString   path;
    QString   output;
};

NREstimate::NREstimate(QObject* const parent)
    : QObject(parent), d(new Private)
{
}

NREstimate::~NREstimate()
{
    delete d;
}

void NREstimate::setImagePath(const QString& path)
{
    d->path = path;
}

QString NREstimate::output() const
{
    return d->output;
}

void NREstimate::postProgress(int p, const QString& t)
{
    qDebug() << "\n" << p << "% - " << t << "\n";
    emit signalProgress(p);
    qApp->processEvents();
}

void NREstimate::estimateNoise()
{
    IplImage* img    = cvLoadImage(d->path.toStdString().c_str());

    int i, j;

    // All the pixels are considered in the sample_count.
    int sample_count = (img->width)*(img->height);

    //we need to convert the image to YCrCb Image
    //step1. Identify the color model
    //step2. Convert the image to YCrCb Image
    IplImage* image  = cvCreateImage(cvGetSize(img), img->depth, 3);
    cvCvtColor(img, image, CV_BGR2YCrCb);
    cvReleaseImage(&img);

    CvMat* points    = cvCreateMat(sample_count, image->nChannels, CV_32FC1);

    // matrix to store the index of the clusters
    CvMat* clusters  = cvCreateMat(sample_count, 1, CV_32SC1);

    float* pointsPtr = (float*)points->data.ptr;

    for(int y=0 ; y < image->height ; y++)
    {
        for (int x=0 ; x < image->width ; x++)
        {
            for (int z=0 ; z < image->nChannels ; z++)
            {
                *pointsPtr++ = cvGet2D(image, y, x).val[z];
            }
        }
    }

    CvArr* centers = 0;

    postProgress(10, "Everything ready for the cvKmeans2 or as it seems to");

    //-- KMEANS ---------------------------------------------------------------------------------------------

    cvKMeans2(points, d->clusterCount, clusters, cvTermCriteria(CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 10, 1.0), 3, 0, 0, centers, 0);

    postProgress(20, "cvKmeans2 succesfully run");

    //-- Divide into cluster->columns, sample->rows, in matrix standard deviation ---------------------------

    int rowPosition[d->clusterCount];

    //the row position array would just make the hold the number of elements in each cluster

    for(i=0 ; i < d->clusterCount ; i++)
    {
        //initializing the cluster count array
        rowPosition[i] = 0;
    }

    int rowIndex, columnIndex;

    for(i=0 ; i < sample_count ; i++)
    {
        columnIndex = clusters->data.i[i];
        rowPosition[columnIndex]++;
    }

/*
    qDebug() << "Lets see what the rowPosition array looks like : ";

    for(i=0 ; i < d->clusterCount ; i++)
    {
        qDebug() << "Cluster : "<<i<<" the count is :"<<rowPosition[i];
    }
*/

    postProgress(30, "array indexed, and ready to find maximum");

    //-- Finding maximum of the rowPosition array ------------------------------------------------------------

    int max = rowPosition[0];

    for(i=1 ; i < d->clusterCount ; i++)
    {
        if(rowPosition[i] > max)
        {
            max = rowPosition[i];
        }
    }

    QString maxString = "";
    maxString.append(QString::number(max));
    cvReleaseImage(&image);   //releasing image to free memory

    postProgress(40, QString("maximum declared = %1").arg(maxString));

    //-- Divide and conquer ---------------------------------------------------------------------------------

    CvMat* sd = cvCreateMat(max, (d->clusterCount * points->cols), CV_32FC1);

    //-- Initialize the rowPosition array -------------------------------------------------------------------

    int rPosition[d->clusterCount];

    for(i=0 ; i < d->clusterCount ; i++)
    {
        rPosition[i]=0;
    }

    float* ptr = (float*)sd->data.ptr;
    int z;

    postProgress(50, "The rowPosition array is ready!");

    for (i=0 ; i < sample_count ; i++)
    {
        columnIndex = clusters->data.i[i];
        rowIndex    = rPosition[columnIndex];

        //moving to the right row
        ptr         = (float*)(sd->data.ptr + rowIndex*(sd->step));

        //moving to the right column
        for(int j=0 ; j < columnIndex ; j++)
        {
            for(z=0 ; z < (points->cols) ; z++)
            {
                ptr++;
            }
        }

        for(z=0 ; z < (points->cols) ; z++)
        {
            *ptr++ = cvGet2D(points, i, z).val[0];
        }

        rPosition[columnIndex] = rPosition[columnIndex] + 1;
    }

    postProgress(60, "sd matrix creation over!");

    //-- This part of the code would involve the sd matrix and make the mean and the std of the data -------------------

    CvScalar std;
    CvScalar mean;
    CvMat* meanStore    = cvCreateMat(d->clusterCount, points->cols, CV_32FC1);
    CvMat* stdStore     = cvCreateMat(d->clusterCount, points->cols, CV_32FC1);
    float* meanStorePtr = (float*)(meanStore->data.ptr);
    float* stdStorePtr  = (float*)(stdStore->data.ptr);

    // The number of non-empty clusters
    int totalcount      = 0;

    for(i=0 ; i < sd->cols ; i++)
    {
        if(rowPosition[(i/points->cols)] >= 1)
        {
            CvMat* workingArr = cvCreateMat(rowPosition[(i/points->cols)], 1, CV_32FC1);
            ptr               = (float*)(workingArr->data.ptr);

            for(j=0 ; j < rowPosition[(i/(points->cols))] ; j++)
            {
                *ptr++ = cvGet2D(sd, j, i).val[0];
            }

            cvAvgSdv(workingArr, &mean, &std);
            *meanStorePtr++ = (float)mean.val[0];
            *stdStorePtr++  = (float)std.val[0];
            totalcount++;
            cvReleaseMat(&workingArr);
        }
    }

    postProgress(70, "Make the mean and the std of the data");

    // -- creating QTextStream (remember to close file) ----------------------------------------------------------------

    QString logFile = d->path;
    logFile         = logFile.section('/', -1);
    logFile         = logFile.left(logFile.indexOf('.'));
    logFile.append("logMeanStd.txt");
    QFile filems (logFile);
    filems.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream ott(&filems);
    meanStorePtr    = (float*)meanStore->data.ptr;
    stdStorePtr     = (float*)stdStore->data.ptr;
    ott << "Mean Data\n";

    for(i=0 ; i < totalcount ; i++)
    {
        ott << *meanStorePtr++;
        ott << "\t";

        if((i+1)%3 == 0)
        {
            ott << "\n";
        }
    }

    ott << "\nStd Data\n";

    for(i=0 ; i < totalcount ; i++)
    {
        ott << *stdStorePtr++;
        ott << "\t";

        if((i+1)%3 == 0)
        {
            ott << "\n";
        }
    }

    filems.close();

    QFile file ("logWeightedMeanStd.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);

    postProgress(80, "Done with the basic work of storing the mean and the std");

    //-- Calculating weighted mean, and weighted std -----------------------------------------------------------

    QString info       = "";
    float weightedMean = 0.0f;
    float weightedStd  = 0.0f;
    float datasd[3];

    for(j=0 ; j < points->cols ; j++)
    {
        meanStorePtr = (float*) meanStore->data.ptr;
        stdStorePtr  = (float*) stdStore->data.ptr;

        for (int moveToChannel=0 ; moveToChannel <= j ; moveToChannel++)
        {
            meanStorePtr++;
            stdStorePtr++;
        }

        for(i=0 ; i < d->clusterCount ; i++)
        {
            if(rowPosition[i] >= 1)
            {
                weightedMean += (*meanStorePtr) * rowPosition[i];
                weightedStd  += (*stdStorePtr)  * rowPosition[i];
                meanStorePtr += points->cols;
                stdStorePtr  += points->cols;
            }
        }

        weightedMean = weightedMean/sample_count;
        weightedStd  = weightedStd/sample_count;
        out << "\nChannel : " << j <<"\n";
        out << "Weighted Mean : " << weightedMean <<"\n";
        out << "Weighted Std  : " << weightedStd <<"\n";
        info.append("\n\nChannel: ");
        info.append(QString::number(j));
        info.append("\nWeighted Mean: ");
        info.append(QString::number(weightedMean));
        info.append("\nWeighted Standard Deviation: ");
        info.append(QString::number(weightedStd));
        datasd[j] = weightedStd;
    }

    postProgress(90, "Info is ready");

    // -- adaptation ---------------------------------------------------------------------------------------

    float L, LSoft, Cr, CrSoft, Cb, CbSoft;
    LSoft = CrSoft = CbSoft = 0.6;

    if(datasd[0] < 7)
        L = datasd[0]-0.98;

    if(datasd[0] >= 7 && datasd[0] < 8)
        L = datasd[0]-1.2;

    if(datasd[0] >= 8 && datasd[0] < 9)
        L = datasd[0]-1.5;
    else
        L = datasd[0]-1.7;

    if(L < 0)
        L = 0;

    if(L > 9)
        L = 9;

    Cr = datasd[1]/2;
    Cb = datasd[1]/2;

    if(Cr > 7)
        Cr = 7;

    if(Cb > 7)
        Cb = 7;

    L  = floorf(L  * 100) / 100;
    Cb = floorf(Cb * 100) / 100;
    Cr = floorf(Cr * 100) / 100;

    d->output = QString();
    d->output.append("\nLum Threshold : ");
    d->output.append(QString::number(L));
    d->output.append("\nLum Softness  : ");
    d->output.append(QString::number(LSoft));
    d->output.append("\nCr Threshold  : ");
    d->output.append(QString::number(Cr));
    d->output.append("\nCr Softness   : ");
    d->output.append(QString::number(CrSoft));
    d->output.append("\nCb Threshold  : ");
    d->output.append(QString::number(Cb));
    d->output.append("\nCb Softness   :");
    d->output.append(QString::number(CbSoft));

    qDebug() <<   "Lum Threshold  = " << L
             << "\nL Softness     = " << LSoft
             << "\nCr Threshold   = " << Cr
             << "\nCr Softness    = " << CrSoft
             << "\nCb  Threshold  = " << Cb
             << "\nCb Softness    = " << CbSoft;

    postProgress(100, "All completed");

    //-- releasing matrices and closing files ----------------------------------------------------------------------

    cvReleaseMat(&sd);
    cvReleaseMat(&stdStore);
    cvReleaseMat(&meanStore);
    cvReleaseMat(&points);
    cvReleaseMat(&clusters);

    file.close();
}
