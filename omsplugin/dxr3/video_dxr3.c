#include <sys/types.h>
#include <oms/oms.h>
#include <oms/plugin/output_video.h>

static int _video_dxr3_open(void *plugin, void *name);
static int _video_dxr3_close(void *plugin);

#if 0
static vo_output_video_t video_dxr3 = {
#endif
vo_output_video_t video_dxr3 = {
  open:   _video_dxr3_open,
  close:  _video_dxr3_close,
  config: NULL,
};

static int _video_dxr3_open(void *plugin, void *name)
{
  LOG(LOG_INFO, "_video_dxr3_open");
  return 0;
}

static int _video_dxr3_close(void *plugin)
{
  LOG(LOG_INFO, "_video_dxr3_close");
  return 0;
}
