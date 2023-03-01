#include "frame_queue.h"
#include <stdio.h>
#include <string.h>

bool FrameBuffer::copy(const MMAL_BUFFER_HEADER_T *buffer) {
    // RTC_DCHECK(buffer != nullptr)
    //     << "Internal Error, MMAL Buffer pointer is NULL";
    // RTC_DCHECK(buffer->length < capacity_)
    //     << "Internal Error, Frame Buffer capacity is smaller then buffer "
    //        "capacity"
    flags_ = buffer->flags;
    std::memcpy(data_, buffer->data, buffer->length);
    length_ = buffer->length;
    // RTC_LOG(INFO) << "Frame copy : " << toString()
    //              << ", size: " << buffer->length;
    return true;
}

bool FrameBuffer::append(const MMAL_BUFFER_HEADER_T *buffer) {
    // RTC_DCHECK(buffer != nullptr)
    //     << "Internal Error, MMAL Buffer pointer is NULL";
    // RTC_DCHECK(buffer->length + length_ < capacity_)
    //     << "Internal Error, Frame Buffer capacity is smaller then buffer "
    //        "capacity";
    flags_ = buffer->flags;
    std::memcpy(data_ + length_, buffer->data, buffer->length);
    length_ += buffer->length;
    // RTC_LOG(INFO) << "Frame append : " << toString()
    //              << ", size: " << buffer->length;
    return true;
}

FrameQueue::FrameQueue() {}
FrameQueue::FrameQueue(size_t capacity, size_t buffer_size) {
    for (size_t index = 0; index < _capacity; index++) {
        FrameBuffer* buffer = new FrameBuffer(_buffer_size);
        _free_list.push_back(buffer);
    }
}

void FrameQueue::init(size_t capacity, size_t buffer_size) {
    printf("FrameQueue init num %d size %d\n", capacity, buffer_size);
    _capacity = capacity;
    _buffer_size = buffer_size;
    _inited = true;
    destroy();
    for (size_t index = 0; index < _capacity; index++) {
        FrameBuffer *buffer = new FrameBuffer(_buffer_size);
        _free_list.push_back(buffer);
    }
}

void FrameQueue::destroy() {
    for (FrameBuffer *buffer : _pending) {
        delete buffer;
    }
    _pending.clear();
    for (FrameBuffer *buffer : _free_list) {
        delete buffer;
    }
    _free_list.clear();
    for (FrameBuffer *buffer : _encoded_frame_queue) {
        delete buffer;
    }
    _encoded_frame_queue.clear();
}

FrameQueue::~FrameQueue() { destroy(); }

void FrameQueue::clear() {
    while (!_encoded_frame_queue.empty()) {
        _free_list.push_back(_encoded_frame_queue.front());
        _encoded_frame_queue.pop_front();
    }
}

size_t FrameQueue::size() const { return _encoded_frame_queue.size(); }

FrameBuffer* FrameQueue::read_front(bool wait_until_timeout) {
    if (_encoded_frame_queue.empty()) {
        return nullptr;
    }
    FrameBuffer* buffer = _encoded_frame_queue.front();
    _encoded_frame_queue.pop_front();
    _free_list.push_back(buffer);
    return buffer;
}

bool FrameQueue::write_back(MMAL_BUFFER_HEADER_T *mmal_frame) {
    if (!_inited || mmal_frame == nullptr) {
        return false;
    }

    if (mmal_frame->length == 0 && mmal_frame->flags == 0) {
        return true;
    }

    if (mmal_frame->length > _buffer_size || mmal_frame->length < 0) {
        return false;
    }

    FrameBuffer* buffer = nullptr;
    if (!_pending.empty()) {
        buffer = _pending.back();
        if (buffer->length() + mmal_frame->length > _buffer_size) {
            _free_list.push_back(_pending.front());
            _pending.clear();
            return false;
        }

        buffer->append(mmal_frame);
        if (buffer->isFrameEnd()) {
            _pending.pop_back();
            _encoded_frame_queue.push_back(buffer);
        }
        return true;
    }

    if (_free_list.empty()) {
        buffer = new FrameBuffer(_buffer_size, true);
    } else {
        while (_free_list.size() >= 1 && (buffer = _free_list.front())->isTemporary() == true) {
            _free_list.pop_front();
            delete buffer;
        }
        _free_list.pop_front();
    }

    buffer->reset();
    buffer->copy(mmal_frame);
    if (buffer->isConfig()) {
        _pending.push_back(buffer);
        return true;
    }

    _encoded_frame_queue.push_back(buffer);
    return true;
}
