












#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libyuv/convert.h"
#include "libyuv/planar_functions.h"
#include "libyuv/scale_argb.h"


bool verbose = false;
bool attenuate = false;
bool unattenuate = false;
int image_width = 0, image_height = 0;  
int dst_width = 0, dst_height = 0;  
int fileindex_org = 0;  
int fileindex_rec = 0;  
int num_rec = 0;  
int num_skip_org = 0;  
int num_frames = 0;  
int filter = 1;  

static __inline uint32 Abs(int32 v) {
  return v >= 0 ? v : -v;
}


bool ExtractResolutionFromFilename(const char* name,
                                   int* width_ptr,
                                   int* height_ptr) {
  
  
  for (int i = 0; name[i]; ++i) {
    if ((name[i] == '.' || name[i] == '_') &&
        name[i + 1] >= '0' && name[i + 1] <= '9') {
      int n = sscanf(name + i + 1, "%dx%d", width_ptr, height_ptr);  
      if (2 == n) {
        return true;
      }
    }
  }
  return false;
}

void PrintHelp(const char * program) {
  printf("%s [-options] src_argb.raw dst_yuv.raw\n", program);
  printf(" -s <width> <height> .... specify source resolution.  "
         "Optional if name contains\n"
         "                          resolution (ie. "
         "name.1920x800_24Hz_P420.yuv)\n"
         "                          Negative value mirrors.\n");
  printf(" -d <width> <height> .... specify destination resolution.\n");
  printf(" -f <filter> ............ 0 = point, 1 = bilinear (default).\n");
  printf(" -skip <src_argb> ....... Number of frame to skip of src_argb\n");
  printf(" -frames <num> .......... Number of frames to convert\n");
  printf(" -attenuate ............. Attenuate the ARGB image\n");
  printf(" -unattenuate ........... Unattenuate the ARGB image\n");
  printf(" -v ..................... verbose\n");
  printf(" -h ..................... this help\n");
  exit(0);
}

void ParseOptions(int argc, const char* argv[]) {
  if (argc <= 1) PrintHelp(argv[0]);
  for (int c = 1; c < argc; ++c) {
    if (!strcmp(argv[c], "-v")) {
      verbose = true;
    } else if (!strcmp(argv[c], "-attenuate")) {
      attenuate = true;
    } else if (!strcmp(argv[c], "-unattenuate")) {
      unattenuate = true;
    } else if (!strcmp(argv[c], "-h") || !strcmp(argv[c], "-help")) {
      PrintHelp(argv[0]);
    } else if (!strcmp(argv[c], "-s") && c + 2 < argc) {
      image_width = atoi(argv[++c]);    
      image_height = atoi(argv[++c]);   
    } else if (!strcmp(argv[c], "-d") && c + 2 < argc) {
      dst_width = atoi(argv[++c]);    
      dst_height = atoi(argv[++c]);   
    } else if (!strcmp(argv[c], "-skip") && c + 1 < argc) {
      num_skip_org = atoi(argv[++c]);   
    } else if (!strcmp(argv[c], "-frames") && c + 1 < argc) {
      num_frames = atoi(argv[++c]);     
    } else if (!strcmp(argv[c], "-f") && c + 1 < argc) {
      filter = atoi(argv[++c]);     
    } else if (argv[c][0] == '-') {
      fprintf(stderr, "Unknown option. %s\n", argv[c]);
    } else if (fileindex_org == 0) {
      fileindex_org = c;
    } else if (fileindex_rec == 0) {
      fileindex_rec = c;
      num_rec = 1;
    } else {
      ++num_rec;
    }
  }
  if (fileindex_org == 0 || fileindex_rec == 0) {
    fprintf(stderr, "Missing filenames\n");
    PrintHelp(argv[0]);
  }
  if (num_skip_org < 0) {
    fprintf(stderr, "Skipped frames incorrect\n");
    PrintHelp(argv[0]);
  }
  if (num_frames < 0) {
    fprintf(stderr, "Number of frames incorrect\n");
    PrintHelp(argv[0]);
  }

  int org_width, org_height;
  int rec_width, rec_height;
  bool org_res_avail = ExtractResolutionFromFilename(argv[fileindex_org],
                                                     &org_width,
                                                     &org_height);
  bool rec_res_avail = ExtractResolutionFromFilename(argv[fileindex_rec],
                                                     &rec_width,
                                                     &rec_height);
  if (image_width == 0 || image_height == 0) {
    if (org_res_avail) {
      image_width = org_width;
      image_height = org_height;
    } else if (rec_res_avail) {
      image_width = rec_width;
      image_height = rec_height;
    } else {
      fprintf(stderr, "Missing dimensions.\n");
      PrintHelp(argv[0]);
    }
  }
  if (dst_width == 0 || dst_height == 0) {
    if (rec_res_avail) {
      dst_width = rec_width;
      dst_height = rec_height;
    } else {
      dst_width = Abs(image_width);
      dst_height = Abs(image_height);
    }
  }
}

static const int kTileX = 32;
static const int kTileY = 32;

static int TileARGBScale(const uint8* src_argb, int src_stride_argb,
                         int src_width, int src_height,
                         uint8* dst_argb, int dst_stride_argb,
                         int dst_width, int dst_height,
                         libyuv::FilterMode filtering) {
  for (int y = 0; y < dst_height; y += kTileY) {
    for (int x = 0; x < dst_width; x += kTileX) {
      int clip_width = kTileX;
      if (x + clip_width > dst_width) {
        clip_width = dst_width - x;
      }
      int clip_height = kTileY;
      if (y + clip_height > dst_height) {
        clip_height = dst_height - y;
      }
      int r = libyuv::ARGBScaleClip(src_argb, src_stride_argb,
                                    src_width, src_height,
                                    dst_argb, dst_stride_argb,
                                    dst_width, dst_height,
                                    x, y, clip_width, clip_height, filtering);
      if (r) {
        return r;
      }
    }
  }
  return 0;
}

int main(int argc, const char* argv[]) {
  ParseOptions(argc, argv);

  
  FILE* const file_org = fopen(argv[fileindex_org], "rb");
  if (file_org == NULL) {
    fprintf(stderr, "Cannot open %s\n", argv[fileindex_org]);
    exit(1);
  }

  
  FILE** file_rec = new FILE* [num_rec];
  memset(file_rec, 0, num_rec * sizeof(FILE*)); 
  for (int cur_rec = 0; cur_rec < num_rec; ++cur_rec) {
    file_rec[cur_rec] = fopen(argv[fileindex_rec + cur_rec], "wb");
    if (file_rec[cur_rec] == NULL) {
      fprintf(stderr, "Cannot open %s\n", argv[fileindex_rec + cur_rec]);
      fclose(file_org);
      for (int i = 0; i < cur_rec; ++i) {
        fclose(file_rec[i]);
      }
      delete[] file_rec;
      exit(1);
    }
  }

  bool org_is_yuv = strstr(argv[fileindex_org], "_P420.") != NULL;
  bool org_is_argb = strstr(argv[fileindex_org], "_ARGB.") != NULL;
  if (!org_is_yuv && !org_is_argb) {
    fprintf(stderr, "Original format unknown %s\n", argv[fileindex_org]);
    exit(1);
  }
  int org_size = Abs(image_width) * Abs(image_height) * 4;  
  
  if (org_is_yuv) {
    const int y_size = Abs(image_width) * Abs(image_height);
    const int uv_size = ((Abs(image_width) + 1) / 2) *
        ((Abs(image_height) + 1) / 2);
    org_size = y_size + 2 * uv_size;  
  }

  const int dst_size = dst_width * dst_height * 4;  
  const int y_size = dst_width * dst_height;
  const int uv_size = ((dst_width + 1) / 2) * ((dst_height + 1) / 2);
  const size_t total_size = y_size + 2 * uv_size;
#if defined(_MSC_VER)
  _fseeki64(file_org,
            static_cast<__int64>(num_skip_org) *
            static_cast<__int64>(org_size), SEEK_SET);
#else
  fseek(file_org, num_skip_org * total_size, SEEK_SET);
#endif

  uint8* const ch_org = new uint8[org_size];
  uint8* const ch_dst = new uint8[dst_size];
  uint8* const ch_rec = new uint8[total_size];
  if (ch_org == NULL || ch_rec == NULL) {
    fprintf(stderr, "No memory available\n");
    fclose(file_org);
    for (int i = 0; i < num_rec; ++i) {
      fclose(file_rec[i]);
    }
    delete[] ch_org;
    delete[] ch_dst;
    delete[] ch_rec;
    delete[] file_rec;
    exit(1);
  }

  if (verbose) {
    printf("Size: %dx%d to %dx%d\n", image_width, image_height,
           dst_width, dst_height);
  }

  int number_of_frames;
  for (number_of_frames = 0; ; ++number_of_frames) {
    if (num_frames && number_of_frames >= num_frames)
      break;

    
    size_t bytes_org = fread(ch_org, sizeof(uint8),
                             static_cast<size_t>(org_size), file_org);
    if (bytes_org < static_cast<size_t>(org_size))
      break;

    
    
    if (org_is_argb && attenuate) {
      libyuv::ARGBAttenuate(ch_org, 0, ch_org, 0, org_size / 4, 1);
    }
    
    if (org_is_argb && unattenuate) {
      libyuv::ARGBUnattenuate(ch_org, 0, ch_org, 0, org_size / 4, 1);
    }

    for (int cur_rec = 0; cur_rec < num_rec; ++cur_rec) {
      
      if (org_is_yuv) {
        int src_width = Abs(image_width);
        int src_height = Abs(image_height);
        int half_src_width = (src_width + 1) / 2;
        int half_src_height = (src_height + 1) / 2;
        int half_dst_width = (dst_width + 1) / 2;
        int half_dst_height = (dst_height + 1) / 2;
        I420Scale(ch_org, src_width,
                  ch_org + src_width * src_height, half_src_width,
                  ch_org + src_width * src_height +
                      half_src_width * half_src_height,  half_src_width,
                  image_width, image_height,
                  ch_rec, dst_width,
                  ch_rec + dst_width * dst_height, half_dst_width,
                  ch_rec + dst_width * dst_height +
                      half_dst_width * half_dst_height,  half_dst_width,
                  dst_width, dst_height,
                      static_cast<libyuv::FilterMode>(filter));
      } else {
        TileARGBScale(ch_org, Abs(image_width) * 4,
                      image_width, image_height,
                      ch_dst, dst_width * 4,
                      dst_width, dst_height,
                      static_cast<libyuv::FilterMode>(filter));
      }
      bool rec_is_yuv = strstr(argv[fileindex_rec + cur_rec], "_P420.") != NULL;
      bool rec_is_argb =
          strstr(argv[fileindex_rec + cur_rec], "_ARGB.") != NULL;
      if (!rec_is_yuv && !rec_is_argb) {
        fprintf(stderr, "Output format unknown %s\n",
                argv[fileindex_rec + cur_rec]);
        continue;  
      }

      
      if (!org_is_yuv && rec_is_yuv) {
        int half_width = (dst_width + 1) / 2;
        int half_height = (dst_height + 1) / 2;
        libyuv::ARGBToI420(ch_dst, dst_width * 4,
                           ch_rec, dst_width,
                           ch_rec + dst_width * dst_height, half_width,
                           ch_rec + dst_width * dst_height +
                               half_width * half_height,  half_width,
                           dst_width, dst_height);
      }

      
      if (rec_is_yuv) {
        size_t bytes_rec = fwrite(ch_rec, sizeof(uint8),
                                  static_cast<size_t>(total_size),
                                  file_rec[cur_rec]);
        if (bytes_rec < static_cast<size_t>(total_size))
          break;
      } else {
        size_t bytes_rec = fwrite(ch_dst, sizeof(uint8),
                                  static_cast<size_t>(dst_size),
                                  file_rec[cur_rec]);
        if (bytes_rec < static_cast<size_t>(dst_size))
          break;
      }
      if (verbose) {
        printf("%5d", number_of_frames);
      }
      if (verbose) {
        printf("\t%s", argv[fileindex_rec + cur_rec]);
        printf("\n");
      }
    }
  }

  fclose(file_org);
  for (int cur_rec = 0; cur_rec < num_rec; ++cur_rec) {
    fclose(file_rec[cur_rec]);
  }
  delete[] ch_org;
  delete[] ch_dst;
  delete[] ch_rec;
  delete[] file_rec;
  return 0;
}
