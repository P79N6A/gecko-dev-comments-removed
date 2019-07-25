

























#include <stddef.h>

#ifdef _MSC_VER
# define INLINE __forceinline
  typedef  unsigned __int64 uint64;
  typedef  unsigned __int32 uint32;
  typedef  unsigned __int16 uint16;
  typedef  unsigned __int8  uint8;
#else
# include <stdint.h>
# define INLINE inline
  typedef  uint64_t  uint64;
  typedef  uint32_t  uint32;
  typedef  uint16_t  uint16;
  typedef  uint8_t   uint8;
#endif


class SpookyHash
{
public:
    
    
    
    static void Hash128(
        const void *message,  
        size_t length,        
        uint64 *hash1,        
        uint64 *hash2);       

    
    
    
    static uint64 Hash64(
        const void *message,  
        size_t length,        
        uint64 seed)          
    {
        uint64 hash1 = seed;
        Hash128(message, length, &hash1, &seed);
        return hash1;
    }

    
    
    
    static uint32 Hash32(
        const void *message,  
        size_t length,        
        uint32 seed)          
    {
        uint64 hash1 = seed, hash2 = seed;
        Hash128(message, length, &hash1, &hash2);
        return (uint32)hash1;
    }

    
    
    
    void Init(
        uint64 seed1,       
        uint64 seed2);      
    
    
    
    
    void Update(
        const void *message,  
        size_t length);       


    
    
    
    
    
    
    
    
    void Final(
        uint64 *hash1,    
        uint64 *hash2);   

    
    
    
    static INLINE uint64 Rot64(uint64 x, int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    static INLINE void Mix(
        const uint64 *data, 
        uint64 &s0, uint64 &s1, uint64 &s2, uint64 &s3,
        uint64 &s4, uint64 &s5, uint64 &s6, uint64 &s7,
        uint64 &s8, uint64 &s9, uint64 &s10,uint64 &s11)
    {
      s0 += data[0];    s2 ^= s10;    s11 ^= s0;    s0 = Rot64(s0,11);    s11 += s1;
      s1 += data[1];    s3 ^= s11;    s0 ^= s1;    s1 = Rot64(s1,32);    s0 += s2;
      s2 += data[2];    s4 ^= s0;    s1 ^= s2;    s2 = Rot64(s2,43);    s1 += s3;
      s3 += data[3];    s5 ^= s1;    s2 ^= s3;    s3 = Rot64(s3,31);    s2 += s4;
      s4 += data[4];    s6 ^= s2;    s3 ^= s4;    s4 = Rot64(s4,17);    s3 += s5;
      s5 += data[5];    s7 ^= s3;    s4 ^= s5;    s5 = Rot64(s5,28);    s4 += s6;
      s6 += data[6];    s8 ^= s4;    s5 ^= s6;    s6 = Rot64(s6,39);    s5 += s7;
      s7 += data[7];    s9 ^= s5;    s6 ^= s7;    s7 = Rot64(s7,57);    s6 += s8;
      s8 += data[8];    s10 ^= s6;    s7 ^= s8;    s8 = Rot64(s8,55);    s7 += s9;
      s9 += data[9];    s11 ^= s7;    s8 ^= s9;    s9 = Rot64(s9,54);    s8 += s10;
      s10 += data[10];    s0 ^= s8;    s9 ^= s10;    s10 = Rot64(s10,22);    s9 += s11;
      s11 += data[11];    s1 ^= s9;    s10 ^= s11;    s11 = Rot64(s11,46);    s10 += s0;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static INLINE void EndPartial(
        uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3,
        uint64 &h4, uint64 &h5, uint64 &h6, uint64 &h7, 
        uint64 &h8, uint64 &h9, uint64 &h10,uint64 &h11)
    {
        h11+= h1;    h2 ^= h11;   h1 = Rot64(h1,44);
	h0 += h2;    h3 ^= h0;    h2 = Rot64(h2,15);
	h1 += h3;    h4 ^= h1;    h3 = Rot64(h3,34);
	h2 += h4;    h5 ^= h2;    h4 = Rot64(h4,21);
	h3 += h5;    h6 ^= h3;    h5 = Rot64(h5,38);
	h4 += h6;    h7 ^= h4;    h6 = Rot64(h6,33);
	h5 += h7;    h8 ^= h5;    h7 = Rot64(h7,10);
	h6 += h8;    h9 ^= h6;    h8 = Rot64(h8,13);
	h7 += h9;    h10^= h7;    h9 = Rot64(h9,38);
	h8 += h10;   h11^= h8;    h10= Rot64(h10,53);
	h9 += h11;   h0 ^= h9;    h11= Rot64(h11,42);
	h10+= h0;    h1 ^= h10;   h0 = Rot64(h0,54);
    }

    static INLINE void End(
        uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3,
        uint64 &h4, uint64 &h5, uint64 &h6, uint64 &h7, 
        uint64 &h8, uint64 &h9, uint64 &h10,uint64 &h11)
    {
        EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
        EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
        EndPartial(h0,h1,h2,h3,h4,h5,h6,h7,h8,h9,h10,h11);
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    static INLINE void ShortMix(uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3)
    {
        h2 = Rot64(h2,50);  h2 += h3;  h0 ^= h2;
        h3 = Rot64(h3,52);  h3 += h0;  h1 ^= h3;
        h0 = Rot64(h0,30);  h0 += h1;  h2 ^= h0;
        h1 = Rot64(h1,41);  h1 += h2;  h3 ^= h1;
        h2 = Rot64(h2,54);  h2 += h3;  h0 ^= h2;
        h3 = Rot64(h3,48);  h3 += h0;  h1 ^= h3;
        h0 = Rot64(h0,38);  h0 += h1;  h2 ^= h0;
        h1 = Rot64(h1,37);  h1 += h2;  h3 ^= h1;
        h2 = Rot64(h2,62);  h2 += h3;  h0 ^= h2;
        h3 = Rot64(h3,34);  h3 += h0;  h1 ^= h3;
        h0 = Rot64(h0,5);   h0 += h1;  h2 ^= h0;
        h1 = Rot64(h1,36);  h1 += h2;  h3 ^= h1;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    static INLINE void ShortEnd(uint64 &h0, uint64 &h1, uint64 &h2, uint64 &h3)
    {
        h3 ^= h2;  h2 = Rot64(h2,15);  h3 += h2;
        h0 ^= h3;  h3 = Rot64(h3,52);  h0 += h3;
        h1 ^= h0;  h0 = Rot64(h0,26);  h1 += h0;
        h2 ^= h1;  h1 = Rot64(h1,51);  h2 += h1;
        h3 ^= h2;  h2 = Rot64(h2,28);  h3 += h2;
        h0 ^= h3;  h3 = Rot64(h3,9);   h0 += h3;
        h1 ^= h0;  h0 = Rot64(h0,47);  h1 += h0;
        h2 ^= h1;  h1 = Rot64(h1,54);  h2 += h1;
        h3 ^= h2;  h2 = Rot64(h2,32);  h3 += h2;
        h0 ^= h3;  h3 = Rot64(h3,25);  h0 += h3;
        h1 ^= h0;  h0 = Rot64(h0,63);  h1 += h0;
    }
    
private:

    
    
    
    
    
    
    static void Short(
        const void *message,
        size_t length,
        uint64 *hash1,
        uint64 *hash2);

    
    static const size_t sc_numVars = 12;

    
    static const size_t sc_blockSize = sc_numVars*8;

    
    static const size_t sc_bufSize = 2*sc_blockSize;

    
    
    
    
    
    
    
    static const uint64 sc_const = 0xdeadbeefdeadbeefLL;

    uint64 m_data[2*sc_numVars];   
    uint64 m_state[sc_numVars];  
    size_t m_length;             
    uint8  m_remainder;          
};



