#include <sys/types.h>
#include <oms/oms.h>
#include <oms/plugin/codec.h>
#include <oms/plugin/output_audio.h>
#include <oms/plugin/output_video.h>

extern plugin_codec_t dxr3_mpeg2;
extern plugin_codec_t dxr3_ac3;
extern plugin_codec_t dxr3_spu;
extern plugin_output_audio_t audio_dxr3;
extern vo_output_video_t video_dxr3;

/**
 * Initialize Plugin.
 **/

int plugin_init (char *whoami)
{
  fprintf(stderr, "plugin_init\n");
  pluginRegister (whoami,
		  PLUGIN_ID_CODEC_VIDEO,
		  "mpg2;mpg1",
		  "dxr3",
		  NULL,
		  &dxr3_mpeg2);
  pluginRegister(whoami,
                  PLUGIN_ID_CODEC_AUDIO,
                  "ac3;mpg1;pcm",
                  "dxr3",
                  NULL,
                  &dxr3_ac3);
  pluginRegister (whoami,
		  PLUGIN_ID_CODEC_SPU,
		  "spu",
		  "dxr3",
		  NULL,
		  &dxr3_spu);
  pluginRegister (whoami,
		  PLUGIN_ID_OUTPUT_AUDIO,
		  NULL,
		  "dxr3",
		  NULL,
		  &audio_dxr3);
  pluginRegister (whoami,
		  PLUGIN_ID_OUTPUT_VIDEO,
		  NULL,
		  "dxr3",
		  NULL,
		  &video_dxr3);
  return 0;
}

/**
 * Cleanup Plugin.
 **/

void plugin_exit (void)
{
}
