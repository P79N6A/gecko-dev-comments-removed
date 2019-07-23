










#include "cdjpeg.h"		
#include <ctype.h>		
#ifdef NEED_SIGNAL_CATCHER
#include <signal.h>		
#endif
#ifdef USE_SETMODE
#include <fcntl.h>		

#include <io.h>			
#endif








#ifdef NEED_SIGNAL_CATCHER

static j_common_ptr sig_cinfo;

void				
signal_catcher (int signum)
{
  if (sig_cinfo != NULL) {
    if (sig_cinfo->err != NULL) 
      sig_cinfo->err->trace_level = 0;
    jpeg_destroy(sig_cinfo);	
  }
  exit(EXIT_FAILURE);
}


GLOBAL(void)
enable_signal_catcher (j_common_ptr cinfo)
{
  sig_cinfo = cinfo;
#ifdef SIGINT			
  signal(SIGINT, signal_catcher);
#endif
#ifdef SIGTERM			
  signal(SIGTERM, signal_catcher);
#endif
}

#endif






#ifdef PROGRESS_REPORT

METHODDEF(void)
progress_monitor (j_common_ptr cinfo)
{
  cd_progress_ptr prog = (cd_progress_ptr) cinfo->progress;
  int total_passes = prog->pub.total_passes + prog->total_extra_passes;
  int percent_done = (int) (prog->pub.pass_counter*100L/prog->pub.pass_limit);

  if (percent_done != prog->percent_done) {
    prog->percent_done = percent_done;
    if (total_passes > 1) {
      fprintf(stderr, "\rPass %d/%d: %3d%% ",
	      prog->pub.completed_passes + prog->completed_extra_passes + 1,
	      total_passes, percent_done);
    } else {
      fprintf(stderr, "\r %3d%% ", percent_done);
    }
    fflush(stderr);
  }
}


GLOBAL(void)
start_progress_monitor (j_common_ptr cinfo, cd_progress_ptr progress)
{
  
  if (cinfo->err->trace_level == 0) {
    progress->pub.progress_monitor = progress_monitor;
    progress->completed_extra_passes = 0;
    progress->total_extra_passes = 0;
    progress->percent_done = -1;
    cinfo->progress = &progress->pub;
  }
}


GLOBAL(void)
end_progress_monitor (j_common_ptr cinfo)
{
  
  if (cinfo->err->trace_level == 0) {
    fprintf(stderr, "\r                \r");
    fflush(stderr);
  }
}

#endif








GLOBAL(boolean)
keymatch (char * arg, const char * keyword, int minchars)
{
  register int ca, ck;
  register int nmatched = 0;

  while ((ca = *arg++) != '\0') {
    if ((ck = *keyword++) == '\0')
      return FALSE;		
    if (isupper(ca))		
      ca = tolower(ca);
    if (ca != ck)
      return FALSE;		
    nmatched++;			
  }
  
  if (nmatched < minchars)
    return FALSE;
  return TRUE;			
}







GLOBAL(FILE *)
read_stdin (void)
{
  FILE * input_file = stdin;

#ifdef USE_SETMODE		
  setmode(fileno(stdin), O_BINARY);
#endif
#ifdef USE_FDOPEN		
  if ((input_file = fdopen(fileno(stdin), READ_BINARY)) == NULL) {
    fprintf(stderr, "Cannot reopen stdin\n");
    exit(EXIT_FAILURE);
  }
#endif
  return input_file;
}


GLOBAL(FILE *)
write_stdout (void)
{
  FILE * output_file = stdout;

#ifdef USE_SETMODE		
  setmode(fileno(stdout), O_BINARY);
#endif
#ifdef USE_FDOPEN		
  if ((output_file = fdopen(fileno(stdout), WRITE_BINARY)) == NULL) {
    fprintf(stderr, "Cannot reopen stdout\n");
    exit(EXIT_FAILURE);
  }
#endif
  return output_file;
}
