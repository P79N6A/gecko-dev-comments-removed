











































void __cdecl
StartRecording()
{
  __asm {
    mov eax, 564d5868h
    mov ebx, 1
    mov cx, 47
    mov dx, 5658h
    in eax, dx
  }
}

void __cdecl
StopRecording()
{
  __asm {
    mov eax, 564d5868h
    mov ebx, 2
    mov cx, 47
    mov dx, 5658h
    in eax, dx 
  }
}
