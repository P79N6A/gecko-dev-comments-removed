

































#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <string>
#include <vector>

#include "google_breakpad/processor/basic_source_line_resolver.h"
#include "processor/logging.h"
#include "processor/network_source_line_server.h"
#include "processor/scoped_ptr.h"
#include "processor/simple_symbol_supplier.h"

using std::string;
using std::vector;
using google_breakpad::BasicSourceLineResolver;
using google_breakpad::NetworkSourceLineServer;
using google_breakpad::scoped_ptr;
using google_breakpad::SimpleSymbolSupplier;

void usage(char *progname) {
  printf("Usage: %s [-p port] [-a listen address] "
         "[-m maximum symbol lines to keep in memory] "
         "<symbol paths>\n", progname);
}

int main(int argc, char **argv) {
  BPLOG_INIT(&argc, &argv);

  unsigned short listen_port = 0;
  char listen_address[1024] = "";
  u_int64_t max_symbol_lines = 0;
  int arg;
  while((arg = getopt(argc, argv, "p:a:m:")) != -1) {
    switch(arg) {
    case 'p': {
      int port_arg = atoi(optarg);
      if (port_arg > -1 && port_arg < 65535) {
        listen_port = port_arg;
      } else {
        fprintf(stderr, "Invalid port number for -p!\n");
        usage(argv[0]);
        return 1;
      }
    }
      break;
    case 'a':
      strncpy(listen_address, optarg, sizeof(listen_address));
      break;
    case 'm':
      max_symbol_lines = atoll(optarg);
      break;
    case '?':
      fprintf(stderr, "Option -%c requires an argument\n", (char)optopt);
      usage(argv[0]);
      break;
    default:
      fprintf(stderr, "Unknown option: -%c\n", (char)arg);
      usage(argv[0]);
      return 1;
    }
  }

  if (optind >= argc) {
    usage(argv[0]);
    return 1;
  }
    
  vector<string> symbol_paths;
  for (int argi = optind; argi < argc; ++argi)
    symbol_paths.push_back(argv[argi]);

  scoped_ptr<SimpleSymbolSupplier> symbol_supplier;
  if (!symbol_paths.empty()) {
    symbol_supplier.reset(new SimpleSymbolSupplier(symbol_paths));
  }
  BasicSourceLineResolver resolver;

  NetworkSourceLineServer server(symbol_supplier.get(), &resolver, listen_port,
                                 
                                 
                                 listen_address[0] == '\0',
                                 listen_address,
                                 max_symbol_lines);
  if (!server.Initialize()) {
    BPLOG(ERROR) << "Failed to initialize server.";
    return 1;
  }

  server.RunForever();
  
  return 0;
}
