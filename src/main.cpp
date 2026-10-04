#include <gst/gst.h>
#include <iostream>
#include <gst/app/gstappsink.h>

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

    // Build pipeline elements
    GstElement *playbin = gst_element_factory_make("playbin", nullptr);
    if (playbin == nullptr) {
        g_printerr("Could not create the GStreamer playbin element.\n");
        g_free(uri);
        return 1;
    }
    g_object_set(playbin, "uri", uri, nullptr);
    g_free(uri);
    
    GstElement *appsink = gst_element_factory_make("appsink", nullptr);
    if (appsink == nullptr) {
        g_printerr("Could not create the GStreamer appsink element\n");
        return 1;
    }
    g_object_set(playbin, "video-sink", appsink, nullptr);

    // Start playing.
    gst_element_set_state(playbin, GST_STATE_PLAYING);
    GstBus *bus = gst_element_get_bus(playbin);

    // Get processed frame from the appsink consecutively before getting an error or EOS.
    bool running = true;
    while(running) {
        // Check if any error was occured
        GstMessage *msg = gst_bus_pop_filtered(
            bus,
            static_cast<GstMessageType>(GST_MESSAGE_ERROR | GST_MESSAGE_EOS)
        );
        if (msg != nullptr) {
            if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_ERROR) {
                GError *error = nullptr;
                gchar *debug_info = nullptr;
                gst_message_parse_error(msg, &error, &debug_info);
                g_printerr("Error from %s: %s\n", GST_OBJECT_NAME(msg->src), error->message);
                if (debug_info != nullptr) {
                    g_printerr("Debug details: %s\n", debug_info);
                }
                g_clear_error(&error);
                g_free(debug_info);
                running = false;
            } else if (GST_MESSAGE_TYPE(msg) == GST_MESSAGE_EOS) {
                g_print("End of stream.\n");
            }
            gst_message_unref(msg);
        }

        if (!running) {
            break;
        }

        // Get a processed frame from the playback (wait up to 100ms)
        GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(appsink), 100 * GST_MSECOND);
        if (sample == nullptr) {
            // Break the loop on EOS.
            if (gst_app_sink_is_eos(GST_APP_SINK(appsink))) {
                break;
            }
            continue;
        }
        
        GstBuffer* frame = gst_sample_get_buffer(sample);
        if (frame != nullptr) {
            GstClockTime pts = GST_BUFFER_PTS(frame); // presentation timestamp
            std::cout << pts << '\n';
        }

        gst_sample_unref(sample);
    }

    // Clean up.
    gst_object_unref(bus);
    gst_element_set_state(appsink, GST_STATE_NULL);
    gst_object_unref(appsink);
    gst_element_set_state(playbin, GST_STATE_NULL);
    gst_object_unref(playbin);
    return 0;
}
