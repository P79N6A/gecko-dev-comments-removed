

















#ifndef _PAGER_H_
#define _PAGER_H_




#ifndef SQLITE_DEFAULT_PAGE_SIZE
# define SQLITE_DEFAULT_PAGE_SIZE 1024
#endif











#ifndef SQLITE_MAX_PAGE_SIZE
# define SQLITE_MAX_PAGE_SIZE 32768
#endif




#define SQLITE_MAX_PAGE 1073741823





typedef unsigned int Pgno;




typedef struct Pager Pager;






#define PAGER_OMIT_JOURNAL  0x0001    /* Do not use a rollback journal */
#define PAGER_NO_READLOCK   0x0002    /* Omit readlocks on readonly files */






int sqlite3pager_open(Pager **ppPager, const char *zFilename,
                     int nExtra, int flags);
void sqlite3pager_set_busyhandler(Pager*, BusyHandler *pBusyHandler);
void sqlite3pager_set_destructor(Pager*, void(*)(void*,int));
void sqlite3pager_set_reiniter(Pager*, void(*)(void*,int));
int sqlite3pager_set_pagesize(Pager*, int);
void sqlite3pager_read_fileheader(Pager*, int, unsigned char*);
void sqlite3pager_set_cachesize(Pager*, int);
int sqlite3pager_close(Pager *pPager);
int sqlite3pager_get(Pager *pPager, Pgno pgno, void **ppPage);
void *sqlite3pager_lookup(Pager *pPager, Pgno pgno);
int sqlite3pager_ref(void*);
int sqlite3pager_unref(void*);
Pgno sqlite3pager_pagenumber(void*);
int sqlite3pager_write(void*);
int sqlite3pager_iswriteable(void*);
int sqlite3pager_overwrite(Pager *pPager, Pgno pgno, void*);
int sqlite3pager_pagecount(Pager*);
int sqlite3pager_truncate(Pager*,Pgno);
int sqlite3pager_begin(void*, int exFlag);
int sqlite3pager_commit(Pager*);
int sqlite3pager_sync(Pager*,const char *zMaster, Pgno);
int sqlite3pager_rollback(Pager*);
int sqlite3pager_isreadonly(Pager*);
int sqlite3pager_stmt_begin(Pager*);
int sqlite3pager_stmt_commit(Pager*);
int sqlite3pager_stmt_rollback(Pager*);
void sqlite3pager_dont_rollback(void*);
void sqlite3pager_dont_write(Pager*, Pgno);
int *sqlite3pager_stats(Pager*);
void sqlite3pager_set_safety_level(Pager*,int,int);
const char *sqlite3pager_filename(Pager*);
const char *sqlite3pager_dirname(Pager*);
const char *sqlite3pager_journalname(Pager*);
int sqlite3pager_nosync(Pager*);
int sqlite3pager_rename(Pager*, const char *zNewName);
void sqlite3pager_set_codec(Pager*,void*(*)(void*,void*,Pgno,int),void*);
int sqlite3pager_movepage(Pager*,void*,Pgno);
int sqlite3pager_reset(Pager*);
int sqlite3pager_release_memory(int);
int sqlite3pager_loadall(Pager*);

#if defined(SQLITE_DEBUG) || defined(SQLITE_TEST)
int sqlite3pager_lockstate(Pager*);
#endif

#ifdef SQLITE_TEST
void sqlite3pager_refdump(Pager*);
int pager3_refinfo_enable;
#endif

#endif 
