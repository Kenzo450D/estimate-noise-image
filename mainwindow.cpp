#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <highgui.h>
#include <opencv/cv.h>
#include <cfloat>
#include <QLabel>
#include <QFileDialog>
#include <QString>
#include <QMessageBox>
#include <QTextStream>
#include <QFile>
#include <QDebug>
#include <cvaux.h>
#include <QColor>

MainWindow::MainWindow(QWidget *parent) :
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
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "",tr("*.*"));
    if (fileName == "") {
        QMessageBox::critical(this, tr("Error"), tr("Could not open File"));
        return;
    }
    ui->lineEdit->setText(fileName);
    IplImage *img;
    img=cvLoadImage(fileName.toStdString().c_str());
    if (img==0) {
        QMessageBox::critical(this, tr("Error"),tr("Could not load image"));
    }
    int width=img->width;
    int height=img->height;
    int step=img->widthStep;
    int channel=img->nChannels;
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
    file=ui->lineEdit->text();
    IplImage *img;
    img=cvLoadImage(file.toStdString().c_str());
    cvNamedWindow("View Image",CV_WINDOW_AUTOSIZE);
    cvMoveWindow("View Image",200,200);
    cvShowImage("View Image",img);
    cvWaitKey(0);
    cvReleaseImage(&img);
}

void MainWindow::on_pushButton_4_clicked()  //for the "Estimate" Button
{
    const int cluster_count=30;
    IplImage* img = cvLoadImage(ui->lineEdit->text().toStdString().c_str());
//    IplImage* img = cvLoadImage("/home/sayantan/WORK/trial images/twilight_clean.jpg");
    int j, sample_count = (img->width)*(img->height);          //sample_count (here all the pixels are considered in the sample_count)

    //we need to convert the image to YCrCb Image
    //step1. Identify the color model
    //step2. Convert the image to YCrCb Image
    IplImage* image=cvCreateImage(cvGetSize(img),img->depth,3);
    cvCvtColor(img,image,CV_BGR2YCrCb);
    cvReleaseImage(&img);

    CvMat* points = cvCreateMat(sample_count,image->nChannels,CV_32FC1);
    CvMat* clusters = cvCreateMat (sample_count, 1, CV_32SC1); //mat to store the index of the clusters

    int i;
    float* pointsPtr=(float*)points->data.ptr;
    for(int y=0;y<image->height;y++) {
        for (int  x=0; x<image->width;x++) {
            for (int z=0; z<image->nChannels;z++) {
                *pointsPtr++=cvGet2D(image,y,x).val[z];
            }
        }
    }
    CvArr* centers;
    ui->progressBar->setValue(10);
    qDebug() <<"\n5% - Everything ready for the cvKmeans2 or as it seems to\n";

    //______________________________________________________KMEANS

    cvKMeans2( points, cluster_count, clusters, cvTermCriteria( CV_TERMCRIT_EPS+CV_TERMCRIT_ITER, 10, 1.0 ),3,0,0,centers,0);
    ui->progressBar->setValue(30);
    qDebug() << "30% cvKmeans2 succesfully run";

    //_______________________________________________________divide into cluster->columns, sample->rows, in matrix sd

    int rowPosition[cluster_count];
    //the row position array would just make the hold the number of elements in each cluster
    for(i=0;i<cluster_count;i++) {  //initializing the cluster count array
        rowPosition[i]=0;
    }
    int rowIndex, columnIndex;
    for(i=0;i<sample_count;i++) {
        columnIndex=clusters->data.i[i];
        rowPosition[columnIndex]++;
    }
//    qDebug() << "Lets see what the rowPosition array looks like : ";
//    for(i=0;i<cluster_count;i++) {
//        qDebug() << "Cluster : "<<i<<" the count is :"<<rowPosition[i];
//    }
    ui->progressBar->setValue(40);
    qDebug() << "40% - array indexed, and ready to find maximum";
    //________________________________________________________finding maximum of the rowPosition array
    int max=rowPosition[0];
    for(i=1;i<cluster_count;i++) {
        if(rowPosition[i]>max)
            max=rowPosition[i];
    }
    ui->progressBar->setValue(50);
    qDebug() << "50% - maximum declared = ";
    QString maxString="";
    maxString.append(QString::number(max));
    qDebug()<< maxString;
    cvReleaseImage(&image);   //releasing image to free memory






    //________________________________________________________divide and conquer
    CvMat* sd = cvCreateMat(max, (cluster_count * points->cols), CV_32FC1);
    //________________________________________________________initialize the rowPosition array
    int rPosition[cluster_count];
    for(i=0;i<cluster_count;i++) {
        rPosition[i]=0;
    }
    float* ptr=(float*)sd->data.ptr;
    qDebug() << "\nThe rowPosition array is ready!";
    int z;
    for (i=0; i<sample_count; i++) {
        columnIndex=clusters->data.i[i];
        rowIndex=rPosition[columnIndex];
        //moving to the right row
        ptr=(float*)(sd->data.ptr + rowIndex*(sd->step));
        //moving to the right column
        for(int j=0;j<columnIndex;j++) {
            for(z=0;z<(points->cols);z++)
                ptr++;
        }
        for(z=0; z<(points->cols); z++) {
            *ptr++=cvGet2D(points,i,z).val[0];
        }
        rPosition[columnIndex]=rPosition[columnIndex] + 1;

    }
    ui->progressBar->setValue(60);
    qDebug() << "60% - sd matrix creation over!";

//    This part of the code would involve the sd matrix and make the mean and the std of the data
    CvScalar std;
    CvScalar mean;
    CvMat* meanStore=cvCreateMat(cluster_count,points->cols,CV_32FC1);
    CvMat* stdStore=cvCreateMat(cluster_count,points->cols,CV_32FC1);
    float* meanStorePtr = (float*)(meanStore->data.ptr);
    float* stdStorePtr = (float*)(stdStore->data.ptr);
    int totalcount=0;
    for(i=0;i<sd->cols;i++) {
        if(rowPosition[(i/points->cols)]>=1) {
            CvMat* workingArr=cvCreateMat(rowPosition[(i/points->cols)],1,CV_32FC1);
            ptr=(float*)(workingArr->data.ptr);
            for(j=0;j<rowPosition[(i/(points->cols))];j++) {
                *ptr++=cvGet2D(sd,j,i).val[0];
            }
            cvAvgSdv(workingArr,&mean,&std);
            *meanStorePtr++=(float)mean.val[0];
            *stdStorePtr++=(float)std.val[0];
            totalcount++;
            cvReleaseMat(&workingArr);
        }
    }
    // totalcount is the number of non-empty clusters
    //__________________________________________________________creating QTextStream (remember to close file)
    QString logFile = ui->lineEdit->text();
    logFile=logFile.section('/',-1);
    logFile=logFile.left(logFile.indexOf('.'));
    logFile=logFile.prepend("/home/sayantan/WORK/samples/");
    logFile.append("logMeanStd.txt");
    QFile filems (logFile);
    filems.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream ott(&filems);
    meanStorePtr=(float*)meanStore->data.ptr;
    stdStorePtr=(float*)stdStore->data.ptr;
    ott << "Mean Data\n";
    for(i=0;i<totalcount;i++) {
        ott << *meanStorePtr++;
        ott << "\t";
        if((i+1)%3==0)
            ott << "\n";
    }
    ott << "\nStd Data\n";
    for(i=0;i<totalcount;i++) {
        ott << *stdStorePtr++;
        ott << "\t";
        if((i+1)%3==0)
            ott << "\n";
    }
    filems.close();
    ui->progressBar->setValue(80);
    qDebug() << "80% - Done with the basic work of storing the mean and the std";
    QFile file ("/home/sayantan/WORK/samples/logWeightedMeanStd.txt");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);



//    QMessageBox::critical(this, tr("Success"), tr("STD array made!"));
//    return;


    //__________________________________________________________calculating weighted mean, and weighted std
    //__________________________________________________________calculating weighted mean, and weighted std
    QString info="";
    float weightedMean=0.0f, weightedStd=0.0f;
    float datasd[3];
    for(j=0;j<points->cols;j++) {
        meanStorePtr=(float*) meanStore->data.ptr;
        stdStorePtr=(float*) stdStore->data.ptr;
        for (int moveToChannel=0;moveToChannel<=j;moveToChannel++) {
            meanStorePtr++;
            stdStorePtr++;
        }
        for(i=0;i<cluster_count;i++) {
            if(rowPosition[i]>=1) {
                weightedMean += (*meanStorePtr) * rowPosition[i];
                weightedStd += (*stdStorePtr) * rowPosition[i];
                meanStorePtr+=points->cols;
                stdStorePtr+=points->cols;
            }
        }
        weightedMean=weightedMean/sample_count;
        weightedStd=weightedStd/sample_count;
        out << "\nChannel : "<<j<<"\n";
        out << "Weighted Mean : "<<weightedMean<<"\n";
        out << "Weighted Std  : "<<weightedStd<<"\n";
        info.append("\n\nChannel: ");
        info.append(QString::number(j));
        info.append("\nWeighted Mean: ");
        info.append(QString::number(weightedMean));
        info.append("\nWeighted Standard Deviation: ");
        info.append(QString::number(weightedStd));
        datasd[j]=weightedStd;
    }
    ui->textEdit->setText(info);
    ui->progressBar->setValue(90);
    //________________________________________________________adaptation
    float L, LSoft, Cr, CrSoft, Cb, CbSoft;
    LSoft=CrSoft=CbSoft=0.6;
    if(datasd[0]<7)
        L=datasd[0]-0.98;
    if(datasd[0]>=7 && datasd[0]<8)
        L=datasd[0]-1.2;
    if(datasd[0]>=8 && datasd[0]<9)
        L=datasd[0]-1.5;
    else
        L=datasd[0]-1.7;
    if(L<0)
        L=0;
    if(L>9)
        L=9;
    Cr=datasd[1]/2;
    Cb=datasd[1]/2;
    if(Cr>7)
        Cr=7;
    if(Cb>7)
        Cb=7;
    L=floorf(L * 100) / 100;
    Cb=floorf(Cb * 100) / 100;
    Cr=floorf(Cr * 100) / 100;
    QString mainOutput;
    mainOutput.append("\nL          :");
    mainOutput.append(QString::number(L));
    mainOutput.append("\nL Softness : ");
    mainOutput.append(QString::number(LSoft));
    mainOutput.append("\nCr         :");
    mainOutput.append(QString::number(Cr));
    mainOutput.append("\nCr Softness: ");
    mainOutput.append(QString::number(CrSoft));
    mainOutput.append("\nCb         :");
    mainOutput.append(QString::number(Cb));
    mainOutput.append("\nCb Softness:");
    mainOutput.append(QString::number(CbSoft));

    qDebug() << "L= "<<L<<"\nL Softness= "<<LSoft<<"\nCr= "<<Cr<<"\nCr Softness="<<CrSoft<<"\nCb= "<<Cb<<"\nCb Softness="<<CbSoft;
    ui->progressBar->setValue(100);
    ui->textEdit->setText(mainOutput);
    ui->textEdit->setTextBackgroundColor(QColor(1,1,0));

    //________________________________________________________releasing matrices and closing files
    cvReleaseMat(&sd);
    cvReleaseMat(&stdStore);
    cvReleaseMat(&meanStore);
    cvReleaseMat(&points);
    cvReleaseMat(&clusters);

    file.close();

}
