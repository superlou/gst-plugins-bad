#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <stdlib.h>
#include <glib/gprintf.h>       // Needed because of  bug in glib includes?
#include <string.h>

//#define GETTEXT_PACKAGE "intertest"


typedef struct _GstInterTest GstInterTest;
struct _GstInterTest
{
  GstElement *pipeline;
  GstBus *bus;
  GMainLoop *main_loop;

  GstElement *source_video_element;
  GstElement *source_audio_element;
  GstElement *sink_video_element;
  GstElement *sink_audio_element;

  GString *channel;

  gboolean paused_for_buffering;
  guint timer_id;
};

GstInterTest *gst_inter_test_new (void);
void gst_inter_test_free (GstInterTest * intertest);
void gst_inter_test_create_pipeline_send (GstInterTest * intertest,
    GString * channel, int pattern, int frequency);
void gst_inter_test_create_pipeline_receive (GstInterTest * intertest,
    GString * channel);
void gst_inter_test_start (GstInterTest * intertest);
void gst_inter_test_stop (GstInterTest * intertest);
void gst_inter_test_free_gst_element (GstElement * element);

static gboolean gst_inter_test_handle_message (GstBus * bus,
    GstMessage * message, gpointer data);
static gboolean onesecond_timer (gpointer priv);
static gboolean onesecond_timer_swap_channel (gpointer priv);


gboolean verbose;

static GOptionEntry entries[] = {
  {"verbose", 'v', 0, G_OPTION_ARG_NONE, &verbose, "Be verbose", NULL},

  {NULL}

};

int
main (int argc, char *argv[])
{
  GError *error = NULL;
  GOptionContext *context;
  GstInterTest *intertest1;
  GstInterTest *intertest2;
  GstInterTest *intertest3;
  GMainLoop *main_loop;

#if !GLIB_CHECK_VERSION (2, 31, 0)
  if (!g_thread_supported ())
    g_thread_init (NULL);
#endif

  context = g_option_context_new ("- Internal src/sink test");
  g_option_context_add_main_entries (context, entries, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_print ("option parsing failed: %s\n", error->message);
    exit (1);
  }
  g_option_context_free (context);

  intertest1 = gst_inter_test_new ();
  gst_inter_test_create_pipeline_send (intertest1, g_string_new ("channel1"), 0,
      440);
  gst_inter_test_start (intertest1);
  intertest1->timer_id = g_timeout_add (1000, onesecond_timer, intertest1);

  intertest2 = gst_inter_test_new ();
  gst_inter_test_create_pipeline_send (intertest2, g_string_new ("channel2"), 1,
      554);
  gst_inter_test_start (intertest2);
  intertest2->timer_id = g_timeout_add (1000, onesecond_timer, intertest2);

  intertest3 = gst_inter_test_new ();
  gst_inter_test_create_pipeline_receive (intertest3,
      g_string_new ("channel1"));
  gst_inter_test_start (intertest3);
  intertest3->timer_id =
      g_timeout_add (1000, onesecond_timer_swap_channel, intertest3);

  main_loop = g_main_loop_new (NULL, TRUE);
  intertest1->main_loop = main_loop;
  intertest2->main_loop = main_loop;
  intertest3->main_loop = main_loop;

  g_main_loop_run (main_loop);
  g_main_loop_unref (main_loop);

  exit (0);
}


GstInterTest *
gst_inter_test_new (void)
{
  GstInterTest *intertest;

  intertest = g_new0 (GstInterTest, 1);

  return intertest;
}

void
gst_inter_test_free_gst_element (GstElement * element)
{
  if (element) {
    gst_object_unref (element);
    element = NULL;
  }
}

void
gst_inter_test_free (GstInterTest * intertest)
{
  gst_inter_test_free_gst_element (intertest->source_video_element);
  gst_inter_test_free_gst_element (intertest->source_audio_element);
  gst_inter_test_free_gst_element (intertest->sink_video_element);
  gst_inter_test_free_gst_element (intertest->sink_audio_element);

  if (intertest->pipeline) {
    gst_element_set_state (intertest->pipeline, GST_STATE_NULL);
    gst_object_unref (intertest->pipeline);
    intertest->pipeline = NULL;
  }

  g_string_free (intertest->channel, TRUE);

  g_free (intertest);
}

void
gst_inter_test_create_pipeline_send (GstInterTest * intertest,
    GString * channel, int pattern, int frequency)
{
  GString *pipe_desc;
  GString *temp_str;
  GstElement *pipeline;
  GError *error = NULL;

  intertest->channel = channel;

  pipe_desc = g_string_new ("");
  temp_str = g_string_new ("");

  g_string_printf (temp_str, "videotestsrc name=source_video pattern=%d",
      pattern);
  g_string_append (pipe_desc, temp_str->str);

  g_string_append (pipe_desc,
      " ! video/x-raw,format=(string)I420,width=320,height=240 ! ");
  g_string_append (pipe_desc, "timeoverlay ! ");

  g_string_printf (temp_str,
      "intervideosink name=sink_video sync=true channel=%s", channel->str);
  g_string_append (pipe_desc, temp_str->str);

  g_string_printf (temp_str,
      " audiotestsrc name=source_audio samplesperbuffer=1600 freq=%d",
      frequency);
  g_string_append (pipe_desc, temp_str->str);
  g_string_append (pipe_desc, " ! audioconvert ! ");
  g_string_append (pipe_desc,
      "interaudiosink name=sink_audio sync=true channel=");
  g_string_append (pipe_desc, channel->str);

  g_string_free (temp_str, TRUE);

  if (verbose)
    g_print ("pipeline: %s\n", pipe_desc->str);

  pipeline = (GstElement *) gst_parse_launch (pipe_desc->str, &error);
  g_string_free (pipe_desc, TRUE);

  if (error) {
    g_print ("pipeline parsing error: %s\n", error->message);
    gst_object_unref (pipeline);
    return;
  }

  intertest->pipeline = pipeline;

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (pipeline), FALSE);
  intertest->bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (intertest->bus, gst_inter_test_handle_message, intertest);

  intertest->source_video_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "source_video");
  intertest->sink_video_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "sink_video");
  intertest->source_audio_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "source_audio");
  intertest->sink_audio_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "sink_audio");
}

void
gst_inter_test_create_pipeline_receive (GstInterTest * intertest,
    GString * channel)
{
  GString *pipe_desc;
  GstElement *pipeline;
  GError *error = NULL;

  intertest->channel = channel;

  pipe_desc = g_string_new ("");

  g_string_append (pipe_desc, "intervideosrc name=source_video channel=");
  g_string_append (pipe_desc, channel->str);
  g_string_append (pipe_desc, " ! queue ! ");
  g_string_append (pipe_desc, "xvimagesink name=sink_video ");
  g_string_append (pipe_desc, "interaudiosrc name=source_audio channel=");
  g_string_append (pipe_desc, channel->str);
  g_string_append (pipe_desc, " ! queue ! ");
  g_string_append (pipe_desc, "alsasink name=sink_audio");

  if (verbose)
    g_print ("pipeline: %s\n", pipe_desc->str);

  pipeline = (GstElement *) gst_parse_launch (pipe_desc->str, &error);
  g_string_free (pipe_desc, TRUE);

  if (error) {
    g_print ("pipeline parsing error: %s\n", error->message);
    gst_object_unref (pipeline);
    return;
  }

  intertest->pipeline = pipeline;

  gst_pipeline_set_auto_flush_bus (GST_PIPELINE (pipeline), FALSE);
  intertest->bus = gst_pipeline_get_bus (GST_PIPELINE (pipeline));
  gst_bus_add_watch (intertest->bus, gst_inter_test_handle_message, intertest);

  intertest->source_video_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "source_video");
  intertest->sink_video_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "sink_video");
  intertest->source_audio_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "source_audio");
  intertest->sink_audio_element =
      gst_bin_get_by_name (GST_BIN (pipeline), "sink_audio");
}

void
gst_inter_test_start (GstInterTest * intertest)
{
  gst_element_set_state (intertest->pipeline, GST_STATE_READY);
}

void
gst_inter_test_stop (GstInterTest * intertest)
{
  gst_element_set_state (intertest->pipeline, GST_STATE_NULL);

  g_source_remove (intertest->timer_id);
}

static void
gst_inter_test_handle_eos (GstInterTest * intertest)
{
  gst_inter_test_stop (intertest);
}

static void
gst_inter_test_handle_error (GstInterTest * intertest, GError * error,
    const char *debug)
{
  g_print ("error: %s\n", error->message);
  gst_inter_test_stop (intertest);
}

static void
gst_inter_test_handle_warning (GstInterTest * intertest, GError * error,
    const char *debug)
{
  g_print ("warning: %s\n", error->message);
}

static void
gst_inter_test_handle_info (GstInterTest * intertest, GError * error,
    const char *debug)
{
  g_print ("info: %s\n", error->message);
}

static void
gst_inter_test_handle_null_to_ready (GstInterTest * intertest)
{
  gst_element_set_state (intertest->pipeline, GST_STATE_PAUSED);

}

static void
gst_inter_test_handle_ready_to_paused (GstInterTest * intertest)
{
  if (!intertest->paused_for_buffering) {
    gst_element_set_state (intertest->pipeline, GST_STATE_PLAYING);
  }
}

static void
gst_inter_test_handle_paused_to_playing (GstInterTest * intertest)
{

}

static void
gst_inter_test_handle_playing_to_paused (GstInterTest * intertest)
{

}

static void
gst_inter_test_handle_paused_to_ready (GstInterTest * intertest)
{

}

static void
gst_inter_test_handle_ready_to_null (GstInterTest * intertest)
{
  //g_main_loop_quit (intertest->main_loop);

}


static gboolean
gst_inter_test_handle_message (GstBus * bus, GstMessage * message,
    gpointer data)
{
  GstInterTest *intertest = (GstInterTest *) data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_EOS:
      gst_inter_test_handle_eos (intertest);
      break;
    case GST_MESSAGE_ERROR:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_error (message, &error, &debug);
      gst_inter_test_handle_error (intertest, error, debug);
      g_error_free (error);
      g_free (debug);
    }
      break;
    case GST_MESSAGE_WARNING:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_warning (message, &error, &debug);
      gst_inter_test_handle_warning (intertest, error, debug);
      g_error_free (error);
      g_free (debug);
    }
      break;
    case GST_MESSAGE_INFO:
    {
      GError *error = NULL;
      gchar *debug;

      gst_message_parse_info (message, &error, &debug);
      gst_inter_test_handle_info (intertest, error, debug);
      g_error_free (error);
      g_free (debug);
    }
      break;
    case GST_MESSAGE_TAG:
    {
      GstTagList *tag_list;

      gst_message_parse_tag (message, &tag_list);
      if (verbose)
        g_print ("tag\n");
    }
      break;
    case GST_MESSAGE_STATE_CHANGED:
    {
      GstState oldstate, newstate, pending;

      gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);
      if (GST_ELEMENT (message->src) == intertest->pipeline) {
        if (verbose)
          g_print ("state change from %s to %s\n",
              gst_element_state_get_name (oldstate),
              gst_element_state_get_name (newstate));
        switch (GST_STATE_TRANSITION (oldstate, newstate)) {
          case GST_STATE_CHANGE_NULL_TO_READY:
            gst_inter_test_handle_null_to_ready (intertest);
            break;
          case GST_STATE_CHANGE_READY_TO_PAUSED:
            gst_inter_test_handle_ready_to_paused (intertest);
            break;
          case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            gst_inter_test_handle_paused_to_playing (intertest);
            break;
          case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
            gst_inter_test_handle_playing_to_paused (intertest);
            break;
          case GST_STATE_CHANGE_PAUSED_TO_READY:
            gst_inter_test_handle_paused_to_ready (intertest);
            break;
          case GST_STATE_CHANGE_READY_TO_NULL:
            gst_inter_test_handle_ready_to_null (intertest);
            break;
          default:
            if (verbose)
              g_print ("unknown state change from %s to %s\n",
                  gst_element_state_get_name (oldstate),
                  gst_element_state_get_name (newstate));
        }
      }
    }
      break;
    case GST_MESSAGE_BUFFERING:
    {
      int percent;
      gst_message_parse_buffering (message, &percent);
      //g_print("buffering %d\n", percent);
      if (!intertest->paused_for_buffering && percent < 100) {
        g_print ("pausing for buffing\n");
        intertest->paused_for_buffering = TRUE;
        gst_element_set_state (intertest->pipeline, GST_STATE_PAUSED);
      } else if (intertest->paused_for_buffering && percent == 100) {
        g_print ("unpausing for buffing\n");
        intertest->paused_for_buffering = FALSE;
        gst_element_set_state (intertest->pipeline, GST_STATE_PLAYING);
      }
    }
      break;
    case GST_MESSAGE_STATE_DIRTY:
    case GST_MESSAGE_CLOCK_PROVIDE:
    case GST_MESSAGE_CLOCK_LOST:
    case GST_MESSAGE_NEW_CLOCK:
    case GST_MESSAGE_STRUCTURE_CHANGE:
    case GST_MESSAGE_STREAM_STATUS:
      break;
    case GST_MESSAGE_STEP_DONE:
    case GST_MESSAGE_APPLICATION:
    case GST_MESSAGE_ELEMENT:
    case GST_MESSAGE_SEGMENT_START:
    case GST_MESSAGE_SEGMENT_DONE:
    case GST_MESSAGE_LATENCY:
    case GST_MESSAGE_ASYNC_START:
    case GST_MESSAGE_ASYNC_DONE:
    case GST_MESSAGE_REQUEST_STATE:
    case GST_MESSAGE_STEP_START:
    default:
      if (verbose) {
        g_print ("message: %s\n", GST_MESSAGE_TYPE_NAME (message));
      }
      break;
    case GST_MESSAGE_QOS:
      break;
  }

  return TRUE;
}



static gboolean
onesecond_timer (gpointer priv)
{
  //GstInterTest *intertest = (GstInterTest *)priv;

  g_print (".\n");

  return TRUE;
}

#define STR_MATCH 0

static gboolean
onesecond_timer_swap_channel (gpointer priv)
{
  GstElement *intervideosrc;
  GstElement *interaudiosrc;
  GstInterTest *intertest = (GstInterTest *) priv;
  GString *video_channel = g_string_new ("");
  GString *audio_channel = g_string_new ("");

  intervideosrc = intertest->source_video_element;
  interaudiosrc = intertest->source_audio_element;

  g_object_get (G_OBJECT (intervideosrc), "channel", video_channel, NULL);
  g_object_get (G_OBJECT (interaudiosrc), "channel", audio_channel, NULL);

  if (strcmp (video_channel->str, "channel1") == STR_MATCH) {
    g_print ("Swapping to channel2.\n");
    g_object_set (G_OBJECT (intervideosrc), "channel", "channel2", NULL);
    g_object_set (G_OBJECT (interaudiosrc), "channel", "channel2", NULL);
  } else {
    g_print ("Swapping to channel1.\n");
    g_object_set (G_OBJECT (intervideosrc), "channel", "channel1", NULL);
    g_object_set (G_OBJECT (interaudiosrc), "channel", "channel1", NULL);
  }

  g_object_get (G_OBJECT (intervideosrc), "channel", video_channel, NULL);
  g_object_get (G_OBJECT (interaudiosrc), "channel", audio_channel, NULL);
  g_printf ("Video: %s, Audio: %s\n", video_channel->str, audio_channel->str);

  g_string_free (video_channel, TRUE);
  g_string_free (audio_channel, TRUE);

  return TRUE;
}
