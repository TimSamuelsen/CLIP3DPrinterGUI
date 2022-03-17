#include "focuscal.h"
#include "ui_focuscal.h"
#include "tl_camera_sdk.h"
#include "tl_camera_sdk_load.h"
#include <QtDebug>
#include "string.h"

#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include "windows.h"

FocusCal::FocusCal(QWidget* parent) :
  QWidget(parent),
  ui(new Ui::FocusCal) {
  ui->setupUi(this);
}

FocusCal::~FocusCal() {
  delete ui;
}

int is_camera_sdk_open = 0;
int is_camera_dll_open =  0;
void* camera_handle = 0;

HANDLE frame_acquired_event = 0;

int report_error_and_cleanup_resources(const char* error_string);
int initialize_camera_resources();
void frame_available_callback(void* sender, unsigned short* image_buffer, int frame_count, unsigned char* metadata, int metadata_size_in_bytes, void* context);
void camera_connect_callback(char* cameraSerialNumber, enum TL_CAMERA_USB_PORT_TYPE usb_bus_speed, void* context);
void camera_disconnect_callback(char* cameraSerialNumber, void* context);

volatile int is_first_frame_finished = 0;

void FocusCal::initCamera() {
  int nErrors = 0;
  if(initialize_camera_resources()) {
    ui->Terminal->append("Failed to init camera resources");
    return;
  }
  // Set the camera connect event callback. This is used to register for run time camera connect events.
  if (tl_camera_set_camera_connect_callback(camera_connect_callback, 0))
    nErrors += report_error_and_cleanup_resources(tl_camera_get_last_error());

  // Set the camera disconnect event callback. This is used to register for run time camera disconnect events.
  if (tl_camera_set_camera_disconnect_callback(camera_disconnect_callback, 0))
    nErrors += report_error_and_cleanup_resources(tl_camera_get_last_error());

  if (tl_camera_disarm(camera_handle))
    printf("Failed to stop the camera!\n");

  nErrors += setExposure(1000);
  nErrors += setGain(33.0, camera_handle);

  // Configure camera for continuous acquisition by setting the number of frames to 0.
  // This project only waits for the first frame before exiting
  if (tl_camera_set_frames_per_trigger_zero_for_unlimited(camera_handle, 1))
    nErrors += report_error_and_cleanup_resources(tl_camera_get_last_error());

  // Set the frame available callback
  if (tl_camera_set_frame_available_callback(camera_handle, frame_available_callback, 0))
    nErrors += report_error_and_cleanup_resources(tl_camera_get_last_error());

  nErrors += SoftwareTrigger();
  printf("nErrors = %d", nErrors);
  ui->Terminal->append("nErrors = " + QString::number(nErrors));
}

int FocusCal::setExposure(long long Exposure) {
  int returnVal = 0;
  if (tl_camera_set_exposure_time(camera_handle, Exposure))
    return report_error_and_cleanup_resources(tl_camera_get_last_error());
  printf("Camera exposure set to %lld\n", Exposure);
  return returnVal;
}

int FocusCal::setGain(double Gain, void* camera_handle) {
  int returnVal = 0;
  int gain_min;
  int gain_max;
  if (tl_camera_get_gain_range(camera_handle, &gain_min, &gain_max)) {
    return report_error_and_cleanup_resources(tl_camera_get_last_error());
  }
  if (gain_max > 0) {
    int gain_index;
    if (tl_camera_convert_decibels_to_gain(camera_handle, Gain, &gain_index)) {
      return report_error_and_cleanup_resources(tl_camera_get_last_error());
    }
    tl_camera_set_gain(camera_handle, gain_index);
  }
  return returnVal;
}

int FocusCal::SoftwareTrigger() {
  int returnVal = 0;

  if (tl_camera_arm(camera_handle, 2)) {
    //TerminalPrint("Camera armed");
    return report_error_and_cleanup_resources(tl_camera_get_last_error());
  }
  printf("Camera armed\n");

  if (tl_camera_issue_software_trigger(camera_handle)) {
    //      TerminalPrint("Software Trigger Error");
    return report_error_and_cleanup_resources(tl_camera_get_last_error());
  }
  printf("Software trigger sent\n");

  // Wait to get an image from the frame available callback
  printf("Waiting for an image...\n");
  ui->Terminal->append("Waiting for an image...");
  for (;;) {
#ifdef _WIN32
    WaitForSingleObject(frame_acquired_event, 5);//INFINITE);
#elif defined __linux__
    if(pthread_mutex_lock(&lock))
      return report_error_and_cleanup_resources("Unable to lock pthread mutex");
    if(pthread_cond_wait(&frame_acquired_event, &lock))
      return report_error_and_cleanup_resources("Unable to wait for frame_acquired_event condition variable");
    if(pthread_mutex_unlock(&lock))
      return report_error_and_cleanup_resources("Unable to unlock pthread mutex");
#endif
    if (is_first_frame_finished) break;
  }

  return returnVal;
}

//void FocusCal:


void FocusCal::on_StageConnectButton_clicked() {
  //USED FOR TESTING RIGHT NOW
  initCamera();
}

void FocusCal::TerminalPrint(QString stringToPrint) {
  ui->Terminal->append(stringToPrint);
}
/*********************************Code from ThorLabs example***************************************/
/*
    Reports the given error string if it is not null and closes any opened resources. Returns the number of errors that occured during cleanup, +1 if error string was not null.
 */
int FocusCal::report_error_and_cleanup_resources(const char* error_string) {
  int num_errors = 0;

  if (error_string) {
    printf("Error: %s\n", error_string);
    num_errors++;
  }

  printf("Closing all resources...\n");

  if (camera_handle) {
    if (tl_camera_close_camera(camera_handle)) {
      printf("Failed to close camera!\n%s\n", tl_camera_get_last_error());
      num_errors++;
    }
    camera_handle = 0;
  }
  if (is_camera_sdk_open) {
    if (tl_camera_close_sdk()) {
      printf("Failed to close SDK!\n");
      num_errors++;
    }
    is_camera_sdk_open = 0;
  }
  if (is_camera_dll_open) {
    if (tl_camera_sdk_dll_terminate()) {
      printf("Failed to close dll!\n");
      num_errors++;
    }
    is_camera_dll_open = 0;
  }

#ifdef _WIN32
  if (frame_acquired_event) {
    if (!CloseHandle(frame_acquired_event)) {
      printf("Failed to close concurrent data structure!\n");
      num_errors++;
    }
  }
#elif defined __linux__
  if(pthread_cond_destroy(&frame_acquired_event)) {
    printf("Failed to destroy condition variable frame_acquired_event!\n");
    num_errors++;
  }
  if(pthread_mutex_destroy(&lock)) {
    printf("Failed to destroy mutex lock!\n");
    num_errors++;
  }
#endif
  printf("Closing resources finished.\n");
  return num_errors;
}

/*
    Initializes camera sdk and opens the first available camera.
    Returns a nonzero value to indicate failure.
 */
int FocusCal::initialize_camera_resources() {
  // Initializes camera dll
  if (tl_camera_sdk_dll_initialize())
    return report_error_and_cleanup_resources("Failed to initialize dll!\n");
  printf("Successfully initialized dll\n");
  is_camera_dll_open = 1;

  // Open the camera SDK
  if (tl_camera_open_sdk())
    return report_error_and_cleanup_resources("Failed to open SDK!\n");
  printf("Successfully opened SDK\n");
  is_camera_sdk_open = 1;

  char camera_ids[1024];
  camera_ids[0] = 0;

  // Discover cameras.
  if (tl_camera_discover_available_cameras(camera_ids, 1024))
    return report_error_and_cleanup_resources(tl_camera_get_last_error());
  printf("camera IDs: %s\n", camera_ids);

  // Check for no cameras.
  if (!strlen(camera_ids))
    return report_error_and_cleanup_resources("Did not find any cameras!\n");

  // Camera IDs are separated by spaces.
  char* p_space = strchr(camera_ids, ' ');
  if (p_space)
    *p_space = '\0'; // isolate the first detected camera
  char first_camera[256];

  // Copy the ID of the first camera to separate buffer (for clarity)
#ifdef _WIN32
  strcpy_s(first_camera, 256, camera_ids);
#elif defined __linux__
  strcpy(first_camera, camera_ids);
#endif
  printf("First camera_id = %s\n", first_camera);

  // Connect to the camera (get a handle to it).
  if (tl_camera_open_camera(first_camera, &camera_handle))
    return report_error_and_cleanup_resources(tl_camera_get_last_error());
  printf("Camera handle = 0x%p\n", camera_handle);

  return 0;
}

// The callback that is registered with the camera
void frame_available_callback(void* sender, unsigned short* image_buffer, int frame_count, unsigned char* metadata, int metadata_size_in_bytes, void* context) {
  if (is_first_frame_finished)
    return;

  qDebug("image buffer = %p\n", image_buffer);
  qDebug("image buffer size = %d\n", sizeof image_buffer);
  qDebug("frame_count = %d\n", frame_count);
  qDebug("meta data buffer = %p\n", metadata);
  qDebug("metadata size in bytes = %d\n", metadata_size_in_bytes);
  is_first_frame_finished = 1;
  // If you need to save the image data for application specific purposes, this would be the place to copy it into separate buffer.
  //imageData = *image_buffer;


  // call get_pending_frame_or_null()
  if (metadata) {
    int metadata_index = 0; // index (in bytes) into metadata
    while (metadata_index < metadata_size_in_bytes) {
      unsigned char* metadata_entry = metadata + metadata_index; // byte pointer to the tag / value pair
      unsigned long value = *(unsigned long*)(metadata_entry + 4);
      qDebug("%.4s: %lu \n", metadata_entry, value);
      // check for any desired tags and operate on value accordingly
      metadata_index += 8; // there are 8 bytes per tag / value pair
    }
  }


  int w, h, d;
  tl_camera_get_image_width(camera_handle, &w);
  tl_camera_get_image_height(camera_handle, &h);
  tl_camera_get_sensor_pixel_size_bytes(camera_handle, &d);
  //unsigned short *output_buffer = 0;
  //output_buffer = (unsigned short *)malloc(sizeof(unsigned short) * h * w); // color image size will be 3x the size of a mono image
  //memcpy(output_buffer, image_buffer, (sizeof(unsigned short) * h * w));


  //unsigned char pixels[w*h*d];
  //memcpy(pixels, image_buffer, w * h * d);
  //cv::Mat mat(h, w, CV_MAKETYPE(cv::DataType<short>::type, 1));
  //memcpy(mat.data, output_buffer, h*w*1 * sizeof(short));

  //int nSize = sizeof(*image_buffer);
  cv::Mat rawData(cv::Size(w, h), CV_16UC1, image_buffer);
  //ushort* pointer_to_data_start = rawData.ptr<ushort>();
  //memcpy(pointer_to_data_start, image_buffer, h * w * sizeof(unsigned short));
  cv::Mat decodedImage  =  cv::imdecode(rawData, cv::IMREAD_GRAYSCALE);
  //cv::imdecode(image_buffer, 0);
  if ( decodedImage.data == NULL ) {
    // Error reading raw image data
  }
  cv::imshow("test", rawData);
  cv::waitKey(0);
#ifdef _WIN32
  SetEvent(frame_acquired_event);
#elif defined __linux__
  pthread_mutex_lock(&lock);
  pthread_cond_broadcast(&frame_acquired_event);
  pthread_mutex_unlock(&lock);
#endif
}

void camera_connect_callback(char* cameraSerialNumber, enum TL_CAMERA_USB_PORT_TYPE usb_bus_speed, void* context) {
  //ui->Terminal->append(sprintf("camera %s connected with bus speed = %d!\n", cameraSerialNumber, usb_bus_speed));
}

void camera_disconnect_callback(char* cameraSerialNumber, void* context) {
  printf("camera %s disconnected!\n", cameraSerialNumber);
}

void readImage() {
  //int    nSize = ...       // Size of buffer
  //uchar* pcBuffer = ...    // Raw buffer data


  // Create a Size(1, nSize) Mat object of 8-bit, single-byte elements
  //Mat rawData( 1, nSize, CV_8UC1, (void*)pcBuffer );

  //Mat decodedImage  =  imdecode( rawData /*, flags */ );
  //if ( //decodedImage.data == NULL )
  //{
  // Error reading raw image data
  //}
}
