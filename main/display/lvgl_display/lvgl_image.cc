#include "lvgl_image.h"
#include "gif/lvgl_gif.h"
#include <cbin_font.h>

#include <esp_log.h>
#include <stdexcept>
#include <cstring>
#include <esp_heap_caps.h>

#define TAG "LvglImage"


LvglRawImage::LvglRawImage(void* data, size_t size) {
    bzero(&image_dsc_, sizeof(image_dsc_));
    image_dsc_.data_size = size;
    image_dsc_.data = static_cast<uint8_t*>(data);
    image_dsc_.header.magic = LV_IMAGE_HEADER_MAGIC;
    image_dsc_.header.cf = LV_COLOR_FORMAT_RAW_ALPHA;
    image_dsc_.header.w = 0;
    image_dsc_.header.h = 0;
}

bool LvglRawImage::IsGif() const {
    auto ptr = (const uint8_t*)image_dsc_.data;
    return ptr[0] == 'G' && ptr[1] == 'I' && ptr[2] == 'F';
}

LvglCBinImage::LvglCBinImage(void* data) {
    image_dsc_ = cbin_img_dsc_create(static_cast<uint8_t*>(data));
}

LvglCBinImage::~LvglCBinImage() {
    if (image_dsc_ != nullptr) {
        cbin_img_dsc_delete(image_dsc_);
    }
}

LvglAllocatedImage::LvglAllocatedImage(void* data, size_t size) {
    bzero(&image_dsc_, sizeof(image_dsc_));
    image_dsc_.data_size = size;
    image_dsc_.data = static_cast<uint8_t*>(data);

    if (lv_image_decoder_get_info(&image_dsc_, &image_dsc_.header) != LV_RESULT_OK) {
        ESP_LOGE(TAG, "Failed to get image info, data: %p size: %u", data, size);
        throw std::runtime_error("Failed to get image info");
    }
}

bool LvglAllocatedImage::IsGif() const {
    auto ptr = (const uint8_t*)image_dsc_.data;
    return image_dsc_.data_size > 3 && ptr[0] == 'G' && ptr[1] == 'I' && ptr[2] == 'F';
}

LvglAllocatedImage::LvglAllocatedImage(void* data, size_t size, int width, int height, int stride, int color_format) {
    bzero(&image_dsc_, sizeof(image_dsc_));
    image_dsc_.data_size = size;
    image_dsc_.data = static_cast<uint8_t*>(data);
    image_dsc_.header.magic = LV_IMAGE_HEADER_MAGIC;
    image_dsc_.header.cf = color_format;
    image_dsc_.header.w = width;
    image_dsc_.header.h = height;
    image_dsc_.header.stride = stride;
}

LvglAllocatedImage::~LvglAllocatedImage() {
    if (image_dsc_.data) {
        heap_caps_free((void*)image_dsc_.data);
        image_dsc_.data = nullptr;
    }
}

// ===== LvglGifImage implementation =====

LvglGifImage::LvglGifImage(void* data, size_t size)
    : raw_data_(data), raw_size_(size) {
    // LvglGif constructor takes lv_img_dsc_t*
    // Create a temporary descriptor with the raw GIF data
    auto* raw_dsc = new lv_img_dsc_t();
    bzero(raw_dsc, sizeof(lv_img_dsc_t));
    raw_dsc->data_size = size;
    raw_dsc->data = static_cast<uint8_t*>(data);
    raw_dsc->header.magic = LV_IMAGE_HEADER_MAGIC;
    raw_dsc->header.cf = LV_COLOR_FORMAT_RAW_ALPHA;

    gif_ = std::make_unique<LvglGif>(raw_dsc);
    delete raw_dsc; // LvglGif copies what it needs

    if (!gif_ || !gif_->IsLoaded()) {
        ESP_LOGE(TAG, "Failed to load GIF animation");
    }
}

LvglGifImage::~LvglGifImage() {
    Stop();
    if (raw_data_) {
        heap_caps_free(raw_data_);
        raw_data_ = nullptr;
    }
}

const lv_img_dsc_t* LvglGifImage::image_dsc() const {
    if (!gif_) return nullptr;
    return gif_->image_dsc();
}

void LvglGifImage::Play(lv_obj_t* widget, int loop_count) {
    if (!gif_ || !gif_->IsLoaded()) return;
    widget_ = widget;

    gif_->SetLoopCount(loop_count);

    // Auto-invalidate the widget on each frame
    if (widget_) {
        gif_->SetFrameCallback([this]() {
            if (widget_) {
                lv_obj_invalidate(widget_);
            }
        });
    }

    gif_->Start();
}

void LvglGifImage::Stop() {
    if (gif_) {
        gif_->Stop();
    }
}

bool LvglGifImage::IsLoaded() const {
    return gif_ && gif_->IsLoaded();
}
