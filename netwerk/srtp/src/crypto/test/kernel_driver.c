












































#include <stdio.h>           
#include <unistd.h>          
#include "crypto_kernel.h"

void
usage(char *prog_name) {
  printf("usage: %s [ -v ][ -d debug_module ]*\n", prog_name);
  exit(255);
}

int
main (int argc, char *argv[]) {
  extern char *optarg;
  int q;
  int do_validation      = 0;
  err_status_t status;

  if (argc == 1)
    usage(argv[0]);

   
  status = crypto_kernel_init();
  if (status) {
    printf("error: crypto_kernel init failed\n");
    exit(1);
  }
  printf("crypto_kernel successfully initalized\n");

  
  while (1) {
    q = getopt(argc, argv, "vd:");
    if (q == -1) 
      break;
    switch (q) {
    case 'v':
      do_validation = 1;
      break;
    case 'd':
      status = crypto_kernel_set_debug_module(optarg, 1);
      if (status) {
	printf("error: set debug module (%s) failed\n", optarg);
	exit(1);
      }
      break;
    default:
      usage(argv[0]);
    }    
  }

  if (do_validation) {
    printf("checking crypto_kernel status...\n");
    status = crypto_kernel_status();
    if (status) {
      printf("failed\n");
      exit(1);
    }
    printf("crypto_kernel passed self-tests\n");
  }

  status = crypto_kernel_shutdown();
  if (status) {
    printf("error: crypto_kernel shutdown failed\n");
    exit(1);
  }
  printf("crypto_kernel successfully shut down\n");
  
  return 0;
}






err_status_t
crypto_kernel_cipher_test(void) {

  

  return err_status_ok;
}
