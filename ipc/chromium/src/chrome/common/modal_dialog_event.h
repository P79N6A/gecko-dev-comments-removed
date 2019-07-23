



#ifndef CHROME_COMMON_MODAL_DIALOG_EVENT_H_
#define CHROME_COMMON_MODAL_DIALOG_EVENT_H_







struct ModalDialogEvent {
#if defined(OS_WIN)
  HANDLE event;
#endif
};

#endif  
