#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>

#include <oms/oms.h>
#include <oms/plugin/codec.h>

#include "dxr3-api.h"
#include "libac3/ac3.h"

#include <inttypes.h>

extern dxr3_state_t state;

extern int output_spdif(
    uint8_t *data_start, 
    uint8_t *data_end,
    int fd);

static int _dxr3_ac3_open  (void *this, void *foo);
static int _dxr3_ac3_close (void *this);
static int _dxr3_ac3_read  (void *this, buf_t *buf, buf_entry_t *buf_entry);

plugin_codec_t dxr3_ac3 = {
    open:         _dxr3_ac3_open,
    close:        _dxr3_ac3_close,
    read:         _dxr3_ac3_read,
    config: NULL,
};

static int _dxr3_ac3_open  (void *this, void *foo) {
  if (dxr3_get_status() == DXR3_STATUS_CLOSED) {
    dxr3_open("/dev/em8300", "/etc/dxr3.ux");
  }
#if 0 /* let em8300 module defaults determine this */
  dxr3_audio_set_mode(DXR3_AUDIOMODE_DIGITALAC3);
#endif
  ac3dec_init();
  return 0;
}

static int _dxr3_ac3_close (void *this) {
    return 0;
}

static int _dxr3_ac3_read  (void *this, buf_t *buf, buf_entry_t *buf_entry) {
    if (buf_entry->flags & BUF_FLAG_PTS_VALID)
	dxr3_audio_set_pts(buf_entry->pts);
    
    dxr3_audio_get_mode();
    if (state.audiomode==DXR3_AUDIOMODE_DIGITALAC3) {
	return output_spdif(buf_entry->data,
			    buf_entry->data+buf_entry->data_len,
			    state.fd_audio);
    } else {
        if (dxr3_ac3.output != NULL) {
	    return ac3dec_decode_data (dxr3_ac3.output, 
				       buf_entry->data,
				       buf_entry->data+buf_entry->data_len);
	}
    }
    return 0;
}
