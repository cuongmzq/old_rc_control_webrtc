#include "rtc/rtc.hpp"
#include "seasocks/PrintfLogger.h"
#include "seasocks/Server.h"
#include "seasocks/StringUtil.h"
#include "seasocks/WebSocket.h"

#include <pigpio.h>
#include "nlohmann/json.hpp"
#include <string>
#include <shared_mutex>

#include "frame_queue.cpp"
#include "dispatchqueue.cpp"
#include "mmal/mmal.h"
//extern "C" {
    #include "video.cpp"
//}
using namespace seasocks;
using namespace std;
//using namespace rtc;
using json = nlohmann::json;

/// Main dispatch queue
DispatchQueue MainThread("Main");

static MMALCAM_BEHAVIOUR_T camcorder_behaviour;

#define VIEWFINDER_LAYER      2
#define DEFAULT_VIDEO_FORMAT  "640x480:h264";
MMAL_RATIONAL_T DEFAULT_FRAME_RATE = {30, 1};
#define DEFAULT_BIT_RATE      100000
#define DEFAULT_CAM_NUM       0

static VCOS_THREAD_T camcorder_thread;
static int stop;
static uint32_t sleepy_time;
static MMAL_BOOL_T stopped_already;
static int recommend_buffer_size;
static int recommend_buffer_num;
//
///*****************************************************************************/
//static void test_mmalcam_dump_stats(const char *title, MMAL_PARAMETER_STATISTICS_T* stats)
//{
//   printf("[%s]\n", title);
//   printf("buffer_count: %u\n", stats->buffer_count);
//   printf("frame_count: %u\n", stats->frame_count);
//   printf("frames_skipped: %u\n", stats->frames_skipped);
//   printf("frames_discarded: %u\n", stats->frames_discarded);
//   printf("eos_seen: %u\n", stats->eos_seen);
//   printf("maximum_frame_bytes: %u\n", stats->maximum_frame_bytes);
//   printf("total_bytes_hi: %u\n", (uint32_t)(stats->total_bytes >> 32));
//   printf("total_bytes_lo: %u\n", (uint32_t)(stats->total_bytes));
//   printf("corrupt_macroblocks: %u\n", stats->corrupt_macroblocks);
//}

/*****************************************************************************/
std::function <void(MMAL_BUFFER_HEADER_T *)> onCamBuffer;
FrameQueue* frame_queue = new FrameQueue();

static void *test_mmal_camcorder(void *id)
{
   MMALCAM_BEHAVIOUR_T *behaviour = (MMALCAM_BEHAVIOUR_T *)id;
   int value;

   value = start_recorder(&stop, behaviour, [&](MMAL_BUFFER_HEADER_T * mmal_frame){
        if (onCamBuffer) {
            onCamBuffer(mmal_frame);
        }
    }, frame_queue);


   LOG_TRACE("Thread terminating, result %d", value);
   return (void *)(uintptr_t)value;
}

void startCamera() {
 VCOS_THREAD_ATTR_T attrs;
   VCOS_STATUS_T status;
   int result = 0;

   vcos_log_register("mmalcam", VCOS_LOG_CATEGORY);
   printf("MMAL Camera Test App\n");

//   signal(SIGINT, test_signal_handler);

   camcorder_behaviour.layer = VIEWFINDER_LAYER;
   camcorder_behaviour.vformat = DEFAULT_VIDEO_FORMAT;
   camcorder_behaviour.zero_copy = 1;
   camcorder_behaviour.bit_rate = DEFAULT_BIT_RATE;
   camcorder_behaviour.focus_test = MMAL_PARAM_FOCUS_MAX;
   camcorder_behaviour.camera_num = DEFAULT_CAM_NUM;
    camcorder_behaviour.frame_rate = DEFAULT_FRAME_RATE;
//    camcorder_behaviour.onBuffer = onBuffer;
//    camcorder_behaviour.uri = "vid.h264";
//    MMAL_STATUS_T status = MMAL_SUCCESS;

    status = vcos_semaphore_create(&camcorder_behaviour.init_sem, "mmalcam-init", 0);
   vcos_assert(status == VCOS_SUCCESS);

    vcos_thread_attr_init(&attrs);
//    printf("End\n");

   if (vcos_thread_create(&camcorder_thread, "mmal camcorder", &attrs, test_mmal_camcorder, &camcorder_behaviour) != VCOS_SUCCESS)   {
      LOG_ERROR("Thread creation failure");
      result = -2;
//      goto error;
   }
//    printf("end2\n");

}


struct ClientTrackData {
    std::shared_ptr<rtc::Track> track;
    std::shared_ptr<rtc::RtcpSrReporter> sender;

    ClientTrackData(std::shared_ptr<rtc::Track> track, std::shared_ptr<rtc::RtcpSrReporter> sender);
};

ClientTrackData::ClientTrackData(shared_ptr<rtc::Track> track, shared_ptr<rtc::RtcpSrReporter> sender) {
    this->track = track;
    this->sender = sender;
}

struct Client {
    enum class State {
        Waiting,
        WaitingForVideo,
        WaitingForAudio,
        Ready
    };
    const std::shared_ptr<rtc::PeerConnection> & peerConnection = _peerConnection;
    Client(std::shared_ptr<rtc::PeerConnection> pc) {
        _peerConnection = pc;
    }
    std::optional<std::shared_ptr<ClientTrackData>> video;
    std::optional<std::shared_ptr<ClientTrackData>> audio;
    std::optional<std::shared_ptr<rtc::DataChannel>> dataChannel{};
    void setState(State state);
    State getState();

private:
    std::shared_mutex _mutex;
    State state = State::Waiting;
    std::string id;
    std::shared_ptr<rtc::PeerConnection> _peerConnection;
};

void Client::setState(State state) {
    std::unique_lock lock(_mutex);
    this->state = state;
}


Client::State Client::getState() {
    std::shared_lock lock(_mutex);
    return state;
}


struct ClientTrack {
    std::string id;
    std::shared_ptr<ClientTrackData> trackData;
    ClientTrack(std::string id, std::shared_ptr<ClientTrackData> trackData);
};

ClientTrack::ClientTrack(string id, shared_ptr<ClientTrackData> trackData) {
    this->id = id;
    this->trackData = trackData;
}

#include <ctime>

#ifdef _MSC_VER
// taken from https://stackoverflow.com/questions/10905892/equivalent-of-gettimeday-for-windows
#include <windows.h>
#include <winsock2.h> // for struct timeval

struct timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

int gettimeofday(struct timeval *tv, struct timezone *tz) {
    if (tv) {
        FILETIME filetime; /* 64-bit value representing the number of 100-nanosecond intervals since
                              January 1, 1601 00:00 UTC */
        ULARGE_INTEGER x;
        ULONGLONG usec;
        static const ULONGLONG epoch_offset_us =
            11644473600000000ULL; /* microseconds betweeen Jan 1,1601 and Jan 1,1970 */

#if _WIN32_WINNT >= _WIN32_WINNT_WIN8
        GetSystemTimePreciseAsFileTime(&filetime);
#else
        GetSystemTimeAsFileTime(&filetime);
#endif
        x.LowPart = filetime.dwLowDateTime;
        x.HighPart = filetime.dwHighDateTime;
        usec = x.QuadPart / 10 - epoch_offset_us;
        tv->tv_sec = time_t(usec / 1000000ULL);
        tv->tv_usec = long(usec % 1000000ULL);
    }
    if (tz) {
        TIME_ZONE_INFORMATION timezone;
        GetTimeZoneInformation(&timezone);
        tz->tz_minuteswest = timezone.Bias;
        tz->tz_dsttime = 0;
    }
    return 0;
}
#else
#include <sys/time.h>
#endif

uint64_t currentTimeInMicroSeconds() {
    struct timeval time;
    gettimeofday(&time, NULL);
    return uint64_t(time.tv_sec) * 1000 * 1000 + time.tv_usec;
}


unordered_map<string, shared_ptr<Client>> clients{};


template <class T> weak_ptr<T> make_weak_ptr(shared_ptr<T> ptr) { return ptr; }
shared_ptr<Client> createPeerConnection(const rtc::Configuration &config,
                                        WebSocket* ws,
                                        string id);

/// all connected clients
rtc::Configuration config;

//Websocket* ws;
enum SignalType {call, answer, addIceCandidate, hangup};

SignalType getType(string s) {
    if (s == "call") return SignalType::call;
    if (s == "answer") return SignalType::answer;
    if (s == "addIceCandidate") return SignalType::addIceCandidate;
    if (s == "hangup") return SignalType::hangup;
}

struct WebRTCHandler : WebSocket::Handler {
    void onConnect(WebSocket* con) override {
//        ws = con;
//        con->send("{\"what\": \"offer\", \"data\": \"\"}");
    }
    void onDisconnect(WebSocket* con) override {
//        ws = nullptr;
        con->send("{\"what\": \"hangup\"}");
    }

    void onData(WebSocket *con, const char* data) override {
        json message = json::parse(data);
        auto it = message.find("what");
        if (it == message.end()) {
            return;
        }
        string type = message["what"];

        auto id = "999";

        switch (getType(type)) {
            case SignalType::call:
                clients.emplace(id, createPeerConnection(config, con, id));
                break;
            case SignalType::answer:
                if (auto jt = clients.find(id); jt != clients.end()) {
                    auto pc = jt->second->peerConnection;
                    auto sdp_mess = message["data"].get<string>();
                    auto sdp_obj = json::parse(sdp_mess);
                    auto sdp = sdp_obj["sdp"];
                    auto description = rtc::Description(sdp, type);
                    pc->setRemoteDescription(description);
                }
                break;
            case SignalType::addIceCandidate:
                if (auto jt = clients.find(id); jt != clients.end()) {
                    cout << "addIceCandidate";
                    auto pc = jt->second->peerConnection;
                    auto data = json::parse(message["data"].get<string>());
                    auto mid = data["sdpMid"].get<string>();
                    auto candidate = data["candidate"].get<string>();
                    pc->addRemoteCandidate(rtc::Candidate(candidate, mid));
                }
                break;
            case SignalType::hangup:
                if (auto jt = clients.find(id); jt != clients.end()) {
                    auto pc = jt->second->peerConnection;
                    pc->close();
                    clients.erase(id);
                }
                break;
        }
    }
};

struct Handler : WebSocket::Handler {

    void onConnect(WebSocket* con) override {
        con->send("Connected");
    }
    void onDisconnect(WebSocket* con) override {
        con->send("Disconnected");
    }

    void onData(WebSocket* con, const char* data) override {
        con->send(data);
    }
};

//WebRTC


shared_ptr<ClientTrackData> addVideo(const shared_ptr<rtc::PeerConnection> pc, const uint8_t payloadType, const uint32_t ssrc, const string cname, const string msid, const function<void (void)> onOpen) {
    auto video = rtc::Description::Video(cname);
    video.addH264Codec(payloadType);
    video.addSSRC(ssrc, cname, msid, cname);
    auto track = pc->addTrack(video);
    // create RTP configuration
    auto rtpConfig = make_shared<rtc::RtpPacketizationConfig>(ssrc, cname, payloadType, rtc::H264RtpPacketizer::defaultClockRate);
    // create packetizer
    auto packetizer = make_shared<rtc::H264RtpPacketizer>(rtc::H264RtpPacketizer::Separator::Length, rtpConfig);
    // create H264 handler
    auto h264Handler = make_shared<rtc::H264PacketizationHandler>(packetizer);
    // add RTCP SR handler
    auto srReporter = make_shared<rtc::RtcpSrReporter>(rtpConfig);
    h264Handler->addToChain(srReporter);
    // add RTCP NACK handler
    auto nackResponder = make_shared<rtc::RtcpNackResponder>();
    h264Handler->addToChain(nackResponder);
    // set handler
    track->setMediaHandler(h264Handler);
    track->onOpen(onOpen);
    auto trackData = make_shared<ClientTrackData>(track, srReporter);
    return trackData;
}
//
//#include <string_view>
//
//template <class T>
//constexpr
//std::string_view
//type_name()
//{
//    using namespace std;
//#ifdef __clang__
//    string_view p = __PRETTY_FUNCTION__;
//    return string_view(p.data() + 34, p.size() - 34 - 1);
//#elif defined(__GNUC__)
//    string_view p = __PRETTY_FUNCTION__;
//#  if __cplusplus < 201402
//    return string_view(p.data() + 36, p.size() - 36 - 1);
//#  else
//    return string_view(p.data() + 49, p.find(';', 49) - 49);
//#  endif
//#elif defined(_MSC_VER)
//    string_view p = __FUNCSIG__;
//    return string_view(p.data() + 84, p.size() - 84 - 7);
//#endif
//}



void startStream() {
//std::function <void(uint8_t*, int, int)> onCamBuffer;
    onCamBuffer = [](MMAL_BUFFER_HEADER_T *mmal_frame) {
//        if ((mmal_frame->flags) & MMAL_BUFFER_HEADER_FLAG_FRAME) {
//            for (uint8_t* i = mmal_frame->data + mmal_frame->offset; i < (mmal_frame->data + mmal_frame->offset + mmal_frame->length); ++i) {
//                fileContents.push_back(*i);
//            }
//        } else if ((mmal_frame->flags) & MMAL_BUFFER_HEADER_FLAG_FRAME_START) {
//            for (uint8_t* i = mmal_frame->data + mmal_frame->offset; i < (mmal_frame->data + mmal_frame->offset + mmal_frame->length); ++i) {
//                fileContents.push_back(*i);
//            }
//            return;
//        } else if ((mmal_frame->flags) & MMAL_BUFFER_HEADER_FLAG_FRAME_END) {
//            for (uint8_t* i = mmal_frame->data + mmal_frame->offset; i < (mmal_frame->data + mmal_frame->offset + mmal_frame->length); ++i) {
//                fileContents.push_back(*i);
//            }
//        } else {
//            for (uint8_t* i = mmal_frame->data + mmal_frame->offset; i < (mmal_frame->data + mmal_frame->offset + mmal_frame->length); ++i) {
//                fileContents.push_back(*i);
//            }
//            return;
//        }
        bool result = frame_queue->write_back(mmal_frame);
        FrameBuffer *buff = frame_queue->read_front();
//
//        if (buff->isMotionVector()) {
//
//            return;
//        }
//        printf("Write length %d \n", buff->length());
//        printf("Data type %s", type_name<decltype(*(buff->data()))>());
//        for (int i = 0; i < buff->length(); ++i) {
//            printf("%s", to_string(*(buff->data() + i)));
//        }
//        printf("\n");
//        printf("isFrame!\n");

//        printf("offset: %d flags %d alloc %d\n", mmal_frame->offset, mmal_frame->flags, mmal_frame->alloc_size);
//        printf("time %d %d\n", mmal_frame->pts, mmal_frame->dts);
        vector<ClientTrack> tracks{};
        for(auto id_client: clients) {
            auto id = id_client.first;
            auto client = id_client.second;
            auto optTrackData = client->video;
            if (client->getState() == Client::State::Ready && optTrackData.has_value()) {
                auto trackData = optTrackData.value();
                tracks.push_back(ClientTrack(id, trackData));
            }
        }

//        std::vector<std::uint8_t> raw_data( (std::uint8_t*)&(mmal_frame->cmd)
//                                   , (std::uint8_t*)&(mmal_frame->cmd) + sizeof(std::uint32_t));

        vector<uint8_t> fileContents;

        for (int i = 0; i < mmal_frame->length; ++i) {
            fileContents.push_back(*(mmal_frame->data + i));
        }

//        printf("Header length %d\n", raw_data.size());
//        for (int i = 0; i < raw_data.size(); ++i) {
//            fileContents.push_back(raw_data[i]);
//        }

//        for (uint8_t* i = mmal_frame->data + mmal_frame->offset; i < (mmal_frame->data + mmal_frame->offset + mmal_frame->length); ++i) {
//            fileContents.push_back(*i);
//        }

        rtc::binary sample = *reinterpret_cast<vector<byte>*>(&fileContents);
//
//        size_t i = 0;
//        while (i < sample.size()) {
//            assert(i + 4 < sample.size());
//            auto lengthPtr = (uint32_t *) (sample.data() + i);
//            uint32_t length = ntohl(*lengthPtr);
//            auto naluStartIndex = i + 4;
//            auto naluEndIndex = naluStartIndex + length;
//            assert(naluEndIndex <= sample.size());
//            auto header = reinterpret_cast<rtc::NalUnitHeader *>(sample.data() + naluStartIndex);
//            auto type = header->unitType();
//            switch (type) {
//                case 7:
////                    previousUnitType7 = {sample.begin() + i, sample.begin() + naluEndIndex};
//                    break;
//                case 8:
////                    previousUnitType8 = {sample.begin() + i, sample.begin() + naluEndIndex};;
//                    break;
//                case 5:
////                    previousUnitType5 = {sample.begin() + i, sample.begin() + naluEndIndex};;
//                    break;
//            }
//            i = naluEndIndex;
//            printf("%d\n", i);
//        }


        uint64_t sampleTime = mmal_frame->dts;
        printf("time %d", sampleTime);
        if (!tracks.empty()) {
            for (auto clientTrack: tracks) {
                auto client = clientTrack.id;
                auto trackData = clientTrack.trackData;
                // sample time is in us, we need to convert it to seconds
                auto elapsedSeconds = double(sampleTime) / (1000 * 1000);
                auto rtpConfig = trackData->sender->rtpConfig;
                // get elapsed time in clock rate
                uint32_t elapsedTimestamp = rtpConfig->secondsToTimestamp(elapsedSeconds);

                // set new timestamp
                rtpConfig->timestamp = rtpConfig->startTimestamp + elapsedTimestamp;

                // get elapsed time in clock rate from last RTCP sender report
                auto reportElapsedTimestamp = rtpConfig->timestamp - trackData->sender->previousReportedTimestamp;
                // check if last report was at least 1 second ago
                if (rtpConfig->timestampToSeconds(reportElapsedTimestamp) > 1) {
                    trackData->sender->setNeedsToReport();
                }

                cout << "Sending " << "video" << " sample with size: " << to_string(sample.size()) << " to " << client << endl;
                try {
                    // send sample
                    trackData->track->send(sample);
                    fileContents.clear();
                } catch (const std::exception &e) {
                    cerr << "Unable to send "<< "video" << " packet: " << e.what() << endl;
                }
            }
        }
    };
}

/// Add client to stream
/// @param client Client
/// @param adding_video True if adding video
void addToStream(shared_ptr<Client> client, bool isAddingVideo) {
    isAddingVideo = true;
//    if (client->getState() == Client::State::Waiting) {
//        printf("Stream Waiting\n");
//        client->setState(Client::State::WaitingForVideo);
//    } else if (client->getState() == Client::State::WaitingForVideo && isAddingVideo) {
        printf("Prepare stream\n");
        // Audio and video tracks are collected now
//        assert(client->video.has_value() && client->audio.has_value());

        auto video = client->video.value();
//        auto audio = client->audio.value();

        auto currentTime_us = double(currentTimeInMicroSeconds());
        auto currentTime_s = currentTime_us / (1000 * 1000);

        // set start time of stream
        video->sender->rtpConfig->setStartTime(currentTime_s, rtc::RtpPacketizationConfig::EpochStart::T1970);
//        audio->sender->rtpConfig->setStartTime(currentTime_s, RtpPacketizationConfig::EpochStart::T1970);

        // start stat recording of RTCP SR
        video->sender->startRecording();
//        audio->sender->startRecording();
//
//        if (avStream.has_value()) {
//            sendInitialNalus(avStream.value(), video);
//        }

        client->setState(Client::State::Ready);
//    }
    if (client->getState() == Client::State::Ready) {
        startStream();
        printf("StartStreaming!\n");

    }
}

// Create and setup a PeerConnection
shared_ptr<Client> createPeerConnection(const rtc::Configuration &config,
                                                WebSocket* ws,
                                                string id) {
    auto pc = make_shared<rtc::PeerConnection>(config);
    auto client = make_shared<Client>(pc);

    pc->onStateChange([id](rtc::PeerConnection::State state) {
        cout << "State: " << state << endl;
        if (state == rtc::PeerConnection::State::Disconnected ||
            state == rtc::PeerConnection::State::Failed ||
            state == rtc::PeerConnection::State::Closed) {
            // remove disconnected client
//            MainThread.dispatch([id]() {
//                clients.erase(id);
//            });
        }
    });

    pc->onGatheringStateChange(
        [wpc = make_weak_ptr(pc), id, ws](rtc::PeerConnection::GatheringState state) {
        cout << "Gathering State: " << state << endl;
        if (state == rtc::PeerConnection::GatheringState::Complete) {
            if(auto pc = wpc.lock()) {
                auto description = pc->localDescription();
                json sdp = {
                    {"type", description->typeString()},
                    {"sdp", string(description.value())}
                };
                json message = {
//                    {"id", id},
//                    {"type", description->typeString()},
//                    {"sdp", string(description.value())}
                    {"what", description->typeString()},
                    {"data", sdp.dump()}
                };
//                std::ostringstream o;
//                cout << "sdp value " << string(description.value());
                // Gathering complete, send answer
//                if (auto ws = wws.lock()) {
                    ws->send(message.dump());
//                }
            }
        }
    });
//
    client->video = addVideo(pc, 102, 1, "video-stream", "stream1", [id, wc = make_weak_ptr(client)]() {
        MainThread.dispatch([wc]() {
            if (auto c = wc.lock()) {
                addToStream(c, true);
            }
        });
        cout << "Video from " << id << " opened" << endl;
    });

//    client->audio = addAudio(pc, 111, 2, "audio-stream", "stream1", [id, wc = make_weak_ptr(client)]() {
//        MainThread.dispatch([wc]() {
//            if (auto c = wc.lock()) {
//                addToStream(c, false);
//            }
//        });
//        cout << "Audio from " << id << " opened" << endl;
//    });

    auto dc = pc->createDataChannel("ping-pong");
    dc->onOpen([id, wdc = make_weak_ptr(dc)]() {
        if (auto dc = wdc.lock()) {
            dc->send("Ping");
        }
    });

    dc->onMessage(nullptr, [id, wdc = make_weak_ptr(dc)](string msg) {
        cout << "Message from " << id << " received: " << msg << endl;
        if (auto dc = wdc.lock()) {
            dc->send("Ping");
        }
    });
    client->dataChannel = dc;

    pc->setLocalDescription();
    return client;
};

int main(int /*argc*/, const char* /*argv*/[]) {
//    if (gpioInitialise() < 0) {
//        return 1; // Failed to initialize.
//    }
//
//    gpioSetMode(26, PI_OUTPUT);
//
//    gpioWrite(26, PI_HIGH);
//    time_sleep(1.0);
//    gpioWrite(26, PI_LOW);
//
//    gpioTerminate(); // Compulsary.
    startCamera();

//    string stunServer = "stun:stun.l.google.com:19302";
//    config.iceServers.emplace_back(stunServer);
    config.disableAutoNegotiation = true;

    Server server(std::make_shared<PrintfLogger>());
    server.addWebSocketHandler("/control", std::make_shared<Handler>());
    server.addWebSocketHandler("/webrtc", std::make_shared<WebRTCHandler>());
    server.serve("static", 9000);
    return 0;
}
