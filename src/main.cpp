#include <gst/gst.h>

int main(int argc, char *argv[]) {
    // Initialize GStreamer.
    gst_init(&argc, &argv);

    const char *filename = argc > 1 ? argv[1] : "sample.mp4";
    GError *uri_error = nullptr;
    gchar *uri = gst_filename_to_uri(filename, &uri_error);
    if (uri == nullptr) {
        g_printerr("Cannot convert %s to a URI: %s\n", filename, uri_error->message);
        g_clear_error(&uri_error);
        return 1;
    }

    GstElement *pipeline = gst_element_factory_make("playbin", nullptr);
    if (pipeline == nullptr) {
        g_printerr("Could not create the GStreamer playbin element.\n");
        g_free(uri);
        return 1;
    }
    g_object_set(pipeline, "uri", uri, nullptr);
    g_free(uri);

    // Start playing.
    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    // Wait until an error or the end of the stream.
    GstBus *bus = gst_element_get_bus(pipeline);
    GstMessage *msg = gst_bus_timed_pop_filtered(
        bus,
        GST_CLOCK_TIME_NONE,
        static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)
    );
    int exit_code = 0;
    if (msg == nullptr) {
        g_printerr("No message received from the GStreamer bus.\n");
        exit_code = 1;
    } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
        GError *error = nullptr;
        gchar *debug_info = nullptr;
        gst_message_parse_error(msg, &error, &debug_info);
        g_printerr("Error from %s: %s\n", GST_OBJECT_NAME(msg->src), error->message);
        if (debug_info != nullptr) {
            g_printerr("Debug details: %s\n", debug_info);
        }
        g_clear_error(&error);
        g_free(debug_info);
        exit_code = 1;
    } else {
        g_print("End of stream.\n");
    }

    // Clean up.
    if (msg != nullptr) {
        gst_message_unref(msg);
    }
    gst_object_unref(bus);
    gst_element_set_state(pipeline, GST_STATE_NULL);
    gst_object_unref(pipeline);
    return exit_code;
}
