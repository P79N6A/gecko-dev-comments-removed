









#include "libyuv/mjpeg_decoder.h"


#include <assert.h>
#ifndef __CLR_VER
#include <setjmp.h>
#define HAVE_SETJMP
#endif
#include <stdio.h>
#include <stdlib.h>

extern "C" {
#include <jpeglib.h>
}

#include <climits>
#include <cstring>

namespace libyuv {

#ifdef HAVE_SETJMP
struct SetJmpErrorMgr {
  jpeg_error_mgr base;  
  jmp_buf setjmp_buffer;
};
#endif

const int MJpegDecoder::kColorSpaceUnknown = JCS_UNKNOWN;
const int MJpegDecoder::kColorSpaceGrayscale = JCS_GRAYSCALE;
const int MJpegDecoder::kColorSpaceRgb = JCS_RGB;
const int MJpegDecoder::kColorSpaceYCbCr = JCS_YCbCr;
const int MJpegDecoder::kColorSpaceCMYK = JCS_CMYK;
const int MJpegDecoder::kColorSpaceYCCK = JCS_YCCK;

MJpegDecoder::MJpegDecoder()
    : has_scanline_padding_(false),
      num_outbufs_(0),
      scanlines_(NULL),
      scanlines_sizes_(NULL),
      databuf_(NULL),
      databuf_strides_(NULL) {
  decompress_struct_ = new jpeg_decompress_struct;
  source_mgr_ = new jpeg_source_mgr;
#ifdef HAVE_SETJMP
  error_mgr_ = new SetJmpErrorMgr;
  decompress_struct_->err = jpeg_std_error(&error_mgr_->base);
  
  error_mgr_->base.error_exit = &ErrorHandler;
#endif
  decompress_struct_->client_data = NULL;
  source_mgr_->init_source = &init_source;
  source_mgr_->fill_input_buffer = &fill_input_buffer;
  source_mgr_->skip_input_data = &skip_input_data;
  source_mgr_->resync_to_restart = &jpeg_resync_to_restart;
  source_mgr_->term_source = &term_source;
  jpeg_create_decompress(decompress_struct_);
  decompress_struct_->src = source_mgr_;
  buf_vec_.buffers = &buf_;
  buf_vec_.len = 1;
}

MJpegDecoder::~MJpegDecoder() {
  jpeg_destroy_decompress(decompress_struct_);
  delete decompress_struct_;
  delete source_mgr_;
#ifdef HAVE_SETJMP
  delete error_mgr_;
#endif
  DestroyOutputBuffers();
}



bool ValidateJpeg(const uint8* sample, size_t sample_size) {
  if (sample_size < 64) {
    
    return false;
  }
  if (sample[0] != 0xff || sample[1] != 0xd8) {
    
    return false;
  }
  bool soi = true;
  int total_eoi = 0;
  for (int i = 2; i < static_cast<int>(sample_size) - 1; ++i) {
    if (sample[i] == 0xff) {
      if (sample[i + 1] == 0xd8) {  
        soi = true;
      } else if (sample[i + 1] == 0xd9) {  
        if (soi) {
          ++total_eoi;
        }
        soi = false;
      }
    }
  }
  if (!total_eoi) {
    
    return false;
  }
  return true;
}

bool MJpegDecoder::LoadFrame(const uint8* src, size_t src_len) {
  if (!ValidateJpeg(src, src_len)) {
    return false;
  }

  buf_.data = src;
  buf_.len = static_cast<int>(src_len);
  buf_vec_.pos = 0;
  decompress_struct_->client_data = &buf_vec_;
#ifdef HAVE_SETJMP
  if (setjmp(error_mgr_->setjmp_buffer)) {
    
    
    return false;
  }
#endif
  if (jpeg_read_header(decompress_struct_, TRUE) != JPEG_HEADER_OK) {
    
    return false;
  }
  AllocOutputBuffers(GetNumComponents());
  for (int i = 0; i < num_outbufs_; ++i) {
    int scanlines_size = GetComponentScanlinesPerImcuRow(i);
    if (scanlines_sizes_[i] != scanlines_size) {
      if (scanlines_[i]) {
        delete scanlines_[i];
      }
      scanlines_[i] = new uint8* [scanlines_size];
      scanlines_sizes_[i] = scanlines_size;
    }

    
    
    
    
    
    
    int databuf_stride = GetComponentStride(i);
    int databuf_size = scanlines_size * databuf_stride;
    if (databuf_strides_[i] != databuf_stride) {
      if (databuf_[i]) {
        delete databuf_[i];
      }
      databuf_[i] = new uint8[databuf_size];
      databuf_strides_[i] = databuf_stride;
    }

    if (GetComponentStride(i) != GetComponentWidth(i)) {
      has_scanline_padding_ = true;
    }
  }
  return true;
}

static int DivideAndRoundUp(int numerator, int denominator) {
  return (numerator + denominator - 1) / denominator;
}

static int DivideAndRoundDown(int numerator, int denominator) {
  return numerator / denominator;
}


int MJpegDecoder::GetWidth() {
  return decompress_struct_->image_width;
}


int MJpegDecoder::GetHeight() {
  return decompress_struct_->image_height;
}



int MJpegDecoder::GetColorSpace() {
  return decompress_struct_->jpeg_color_space;
}


int MJpegDecoder::GetNumComponents() {
  return decompress_struct_->num_components;
}


int MJpegDecoder::GetHorizSampFactor(int component) {
  return decompress_struct_->comp_info[component].h_samp_factor;
}

int MJpegDecoder::GetVertSampFactor(int component) {
  return decompress_struct_->comp_info[component].v_samp_factor;
}

int MJpegDecoder::GetHorizSubSampFactor(int component) {
  return decompress_struct_->max_h_samp_factor /
      GetHorizSampFactor(component);
}

int MJpegDecoder::GetVertSubSampFactor(int component) {
  return decompress_struct_->max_v_samp_factor /
      GetVertSampFactor(component);
}

int MJpegDecoder::GetImageScanlinesPerImcuRow() {
  return decompress_struct_->max_v_samp_factor * DCTSIZE;
}

int MJpegDecoder::GetComponentScanlinesPerImcuRow(int component) {
  int vs = GetVertSubSampFactor(component);
  return DivideAndRoundUp(GetImageScanlinesPerImcuRow(), vs);
}

int MJpegDecoder::GetComponentWidth(int component) {
  int hs = GetHorizSubSampFactor(component);
  return DivideAndRoundUp(GetWidth(), hs);
}

int MJpegDecoder::GetComponentHeight(int component) {
  int vs = GetVertSubSampFactor(component);
  return DivideAndRoundUp(GetHeight(), vs);
}


int MJpegDecoder::GetComponentStride(int component) {
  return (GetComponentWidth(component) + DCTSIZE - 1) & ~(DCTSIZE - 1);
}

int MJpegDecoder::GetComponentSize(int component) {
  return GetComponentWidth(component) * GetComponentHeight(component);
}

bool MJpegDecoder::UnloadFrame() {
#ifdef HAVE_SETJMP
  if (setjmp(error_mgr_->setjmp_buffer)) {
    
    
    return false;
  }
#endif
  jpeg_abort_decompress(decompress_struct_);
  return true;
}

static void CopyRows(uint8* source, int source_stride,
                     uint8* dest, int pixels, int numrows) {
  for (int i = 0; i < numrows; ++i) {
    memcpy(dest, source, pixels);
    dest += pixels;
    source += source_stride;
  }
}


bool MJpegDecoder::DecodeToBuffers(
    uint8** planes, int dst_width, int dst_height) {
  if (dst_width != GetWidth() ||
      dst_height > GetHeight()) {
    
    return false;
  }
#ifdef HAVE_SETJMP
  if (setjmp(error_mgr_->setjmp_buffer)) {
    
    
    
    return false;
  }
#endif
  if (!StartDecode()) {
    return false;
  }
  SetScanlinePointers(databuf_);
  int lines_left = dst_height;
  
  
  
  int skip = (GetHeight() - dst_height) / 2;
  if (skip > 0) {
    
    
    while (skip >= GetImageScanlinesPerImcuRow()) {
      if (!DecodeImcuRow()) {
        FinishDecode();
        return false;
      }
      skip -= GetImageScanlinesPerImcuRow();
    }
    if (skip > 0) {
      
      
      if (!DecodeImcuRow()) {
        FinishDecode();
        return false;
      }
      for (int i = 0; i < num_outbufs_; ++i) {
        
        assert(skip % GetVertSubSampFactor(i) == 0);
        int rows_to_skip =
            DivideAndRoundDown(skip, GetVertSubSampFactor(i));
        int scanlines_to_copy = GetComponentScanlinesPerImcuRow(i) -
                                rows_to_skip;
        int data_to_skip = rows_to_skip * GetComponentStride(i);
        CopyRows(databuf_[i] + data_to_skip, GetComponentStride(i),
                 planes[i], GetComponentWidth(i), scanlines_to_copy);
        planes[i] += scanlines_to_copy * GetComponentWidth(i);
      }
      lines_left -= (GetImageScanlinesPerImcuRow() - skip);
    }
  }

  
  for (; lines_left > GetImageScanlinesPerImcuRow();
         lines_left -= GetImageScanlinesPerImcuRow()) {
    if (!DecodeImcuRow()) {
      FinishDecode();
      return false;
    }
    for (int i = 0; i < num_outbufs_; ++i) {
      int scanlines_to_copy = GetComponentScanlinesPerImcuRow(i);
      CopyRows(databuf_[i], GetComponentStride(i),
               planes[i], GetComponentWidth(i), scanlines_to_copy);
      planes[i] += scanlines_to_copy * GetComponentWidth(i);
    }
  }

  if (lines_left > 0) {
    
    if (!DecodeImcuRow()) {
      FinishDecode();
      return false;
    }
    for (int i = 0; i < num_outbufs_; ++i) {
      int scanlines_to_copy =
          DivideAndRoundUp(lines_left, GetVertSubSampFactor(i));
      CopyRows(databuf_[i], GetComponentStride(i),
               planes[i], GetComponentWidth(i), scanlines_to_copy);
      planes[i] += scanlines_to_copy * GetComponentWidth(i);
    }
  }
  return FinishDecode();
}

bool MJpegDecoder::DecodeToCallback(CallbackFunction fn, void* opaque,
    int dst_width, int dst_height) {
  if (dst_width != GetWidth() ||
      dst_height > GetHeight()) {
    
    return false;
  }
#ifdef HAVE_SETJMP
  if (setjmp(error_mgr_->setjmp_buffer)) {
    
    
    
    return false;
  }
#endif
  if (!StartDecode()) {
    return false;
  }
  SetScanlinePointers(databuf_);
  int lines_left = dst_height;
  
  int skip = (GetHeight() - dst_height) / 2;
  if (skip > 0) {
    while (skip >= GetImageScanlinesPerImcuRow()) {
      if (!DecodeImcuRow()) {
        FinishDecode();
        return false;
      }
      skip -= GetImageScanlinesPerImcuRow();
    }
    if (skip > 0) {
      
      if (!DecodeImcuRow()) {
        FinishDecode();
        return false;
      }
      for (int i = 0; i < num_outbufs_; ++i) {
        
        assert(skip % GetVertSubSampFactor(i) == 0);
        int rows_to_skip = DivideAndRoundDown(skip, GetVertSubSampFactor(i));
        int data_to_skip = rows_to_skip * GetComponentStride(i);
        
        
        databuf_[i] += data_to_skip;
      }
      int scanlines_to_copy = GetImageScanlinesPerImcuRow() - skip;
      (*fn)(opaque, databuf_, databuf_strides_, scanlines_to_copy);
      
      for (int i = 0; i < num_outbufs_; ++i) {
        int rows_to_skip = DivideAndRoundDown(skip, GetVertSubSampFactor(i));
        int data_to_skip = rows_to_skip * GetComponentStride(i);
        databuf_[i] -= data_to_skip;
      }
      lines_left -= scanlines_to_copy;
    }
  }
  
  for (; lines_left >= GetImageScanlinesPerImcuRow();
         lines_left -= GetImageScanlinesPerImcuRow()) {
    if (!DecodeImcuRow()) {
      FinishDecode();
      return false;
    }
    (*fn)(opaque, databuf_, databuf_strides_, GetImageScanlinesPerImcuRow());
  }
  if (lines_left > 0) {
    
    if (!DecodeImcuRow()) {
      FinishDecode();
      return false;
    }
    (*fn)(opaque, databuf_, databuf_strides_, lines_left);
  }
  return FinishDecode();
}

void MJpegDecoder::init_source(j_decompress_ptr cinfo) {
  fill_input_buffer(cinfo);
}

boolean MJpegDecoder::fill_input_buffer(j_decompress_ptr cinfo) {
  BufferVector* buf_vec = static_cast<BufferVector*>(cinfo->client_data);
  if (buf_vec->pos >= buf_vec->len) {
    assert(0 && "No more data");
    
    return FALSE;
  }
  cinfo->src->next_input_byte = buf_vec->buffers[buf_vec->pos].data;
  cinfo->src->bytes_in_buffer = buf_vec->buffers[buf_vec->pos].len;
  ++buf_vec->pos;
  return TRUE;
}

void MJpegDecoder::skip_input_data(j_decompress_ptr cinfo,
                                   long num_bytes) {  
  cinfo->src->next_input_byte += num_bytes;
}

void MJpegDecoder::term_source(j_decompress_ptr cinfo) {
  
}

#ifdef HAVE_SETJMP
void MJpegDecoder::ErrorHandler(j_common_ptr cinfo) {
  
  
  
  
  
  
  char buf[JMSG_LENGTH_MAX];
  (*cinfo->err->format_message)(cinfo, buf);
  

  SetJmpErrorMgr* mgr = reinterpret_cast<SetJmpErrorMgr*>(cinfo->err);
  
  
  longjmp(mgr->setjmp_buffer, 1);
}
#endif

void MJpegDecoder::AllocOutputBuffers(int num_outbufs) {
  if (num_outbufs != num_outbufs_) {
    
    
    
    DestroyOutputBuffers();

    scanlines_ = new uint8** [num_outbufs];
    scanlines_sizes_ = new int[num_outbufs];
    databuf_ = new uint8* [num_outbufs];
    databuf_strides_ = new int[num_outbufs];

    for (int i = 0; i < num_outbufs; ++i) {
      scanlines_[i] = NULL;
      scanlines_sizes_[i] = 0;
      databuf_[i] = NULL;
      databuf_strides_[i] = 0;
    }

    num_outbufs_ = num_outbufs;
  }
}

void MJpegDecoder::DestroyOutputBuffers() {
  for (int i = 0; i < num_outbufs_; ++i) {
    delete [] scanlines_[i];
    delete [] databuf_[i];
  }
  delete [] scanlines_;
  delete [] databuf_;
  delete [] scanlines_sizes_;
  delete [] databuf_strides_;
  scanlines_ = NULL;
  databuf_ = NULL;
  scanlines_sizes_ = NULL;
  databuf_strides_ = NULL;
  num_outbufs_ = 0;
}


bool MJpegDecoder::StartDecode() {
  decompress_struct_->raw_data_out = TRUE;
  decompress_struct_->dct_method = JDCT_IFAST;  
  decompress_struct_->dither_mode = JDITHER_NONE;
  decompress_struct_->do_fancy_upsampling = false;  
  decompress_struct_->enable_2pass_quant = false;  
  decompress_struct_->do_block_smoothing = false;  

  if (!jpeg_start_decompress(decompress_struct_)) {
    
    return false;
  }
  return true;
}

bool MJpegDecoder::FinishDecode() {
  
  
  jpeg_abort_decompress(decompress_struct_);
  return true;
}

void MJpegDecoder::SetScanlinePointers(uint8** data) {
  for (int i = 0; i < num_outbufs_; ++i) {
    uint8* data_i = data[i];
    for (int j = 0; j < scanlines_sizes_[i]; ++j) {
      scanlines_[i][j] = data_i;
      data_i += GetComponentStride(i);
    }
  }
}

inline bool MJpegDecoder::DecodeImcuRow() {
  return static_cast<unsigned int>(GetImageScanlinesPerImcuRow()) ==
      jpeg_read_raw_data(decompress_struct_,
                         scanlines_,
                         GetImageScanlinesPerImcuRow());
}


JpegSubsamplingType MJpegDecoder::JpegSubsamplingTypeHelper(
    int* subsample_x, int* subsample_y, int number_of_components) {
  if (number_of_components == 3) {  
    if (subsample_x[0] == 1 && subsample_y[0] == 1 &&
        subsample_x[1] == 2 && subsample_y[1] == 2 &&
        subsample_x[2] == 2 && subsample_y[2] == 2) {
      return kJpegYuv420;
    } else if (subsample_x[0] == 1 && subsample_y[0] == 1 &&
        subsample_x[1] == 2 && subsample_y[1] == 1 &&
        subsample_x[2] == 2 && subsample_y[2] == 1) {
      return kJpegYuv422;
    } else if (subsample_x[0] == 1 && subsample_y[0] == 1 &&
        subsample_x[1] == 1 && subsample_y[1] == 1 &&
        subsample_x[2] == 1 && subsample_y[2] == 1) {
      return kJpegYuv444;
    }
  } else if (number_of_components == 1) {  
    if (subsample_x[0] == 1 && subsample_y[0] == 1) {
      return kJpegYuv400;
    }
  }
  return kJpegUnknown;
}

}  
