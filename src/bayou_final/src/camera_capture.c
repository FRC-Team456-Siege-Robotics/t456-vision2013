/*
**   Capture camera image and write movie
**   Team 456, Vicksburg, MS
**   www.seigerobotics.org
** 
*/

#include "opencv2/highgui/highgui.hpp"
#include "opencv2/legacy/legacy.hpp"
#include <time.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>

#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

#include "camera_info.h"
#include "target_info.h"

#define TRUE 1
#define FALSE 0

/*
**  GLOBAL variables
*/
int           STOP = FALSE;
CvCapture*    camera = 0;
double        fps_sum = 0.0;
int           frame_cnt = 1;
IplImage      *image = 0;

CvFont        font;

char target_message[100];
int  target_message_length;

double time_sum = 0;

camera_struct  camera_info;

tracking_struct tracking;
lut_struct lut_3pt;
lut_struct lut_2pt;

int  pid;
char filename[120];

/*
**  Local function prototypes
*/
void done();

/*
**  External function prototypes
*/
extern void T456_parse_vision( char * );
extern void T456_print_camera_and_tracking_settings();

/*
**  External server function prototypes
*/
void T456_start_server(void);
void T456_stop_server(void);

/*
**  Error trapping
*/

void sig_handler( int signo )
{
   /* signal detected, try to stop the program */
   STOP = TRUE;
}

/*
** ================ BEGIN MAIN SECTION =======================
*/
void camera_capture( int argc, char** argv )
{
    double t1, t2, t3, t4, fps;
    int  targ_selected = 0;
    int  camera_img_height, camera_img_width, camera_img_fps;
    int  waitkey_delay = 2;

    CvMat *image_gray = 0;
    CvMat *image_binary = 0;

    IplConvKernel *morph_kernel;

    CvSeq *contours;

    int i;
    
    CvSize imgSize;
    CvVideoWriter *writer;
    
    /*
    **  See if we can catch signals
    */
    if ( signal(SIGTERM, sig_handler) == SIG_IGN) 
       signal(SIGTERM, SIG_IGN);
    if ( signal(SIGHUP, sig_handler) == SIG_IGN) 
       signal(SIGHUP, SIG_IGN);
    if ( signal(SIGINT, sig_handler) == SIG_IGN) 
       signal(SIGINT, SIG_IGN);

    /*  
    **  Capture images from webcam /dev/video0 
    **   /dev/video0 = 0
    **   /dev/video1 = 1
    */
    if ( argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0])))
    {
       printf(" Capturing image from camera\n");
       camera=cvCaptureFromCAM( argc == 2 ? argv[1][0] - '0' : 0 );
    } 
    else 
    {
       /* 
       **  Capture image from file at command line
       **   useful for playback video and testing
       */
       camera = cvCaptureFromFile( argv[1] );
    }


    /*
    **   Check and see if camera/file capture is valid
    */
    if (!camera) {
        printf("camera or image is null\n");
        return;
    }

    /*
    **  Get camera properties
    */
    camera_img_width = cvGetCaptureProperty(camera, CV_CAP_PROP_FRAME_WIDTH);
    camera_img_height = cvGetCaptureProperty(camera, CV_CAP_PROP_FRAME_HEIGHT);
    camera_img_fps = cvGetCaptureProperty(camera, CV_CAP_PROP_FPS);

    cvSetCaptureProperty( camera, CV_CAP_PROP_EXPOSURE, 500.0);
    cvSetCaptureProperty( camera, CV_CAP_PROP_EXPOSURE, 500.0);
    cvSetCaptureProperty( camera, CV_CAP_PROP_AUTO_EXPOSURE, 0.0);

    imgSize.width = camera_img_width;
    imgSize.height = camera_img_height;

    /*
    **  Parse the config file
    */
    T456_parse_vision( "../config/t456-vision.ini" );
    T456_print_camera_and_tracking_settings();

    /*
    **  Start server listening on port 8080
    */
    T456_start_server();

    printf("camera_img_fps: %d\n", camera_img_fps);
 
    if (camera_img_fps < 0 ) {
       camera_img_fps = 30;
       printf("camera_img_fps: %d\n", camera_img_fps);
    }

    pid = (int) getpid();

    sprintf(filename,"/home/panda/Videos/pts_out_%05d.mjpg",pid);

    writer = cvCreateVideoWriter(
                filename,
                CV_FOURCC('M','J','P','G'),
//                CV_FOURCC('Y','U','Y','2'),
//                CV_FOURCC('A','V','I'),
                camera_img_fps,
                imgSize, 1
             );    

    /*
    **  Time estimation variable
    */
    t1 = (double)cvGetTickCount();

    /*
    **   Process images until key press
    */

    while (1)
    {

       /*
       **  Grab initial frame from image or movie file
       */
       image = cvQueryFrame(camera);
       if ( !image ) {
          done();
          return;
       }

       cvWriteFrame(writer, image);
        
        /*
        **  pass selected target information into target message
        **   for the webservice
        */
        target_message_length =
              snprintf(target_message, sizeof(target_message),
              "%06d,-99,000000,000000,000000,0000", frame_cnt);


        /*  
        ** keep time of processing 
        */
        t2 = (double)cvGetTickCount();
        fps = 1000.0 / ((t2-t1)/(cvGetTickFrequency()*1000.));
        fps_sum = fps_sum + fps;
        frame_cnt++;

        if ( (frame_cnt % 30) == 0 )
           printf("frame: %d time: %gms  fps: %.2g\n",
                    frame_cnt, (t2-t1)/(cvGetTickFrequency()*1000.),fps);

        t1 = t2;

        /*
        **  If we catch a stop or kill signal
        */
        if ( STOP ) {
          break;
        }
    }

    /*
    **  Release camera resources
    */
    cvReleaseVideoWriter(&writer);
    cvReleaseCapture(&camera);

    /*
    **  Print out timing information
    */
    done();

    /*
    **  Stop server listening on port 8080
    */
    T456_stop_server();

}

/*
** ==================== END MAIN SECTION =======================
*/

void done()
{
  printf("total frames: %d\n", frame_cnt);
  printf("average fps: %lf\n", fps_sum / (double)frame_cnt);
}


/*
**  TEST MAIN SECTION
*/

int main(int argc, char **argv)
{

  camera_capture( argc, argv );

  exit(1);
}


