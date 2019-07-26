












































#include <stdio.h>           
#include <unistd.h>          
#include "crypto_kernel.h"







#define BUF_LEN (MAX_PRINT_STRING_LEN/2)

void
usage(char *prog_name) {
  printf("usage: %s -n <num_bytes> [-l][ -d debug_module ]*\n"
	 "   -n <num>   output <num> random bytes, where <num>"
	 " is between zero and %d\n"
	 "   -l         list the avaliable debug modules\n"
	 "   -d <mod>   turn on debugging module <mod>\n", 
	 prog_name, BUF_LEN);
  exit(255);
}

int
main (int argc, char *argv[]) {
  extern char *optarg;
  int q;
  int num_octets = 0;
  unsigned do_list_mods = 0;
  err_status_t status;

  if (argc == 1)
    usage(argv[0]);

   
  status = crypto_kernel_init();
  if (status) {
    printf("error: crypto_kernel init failed\n");
    exit(1);
  }

  
  while (1) {
    q = getopt(argc, argv, "ld:n:");
    if (q == -1) 
      break;
    switch (q) {
    case 'd':
      status = crypto_kernel_set_debug_module(optarg, 1);
      if (status) {
	printf("error: set debug module (%s) failed\n", optarg);
	exit(1);
      }
      break;
    case 'l':
      do_list_mods = 1;
      break;
    case 'n':
      num_octets = atoi(optarg);
      if (num_octets < 0 || num_octets > BUF_LEN)
	usage(argv[0]);
      break;
    default:
      usage(argv[0]);
    }    
  }

  if (do_list_mods) {
    status = crypto_kernel_list_debug_modules();
    if (status) {
      printf("error: list of debug modules failed\n");
      exit(1);
    }
  }

  if (num_octets > 0) {
    uint8_t buffer[BUF_LEN];
    
    status = crypto_get_random(buffer, num_octets);
    if (status) {
      printf("error: failure in random source\n");
    } else {
      printf("%s\n", octet_string_hex_string(buffer, num_octets));
    }
  }

  status = crypto_kernel_shutdown();
  if (status) {
    printf("error: crypto_kernel shutdown failed\n");
    exit(1);
  }
  
  return 0;
}

