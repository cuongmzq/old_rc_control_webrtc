#ifndef __FRAME_QUEUE_H__
#define __FRAME_QUEUE_H__
#include <bitset>
#include <deque>
#include <memory>
#include <mutex>
#include <vector>

#include "interface/mmal/mmal.h"

constexpr int kFrameBufferFlagSize = 8;

constexpr int kFrameFlagEOS = 0;
constexpr int kFrameFlagStart = 1;
constexpr int kFrameFlagEnd = 2;
constexpr int kFrameFlagKeyFrame = 3;
constexpr int kFrameFlagConfig = 5;
constexpr int kFrameFlagIdeInfo = 7;  // inline motion vector


struct FrameBuffer {
    explicit FrameBuffer(size_t capacity) : FrameBuffer(capacity, false) {}
    explicit FrameBuffer(size_t capacity, bool temporary) {
        data_ = static_cast<uint8_t *>(malloc(capacity));
        capacity_ = capacity;
        length_ = 0;
        temporary_ = temporary;
    }
    ~FrameBuffer() { free(data_); }

    inline bool isKeyFrame() const { return flags_[kFrameFlagKeyFrame]; }
    inline bool isFrame() const {
        return flags_[kFrameFlagStart] && flags_[kFrameFlagEnd];
    }
    inline bool isFrameEnd() const { return flags_[kFrameFlagEnd]; }
    inline bool isMotionVector() const { return flags_[kFrameFlagIdeInfo]; }
    inline bool isConfig() const { return flags_[kFrameFlagConfig]; }
    inline bool isEOS() const { return flags_[kFrameFlagEOS]; }
    inline void reset() {
        flags_.reset();
        length_ = 0;
    }
    inline size_t length() const { return length_; }
    inline uint8_t *data() const { return data_; }
    inline std::string toString() {
        return flags_.to_string<char, std::string::traits_type,
                                std::string::allocator_type>();
    }
    inline bool isTemporary() const { return temporary_; }
    // disable copy consructor
    FrameBuffer(const FrameBuffer &) = delete;
    FrameBuffer &operator=(const FrameBuffer &) = delete;
    bool copy(const MMAL_BUFFER_HEADER_T *buffer);
    bool append(const MMAL_BUFFER_HEADER_T *buffer);

   private:
    bool temporary_;
    std::bitset<kFrameBufferFlagSize> flags_;
    uint8_t *data_;
    size_t length_;
    size_t capacity_;
};


class FrameQueue {
    public:
        FrameQueue();
        FrameQueue(size_t capacity, size_t buffer_size);
        ~FrameQueue();
        void init(size_t capacity, size_t buffer_size);
        FrameBuffer *read_front(bool wait_until_timeout = true);

        size_t size() const;
        void clear();
        bool write_back(MMAL_BUFFER_HEADER_T *buffer);

    private:
        static int kEventWaitPeriod;
        //mutex
        void destroy();

        bool _inited;
        size_t _capacity, _buffer_size;
        std::deque<FrameBuffer *> _encoded_frame_queue;
        std::deque<FrameBuffer *> _free_list;
        std::vector<FrameBuffer *> _pending;
};
#endif
