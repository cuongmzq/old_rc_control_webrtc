#include "video.c"

static MMALCAM_BEHAVIOUR_T camcorder_behaviour;

#define VIEWFINDER_LAYER      2
#define DEFAULT_VIDEO_FORMAT  "640x480:h264";
MMAL_RATIONAL_T DEFAULT_FRAME_RATE = {30, 1};
#define DEFAULT_BIT_RATE      100000
#define DEFAULT_CAM_NUM       0

static VCOS_THREAD_T camcorder_thread;
static int stop;
static uint32_t sleepy_time;
static MMAL_BOOL_T stopped_already;

/*****************************************************************************/
static void test_mmalcam_dump_stats(const char *title, MMAL_PARAMETER_STATISTICS_T* stats)
{
   printf("[%s]\n", title);
   printf("buffer_count: %u\n", stats->buffer_count);
   printf("frame_count: %u\n", stats->frame_count);
   printf("frames_skipped: %u\n", stats->frames_skipped);
   printf("frames_discarded: %u\n", stats->frames_discarded);
   printf("eos_seen: %u\n", stats->eos_seen);
   printf("maximum_frame_bytes: %u\n", stats->maximum_frame_bytes);
   printf("total_bytes_hi: %u\n", (uint32_t)(stats->total_bytes >> 32));
   printf("total_bytes_lo: %u\n", (uint32_t)(stats->total_bytes));
   printf("corrupt_macroblocks: %u\n", stats->corrupt_macroblocks);
}


/*****************************************************************************/
static void *test_mmal_camcorder(void *id)
{
   MMALCAM_BEHAVIOUR_T *behaviour = (MMALCAM_BEHAVIOUR_T *)id;
   int value;

   value = start_recorder(&stop, behaviour);

   LOG_TRACE("Thread terminating, result %d", value);
   return (void *)(uintptr_t)value;
}

/*****************************************************************************/
static void test_signal_handler(int signum)
{
   (void)signum;

   if (stopped_already)
   {
      LOG_ERROR("Killing program");
      exit(255);
   }
   else
   {
      LOG_ERROR("Stopping normally. CTRL+C again to kill program");
      stop = 1;
      stopped_already = 1;
   }
}

int main() {
   VCOS_THREAD_ATTR_T attrs;
   VCOS_STATUS_T status;
   int result = 0;

   vcos_log_register("mmalcam", VCOS_LOG_CATEGORY);
   printf("MMAL Camera Test App\n");

   signal(SIGINT, test_signal_handler);

   camcorder_behaviour.layer = VIEWFINDER_LAYER;
   camcorder_behaviour.vformat = DEFAULT_VIDEO_FORMAT;
   camcorder_behaviour.zero_copy = 1;
   camcorder_behaviour.bit_rate = DEFAULT_BIT_RATE;
   camcorder_behaviour.focus_test = MMAL_PARAM_FOCUS_MAX;
   camcorder_behaviour.camera_num = DEFAULT_CAM_NUM;
    camcorder_behaviour.frame_rate = DEFAULT_FRAME_RATE;
    camcorder_behaviour.uri = "vid.h264";
//    MMAL_STATUS_T status = MMAL_SUCCESS;

    status = vcos_semaphore_create(&camcorder_behaviour.init_sem, "mmalcam-init", 0);
   vcos_assert(status == VCOS_SUCCESS);

    vcos_thread_attr_init(&attrs);
   if (vcos_thread_create(&camcorder_thread, "mmal camcorder", &attrs, test_mmal_camcorder, &camcorder_behaviour) != VCOS_SUCCESS)
   {
      LOG_ERROR("Thread creation failure");
      result = -2;
      goto error;
   }

    vcos_semaphore_wait(&camcorder_behaviour.init_sem);
    if (sleepy_time != 0)
   {
      sleep(sleepy_time);
      stop = 1;
   }

error:
    printf("ENDED");
   LOG_TRACE("Waiting for camcorder thread to terminate");
   vcos_thread_join(&camcorder_thread, NULL);

   test_mmalcam_dump_stats("Render", &camcorder_behaviour.render_stats);
   if (camcorder_behaviour.uri)
      test_mmalcam_dump_stats("Encoder", &camcorder_behaviour.encoder_stats);

   vcos_semaphore_delete(&camcorder_behaviour.init_sem);

//    int result = start_recorder(&stop, &camcorder_behaviour);
//    printf("Stopped");
    return result;
}
