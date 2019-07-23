
































#ifndef CLIENT_MAC_HANDLER_BREAKPAD_NLIST_H__

typedef struct nlist_64 breakpad_nlist;

int
breakpad_nlist_64(const char *name,
                  breakpad_nlist *list,
                  const char **symbolNames);

#endif  
