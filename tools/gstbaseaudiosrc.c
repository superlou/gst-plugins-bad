% ClassName
GstBaseAudioSrc
% TYPE_CLASS_NAME
GST_TYPE_BASE_AUDIO_SRC
% pads
srcpad-simple
% pkg-config
gstreamer-audio-0.10
% includes
#include <gst/audio/gstbaseaudiosrc.h>
% prototypes
static GstRingBuffer *gst_replace_create_ringbuffer (GstBaseAudioSrc * src);
% declare-class
  GstBaseAudioSrcClass *base_audio_src_class = GST_BASE_AUDIO_SRC_CLASS (klass);
% set-methods
  base_audio_src_class->create_ringbuffer = GST_DEBUG_FUNCPTR (gst_replace_create_ringbuffer);
% methods

static GstRingBuffer *
gst_replace_create_ringbuffer (GstBaseAudioSrc * src)
{

  return NULL;
}
% end
