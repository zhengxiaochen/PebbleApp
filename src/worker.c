#include <pebble_worker.h> 
#include <stdlib.h>
  
#define BFSIZE 62 //max 57
static const uint32_t AXIS_LOG_TAG = 0x5; 

enum {
  TODO_KEY_APPEND,
  TODO_KEY_DELETE,
  TODO_KEY_MOVE,
  TODO_KEY_TOGGLE_STATE,
  TODO_KEY_FETCH,
};

DictionaryIterator *iter;
Tuple *text_tuple;
char text_buffer[100];
char data_buffer[BFSIZE];
char *p;
char *ctime(const time_t *timep); 
int x[1024];
int y[1024];
int z[1024];
char ax[2];
char ay[2];
char az[2];
char aa[2];
char at[8];

//Define data structure for sending data. 
typedef struct {  
  uint32_t tag; //tag of the log
  DataLoggingSessionRef logging_session; // logging session
  char value[]; //string to write into the logging session
} AxisData;
static AxisData axis_datas; 

//collect data from accelerometer
int i=1;
static void data_handler(AccelData *data, uint32_t num_samples) {
  if (i%3!=0){
    i++;
  }else{
    int16_t angle = atan2_lookup(data[0].z, data[0].x) * 360 / TRIG_MAX_ANGLE - 270; // Calculate arm angle  
    //Change millisecond to readable data time string
    time_t t = data[0].timestamp/1000;
    struct tm * timeinfo=gmtime(&t);
    char timebuffer[BFSIZE];
    char timebuf[15];
    int  end = data[0].timestamp%1000;   
    strftime( timebuffer,BFSIZE,"%H:%M:%S %Y/%m/%d", timeinfo); //"%Y/%m/%d %H:%M:%S"
    strftime( timebuf,15,"%Y%m%d%H%M%S", timeinfo); //"%Y/%m/%d %H:%M:%S"
  
    snprintf(data_buffer, BFSIZE, "{'x':%d,'y':%d,'z':%d,'a':%d,'t':%s%03d}", data[0].x, data[0].y, data[0].z, angle, timebuf,end);
    
    AxisData *axis_data0 = &axis_datas; 
    strcpy(axis_data0->value, data_buffer);  
    //Writing data to logging session
    data_logging_log(axis_data0->logging_session, &axis_data0->value,1);   
      
    if (i==300){
      //Finish a logging session and creat a new one
      AxisData *axis_data = &axis_datas;
      data_logging_finish(axis_data->logging_session); //Finish the old session
      axis_data->logging_session = data_logging_create(AXIS_LOG_TAG, DATA_LOGGING_BYTE_ARRAY, BFSIZE, false); //Create a new session every one minute
         
      i=1;
    }else{
      i++;
    }    
  }
}

static void worker_init() {
  AxisData *axis_data = &axis_datas;   
  axis_data->logging_session = data_logging_create(AXIS_LOG_TAG, DATA_LOGGING_BYTE_ARRAY, BFSIZE, false);  
  
  // Subscribe to the accelerometer data service
  int num_samples = 1;
  accel_data_service_subscribe(num_samples, data_handler);
  // Choose update rate
  accel_service_set_sampling_rate(ACCEL_SAMPLING_10HZ); 
}

static void worker_deinit() {
  AxisData *axis_data = &axis_datas;
  data_logging_finish(axis_data->logging_session);    
  accel_data_service_unsubscribe();
}

int main(void) {
  worker_init();
  worker_event_loop();  
  worker_deinit();
}
