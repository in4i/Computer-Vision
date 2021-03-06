#include <pxcsensemanager.h>
#include <pxcsession.h>
#include <pxccapturemanager.h>

#include <FaceCoreModule.h>
#include <ImageFormatConverter.h>
#include <iostream>

#define WIDTH 640
#define HEIGHT 480

void drawFaceInfo( cv::Mat img, MFaceInfo info )
{
    PXCRectI32 rect = info.rect;
    int id = (int)info.id;
    cv::rectangle(
        img,
        cv::Rect(rect.x, rect.y, rect.w, rect.h),
        cv::Scalar(0, 0, 255)
    );

    std::stringstream convert;
    convert << id;
    std::string str = convert.str();
    cv::putText( img, cv::String(str), cv::Point(rect.x, rect.y), 3, 1, cv::Scalar(0, 255, 0), 1, 8, 0);

    int length = MAX_LANDMARK;
    for( int p = 0; p < length; p++ ){
        PXCPointF32 point = info.points[p].image;
        cv::circle( img, cv::Point((int)point.x, (int)point.y), 3, cv::Scalar(0, 255, 0), 1, 8, 0);
    }
}

void main()
{
    PXCSenseManager *psm = NULL;
    psm = PXCSenseManager::CreateInstance();
    if ( psm == NULL ){
        std::cout<<"Unabel to create the PXCSenseManager"<<std::endl;
        return;
    }

    std::cout<<"OpenCV Library Version:"<<CV_VERSION<<std::endl;

    PXCSession::ImplVersion version = psm->QuerySession()->QueryVersion();
    std::cout<<"RealSense SDK Version:"<<version.major<<"."<<version.minor<<std::endl;

    psm->EnableStream( PXCCapture::STREAM_TYPE_COLOR, WIDTH, HEIGHT);
    psm->EnableStream( PXCCapture::STREAM_TYPE_DEPTH, WIDTH, HEIGHT);

    MFaceCoreModule *faceHandler = new MFaceCoreModule( psm );
    if( faceHandler == NULL ){
        return;
    }
    faceHandler->initialize();

    // Set the coordinate system
    PXCSession *session = psm->QuerySession();
    session->SetCoordinateSystem( PXCSession::COORDINATE_SYSTEM_REAR_OPENCV );

    if( psm->Init() != PXC_STATUS_NO_ERROR ){
        std::cout<<"Unable to Init the PXCSenseManager"<<std::endl;
        return;
    }

    ImageFormatConverter *converter = new ImageFormatConverter();
    if( converter == NULL ){
        return;
    }

    PXCImage *colorIm, *depthIm;
    cv::Mat colorMat, depthMat;
    while(true){
        if( psm->AcquireFrame(true) < PXC_STATUS_NO_ERROR ){
            break;
        }
        PXCCapture::Sample *sample = psm->QuerySample();
        if( sample ){
            MFaceResult result;
            faceHandler->detect( result );

            if( sample->color ){
                colorIm = sample->color;
                colorMat = converter->convertPXCImageToOpenCVMat( colorIm, ImageFormatConverter::ImageFormat::STREAM_TYPE_COLOR );
            }
            if( sample->depth ){
                depthIm = sample->depth;
                depthMat = converter->convertPXCImageToOpenCVMat( depthIm, ImageFormatConverter::ImageFormat::STREAM_TYPE_DEPTH );
            }

            int counts = result.getFaceCount();
            for( int i = 0; i < counts; i++ ){
                MFaceInfo info = result.getFaceInfo(i);
                drawFaceInfo( colorMat, info );
            }

            cv::imshow("Color Image on OpenCV Format", colorMat);
            cv::imshow("Depth Image on OpenCV Format", depthMat);
            cv::waitKey(1);
        }
        psm->ReleaseFrame();
    }

    colorIm->Release();
    depthIm->Release();
    delete converter;
    delete faceHandler;
    psm->Release();
}
