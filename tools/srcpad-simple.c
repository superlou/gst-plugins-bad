
% instance-members
  GstPad *srcpad;
% prototypes
% pad-template
static GstStaticPadTemplate gst_replace_src_template =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/unknown")
    );

% base-init
  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&gst_replace_src_template));
% instance-init

  replace->srcpad = gst_pad_new_from_static_template (&gst_replace_src_template
      ,     
            "src");
% methods
% end

