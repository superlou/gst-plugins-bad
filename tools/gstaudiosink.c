% ClassName
GstAudioSink
% TYPE_CLASS_NAME
GST_TYPE_AUDIO_SINK
% pads
sinkpad-simple
% pkg-config
gstreamer-audio-0.10
% includes
#include <gst/audio/gstaudiosink.h>
% prototypes
static gboolean gst_replace_open (GstAudioSink * sink);
static gboolean
gst_replace_prepare (GstAudioSink * sink, GstRingBufferSpec * spec);
static gboolean gst_replace_unprepare (GstAudioSink * sink);
static gboolean gst_replace_close (GstAudioSink * sink);
static guint gst_replace_write (GstAudioSink * sink, gpointer data, guint length);
static guint gst_replace_delay (GstAudioSink * sink);
static void gst_replace_reset (GstAudioSink * sink);
% declare-class
  GstAudioSinkClass *audio_sink_class = GST_AUDIO_SINK_CLASS (klass);
% set-methods
  audio_sink_class->open = GST_DEBUG_FUNCPTR (gst_replace_open);
  audio_sink_class->prepare = GST_DEBUG_FUNCPTR (gst_replace_prepare);
  audio_sink_class->unprepare = GST_DEBUG_FUNCPTR (gst_replace_unprepare);
  audio_sink_class->close = GST_DEBUG_FUNCPTR (gst_replace_close);
  audio_sink_class->write = GST_DEBUG_FUNCPTR (gst_replace_write);
  audio_sink_class->delay = GST_DEBUG_FUNCPTR (gst_replace_delay);
  audio_sink_class->reset = GST_DEBUG_FUNCPTR (gst_replace_reset);
% methods

static gboolean
gst_replace_open (GstAudioSink * sink)
{
  return FALSE;
}

static gboolean
gst_replace_prepare (GstAudioSink * sink, GstRingBufferSpec * spec)
{
  return FALSE;
}

static gboolean
gst_replace_unprepare (GstAudioSink * sink)
{
  return FALSE;
}

static gboolean
gst_replace_close (GstAudioSink * sink)
{
  return FALSE;
}

static guint
gst_replace_write (GstAudioSink * sink, gpointer data, guint length)
{
  return 0;
}

static guint
gst_replace_delay (GstAudioSink * sink)
{
  return 0;
}

static void
gst_replace_reset (GstAudioSink * sink)
{
}
% end
