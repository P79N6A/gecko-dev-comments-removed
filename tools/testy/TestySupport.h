



#ifdef __cplusplus
extern "C" {
#endif

PR_EXPORT(int)  Testy_LogInit(const char* fileName);
PR_EXPORT(void) Testy_LogShutdown();

PR_EXPORT(void) Testy_LogStart(const char* name);
PR_EXPORT(void) Testy_LogComment(const char* name, const char* comment);
PR_EXPORT(void) Testy_LogEnd(const char* name, bool passed);

#ifdef __cplusplus
}
#endif
