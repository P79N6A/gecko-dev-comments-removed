







































#ifndef _zipstruct_h
#define _zipstruct_h








typedef struct ZipLocal_
  {
  unsigned char signature [4];
  unsigned char word [2];
  unsigned char bitflag [2];
  unsigned char method [2];
  unsigned char time [2];
  unsigned char date [2];
  unsigned char crc32 [4];
  unsigned char size [4];
  unsigned char orglen [4];
  unsigned char filename_len [2];
  unsigned char extrafield_len [2];
} ZipLocal;





 
#define ZIPLOCAL_SIZE (4+2+2+2+2+2+4+4+4+2+2)

typedef struct ZipCentral_
  {
  unsigned char signature [4];
  unsigned char version_made_by [2];
  unsigned char version [2];
  unsigned char bitflag [2];
  unsigned char method [2];
  unsigned char time [2];
  unsigned char date [2];
  unsigned char crc32 [4];
  unsigned char size [4];
  unsigned char orglen [4];
  unsigned char filename_len [2];
  unsigned char extrafield_len [2];
  unsigned char commentfield_len [2];
  unsigned char diskstart_number [2];
  unsigned char internal_attributes [2];
  unsigned char external_attributes [4];
  unsigned char localhdr_offset [4];
} ZipCentral;





 
#define ZIPCENTRAL_SIZE (4+2+2+2+2+2+2+4+4+4+2+2+2+2+2+4+4)

typedef struct ZipEnd_
  {
  unsigned char signature [4];
  unsigned char disk_nr [2];
  unsigned char start_central_dir [2];
  unsigned char total_entries_disk [2];
  unsigned char total_entries_archive [2];
  unsigned char central_dir_size [4];
  unsigned char offset_central_dir [4];
  unsigned char commentfield_len [2];
} ZipEnd;





 
#define ZIPEND_SIZE (4+2+2+2+2+4+4+2)


#define LOCALSIG    0x04034B50l
#define CENTRALSIG  0x02014B50l
#define ENDSIG      0x06054B50l


#define STORED            0
#define SHRUNK            1
#define REDUCED1          2
#define REDUCED2          3
#define REDUCED3          4
#define REDUCED4          5
#define IMPLODED          6
#define TOKENIZED         7
#define DEFLATED          8


#endif 
