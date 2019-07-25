





























#ifndef UTIL_SNAPPY_OPENSOURCE_SNAPPY_TEST_H_
#define UTIL_SNAPPY_OPENSOURCE_SNAPPY_TEST_H_

#include "snappy-stubs-internal.h"

#include <stdio.h>
#include <stdarg.h>

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#include <sys/time.h>

#ifdef HAVE_WINDOWS_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <string>

#ifdef HAVE_GTEST

#include <gtest/gtest.h>
#undef TYPED_TEST
#define TYPED_TEST TEST
#define INIT_GTEST(argc, argv) ::testing::InitGoogleTest(argc, *argv)

#else



#define TEST(test_case, test_subcase) \
  void Test_ ## test_case ## _ ## test_subcase()
#define INIT_GTEST(argc, argv)

#define TYPED_TEST TEST
#define EXPECT_EQ CHECK_EQ
#define EXPECT_NE CHECK_NE
#define EXPECT_FALSE(cond) CHECK(!(cond))

#endif

#ifdef HAVE_GFLAGS

#include <gflags/gflags.h>




#define InitGoogle(argv0, argc, argv, remove_flags) \
  INIT_GTEST(argc, argv); \
  google::ParseCommandLineFlags(argc, argv, remove_flags);

#else



#define DEFINE_int32(flag_name, default_value, description) \
  static int FLAGS_ ## flag_name = default_value;

#define InitGoogle(argv0, argc, argv, remove_flags) \
  INIT_GTEST(argc, argv)

#endif

#ifdef HAVE_LIBZ
#include "zlib.h"
#endif

#ifdef HAVE_LIBLZO2
#include "lzo/lzo1x.h"
#endif

#ifdef HAVE_LIBLZF
extern "C" {
#include "lzf.h"
}
#endif

#ifdef HAVE_LIBFASTLZ
#include "fastlz.h"
#endif

#ifdef HAVE_LIBQUICKLZ
#include "quicklz.h"
#endif

namespace {
namespace File {
  void Init() { }

  void ReadFileToStringOrDie(const char* filename, string* data) {
    FILE* fp = fopen(filename, "rb");
    if (fp == NULL) {
      perror(filename);
      exit(1);
    }

    data->clear();
    while (!feof(fp)) {
      char buf[4096];
      size_t ret = fread(buf, 1, 4096, fp);
      if (ret == -1) {
        perror("fread");
        exit(1);
      }
      data->append(string(buf, ret));
    }

    fclose(fp);
  }

  void ReadFileToStringOrDie(const string& filename, string* data) {
    ReadFileToStringOrDie(filename.c_str(), data);
  }

  void WriteStringToFileOrDie(const string& str, const char* filename) {
    FILE* fp = fopen(filename, "wb");
    if (fp == NULL) {
      perror(filename);
      exit(1);
    }

    int ret = fwrite(str.data(), str.size(), 1, fp);
    if (ret != 1) {
      perror("fwrite");
      exit(1);
    }

    fclose(fp);
  }
}  
}  

namespace snappy {

#define FLAGS_test_random_seed 301
typedef string TypeParam;

void Test_CorruptedTest_VerifyCorrupted();
void Test_Snappy_SimpleTests();
void Test_Snappy_MaxBlowup();
void Test_Snappy_RandomData();
void Test_Snappy_FourByteOffset();
void Test_SnappyCorruption_TruncatedVarint();
void Test_SnappyCorruption_UnterminatedVarint();
void Test_Snappy_ReadPastEndOfBuffer();
void Test_Snappy_FindMatchLength();
void Test_Snappy_FindMatchLengthRandom();

string ReadTestDataFile(const string& base);



string StringPrintf(const char* format, ...);


class ACMRandom {
 public:
  explicit ACMRandom(uint32 seed) : seed_(seed) {}

  int32 Next();

  int32 Uniform(int32 n) {
    return Next() % n;
  }
  uint8 Rand8() {
    return static_cast<uint8>((Next() >> 1) & 0x000000ff);
  }
  bool OneIn(int X) { return Uniform(X) == 0; }

  
  
  
  int32 Skewed(int max_log);

 private:
  static const uint32 M = 2147483647L;   
  uint32 seed_;
};

inline int32 ACMRandom::Next() {
  static const uint64 A = 16807;  
  
  
  
  
  
  
  uint64 product = seed_ * A;

  
  seed_ = (product >> 31) + (product & M);
  
  
  if (seed_ > M) {
    seed_ -= M;
  }
  return seed_;
}

inline int32 ACMRandom::Skewed(int max_log) {
  const int32 base = (Next() - 1) % (max_log+1);
  return (Next() - 1) & ((1u << base)-1);
}



class CycleTimer {
 public:
  CycleTimer() : real_time_us_(0) {}

  void Start() {
#ifdef WIN32
    QueryPerformanceCounter(&start_);
#else
    gettimeofday(&start_, NULL);
#endif
  }

  void Stop() {
#ifdef WIN32
    LARGE_INTEGER stop;
    LARGE_INTEGER frequency;
    QueryPerformanceCounter(&stop);
    QueryPerformanceFrequency(&frequency);

    double elapsed = static_cast<double>(stop.QuadPart - start_.QuadPart) /
        frequency.QuadPart;
    real_time_us_ += elapsed * 1e6 + 0.5;
#else
    struct timeval stop;
    gettimeofday(&stop, NULL);

    real_time_us_ += 1000000 * (stop.tv_sec - start_.tv_sec);
    real_time_us_ += (stop.tv_usec - start_.tv_usec);
#endif
  }

  double Get() {
    return real_time_us_ * 1e-6;
  }

 private:
  int64 real_time_us_;
#ifdef WIN32
  LARGE_INTEGER start_;
#else
  struct timeval start_;
#endif
};



typedef void (*BenchmarkFunction)(int, int);

class Benchmark {
 public:
  Benchmark(const string& name, BenchmarkFunction function) :
      name_(name), function_(function) {}

  Benchmark* DenseRange(int start, int stop) {
    start_ = start;
    stop_ = stop;
    return this;
  }

  void Run();

 private:
  const string name_;
  const BenchmarkFunction function_;
  int start_, stop_;
};
#define BENCHMARK(benchmark_name) \
  Benchmark* Benchmark_ ## benchmark_name = \
          (new Benchmark(#benchmark_name, benchmark_name))

extern Benchmark* Benchmark_BM_UFlat;
extern Benchmark* Benchmark_BM_UValidate;
extern Benchmark* Benchmark_BM_ZFlat;

void ResetBenchmarkTiming();
void StartBenchmarkTiming();
void StopBenchmarkTiming();
void SetBenchmarkLabel(const string& str);
void SetBenchmarkBytesProcessed(int64 bytes);

#ifdef HAVE_LIBZ


class ZLib {
 public:
  ZLib();
  ~ZLib();

  
  
  void Reinit();

  
  
  
  
  
  
  void Reset();

  
  
  
  
  static int MinCompressbufSize(int uncompress_size) {
    return uncompress_size + uncompress_size/1000 + 40;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  int Compress(Bytef *dest, uLongf *destLen,
               const Bytef *source, uLong sourceLen);

  
  
  
  
  
  int Uncompress(Bytef *dest, uLongf *destLen,
                 const Bytef *source, uLong sourceLen);

  
  
  
  
  
  
  

  int UncompressAtMost(Bytef *dest, uLongf *destLen,
                       const Bytef *source, uLong *sourceLen);

  
  
  
  
  bool UncompressChunkDone();

 private:
  int InflateInit();       
  int DeflateInit();       

  
  int CompressInit(Bytef *dest, uLongf *destLen,
                   const Bytef *source, uLong *sourceLen);
  int UncompressInit(Bytef *dest, uLongf *destLen,
                     const Bytef *source, uLong *sourceLen);
  
  
  
  void UncompressErrorInit();

  
  int CompressChunkOrAll(Bytef *dest, uLongf *destLen,
                         const Bytef *source, uLong sourceLen,
                         int flush_mode);
  int CompressAtMostOrAll(Bytef *dest, uLongf *destLen,
                          const Bytef *source, uLong *sourceLen,
                          int flush_mode);

  
  int UncompressChunkOrAll(Bytef *dest, uLongf *destLen,
                           const Bytef *source, uLong sourceLen,
                           int flush_mode);

  int UncompressAtMostOrAll(Bytef *dest, uLongf *destLen,
                            const Bytef *source, uLong *sourceLen,
                            int flush_mode);

  
  
  
  void CompressErrorInit();

  int compression_level_;   
  int window_bits_;         
  int mem_level_;           
                            
  z_stream comp_stream_;    
  bool comp_init_;          
  z_stream uncomp_stream_;  
  bool uncomp_init_;        

  
  bool first_chunk_;       
};

#endif  

}  

DECLARE_bool(run_microbenchmarks);

static void RunSpecifiedBenchmarks() {
  if (!FLAGS_run_microbenchmarks) {
    return;
  }

  fprintf(stderr, "Running microbenchmarks.\n");
#ifndef NDEBUG
  fprintf(stderr, "WARNING: Compiled with assertions enabled, will be slow.\n");
#endif
#ifndef __OPTIMIZE__
  fprintf(stderr, "WARNING: Compiled without optimization, will be slow.\n");
#endif
  fprintf(stderr, "Benchmark            Time(ns)    CPU(ns) Iterations\n");
  fprintf(stderr, "---------------------------------------------------\n");

  snappy::Benchmark_BM_UFlat->Run();
  snappy::Benchmark_BM_UValidate->Run();
  snappy::Benchmark_BM_ZFlat->Run();

  fprintf(stderr, "\n");
}

#ifndef HAVE_GTEST

static inline int RUN_ALL_TESTS() {
  fprintf(stderr, "Running correctness tests.\n");
  snappy::Test_CorruptedTest_VerifyCorrupted();
  snappy::Test_Snappy_SimpleTests();
  snappy::Test_Snappy_MaxBlowup();
  snappy::Test_Snappy_RandomData();
  snappy::Test_Snappy_FourByteOffset();
  snappy::Test_SnappyCorruption_TruncatedVarint();
  snappy::Test_SnappyCorruption_UnterminatedVarint();
  snappy::Test_Snappy_ReadPastEndOfBuffer();
  snappy::Test_Snappy_FindMatchLength();
  snappy::Test_Snappy_FindMatchLengthRandom();
  fprintf(stderr, "All tests passed.\n");

  return 0;
}

#endif  


namespace snappy {

static void CompressFile(const char* fname);
static void UncompressFile(const char* fname);
static void MeasureFile(const char* fname);

}  

using snappy::CompressFile;
using snappy::UncompressFile;
using snappy::MeasureFile;

#endif  
