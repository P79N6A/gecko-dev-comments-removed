





























#include "snappy-test.h"

#ifdef HAVE_WINDOWS_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

#include <algorithm>

DEFINE_bool(run_microbenchmarks, true,
            "Run microbenchmarks before doing anything else.");

namespace snappy {

string ReadTestDataFile(const string& base) {
  string contents;
  const char* srcdir = getenv("srcdir");  
  if (srcdir) {
    File::ReadFileToStringOrDie(
        string(srcdir) + "/testdata/" + base, &contents);
  } else {
    File::ReadFileToStringOrDie("testdata/" + base, &contents);
  }
  return contents;
}

string StringPrintf(const char* format, ...) {
  char buf[4096];
  va_list ap;
  va_start(ap, format);
  vsnprintf(buf, sizeof(buf), format, ap);
  va_end(ap);
  return buf;
}

bool benchmark_running = false;
int64 benchmark_real_time_us = 0;
int64 benchmark_cpu_time_us = 0;
string *benchmark_label = NULL;
int64 benchmark_bytes_processed = 0;

void ResetBenchmarkTiming() {
  benchmark_real_time_us = 0;
  benchmark_cpu_time_us = 0;
}

#ifdef WIN32
LARGE_INTEGER benchmark_start_real;
FILETIME benchmark_start_cpu;
#else  
struct timeval benchmark_start_real;
struct rusage benchmark_start_cpu;
#endif  

void StartBenchmarkTiming() {
#ifdef WIN32
  QueryPerformanceCounter(&benchmark_start_real);
  FILETIME dummy;
  CHECK(GetProcessTimes(
      GetCurrentProcess(), &dummy, &dummy, &dummy, &benchmark_start_cpu));
#else
  gettimeofday(&benchmark_start_real, NULL);
  if (getrusage(RUSAGE_SELF, &benchmark_start_cpu) == -1) {
    perror("getrusage(RUSAGE_SELF)");
    exit(1);
  }
#endif
  benchmark_running = true;
}

void StopBenchmarkTiming() {
  if (!benchmark_running) {
    return;
  }

#ifdef WIN32
  LARGE_INTEGER benchmark_stop_real;
  LARGE_INTEGER benchmark_frequency;
  QueryPerformanceCounter(&benchmark_stop_real);
  QueryPerformanceFrequency(&benchmark_frequency);

  double elapsed_real = static_cast<double>(
      benchmark_stop_real.QuadPart - benchmark_start_real.QuadPart) /
      benchmark_frequency.QuadPart;
  benchmark_real_time_us += elapsed_real * 1e6 + 0.5;

  FILETIME benchmark_stop_cpu, dummy;
  CHECK(GetProcessTimes(
      GetCurrentProcess(), &dummy, &dummy, &dummy, &benchmark_stop_cpu));

  ULARGE_INTEGER start_ulargeint;
  start_ulargeint.LowPart = benchmark_start_cpu.dwLowDateTime;
  start_ulargeint.HighPart = benchmark_start_cpu.dwHighDateTime;

  ULARGE_INTEGER stop_ulargeint;
  stop_ulargeint.LowPart = benchmark_stop_cpu.dwLowDateTime;
  stop_ulargeint.HighPart = benchmark_stop_cpu.dwHighDateTime;

  benchmark_cpu_time_us +=
      (stop_ulargeint.QuadPart - start_ulargeint.QuadPart + 5) / 10;
#else  
  struct timeval benchmark_stop_real;
  gettimeofday(&benchmark_stop_real, NULL);
  benchmark_real_time_us +=
      1000000 * (benchmark_stop_real.tv_sec - benchmark_start_real.tv_sec);
  benchmark_real_time_us +=
      (benchmark_stop_real.tv_usec - benchmark_start_real.tv_usec);

  struct rusage benchmark_stop_cpu;
  if (getrusage(RUSAGE_SELF, &benchmark_stop_cpu) == -1) {
    perror("getrusage(RUSAGE_SELF)");
    exit(1);
  }
  benchmark_cpu_time_us += 1000000 * (benchmark_stop_cpu.ru_utime.tv_sec -
                                      benchmark_start_cpu.ru_utime.tv_sec);
  benchmark_cpu_time_us += (benchmark_stop_cpu.ru_utime.tv_usec -
                            benchmark_start_cpu.ru_utime.tv_usec);
#endif  

  benchmark_running = false;
}

void SetBenchmarkLabel(const string& str) {
  if (benchmark_label) {
    delete benchmark_label;
  }
  benchmark_label = new string(str);
}

void SetBenchmarkBytesProcessed(int64 bytes) {
  benchmark_bytes_processed = bytes;
}

struct BenchmarkRun {
  int64 real_time_us;
  int64 cpu_time_us;
};

struct BenchmarkCompareCPUTime {
  bool operator() (const BenchmarkRun& a, const BenchmarkRun& b) const {
    return a.cpu_time_us < b.cpu_time_us;
  }
};

void Benchmark::Run() {
  for (int test_case_num = start_; test_case_num <= stop_; ++test_case_num) {
    
    
    const int kCalibrateIterations = 100;
    ResetBenchmarkTiming();
    StartBenchmarkTiming();
    (*function_)(kCalibrateIterations, test_case_num);
    StopBenchmarkTiming();

    
    
    
    const int kNumRuns = 5;
    const int kMedianPos = kNumRuns / 2;
    int num_iterations = 0;
    if (benchmark_real_time_us > 0) {
      num_iterations = 200000 * kCalibrateIterations / benchmark_real_time_us;
    }
    num_iterations = max(num_iterations, kCalibrateIterations);
    BenchmarkRun benchmark_runs[kNumRuns];

    for (int run = 0; run < kNumRuns; ++run) {
      ResetBenchmarkTiming();
      StartBenchmarkTiming();
      (*function_)(num_iterations, test_case_num);
      StopBenchmarkTiming();

      benchmark_runs[run].real_time_us = benchmark_real_time_us;
      benchmark_runs[run].cpu_time_us = benchmark_cpu_time_us;
    }

    nth_element(benchmark_runs,
                benchmark_runs + kMedianPos,
                benchmark_runs + kNumRuns,
                BenchmarkCompareCPUTime());
    int64 real_time_us = benchmark_runs[kMedianPos].real_time_us;
    int64 cpu_time_us = benchmark_runs[kMedianPos].cpu_time_us;
    int64 bytes_per_second = benchmark_bytes_processed * 1000000 / cpu_time_us;

    string heading = StringPrintf("%s/%d", name_.c_str(), test_case_num);
    string human_readable_speed;
    if (bytes_per_second < 1024) {
      human_readable_speed = StringPrintf("%dB/s", bytes_per_second);
    } else if (bytes_per_second < 1024 * 1024) {
      human_readable_speed = StringPrintf(
          "%.1fkB/s", bytes_per_second / 1024.0f);
    } else if (bytes_per_second < 1024 * 1024 * 1024) {
      human_readable_speed = StringPrintf(
          "%.1fMB/s", bytes_per_second / (1024.0f * 1024.0f));
    } else {
      human_readable_speed = StringPrintf(
          "%.1fGB/s", bytes_per_second / (1024.0f * 1024.0f * 1024.0f));
    }

    fprintf(stderr,
#ifdef WIN32
            "%-18s %10I64d %10I64d %10d %s  %s\n",
#else
            "%-18s %10lld %10lld %10d %s  %s\n",
#endif
            heading.c_str(),
            static_cast<long long>(real_time_us * 1000 / num_iterations),
            static_cast<long long>(cpu_time_us * 1000 / num_iterations),
            num_iterations,
            human_readable_speed.c_str(),
            benchmark_label->c_str());
  }
}

#ifdef HAVE_LIBZ

ZLib::ZLib()
    : comp_init_(false),
      uncomp_init_(false) {
  Reinit();
}

ZLib::~ZLib() {
  if (comp_init_)   { deflateEnd(&comp_stream_); }
  if (uncomp_init_) { inflateEnd(&uncomp_stream_); }
}

void ZLib::Reinit() {
  compression_level_ = Z_DEFAULT_COMPRESSION;
  window_bits_ = MAX_WBITS;
  mem_level_ =  8;  
  if (comp_init_) {
    deflateEnd(&comp_stream_);
    comp_init_ = false;
  }
  if (uncomp_init_) {
    inflateEnd(&uncomp_stream_);
    uncomp_init_ = false;
  }
  first_chunk_ = true;
}

void ZLib::Reset() {
  first_chunk_ = true;
}






void ZLib::CompressErrorInit() {
  deflateEnd(&comp_stream_);
  comp_init_ = false;
  Reset();
}

int ZLib::DeflateInit() {
  return deflateInit2(&comp_stream_,
                      compression_level_,
                      Z_DEFLATED,
                      window_bits_,
                      mem_level_,
                      Z_DEFAULT_STRATEGY);
}

int ZLib::CompressInit(Bytef *dest, uLongf *destLen,
                       const Bytef *source, uLong *sourceLen) {
  int err;

  comp_stream_.next_in = (Bytef*)source;
  comp_stream_.avail_in = (uInt)*sourceLen;
  if ((uLong)comp_stream_.avail_in != *sourceLen) return Z_BUF_ERROR;
  comp_stream_.next_out = dest;
  comp_stream_.avail_out = (uInt)*destLen;
  if ((uLong)comp_stream_.avail_out != *destLen) return Z_BUF_ERROR;

  if ( !first_chunk_ )   
    return Z_OK;

  if (comp_init_) {      
    err = deflateReset(&comp_stream_);
    if (err != Z_OK) {
      LOG(WARNING) << "ERROR: Can't reset compress object; creating a new one";
      deflateEnd(&comp_stream_);
      comp_init_ = false;
    }
  }
  if (!comp_init_) {     
    comp_stream_.zalloc = (alloc_func)0;
    comp_stream_.zfree = (free_func)0;
    comp_stream_.opaque = (voidpf)0;
    err = DeflateInit();
    if (err != Z_OK) return err;
    comp_init_ = true;
  }
  return Z_OK;
}







int ZLib::CompressAtMostOrAll(Bytef *dest, uLongf *destLen,
                              const Bytef *source, uLong *sourceLen,
                              int flush_mode) {   
  int err;

  if ( (err=CompressInit(dest, destLen, source, sourceLen)) != Z_OK )
    return err;

  
  int compressed_size = comp_stream_.total_out;

  
  if ( first_chunk_ ) {
    first_chunk_ = false;
  }

  
  
  err = deflate(&comp_stream_, flush_mode);

  const uLong source_bytes_consumed = *sourceLen - comp_stream_.avail_in;
  *sourceLen = comp_stream_.avail_in;

  if ((err == Z_STREAM_END || err == Z_OK)
      && comp_stream_.avail_in == 0
      && comp_stream_.avail_out != 0 ) {
    
    ;
  } else if (err == Z_STREAM_END && comp_stream_.avail_in > 0) {
    return Z_BUF_ERROR;                            
  } else if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR) {
    
    CompressErrorInit();
    return err;
  } else if (comp_stream_.avail_out == 0) {     
    err = Z_BUF_ERROR;
  }

  assert(err == Z_OK || err == Z_STREAM_END || err == Z_BUF_ERROR);
  if (err == Z_STREAM_END)
    err = Z_OK;

  
  compressed_size = comp_stream_.total_out - compressed_size;  
  *destLen = compressed_size;

  return err;
}

int ZLib::CompressChunkOrAll(Bytef *dest, uLongf *destLen,
                             const Bytef *source, uLong sourceLen,
                             int flush_mode) {   
  const int ret =
    CompressAtMostOrAll(dest, destLen, source, &sourceLen, flush_mode);
  if (ret == Z_BUF_ERROR)
    CompressErrorInit();
  return ret;
}



int ZLib::Compress(Bytef *dest, uLongf *destLen,
                   const Bytef *source, uLong sourceLen) {
  int err;
  const uLongf orig_destLen = *destLen;
  if ( (err=CompressChunkOrAll(dest, destLen, source, sourceLen,
                               Z_FINISH)) != Z_OK )
    return err;
  Reset();         

  return Z_OK;
}




int ZLib::InflateInit() {
  return inflateInit2(&uncomp_stream_, MAX_WBITS);
}




void ZLib::UncompressErrorInit() {
  inflateEnd(&uncomp_stream_);
  uncomp_init_ = false;
  Reset();
}

int ZLib::UncompressInit(Bytef *dest, uLongf *destLen,
                         const Bytef *source, uLong *sourceLen) {
  int err;

  uncomp_stream_.next_in = (Bytef*)source;
  uncomp_stream_.avail_in = (uInt)*sourceLen;
  
  if ((uLong)uncomp_stream_.avail_in != *sourceLen) return Z_BUF_ERROR;

  uncomp_stream_.next_out = dest;
  uncomp_stream_.avail_out = (uInt)*destLen;
  if ((uLong)uncomp_stream_.avail_out != *destLen) return Z_BUF_ERROR;

  if ( !first_chunk_ )   
    return Z_OK;

  if (uncomp_init_) {    
    err = inflateReset(&uncomp_stream_);
    if (err != Z_OK) {
      LOG(WARNING)
        << "ERROR: Can't reset uncompress object; creating a new one";
      UncompressErrorInit();
    }
  }
  if (!uncomp_init_) {
    uncomp_stream_.zalloc = (alloc_func)0;
    uncomp_stream_.zfree = (free_func)0;
    uncomp_stream_.opaque = (voidpf)0;
    err = InflateInit();
    if (err != Z_OK) return err;
    uncomp_init_ = true;
  }
  return Z_OK;
}





int ZLib::UncompressAtMostOrAll(Bytef *dest, uLongf *destLen,
                                const Bytef *source, uLong *sourceLen,
                                int flush_mode) {  
  int err = Z_OK;

  if ( (err=UncompressInit(dest, destLen, source, sourceLen)) != Z_OK ) {
    LOG(WARNING) << "UncompressInit: Error: " << err << " SourceLen: "
                 << *sourceLen;
    return err;
  }

  
  const uLong old_total_out = uncomp_stream_.total_out;

  
  const uLong old_total_in = uncomp_stream_.total_in;

  
  if ( first_chunk_ ) {
    first_chunk_ = false;                          

    
    
    
    if ( *sourceLen == 0 ) {
      *destLen = 0;
      return Z_OK;
    }
  }

  
  
  

  
  err = inflate(&uncomp_stream_, flush_mode);

  
  const uLong bytes_read = uncomp_stream_.total_in - old_total_in;
  CHECK_LE(source + bytes_read, source + *sourceLen);
  *sourceLen = uncomp_stream_.avail_in;

  if ((err == Z_STREAM_END || err == Z_OK)  
             && uncomp_stream_.avail_in == 0) {    
    ;
  } else if (err == Z_STREAM_END && uncomp_stream_.avail_in > 0) {
    LOG(WARNING)
      << "UncompressChunkOrAll: Received some extra data, bytes total: "
      << uncomp_stream_.avail_in << " bytes: "
      << string(reinterpret_cast<const char *>(uncomp_stream_.next_in),
                min(int(uncomp_stream_.avail_in), 20));
    UncompressErrorInit();
    return Z_DATA_ERROR;       
  } else if (err != Z_OK && err != Z_STREAM_END && err != Z_BUF_ERROR) {
    
    LOG(WARNING) << "UncompressChunkOrAll: Error: " << err
                 << " avail_out: " << uncomp_stream_.avail_out;
    UncompressErrorInit();
    return err;
  } else if (uncomp_stream_.avail_out == 0) {
    err = Z_BUF_ERROR;
  }

  assert(err == Z_OK || err == Z_BUF_ERROR || err == Z_STREAM_END);
  if (err == Z_STREAM_END)
    err = Z_OK;

  *destLen = uncomp_stream_.total_out - old_total_out;  

  return err;
}

int ZLib::UncompressChunkOrAll(Bytef *dest, uLongf *destLen,
                               const Bytef *source, uLong sourceLen,
                               int flush_mode) {  
  const int ret =
    UncompressAtMostOrAll(dest, destLen, source, &sourceLen, flush_mode);
  if (ret == Z_BUF_ERROR)
    UncompressErrorInit();
  return ret;
}

int ZLib::UncompressAtMost(Bytef *dest, uLongf *destLen,
                          const Bytef *source, uLong *sourceLen) {
  return UncompressAtMostOrAll(dest, destLen, source, sourceLen, Z_SYNC_FLUSH);
}





bool ZLib::UncompressChunkDone() {
  assert(!first_chunk_ && uncomp_init_);
  
  
  
  Bytef dummyin, dummyout;
  uLongf dummylen = 0;
  if ( UncompressChunkOrAll(&dummyout, &dummylen, &dummyin, 0, Z_FINISH)
       != Z_OK ) {
    return false;
  }

  
  Reset();

  return true;
}









int ZLib::Uncompress(Bytef *dest, uLongf *destLen,
                     const Bytef *source, uLong sourceLen) {
  int err;
  if ( (err=UncompressChunkOrAll(dest, destLen, source, sourceLen,
                                 Z_FINISH)) != Z_OK ) {
    Reset();                           
    return err;
  }
  if ( !UncompressChunkDone() )        
    return Z_DATA_ERROR;
  return Z_OK;  
}

#endif  

}  
