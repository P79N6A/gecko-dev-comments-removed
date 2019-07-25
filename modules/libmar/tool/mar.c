





































#include <stdio.h>
#include <stdlib.h>
#include "mar.h"
#include "mar_cmdline.h"

#ifdef XP_WIN
#include <windows.h>
#include <direct.h>
#define chdir _chdir
#else
#include <unistd.h>
#endif

#if !defined(NO_SIGN_VERIFY) && (!defined(XP_WIN) || defined(MAR_NSS))
int NSSInitCryptoContext(const char *NSSConfigDir);
#endif

int mar_repackage_and_sign(const char *NSSConfigDir,
                           const char *certName, 
                           const char *src, 
                           const char * dest);

static void print_usage() {
  printf("usage:\n");
  printf("  mar [-C workingDir] {-c|-x|-t} archive.mar [files...]\n");
#ifndef NO_SIGN_VERIFY
  printf("  mar [-C workingDir] -d NSSConfigDir -n certname -s "
         "archive.mar out_signed_archive.mar\n");

#if defined(XP_WIN) && !defined(MAR_NSS)
  printf("  mar [-C workingDir] -D DERFilePath -v signed_archive.mar\n");
#else 
  printf("  mar [-C workingDir] -d NSSConfigDir -n certname "
    "-v signed_archive.mar\n");
#endif
#endif
}

static int mar_test_callback(MarFile *mar, 
                             const MarItem *item, 
                             void *unused) {
  printf("%u\t0%o\t%s\n", item->length, item->flags, item->name);
  return 0;
}

static int mar_test(const char *path) {
  MarFile *mar;

  mar = mar_open(path);
  if (!mar)
    return -1;

  printf("SIZE\tMODE\tNAME\n");
  mar_enum_items(mar, mar_test_callback, NULL);

  mar_close(mar);
  return 0;
}

int main(int argc, char **argv) {
  char *NSSConfigDir = NULL;
  char *certName = NULL;
#if defined(XP_WIN) && !defined(MAR_NSS) && !defined(NO_SIGN_VERIFY)
  HANDLE certFile;
  DWORD fileSize;
  DWORD read;
  char *certBuffer;
  char *DERFilePath = NULL;
#endif

  if (argc < 3) {
    print_usage();
    return -1;
  }

  while (argc > 0) {
    if (argv[1][0] == '-' && (argv[1][1] == 'c' || 
        argv[1][1] == 't' || argv[1][1] == 'x' || 
        argv[1][1] == 'v' || argv[1][1] == 's')) {
      break;
    
    } else if (argv[1][0] == '-' && argv[1][1] == 'C') {
      chdir(argv[2]);
      argv += 2;
      argc -= 2;
    } 
#if defined(XP_WIN) && !defined(MAR_NSS) && !defined(NO_SIGN_VERIFY)
    
    else if (argv[1][0] == '-' && argv[1][1] == 'D') {
      DERFilePath = argv[2];
      argv += 2;
      argc -= 2;
    }
#endif
    
    else if (argv[1][0] == '-' && argv[1][1] == 'd') {
      NSSConfigDir = argv[2];
      argv += 2;
      argc -= 2;
     
    } else if (argv[1][0] == '-' && argv[1][1] == 'n') {
      certName = argv[2];
      argv += 2;
      argc -= 2;
    } else {
      print_usage();
      return -1;
    }
  }

  if (argv[1][0] != '-') {
    print_usage();
    return -1;
  }

  switch (argv[1][1]) {
  case 'c':
    return mar_create(argv[2], argc - 3, argv + 3);
  case 't':
    return mar_test(argv[2]);
  case 'x':
    return mar_extract(argv[2]);

#ifndef NO_SIGN_VERIFY
  case 'v':

#if defined(XP_WIN) && !defined(MAR_NSS)
    if (!DERFilePath) {
      print_usage();
      return -1;
    }
    

    certFile = CreateFileA(DERFilePath, GENERIC_READ, 
                           FILE_SHARE_READ | 
                           FILE_SHARE_WRITE | 
                           FILE_SHARE_DELETE, 
                           NULL, 
                           OPEN_EXISTING, 
                           0, NULL);
    if (INVALID_HANDLE_VALUE == certFile) {
      return -1;
    }
    fileSize = GetFileSize(certFile, NULL);
    certBuffer = malloc(fileSize);
    if (!ReadFile(certFile, certBuffer, fileSize, &read, NULL) || 
        fileSize != read) {
      CloseHandle(certFile);
      free(certBuffer);
      return -1;
    }
    CloseHandle(certFile);

    if (mar_verify_signature(argv[2], certBuffer, fileSize, NULL)) {
      int oldMar = 0;
      free(certBuffer);

      
      if (is_old_mar(argv[2], &oldMar)) {
        fprintf(stderr, "ERROR: could not determine if MAR is old or new.\n");
      } else if (oldMar) {
        fprintf(stderr, "ERROR: The MAR file is in the old format so has"
                        " no signature to verify.\n");
      }
      return -1;
    }

    free(certBuffer);
    return 0;
#else
    if (!NSSConfigDir || !certName) {
      print_usage();
      return -1;
    }

    if (NSSInitCryptoContext(NSSConfigDir)) {
      fprintf(stderr, "ERROR: Could not initialize crypto library.\n");
      return -1;
    }

    return mar_verify_signature(argv[2], NULL, 0, 
                                certName);

#endif
  case 's':
    if (!NSSConfigDir || !certName || argc < 4) {
      print_usage();
      return -1;
    }
    return mar_repackage_and_sign(NSSConfigDir, certName, argv[2], argv[3]);
#endif

  default:
    print_usage();
    return -1;
  }

  return 0;
}
