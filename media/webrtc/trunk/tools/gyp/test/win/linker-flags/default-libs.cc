



#include <windows.h>
#include <delayimp.h>
#include <odbcinst.h>
#include <shlobj.h>
#include <sql.h>
#include <stdio.h>




extern "C" void* __puiHead; 

int main() {
  CopyFile(0, 0, 0); 
  MessageBox(0, 0, 0, 0); 
  CreateDC(0, 0, 0, 0); 
  AddPrinter(0, 0, 0); 
  FindText(0); 
  ClearEventLog(0, 0); 
  SHGetSettings(0, 0); 
  OleFlushClipboard(); 
  VarAdd(0, 0, 0); 
  printf("%p", &CLSID_FileOpenDialog); 
  SQLAllocHandle(0, 0, 0); 
  return 0;
}
