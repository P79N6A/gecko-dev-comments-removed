






















































#include "aes.h"
#include <stdio.h>
#include <string.h>

void
usage(char *prog_name) {
  printf("usage: %s <key> <plaintext> [-v]\n", prog_name);
  exit(255);
}

#define AES_MAX_KEY_LEN 32

int
main (int argc, char *argv[]) {
  v128_t data;
  uint8_t key[AES_MAX_KEY_LEN];
  aes_expanded_key_t exp_key;
  int key_len, len;
  int verbose;
  err_status_t status;

  if (argc == 3) {
    
    verbose = 0;
  } else if (argc == 4) {
    if (strncmp(argv[3], "-v", 2) == 0) {
      
      verbose = 1;
    } else {
      
      usage(argv[0]);
    }
  } else {
    
    usage(argv[0]);
  }
  
  
  if (strlen(argv[1]) > AES_MAX_KEY_LEN*2) {
    fprintf(stderr, 
	    "error: too many digits in key "
	    "(should be at most %d hexadecimal digits, found %u)\n",
	    AES_MAX_KEY_LEN*2, (unsigned)strlen(argv[1]));
    exit(1);    
  }
  len = hex_string_to_octet_string((char*)key, argv[1], AES_MAX_KEY_LEN*2);
  
  if (len != 32 && len != 48 && len != 64) {
    fprintf(stderr, 
	    "error: bad number of digits in key "
	    "(should be 32/48/64 hexadecimal digits, found %d)\n",
	    len);
    exit(1);    
  } 
  key_len = len/2;
      
  
  if (strlen(argv[2]) > 16*2) {
    fprintf(stderr, 
	    "error: too many digits in plaintext "
	    "(should be %d hexadecimal digits, found %u)\n",
	    16*2, (unsigned)strlen(argv[2]));
    exit(1);    
  }
  len = hex_string_to_octet_string((char *)(&data), argv[2], 16*2);
  
  if (len < 16*2) {
    fprintf(stderr, 
	    "error: too few digits in plaintext "
	    "(should be %d hexadecimal digits, found %d)\n",
	    16*2, len);
    exit(1);    
  }

  if (verbose) {
    
    printf("plaintext:\t%s\n", octet_string_hex_string((uint8_t *)&data, 16));
  }

  
  status = aes_expand_encryption_key(key, key_len, &exp_key);
  if (status) {
    fprintf(stderr,
	    "error: AES key expansion failed.\n");
    exit(1);
  }

  aes_encrypt(&data, &exp_key);

  
  if (verbose) {
    printf("key:\t\t%s\n", octet_string_hex_string(key, key_len));
    printf("ciphertext:\t");
  }
  printf("%s\n", v128_hex_string(&data));

  return 0;
}

