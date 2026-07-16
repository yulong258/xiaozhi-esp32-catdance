#pragma once

#include <lvgl.h>
#include <memory>
#include <functional>

// Wrap around lv_img_dsc_t
class LvglImage {
public:
    virtual const lv_img_dsc_t* image_dsc() const = 0;
    virtual bool IsGif() const { return false; }
    virtual ~LvglImage() = default;
};


class LvglRawImage : public LvglImage {
public:
    LvglRawImage(void* data, size_t size);
    virtual const lv_img_dsc_t* image_dsc() const override { return &image_dsc_; }
    virtual bool IsGif() const;

private:
    lv_img_dsc_t image_dsc_;
};

class LvglCBinImage : public LvglImage {
public:
    LvglCBinImage(void* data);
    virtual ~LvglCBinImage();
    virtual const lv_img_dsc_t* image_dsc() const override { return image_dsc_; }

private:
    lv_img_dsc_t* image_dsc_ = nullptr;
};

class LvglSourceImage : public LvglImage {
public:
    LvglSourceImage(const lv_img_dsc_t* image_dsc) : image_dsc_(image_dsc) {}
    virtual const lv_img_dsc_t* image_dsc() const override { return image_dsc_; }

private:
    const lv_img_dsc_t* image_dsc_;
};

class LvglAllocatedImage : public LvglImage {
public:
    LvglAllocatedImage(void* data, size_t size);
    LvglAllocatedImage(void* data, size_t size, int width, int height, int stride, int color_format);
    virtual ~LvglAllocatedImage();
    virtual const lv_img_dsc_t* image_dsc() const override { return &image_dsc_; }
    virtual bool IsGif() const override;

private:
    lv_img_dsc_t image_dsc_;
};

// Forward declare LvglGif
class LvglGif;

/**
 * LvglGifImage - Animated GIF image that wraps LvglGif.
 * Inherits LvglImage so it can be used with LcdDisplay::SetPreviewImage().
 * Calls LvglGif internally to decode and play the animation.
 */
class LvglGifImage : public LvglImage {
public:
    LvglGifImage(void* data, size_t size);
    virtual ~LvglGifImage() override;
    virtual const lv_img_dsc_t* image_dsc() const override;
    virtual bool IsGif() const override { return true; }

    /** Start playing the GIF. @param loop_count 0=infinite */
    void Play(lv_obj_t* widget, int loop_count = 0);

    /** Stop playing. */
    void Stop();

    /** Check if GIF loaded successfully. */
    bool IsLoaded() const;

private:
    std::unique_ptr<LvglGif> gif_;
    void* raw_data_ = nullptr;
    size_t raw_size_ = 0;
    lv_obj_t* widget_ = nullptr;
};
