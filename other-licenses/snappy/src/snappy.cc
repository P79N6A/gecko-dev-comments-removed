



























#include "snappy.h"
#include "snappy-internal.h"
#include "snappy-sinksource.h"

#include <stdio.h>

#include <algorithm>
#include <string>
#include <vector>


namespace snappy {






static inline uint32 HashBytes(uint32 bytes, int shift) {
  uint32 kMul = 0x1e35a7bd;
  return (bytes * kMul) >> shift;
}
static inline uint32 Hash(const char* p, int shift) {
  return HashBytes(UNALIGNED_LOAD32(p), shift);
}

size_t MaxCompressedLength(size_t source_len) {
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  return 32 + source_len + source_len/6;
}

enum {
  LITERAL = 0,
  COPY_1_BYTE_OFFSET = 1,  
  COPY_2_BYTE_OFFSET = 2,
  COPY_4_BYTE_OFFSET = 3
};












static inline void IncrementalCopy(const char* src, char* op, int len) {
  DCHECK_GT(len, 0);
  do {
    *op++ = *src++;
  } while (--len > 0);
}
































namespace {

const int kMaxIncrementCopyOverflow = 10;

}  

static inline void IncrementalCopyFastPath(const char* src, char* op, int len) {
  while (op - src < 8) {
    UNALIGNED_STORE64(op, UNALIGNED_LOAD64(src));
    len -= op - src;
    op += op - src;
  }
  while (len > 0) {
    UNALIGNED_STORE64(op, UNALIGNED_LOAD64(src));
    src += 8;
    op += 8;
    len -= 8;
  }
}

static inline char* EmitLiteral(char* op,
                                const char* literal,
                                int len,
                                bool allow_fast_path) {
  int n = len - 1;      
  if (n < 60) {
    
    *op++ = LITERAL | (n << 2);

    
    
    
    
    
    
    
    
    
    
    if (allow_fast_path && len <= 16) {
      UNALIGNED_STORE64(op, UNALIGNED_LOAD64(literal));
      UNALIGNED_STORE64(op + 8, UNALIGNED_LOAD64(literal + 8));
      return op + len;
    }
  } else {
    
    char* base = op;
    int count = 0;
    op++;
    while (n > 0) {
      *op++ = n & 0xff;
      n >>= 8;
      count++;
    }
    assert(count >= 1);
    assert(count <= 4);
    *base = LITERAL | ((59+count) << 2);
  }
  memcpy(op, literal, len);
  return op + len;
}

static inline char* EmitCopyLessThan64(char* op, int offset, int len) {
  DCHECK_LE(len, 64);
  DCHECK_GE(len, 4);
  DCHECK_LT(offset, 65536);

  if ((len < 12) && (offset < 2048)) {
    int len_minus_4 = len - 4;
    assert(len_minus_4 < 8);            
    *op++ = COPY_1_BYTE_OFFSET | ((len_minus_4) << 2) | ((offset >> 8) << 5);
    *op++ = offset & 0xff;
  } else {
    *op++ = COPY_2_BYTE_OFFSET | ((len-1) << 2);
    LittleEndian::Store16(op, offset);
    op += 2;
  }
  return op;
}

static inline char* EmitCopy(char* op, int offset, int len) {
  
  while (len >= 68) {
    op = EmitCopyLessThan64(op, offset, 64);
    len -= 64;
  }

  
  if (len > 64) {
    op = EmitCopyLessThan64(op, offset, 60);
    len -= 60;
  }

  
  op = EmitCopyLessThan64(op, offset, len);
  return op;
}


bool GetUncompressedLength(const char* start, size_t n, size_t* result) {
  uint32 v = 0;
  const char* limit = start + n;
  if (Varint::Parse32WithLimit(start, limit, &v) != NULL) {
    *result = v;
    return true;
  } else {
    return false;
  }
}

namespace internal {
uint16* WorkingMemory::GetHashTable(size_t input_size, int* table_size) {
  
  
  
  
  assert(kMaxHashTableSize >= 256);
  int htsize = 256;
  while (htsize < kMaxHashTableSize && htsize < input_size) {
    htsize <<= 1;
  }
  CHECK_EQ(0, htsize & (htsize - 1)) << ": must be power of two";
  CHECK_LE(htsize, kMaxHashTableSize) << ": hash table too large";

  uint16* table;
  if (htsize <= ARRAYSIZE(small_table_)) {
    table = small_table_;
  } else {
    if (large_table_ == NULL) {
      large_table_ = new uint16[kMaxHashTableSize];
    }
    table = large_table_;
  }

  *table_size = htsize;
  memset(table, 0, htsize * sizeof(*table));
  return table;
}
}  






static inline uint32 GetUint32AtOffset(uint64 v, int offset) {
  DCHECK(0 <= offset && offset <= 4) << offset;
  return v >> (LittleEndian::IsLittleEndian() ? 8 * offset : 32 - 8 * offset);
}












namespace internal {
char* CompressFragment(const char* input,
                       size_t input_size,
                       char* op,
                       uint16* table,
                       const int table_size) {
  
  const char* ip = input;
  CHECK_LE(input_size, kBlockSize);
  CHECK_EQ(table_size & (table_size - 1), 0) << ": table must be power of two";
  const int shift = 32 - Bits::Log2Floor(table_size);
  DCHECK_EQ(kuint32max >> shift, table_size - 1);
  const char* ip_end = input + input_size;
  const char* base_ip = ip;
  
  
  const char* next_emit = ip;

  const int kInputMarginBytes = 15;
  if (PREDICT_TRUE(input_size >= kInputMarginBytes)) {
    const char* ip_limit = input + input_size - kInputMarginBytes;

    for (uint32 next_hash = Hash(++ip, shift); ; ) {
      DCHECK_LT(next_emit, ip);
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      uint32 skip = 32;

      const char* next_ip = ip;
      const char* candidate;
      do {
        ip = next_ip;
        uint32 hash = next_hash;
        DCHECK_EQ(hash, Hash(ip, shift));
        uint32 bytes_between_hash_lookups = skip++ >> 5;
        next_ip = ip + bytes_between_hash_lookups;
        if (PREDICT_FALSE(next_ip > ip_limit)) {
          goto emit_remainder;
        }
        next_hash = Hash(next_ip, shift);
        candidate = base_ip + table[hash];
        DCHECK_GE(candidate, base_ip);
        DCHECK_LT(candidate, ip);

        table[hash] = ip - base_ip;
      } while (PREDICT_TRUE(UNALIGNED_LOAD32(ip) !=
                            UNALIGNED_LOAD32(candidate)));

      
      
      
      DCHECK_LE(next_emit + 16, ip_end);
      op = EmitLiteral(op, next_emit, ip - next_emit, true);

      
      
      
      
      
      
      
      
      uint64 input_bytes = 0;
      uint32 candidate_bytes = 0;

      do {
        
        
        const char* base = ip;
        int matched = 4 + FindMatchLength(candidate + 4, ip + 4, ip_end);
        ip += matched;
        int offset = base - candidate;
        DCHECK_EQ(0, memcmp(base, candidate, matched));
        op = EmitCopy(op, offset, matched);
        
        
        const char* insert_tail = ip - 1;
        next_emit = ip;
        if (PREDICT_FALSE(ip >= ip_limit)) {
          goto emit_remainder;
        }
        input_bytes = UNALIGNED_LOAD64(insert_tail);
        uint32 prev_hash = HashBytes(GetUint32AtOffset(input_bytes, 0), shift);
        table[prev_hash] = ip - base_ip - 1;
        uint32 cur_hash = HashBytes(GetUint32AtOffset(input_bytes, 1), shift);
        candidate = base_ip + table[cur_hash];
        candidate_bytes = UNALIGNED_LOAD32(candidate);
        table[cur_hash] = ip - base_ip;
      } while (GetUint32AtOffset(input_bytes, 1) == candidate_bytes);

      next_hash = HashBytes(GetUint32AtOffset(input_bytes, 2), shift);
      ++ip;
    }
  }

 emit_remainder:
  
  if (next_emit < ip_end) {
    op = EmitLiteral(op, next_emit, ip_end - next_emit, false);
  }

  return op;
}
}  



























static const uint32 wordmask[] = {
  0u, 0xffu, 0xffffu, 0xffffffu, 0xffffffffu
};












static const uint16 char_table[256] = {
  0x0001, 0x0804, 0x1001, 0x2001, 0x0002, 0x0805, 0x1002, 0x2002,
  0x0003, 0x0806, 0x1003, 0x2003, 0x0004, 0x0807, 0x1004, 0x2004,
  0x0005, 0x0808, 0x1005, 0x2005, 0x0006, 0x0809, 0x1006, 0x2006,
  0x0007, 0x080a, 0x1007, 0x2007, 0x0008, 0x080b, 0x1008, 0x2008,
  0x0009, 0x0904, 0x1009, 0x2009, 0x000a, 0x0905, 0x100a, 0x200a,
  0x000b, 0x0906, 0x100b, 0x200b, 0x000c, 0x0907, 0x100c, 0x200c,
  0x000d, 0x0908, 0x100d, 0x200d, 0x000e, 0x0909, 0x100e, 0x200e,
  0x000f, 0x090a, 0x100f, 0x200f, 0x0010, 0x090b, 0x1010, 0x2010,
  0x0011, 0x0a04, 0x1011, 0x2011, 0x0012, 0x0a05, 0x1012, 0x2012,
  0x0013, 0x0a06, 0x1013, 0x2013, 0x0014, 0x0a07, 0x1014, 0x2014,
  0x0015, 0x0a08, 0x1015, 0x2015, 0x0016, 0x0a09, 0x1016, 0x2016,
  0x0017, 0x0a0a, 0x1017, 0x2017, 0x0018, 0x0a0b, 0x1018, 0x2018,
  0x0019, 0x0b04, 0x1019, 0x2019, 0x001a, 0x0b05, 0x101a, 0x201a,
  0x001b, 0x0b06, 0x101b, 0x201b, 0x001c, 0x0b07, 0x101c, 0x201c,
  0x001d, 0x0b08, 0x101d, 0x201d, 0x001e, 0x0b09, 0x101e, 0x201e,
  0x001f, 0x0b0a, 0x101f, 0x201f, 0x0020, 0x0b0b, 0x1020, 0x2020,
  0x0021, 0x0c04, 0x1021, 0x2021, 0x0022, 0x0c05, 0x1022, 0x2022,
  0x0023, 0x0c06, 0x1023, 0x2023, 0x0024, 0x0c07, 0x1024, 0x2024,
  0x0025, 0x0c08, 0x1025, 0x2025, 0x0026, 0x0c09, 0x1026, 0x2026,
  0x0027, 0x0c0a, 0x1027, 0x2027, 0x0028, 0x0c0b, 0x1028, 0x2028,
  0x0029, 0x0d04, 0x1029, 0x2029, 0x002a, 0x0d05, 0x102a, 0x202a,
  0x002b, 0x0d06, 0x102b, 0x202b, 0x002c, 0x0d07, 0x102c, 0x202c,
  0x002d, 0x0d08, 0x102d, 0x202d, 0x002e, 0x0d09, 0x102e, 0x202e,
  0x002f, 0x0d0a, 0x102f, 0x202f, 0x0030, 0x0d0b, 0x1030, 0x2030,
  0x0031, 0x0e04, 0x1031, 0x2031, 0x0032, 0x0e05, 0x1032, 0x2032,
  0x0033, 0x0e06, 0x1033, 0x2033, 0x0034, 0x0e07, 0x1034, 0x2034,
  0x0035, 0x0e08, 0x1035, 0x2035, 0x0036, 0x0e09, 0x1036, 0x2036,
  0x0037, 0x0e0a, 0x1037, 0x2037, 0x0038, 0x0e0b, 0x1038, 0x2038,
  0x0039, 0x0f04, 0x1039, 0x2039, 0x003a, 0x0f05, 0x103a, 0x203a,
  0x003b, 0x0f06, 0x103b, 0x203b, 0x003c, 0x0f07, 0x103c, 0x203c,
  0x0801, 0x0f08, 0x103d, 0x203d, 0x1001, 0x0f09, 0x103e, 0x203e,
  0x1801, 0x0f0a, 0x103f, 0x203f, 0x2001, 0x0f0b, 0x1040, 0x2040
};



#ifndef NDEBUG
DEFINE_bool(snappy_dump_decompression_table, false,
            "If true, we print the decompression table at startup.");

static uint16 MakeEntry(unsigned int extra,
                        unsigned int len,
                        unsigned int copy_offset) {
  
  DCHECK_EQ(extra,       extra & 0x7);          
  DCHECK_EQ(copy_offset, copy_offset & 0x7);    
  DCHECK_EQ(len,         len & 0x7f);           
  return len | (copy_offset << 8) | (extra << 11);
}

static void ComputeTable() {
  uint16 dst[256];

  
  int assigned = 0;
  for (int i = 0; i < 256; i++) {
    dst[i] = 0xffff;
  }

  
  for (unsigned int len = 1; len <= 60; len++) {
    dst[LITERAL | ((len-1) << 2)] = MakeEntry(0, len, 0);
    assigned++;
  }

  
  
  for (unsigned int extra_bytes = 1; extra_bytes <= 4; extra_bytes++) {
    
    
    dst[LITERAL | ((extra_bytes+59) << 2)] = MakeEntry(extra_bytes, 1, 0);
    assigned++;
  }

  
  
  
  
  
  
  
  for (unsigned int len = 4; len < 12; len++) {
    for (unsigned int offset = 0; offset < 2048; offset += 256) {
      dst[COPY_1_BYTE_OFFSET | ((len-4)<<2) | ((offset>>8)<<5)] =
        MakeEntry(1, len, offset>>8);
      assigned++;
    }
  }

  
  
  for (unsigned int len = 1; len <= 64; len++) {
    dst[COPY_2_BYTE_OFFSET | ((len-1)<<2)] = MakeEntry(2, len, 0);
    assigned++;
  }

  
  
  for (unsigned int len = 1; len <= 64; len++) {
    dst[COPY_4_BYTE_OFFSET | ((len-1)<<2)] = MakeEntry(4, len, 0);
    assigned++;
  }

  
  CHECK_EQ(assigned, 256);
  for (int i = 0; i < 256; i++) {
    CHECK_NE(dst[i], 0xffff);
  }

  if (FLAGS_snappy_dump_decompression_table) {
    printf("static const uint16 char_table[256] = {\n  ");
    for (int i = 0; i < 256; i++) {
      printf("0x%04x%s",
             dst[i],
             ((i == 255) ? "\n" : (((i%8) == 7) ? ",\n  " : ", ")));
    }
    printf("};\n");
  }

  
  for (int i = 0; i < 256; i++) {
    CHECK_EQ(dst[i], char_table[i]);
  }
}
REGISTER_MODULE_INITIALIZER(snappy, ComputeTable());
#endif 


class SnappyDecompressor {
 private:
  Source*       reader_;         
  const char*   ip_;             
  const char*   ip_limit_;       
  uint32        peeked_;         
  bool          eof_;            
  char          scratch_[5];     

  
  
  
  
  
  bool RefillTag();

 public:
  explicit SnappyDecompressor(Source* reader)
      : reader_(reader),
        ip_(NULL),
        ip_limit_(NULL),
        peeked_(0),
        eof_(false) {
  }

  ~SnappyDecompressor() {
    
    reader_->Skip(peeked_);
  }

  
  bool eof() const {
    return eof_;
  }

  
  
  
  bool ReadUncompressedLength(uint32* result) {
    DCHECK(ip_ == NULL);       
    
    *result = 0;
    uint32 shift = 0;
    while (true) {
      if (shift >= 32) return false;
      size_t n;
      const char* ip = reader_->Peek(&n);
      if (n == 0) return false;
      const unsigned char c = *(reinterpret_cast<const unsigned char*>(ip));
      reader_->Skip(1);
      *result |= static_cast<uint32>(c & 0x7f) << shift;
      if (c < 128) {
        break;
      }
      shift += 7;
    }
    return true;
  }

  
  
  template <class Writer>
  void DecompressAllTags(Writer* writer) {
    const char* ip = ip_;
    for ( ;; ) {
      if (ip_limit_ - ip < 5) {
        ip_ = ip;
        if (!RefillTag()) return;
        ip = ip_;
      }

      const unsigned char c = *(reinterpret_cast<const unsigned char*>(ip++));

      if ((c & 0x3) == LITERAL) {
        uint32 literal_length = c >> 2;
        if (PREDICT_FALSE(literal_length >= 60)) {
          
          const uint32 literal_length_length = literal_length - 59;
          literal_length =
              LittleEndian::Load32(ip) & wordmask[literal_length_length];
          ip += literal_length_length;
        }
        ++literal_length;

        uint32 avail = ip_limit_ - ip;
        while (avail < literal_length) {
          if (!writer->Append(ip, avail, false)) return;
          literal_length -= avail;
          reader_->Skip(peeked_);
          size_t n;
          ip = reader_->Peek(&n);
          avail = n;
          peeked_ = avail;
          if (avail == 0) return;  
          ip_limit_ = ip + avail;
        }
        bool allow_fast_path = (avail >= 16);
        if (!writer->Append(ip, literal_length, allow_fast_path)) {
          return;
        }
        ip += literal_length;
      } else {
        const uint32 entry = char_table[c];
        const uint32 trailer = LittleEndian::Load32(ip) & wordmask[entry >> 11];
        const uint32 length = entry & 0xff;
        ip += entry >> 11;

        
        
        
        const uint32 copy_offset = entry & 0x700;
        if (!writer->AppendFromSelf(copy_offset + trailer, length)) {
          return;
        }
      }
    }
  }
};

bool SnappyDecompressor::RefillTag() {
  const char* ip = ip_;
  if (ip == ip_limit_) {
    
    reader_->Skip(peeked_);   
    size_t n;
    ip = reader_->Peek(&n);
    peeked_ = n;
    if (n == 0) {
      eof_ = true;
      return false;
    }
    ip_limit_ = ip + n;
  }

  
  DCHECK_LT(ip, ip_limit_);
  const unsigned char c = *(reinterpret_cast<const unsigned char*>(ip));
  const uint32 entry = char_table[c];
  const uint32 needed = (entry >> 11) + 1;  
  DCHECK_LE(needed, sizeof(scratch_));

  
  uint32 nbuf = ip_limit_ - ip;
  if (nbuf < needed) {
    
    
    
    
    memmove(scratch_, ip, nbuf);
    reader_->Skip(peeked_);  
    peeked_ = 0;
    while (nbuf < needed) {
      size_t length;
      const char* src = reader_->Peek(&length);
      if (length == 0) return false;
      uint32 to_add = min<uint32>(needed - nbuf, length);
      memcpy(scratch_ + nbuf, src, to_add);
      nbuf += to_add;
      reader_->Skip(to_add);
    }
    DCHECK_EQ(nbuf, needed);
    ip_ = scratch_;
    ip_limit_ = scratch_ + needed;
  } else if (nbuf < 5) {
    
    
    memmove(scratch_, ip, nbuf);
    reader_->Skip(peeked_);  
    peeked_ = 0;
    ip_ = scratch_;
    ip_limit_ = scratch_ + nbuf;
  } else {
    
    ip_ = ip;
  }
  return true;
}

template <typename Writer>
static bool InternalUncompress(Source* r,
                               Writer* writer,
                               uint32 max_len) {
  
  SnappyDecompressor decompressor(r);
  uint32 uncompressed_len = 0;
  if (!decompressor.ReadUncompressedLength(&uncompressed_len)) return false;
  
  if (static_cast<uint64>(uncompressed_len) > max_len) {
    return false;
  }

  writer->SetExpectedLength(uncompressed_len);

  
  decompressor.DecompressAllTags(writer);
  return (decompressor.eof() && writer->CheckLength());
}

bool GetUncompressedLength(Source* source, uint32* result) {
  SnappyDecompressor decompressor(source);
  return decompressor.ReadUncompressedLength(result);
}

size_t Compress(Source* reader, Sink* writer) {
  size_t written = 0;
  int N = reader->Available();
  char ulength[Varint::kMax32];
  char* p = Varint::Encode32(ulength, N);
  writer->Append(ulength, p-ulength);
  written += (p - ulength);

  internal::WorkingMemory wmem;
  char* scratch = NULL;
  char* scratch_output = NULL;

  while (N > 0) {
    
    size_t fragment_size;
    const char* fragment = reader->Peek(&fragment_size);
    DCHECK_NE(fragment_size, 0) << ": premature end of input";
    const int num_to_read = min(N, kBlockSize);
    size_t bytes_read = fragment_size;

    int pending_advance = 0;
    if (bytes_read >= num_to_read) {
      
      pending_advance = num_to_read;
      fragment_size = num_to_read;
    } else {
      
      if (scratch == NULL) {
        
        
        
        scratch = new char[num_to_read];
      }
      memcpy(scratch, fragment, bytes_read);
      reader->Skip(bytes_read);

      while (bytes_read < num_to_read) {
        fragment = reader->Peek(&fragment_size);
        size_t n = min<size_t>(fragment_size, num_to_read - bytes_read);
        memcpy(scratch + bytes_read, fragment, n);
        bytes_read += n;
        reader->Skip(n);
      }
      DCHECK_EQ(bytes_read, num_to_read);
      fragment = scratch;
      fragment_size = num_to_read;
    }
    DCHECK_EQ(fragment_size, num_to_read);

    
    int table_size;
    uint16* table = wmem.GetHashTable(num_to_read, &table_size);

    
    const int max_output = MaxCompressedLength(num_to_read);

    
    
    if (scratch_output == NULL) {
      scratch_output = new char[max_output];
    } else {
      
      
      
    }
    char* dest = writer->GetAppendBuffer(max_output, scratch_output);
    char* end = internal::CompressFragment(fragment, fragment_size,
                                           dest, table, table_size);
    writer->Append(dest, end - dest);
    written += (end - dest);

    N -= num_to_read;
    reader->Skip(pending_advance);
  }

  delete[] scratch;
  delete[] scratch_output;

  return written;
}








class SnappyArrayWriter {
 private:
  char* base_;
  char* op_;
  char* op_limit_;

 public:
  inline explicit SnappyArrayWriter(char* dst)
      : base_(dst),
        op_(dst) {
  }

  inline void SetExpectedLength(size_t len) {
    op_limit_ = op_ + len;
  }

  inline bool CheckLength() const {
    return op_ == op_limit_;
  }

  inline bool Append(const char* ip, uint32 len, bool allow_fast_path) {
    char* op = op_;
    const int space_left = op_limit_ - op;
    if (allow_fast_path && len <= 16 && space_left >= 16) {
      
      UNALIGNED_STORE64(op, UNALIGNED_LOAD64(ip));
      UNALIGNED_STORE64(op + 8, UNALIGNED_LOAD64(ip + 8));
    } else {
      if (space_left < len) {
        return false;
      }
      memcpy(op, ip, len);
    }
    op_ = op + len;
    return true;
  }

  inline bool AppendFromSelf(uint32 offset, uint32 len) {
    char* op = op_;
    const int space_left = op_limit_ - op;

    if (op - base_ <= offset - 1u) {  
      return false;
    }
    if (len <= 16 && offset >= 8 && space_left >= 16) {
      
      UNALIGNED_STORE64(op, UNALIGNED_LOAD64(op - offset));
      UNALIGNED_STORE64(op + 8, UNALIGNED_LOAD64(op - offset + 8));
    } else {
      if (space_left >= len + kMaxIncrementCopyOverflow) {
        IncrementalCopyFastPath(op - offset, op, len);
      } else {
        if (space_left < len) {
          return false;
        }
        IncrementalCopy(op - offset, op, len);
      }
    }

    op_ = op + len;
    return true;
  }
};

bool RawUncompress(const char* compressed, size_t n, char* uncompressed) {
  ByteArraySource reader(compressed, n);
  return RawUncompress(&reader, uncompressed);
}

bool RawUncompress(Source* compressed, char* uncompressed) {
  SnappyArrayWriter output(uncompressed);
  return InternalUncompress(compressed, &output, kuint32max);
}

bool Uncompress(const char* compressed, size_t n, string* uncompressed) {
  size_t ulength;
  if (!GetUncompressedLength(compressed, n, &ulength)) {
    return false;
  }
  
  if ((static_cast<uint64>(ulength) + uncompressed->size()) >
      uncompressed->max_size()) {
    return false;
  }
  STLStringResizeUninitialized(uncompressed, ulength);
  return RawUncompress(compressed, n, string_as_array(uncompressed));
}



class SnappyDecompressionValidator {
 private:
  size_t expected_;
  size_t produced_;

 public:
  inline SnappyDecompressionValidator() : produced_(0) { }
  inline void SetExpectedLength(size_t len) {
    expected_ = len;
  }
  inline bool CheckLength() const {
    return expected_ == produced_;
  }
  inline bool Append(const char* ip, uint32 len, bool allow_fast_path) {
    produced_ += len;
    return produced_ <= expected_;
  }
  inline bool AppendFromSelf(uint32 offset, uint32 len) {
    if (produced_ <= offset - 1u) return false;  
    produced_ += len;
    return produced_ <= expected_;
  }
};

bool IsValidCompressedBuffer(const char* compressed, size_t n) {
  ByteArraySource reader(compressed, n);
  SnappyDecompressionValidator writer;
  return InternalUncompress(&reader, &writer, kuint32max);
}

void RawCompress(const char* input,
                 size_t input_length,
                 char* compressed,
                 size_t* compressed_length) {
  ByteArraySource reader(input, input_length);
  UncheckedByteArraySink writer(compressed);
  Compress(&reader, &writer);

  
  *compressed_length = (writer.CurrentDestination() - compressed);
}

size_t Compress(const char* input, size_t input_length, string* compressed) {
  
  compressed->resize(MaxCompressedLength(input_length));

  size_t compressed_length;
  RawCompress(input, input_length, string_as_array(compressed),
              &compressed_length);
  compressed->resize(compressed_length);
  return compressed_length;
}


} 

