






















#include "jinclude.h"
#include "jpeglib.h"
#include "jversion.h"
#include "jerror.h"

#ifdef USE_WINDOWS_MESSAGEBOX
#include <windows.h>
#endif

#ifndef EXIT_FAILURE            
#define EXIT_FAILURE  1
#endif










#define JMESSAGE(code,string)   string ,

const char * const jpeg_std_message_table[] = {
#include "jerror.h"
  NULL
};















METHODDEF(void)
error_exit (j_common_ptr cinfo)
{
  
  (*cinfo->err->output_message) (cinfo);

  
  jpeg_destroy(cinfo);

  exit(EXIT_FAILURE);
}

















METHODDEF(void)
output_message (j_common_ptr cinfo)
{
  char buffer[JMSG_LENGTH_MAX];

  
  (*cinfo->err->format_message) (cinfo, buffer);

#ifdef USE_WINDOWS_MESSAGEBOX
  
  MessageBox(GetActiveWindow(), buffer, "JPEG Library Error",
             MB_OK | MB_ICONERROR);
#else
  
  fprintf(stderr, "%s\n", buffer);
#endif
}













METHODDEF(void)
emit_message (j_common_ptr cinfo, int msg_level)
{
  struct jpeg_error_mgr * err = cinfo->err;

  if (msg_level < 0) {
    



    if (err->num_warnings == 0 || err->trace_level >= 3)
      (*err->output_message) (cinfo);
    
    err->num_warnings++;
  } else {
    
    if (err->trace_level >= msg_level)
      (*err->output_message) (cinfo);
  }
}









METHODDEF(void)
format_message (j_common_ptr cinfo, char * buffer)
{
  struct jpeg_error_mgr * err = cinfo->err;
  int msg_code = err->msg_code;
  const char * msgtext = NULL;
  const char * msgptr;
  char ch;
  boolean isstring;

  
  if (msg_code > 0 && msg_code <= err->last_jpeg_message) {
    msgtext = err->jpeg_message_table[msg_code];
  } else if (err->addon_message_table != NULL &&
             msg_code >= err->first_addon_message &&
             msg_code <= err->last_addon_message) {
    msgtext = err->addon_message_table[msg_code - err->first_addon_message];
  }

  
  if (msgtext == NULL) {
    err->msg_parm.i[0] = msg_code;
    msgtext = err->jpeg_message_table[0];
  }

  
  isstring = FALSE;
  msgptr = msgtext;
  while ((ch = *msgptr++) != '\0') {
    if (ch == '%') {
      if (*msgptr == 's') isstring = TRUE;
      break;
    }
  }

  
  if (isstring)
    sprintf(buffer, msgtext, err->msg_parm.s);
  else
    sprintf(buffer, msgtext,
            err->msg_parm.i[0], err->msg_parm.i[1],
            err->msg_parm.i[2], err->msg_parm.i[3],
            err->msg_parm.i[4], err->msg_parm.i[5],
            err->msg_parm.i[6], err->msg_parm.i[7]);
}










METHODDEF(void)
reset_error_mgr (j_common_ptr cinfo)
{
  cinfo->err->num_warnings = 0;
  
  cinfo->err->msg_code = 0;     
}












GLOBAL(struct jpeg_error_mgr *)
jpeg_std_error (struct jpeg_error_mgr * err)
{
  err->error_exit = error_exit;
  err->emit_message = emit_message;
  err->output_message = output_message;
  err->format_message = format_message;
  err->reset_error_mgr = reset_error_mgr;

  err->trace_level = 0;         
  err->num_warnings = 0;        
  err->msg_code = 0;            

  
  err->jpeg_message_table = jpeg_std_message_table;
  err->last_jpeg_message = (int) JMSG_LASTMSGCODE - 1;

  err->addon_message_table = NULL;
  err->first_addon_message = 0; 
  err->last_addon_message = 0;

  return err;
}
