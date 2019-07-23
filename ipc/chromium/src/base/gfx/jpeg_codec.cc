



#include "base/gfx/jpeg_codec.h"

#include <setjmp.h>

#include "base/logging.h"
#include "base/scoped_ptr.h"
#include "skia/include/SkBitmap.h"

extern "C" {
#include "third_party/libjpeg/jpeglib.h"
}



namespace {


struct CoderErrorMgr {
  jpeg_error_mgr pub;
  jmp_buf setjmp_buffer;
};

void ErrorExit(jpeg_common_struct* cinfo) {
  CoderErrorMgr *err = reinterpret_cast<CoderErrorMgr*>(cinfo->err);

  
  longjmp(err->setjmp_buffer, false);
}

}  






namespace {


const static int initial_output_buffer_size = 8192;

struct JpegEncoderState {
  JpegEncoderState(std::vector<unsigned char>* o)
      : out(o),
        image_buffer_used(0) {
  }

  
  
  
  std::vector<unsigned char>* out;

  
  size_t image_buffer_used;
};








void InitDestination(jpeg_compress_struct* cinfo) {
  JpegEncoderState* state = static_cast<JpegEncoderState*>(cinfo->client_data);
  DCHECK(state->image_buffer_used == 0) << "initializing after use";

  state->out->resize(initial_output_buffer_size);
  state->image_buffer_used = 0;

  cinfo->dest->next_output_byte = &(*state->out)[0];
  cinfo->dest->free_in_buffer = initial_output_buffer_size;
}













boolean EmptyOutputBuffer(jpeg_compress_struct* cinfo) {
  JpegEncoderState* state = static_cast<JpegEncoderState*>(cinfo->client_data);

  
  state->image_buffer_used = state->out->size();

  
  state->out->resize(state->out->size() * 2);

  
  cinfo->dest->next_output_byte = &(*state->out)[state->image_buffer_used];
  cinfo->dest->free_in_buffer = state->out->size() - state->image_buffer_used;
  return 1;
}








void TermDestination(jpeg_compress_struct* cinfo) {
  JpegEncoderState* state = static_cast<JpegEncoderState*>(cinfo->client_data);
  DCHECK(state->out->size() >= state->image_buffer_used);

  
  state->image_buffer_used = cinfo->dest->next_output_byte - &(*state->out)[0];
  DCHECK(state->image_buffer_used < state->out->size()) <<
    "JPEG library busted, got a bad image buffer size";

  
  state->out->resize(state->image_buffer_used);
}





void StripAlpha(const unsigned char* rgba, int pixel_width, unsigned char* rgb)
{
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &rgba[x * 4];
    unsigned char* pixel_out = &rgb[x * 3];
    pixel_out[0] = pixel_in[0];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[2];
  }
}





void BGRAtoRGB(const unsigned char* bgra, int pixel_width, unsigned char* rgb)
{
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &bgra[x * 4];
    unsigned char* pixel_out = &rgb[x * 3];
    pixel_out[0] = pixel_in[2];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[0];
  }
}




class CompressDestroyer {
 public:
  CompressDestroyer() : cinfo_(NULL) {
  }
  ~CompressDestroyer() {
    DestroyManagedObject();
  }
  void SetManagedObject(jpeg_compress_struct* ci) {
    DestroyManagedObject();
    cinfo_ = ci;
  }
  void DestroyManagedObject() {
    if (cinfo_) {
      jpeg_destroy_compress(cinfo_);
      cinfo_ = NULL;
    }
  }
 private:
  jpeg_compress_struct* cinfo_;
};

}  

bool JPEGCodec::Encode(const unsigned char* input, ColorFormat format,
                       int w, int h, int row_byte_width,
                       int quality, std::vector<unsigned char>* output) {
  jpeg_compress_struct cinfo;
  CompressDestroyer destroyer;
  output->clear();

  
  
  CoderErrorMgr errmgr;
  cinfo.err = jpeg_std_error(&errmgr.pub);
  errmgr.pub.error_exit = ErrorExit;
  
  if (setjmp(errmgr.setjmp_buffer)) {
    
    
    
    
    
    destroyer.DestroyManagedObject();
    return false;
  }

  
  jpeg_create_compress(&cinfo);
  destroyer.SetManagedObject(&cinfo);

  cinfo.image_width = w;
  cinfo.image_height = h;
  cinfo.input_components = 3;
  cinfo.in_color_space = JCS_RGB;
  cinfo.data_precision = 8;

  jpeg_set_defaults(&cinfo);
  jpeg_set_quality(&cinfo, quality, 1);  

  
  jpeg_destination_mgr destmgr;
  destmgr.init_destination = InitDestination;
  destmgr.empty_output_buffer = EmptyOutputBuffer;
  destmgr.term_destination = TermDestination;
  cinfo.dest = &destmgr;

  JpegEncoderState state(output);
  cinfo.client_data = &state;

  jpeg_start_compress(&cinfo, 1);

  
  if (format == FORMAT_RGB) {
    
    while (cinfo.next_scanline < cinfo.image_height) {
      const unsigned char* row = &input[cinfo.next_scanline * row_byte_width];
      jpeg_write_scanlines(&cinfo, const_cast<unsigned char**>(&row), 1);
    }
  } else {
    
    void (*converter)(const unsigned char* in, int w, unsigned char* rgb);
    if (format == FORMAT_RGBA) {
      converter = StripAlpha;
    } else if (format == FORMAT_BGRA) {
      converter = BGRAtoRGB;
    } else {
      NOTREACHED() << "Invalid pixel format";
      return false;
    }

    
    unsigned char* row = new unsigned char[w * 3];

    while (cinfo.next_scanline < cinfo.image_height) {
      converter(&input[cinfo.next_scanline * row_byte_width], w, row);
      jpeg_write_scanlines(&cinfo, &row, 1);
    }
    delete[] row;
  }

  jpeg_finish_compress(&cinfo);
  return true;
}



namespace {

struct JpegDecoderState {
  JpegDecoderState(const unsigned char* in, size_t len)
      : input_buffer(in), input_buffer_length(len) {
  }

  const unsigned char* input_buffer;
  size_t input_buffer_length;
};







void InitSource(j_decompress_ptr cinfo) {
  JpegDecoderState* state = static_cast<JpegDecoderState*>(cinfo->client_data);
  cinfo->src->next_input_byte = state->input_buffer;
  cinfo->src->bytes_in_buffer = state->input_buffer_length;
}














boolean FillInputBuffer(j_decompress_ptr cinfo) {
  return false;
}














void SkipInputData(j_decompress_ptr cinfo, long num_bytes) {
  if (num_bytes > static_cast<long>(cinfo->src->bytes_in_buffer)) {
    
    
    
    cinfo->src->next_input_byte += cinfo->src->bytes_in_buffer;
    cinfo->src->bytes_in_buffer = 0;
  } else if (num_bytes > 0) {
    cinfo->src->bytes_in_buffer -= static_cast<size_t>(num_bytes);
    cinfo->src->next_input_byte += num_bytes;
  }
}







void TermSource(j_decompress_ptr cinfo) {
}



void AddAlpha(const unsigned char* rgb, int pixel_width, unsigned char* rgba) {
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &rgb[x * 3];
    unsigned char* pixel_out = &rgba[x * 4];
    pixel_out[0] = pixel_in[0];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[2];
    pixel_out[3] = 0xff;
  }
}



void RGBtoBGRA(const unsigned char* bgra, int pixel_width, unsigned char* rgb)
{
  for (int x = 0; x < pixel_width; x++) {
    const unsigned char* pixel_in = &bgra[x * 3];
    unsigned char* pixel_out = &rgb[x * 4];
    pixel_out[0] = pixel_in[2];
    pixel_out[1] = pixel_in[1];
    pixel_out[2] = pixel_in[0];
    pixel_out[3] = 0xff;
  }
}




class DecompressDestroyer {
 public:
  DecompressDestroyer() : cinfo_(NULL) {
  }
  ~DecompressDestroyer() {
    DestroyManagedObject();
  }
  void SetManagedObject(jpeg_decompress_struct* ci) {
    DestroyManagedObject();
    cinfo_ = ci;
  }
  void DestroyManagedObject() {
    if (cinfo_) {
      jpeg_destroy_decompress(cinfo_);
      cinfo_ = NULL;
    }
  }
 private:
  jpeg_decompress_struct* cinfo_;
};

}  

bool JPEGCodec::Decode(const unsigned char* input, size_t input_size,
                       ColorFormat format, std::vector<unsigned char>* output,
                       int* w, int* h) {
  jpeg_decompress_struct cinfo;
  DecompressDestroyer destroyer;
  output->clear();

  
  
  CoderErrorMgr errmgr;
  cinfo.err = jpeg_std_error(&errmgr.pub);
  errmgr.pub.error_exit = ErrorExit;
  
  if (setjmp(errmgr.setjmp_buffer)) {
    
    
    
    destroyer.DestroyManagedObject();
    return false;
  }

  
  
  jpeg_create_decompress(&cinfo);
  destroyer.SetManagedObject(&cinfo);

  
  jpeg_source_mgr srcmgr;
  srcmgr.init_source = InitSource;
  srcmgr.fill_input_buffer = FillInputBuffer;
  srcmgr.skip_input_data = SkipInputData;
  srcmgr.resync_to_restart = jpeg_resync_to_restart;  
  srcmgr.term_source = TermSource;
  cinfo.src = &srcmgr;

  JpegDecoderState state(input, input_size);
  cinfo.client_data = &state;

  
  if (jpeg_read_header(&cinfo, true) != JPEG_HEADER_OK)
    return false;

  
  switch (cinfo.jpeg_color_space) {
    case JCS_GRAYSCALE:
    case JCS_RGB:
    case JCS_YCbCr:
      cinfo.out_color_space = JCS_RGB;
      break;
    case JCS_CMYK:
    case JCS_YCCK:
    default:
      
      
      
      return false;
  }
  cinfo.output_components = 3;

  jpeg_calc_output_dimensions(&cinfo);
  *w = cinfo.output_width;
  *h = cinfo.output_height;

  jpeg_start_decompress(&cinfo);

  
  
  int row_read_stride = cinfo.output_width * cinfo.output_components;

  if (format == FORMAT_RGB) {
    
    int row_write_stride = row_read_stride;
    output->resize(row_write_stride * cinfo.output_height);

    for (int row = 0; row < static_cast<int>(cinfo.output_height); row++) {
      unsigned char* rowptr = &(*output)[row * row_write_stride];
      if (!jpeg_read_scanlines(&cinfo, &rowptr, 1))
        return false;
    }
  } else {
    
    
    
    int row_write_stride;
    void (*converter)(const unsigned char* rgb, int w, unsigned char* out);
    if (format == FORMAT_RGBA) {
      row_write_stride = cinfo.output_width * 4;
      converter = AddAlpha;
    } else if (format == FORMAT_BGRA) {
      row_write_stride = cinfo.output_width * 4;
      converter = RGBtoBGRA;
    } else {
      NOTREACHED() << "Invalid pixel format";
      jpeg_destroy_decompress(&cinfo);
      return false;
    }

    output->resize(row_write_stride * cinfo.output_height);

    scoped_array<unsigned char> row_data(new unsigned char[row_read_stride]);
    unsigned char* rowptr = row_data.get();
    for (int row = 0; row < static_cast<int>(cinfo.output_height); row++) {
      if (!jpeg_read_scanlines(&cinfo, &rowptr, 1))
        return false;
      converter(rowptr, *w, &(*output)[row * row_write_stride]);
    }
  }

  jpeg_finish_decompress(&cinfo);
  jpeg_destroy_decompress(&cinfo);
  return true;
}


SkBitmap* JPEGCodec::Decode(const unsigned char* input, size_t input_size) {
  int w, h;
  std::vector<unsigned char> data_vector;
  
  if (!Decode(input, input_size, FORMAT_BGRA, &data_vector, &w, &h))
    return NULL;

  
  int data_length = w * h * 4;

  SkBitmap* bitmap = new SkBitmap();
  bitmap->setConfig(SkBitmap::kARGB_8888_Config, w, h);
  bitmap->allocPixels();
  memcpy(bitmap->getAddr32(0, 0), &data_vector[0], data_length);

  return bitmap;
}
