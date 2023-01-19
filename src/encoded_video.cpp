#include <iostream>
#include <thread>
#include <vector>
#include <csignal>
#include <gst/gst.h>
#include <gst/app/gstappsrc.h>

// Includes common necessary includes for development using depthai library
#include "depthai/depthai.hpp"
#include "gst/gstpipeline.h"
#include "gst/gstutils.h"

constexpr int WIDTH = 1280;
constexpr int HEIGHT = 720;

// Keyboard interrupt (Ctrl + C) detected
static std::atomic<bool> alive{true};
static void sigintHandler(int signum) {
    alive = false;
}

int main(int argc, char** argv) {
    std::signal(SIGINT, &sigintHandler);
    // Init Gstreamer
    if (!gst_is_initialized()) {
        gst_init(&argc, &argv);
    }

    GMainLoop *loop;

    loop = g_main_loop_new(nullptr, false);
    std::thread runThread(g_main_loop_run, loop);

    GstElement* gstPipeline = gst_pipeline_new("gstPipeline");
    GstElement* appsrc =  gst_element_factory_make("appsrc", "appsrc");
    GstElement* queue =  gst_element_factory_make("queue", "queue");
    GstElement* h264parse =  gst_element_factory_make("h264parse", "h264parse");
    GstElement* decoder =  gst_element_factory_make("avdec_h264", "decoder");

    GstElement* autovideosink =  gst_element_factory_make("autovideosink", "autovideosink");

    GstCaps* appsrc_caps = gst_caps_new_simple("video/x-h264",
        "framerate", GST_TYPE_FRACTION, 30, 1,
        "stream-format", G_TYPE_STRING, "byte-stream",
        "pixel-aspect-ratio", GST_TYPE_FRACTION, 1, 1,
        "interlace-mode", G_TYPE_STRING, "progressive",
        nullptr);

    g_object_set(appsrc, "do_timestamp", TRUE, "is-live", TRUE, nullptr);
    g_object_set(appsrc, "caps", appsrc_caps, "format", GST_FORMAT_TIME, nullptr);

    g_object_set(autovideosink, "sync", FALSE, NULL);
    gst_caps_unref(appsrc_caps);
    gst_bin_add_many(GST_BIN(gstPipeline), appsrc, queue, h264parse, decoder, autovideosink, NULL);

    if(gst_element_link_many(appsrc, queue, h264parse, decoder, autovideosink, NULL) != TRUE){
        g_printerr ("Elements could not be linked.\n");
        gst_object_unref (gstPipeline);
        return -1;
    }

    if(gst_element_set_state(GST_ELEMENT(gstPipeline), GST_STATE_PLAYING) == GST_STATE_CHANGE_FAILURE){
        g_printerr ("Unable to set the pipeline to the playing state.\n");
        gst_object_unref (gstPipeline);
        return -1;
    }

    auto push_image_data = [&](GstFlowReturn &returnValue, GstMapInfo &mapInfo, const std::vector<uint8_t>& data){
        auto buffer = gst_buffer_new_allocate(nullptr, data.size(), nullptr);
        gst_buffer_map(buffer, &mapInfo, GST_MAP_WRITE);
        memcpy((guint8*)mapInfo.data, data.data(), gst_buffer_get_size(buffer));

        g_signal_emit_by_name(appsrc, "push-buffer", buffer, &returnValue);

        gst_buffer_unmap(buffer, &mapInfo);
        gst_buffer_unref(buffer);
    };

    // Create pipeline
    dai::Pipeline pipeline;

    // Define sources and outputs
    auto camRgb = pipeline.create<dai::node::ColorCamera>();
    auto videoEnc = pipeline.create<dai::node::VideoEncoder>();
    auto xout = pipeline.create<dai::node::XLinkOut>();

    xout->setStreamName("h264");

    // Properties
    camRgb->setBoardSocket(dai::CameraBoardSocket::RGB);
    camRgb->setResolution(dai::ColorCameraProperties::SensorResolution::THE_1080_P);
    videoEnc->setDefaultProfilePreset(30, dai::VideoEncoderProperties::Profile::H264_MAIN);

    // Linking
    camRgb->video.link(videoEnc->input);
    videoEnc->bitstream.link(xout->input);

    // Connect to device and start pipeline
    dai::DeviceInfo deviceInfo;
    deviceInfo.name = "192.168.1.211";
    deviceInfo.protocol = X_LINK_TCP_IP;
    dai::Device device(pipeline);

    // Output queue will be used to get the encoded data from the output defined above
    auto video = device.getOutputQueue("h264", 30, false);

    auto callback = [&](std::shared_ptr<dai::ADatatype> data){
        // auto videoIn = video->get<dai::ImgFrame>();
        auto frame = dynamic_cast<dai::ImgFrame*>(data.get());

        if(!frame){
            return;
        }

        std::cout << "Data size: " << frame->getData().size() << std::endl;
        GstFlowReturn ret;
        GstMapInfo map;
        push_image_data(ret, map, frame->getData());
    };

    video->addCallback(callback);

    while(alive) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(1s);
    }
    return 0;
}

