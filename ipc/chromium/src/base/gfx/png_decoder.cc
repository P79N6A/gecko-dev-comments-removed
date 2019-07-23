



#include "base/gfx/png_decoder.h"

#include "base/logging.h"
#include "skia/include/SkBitmap.h"

extern "C" {
#include "third_party/libpng/png.h"
}

namespace {


void ConvertBetweenBGRAandRGBA(const unsigned char* input, int pixel_width,
                               unsigned char* output) {
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &input[x * 4];
    unsigned char* pixel_out = &output[x * 4];
    pixel_out[0] = pixel_in[2];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[0];
    pixel_out[3] = pixel_in[3];
  }
}

void ConvertRGBAtoRGB(const unsigned char* rgba, int pixel_width,
                      unsigned char* rgb) {
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &rgba[x * 4];
    unsigned char* pixel_out = &rgb[x * 3];
    pixel_out[0] = pixel_in[0];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[2];
  }
}

}  






namespace {


const double kMaxGamma = 21474.83;  
const double kDefaultGamma = 2.2;
const double kInverseGamma = 1.0 / kDefaultGamma;


const png_uint_32 kMaxSize = 4096;

class PngDecoderState {
 public:
  PngDecoderState(PNGDecoder::ColorFormat ofmt, std::vector<unsigned char>* o)
      : output_format(ofmt),
        output_channels(0),
        output(o),
        row_converter(NULL),
        width(0),
        height(0),
        done(false) {
  }

  PNGDecoder::ColorFormat output_format;
  int output_channels;

  std::vector<unsigned char>* output;

  
  
  void (*row_converter)(const unsigned char* in, int w, unsigned char* out);

  
  int width;
  int height;

  
  bool done;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(PngDecoderState);
};

void ConvertRGBtoRGBA(const unsigned char* rgb, int pixel_width,
                      unsigned char* rgba) {
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &rgb[x * 3];
    unsigned char* pixel_out = &rgba[x * 4];
    pixel_out[0] = pixel_in[0];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[2];
    pixel_out[3] = 0xff;
  }
}

void ConvertRGBtoBGRA(const unsigned char* rgb, int pixel_width,
                      unsigned char* bgra) {
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &rgb[x * 3];
    unsigned char* pixel_out = &bgra[x * 4];
    pixel_out[0] = pixel_in[2];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[0];
    pixel_out[3] = 0xff;
  }
}



void DecodeInfoCallback(png_struct* png_ptr, png_info* info_ptr) {
  PngDecoderState* state = static_cast<PngDecoderState*>(
      png_get_progressive_ptr(png_ptr));

  int bit_depth, color_type, interlace_type, compression_type;
  int filter_type, channels;
  png_uint_32 w, h;
  png_get_IHDR(png_ptr, info_ptr, &w, &h, &bit_depth, &color_type,
               &interlace_type, &compression_type, &filter_type);

  
  
  if (w > kMaxSize || h > kMaxSize)
    longjmp(png_ptr->jmpbuf, 1);
  state->width = static_cast<int>(w);
  state->height = static_cast<int>(h);

  
  if (color_type == PNG_COLOR_TYPE_PALETTE ||
      (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8))
    png_set_expand(png_ptr);

  
  if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
    png_set_expand(png_ptr);

  
  if (bit_depth == 16)
    png_set_strip_16(png_ptr);

  
  if (color_type == PNG_COLOR_TYPE_GRAY ||
      color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
    png_set_gray_to_rgb(png_ptr);

  
  double gamma;
  if (png_get_gAMA(png_ptr, info_ptr, &gamma)) {
    if (gamma <= 0.0 || gamma > kMaxGamma) {
      gamma = kInverseGamma;
      png_set_gAMA(png_ptr, info_ptr, gamma);
    }
    png_set_gamma(png_ptr, kDefaultGamma, gamma);
  } else {
    png_set_gamma(png_ptr, kDefaultGamma, kInverseGamma);
  }

  
  if (interlace_type == PNG_INTERLACE_ADAM7)
    png_set_interlace_handling(png_ptr);

  
  png_read_update_info(png_ptr, info_ptr);
  channels = png_get_channels(png_ptr, info_ptr);

  
  if (channels == 3) {
    switch (state->output_format) {
      case PNGDecoder::FORMAT_RGB:
        state->row_converter = NULL;  
        state->output_channels = 3;
        break;
      case PNGDecoder::FORMAT_RGBA:
        state->row_converter = &ConvertRGBtoRGBA;
        state->output_channels = 4;
        break;
      case PNGDecoder::FORMAT_BGRA:
        state->row_converter = &ConvertRGBtoBGRA;
        state->output_channels = 4;
        break;
      default:
        NOTREACHED() << "Unknown output format";
        break;
    }
  } else if (channels == 4) {
    switch (state->output_format) {
      case PNGDecoder::FORMAT_RGB:
        state->row_converter = &ConvertRGBAtoRGB;
        state->output_channels = 3;
        break;
      case PNGDecoder::FORMAT_RGBA:
        state->row_converter = NULL;  
        state->output_channels = 4;
        break;
      case PNGDecoder::FORMAT_BGRA:
        state->row_converter = &ConvertBetweenBGRAandRGBA;
        state->output_channels = 4;
        break;
      default:
        NOTREACHED() << "Unknown output format";
        break;
    }
  } else {
    NOTREACHED() << "Unknown input channels";
    longjmp(png_ptr->jmpbuf, 1);
  }

  state->output->resize(state->width * state->output_channels * state->height);
}

void DecodeRowCallback(png_struct* png_ptr, png_byte* new_row,
                       png_uint_32 row_num, int pass) {
  PngDecoderState* state = static_cast<PngDecoderState*>(
      png_get_progressive_ptr(png_ptr));

  DCHECK(pass == 0) << "We didn't turn on interlace handling, but libpng is "
                       "giving us interlaced data.";
  if (static_cast<int>(row_num) > state->height) {
    NOTREACHED() << "Invalid row";
    return;
  }

  unsigned char* dest = &(*state->output)[
      state->width * state->output_channels * row_num];
  if (state->row_converter)
    state->row_converter(new_row, state->width, dest);
  else
    memcpy(dest, new_row, state->width * state->output_channels);
}

void DecodeEndCallback(png_struct* png_ptr, png_info* info) {
  PngDecoderState* state = static_cast<PngDecoderState*>(
      png_get_progressive_ptr(png_ptr));

  
  
  state->done = true;
}



class PngReadStructDestroyer {
 public:
  PngReadStructDestroyer(png_struct** ps, png_info** pi) : ps_(ps), pi_(pi) {
  }
  ~PngReadStructDestroyer() {
    png_destroy_read_struct(ps_, pi_, NULL);
  }
 private:
  png_struct** ps_;
  png_info** pi_;
};

}  


bool PNGDecoder::Decode(const unsigned char* input, size_t input_size,
                      ColorFormat format, std::vector<unsigned char>* output,
                      int* w, int* h) {
  if (input_size < 8)
    return false;  

  
  if (png_sig_cmp(const_cast<unsigned char*>(input), 0, 8) != 0)
    return false;

  png_struct* png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
                                               png_voidp_NULL,
                                               png_error_ptr_NULL,
                                               png_error_ptr_NULL);
  if (!png_ptr)
    return false;

  png_info* info_ptr = png_create_info_struct(png_ptr);
  if (!info_ptr) {
    png_destroy_read_struct(&png_ptr, NULL, NULL);
    return false;
  }

  PngReadStructDestroyer destroyer(&png_ptr, &info_ptr);
  if (setjmp(png_jmpbuf(png_ptr))) {
    
    
    
    return false;
  }

  PngDecoderState state(format, output);

  png_set_progressive_read_fn(png_ptr, &state, &DecodeInfoCallback,
                              &DecodeRowCallback, &DecodeEndCallback);
  png_process_data(png_ptr,
                   info_ptr,
                   const_cast<unsigned char*>(input),
                   input_size);

  if (!state.done) {
    
    
    output->clear();
    return false;
  }

  *w = state.width;
  *h = state.height;
  return true;
}


bool PNGDecoder::Decode(const std::vector<unsigned char>* data,
                        SkBitmap* bitmap) {
  DCHECK(bitmap);
  if (!data || data->empty())
    return false;
  int width, height;
  std::vector<unsigned char> decoded_data;
  if (PNGDecoder::Decode(&data->front(), data->size(), PNGDecoder::FORMAT_BGRA,
                         &decoded_data, &width, &height)) {
    bitmap->setConfig(SkBitmap::kARGB_8888_Config, width, height);
    bitmap->allocPixels();
    memcpy(bitmap->getPixels(), &decoded_data.front(), width * height * 4);
    return true;
  }
  return false;
}


SkBitmap* PNGDecoder::CreateSkBitmapFromBGRAFormat(
    std::vector<unsigned char>& bgra, int width, int height) {
  SkBitmap* bitmap = new SkBitmap();
  bitmap->setConfig(SkBitmap::kARGB_8888_Config, width, height);
  bitmap->allocPixels();

  bool opaque = false;
  unsigned char* bitmap_data =
      reinterpret_cast<unsigned char*>(bitmap->getAddr32(0, 0));
  for (int i = width * height * 4 - 4; i >= 0; i -= 4) {
    unsigned char alpha = bgra[i + 3];
    if (!opaque && alpha != 255) {
      opaque = false;
    }
    bitmap_data[i + 3] = alpha;
    bitmap_data[i] = (bgra[i] * alpha) >> 8;
    bitmap_data[i + 1] = (bgra[i + 1] * alpha) >> 8;
    bitmap_data[i + 2] = (bgra[i + 2] * alpha) >> 8;
  }

  bitmap->setIsOpaque(opaque);
  return bitmap;
}
