


















































































#ifndef _r_errors_h
#define _r_errors_h

#define R_NO_MEMORY      1 /*out of memory*/
#define R_NOT_FOUND	 2 /*Item not found*/
#define R_INTERNAL	 3 /*Unspecified internal error*/
#define R_ALREADY	 4 /*Action already done*/
#define R_EOD            5 /*end of data*/
#define R_BAD_ARGS       6 /*Bad arguments*/
#define R_BAD_DATA	 7 /*Bad data*/
#define R_WOULDBLOCK     8 /*Operation would block */
#define R_QUEUED         9 /*Operation was queued */
#define R_FAILED        10 /*Operation failed */
#define R_REJECTED      11 /* We don't care about this */
#define R_INTERRUPTED   12 /* Operation interrupted */
#define R_IO_ERROR      13 /* I/O Error */
#define R_NOT_PERMITTED 14 /* Permission denied */
#define R_RETRY         15 /* Retry possible */

#define NR_ERROR_MAPPING {\
    { R_NO_MEMORY,        "Cannot allocate memory" },\
    { R_NOT_FOUND,        "Item not found" },\
    { R_INTERNAL,         "Internal failure" },\
    { R_ALREADY,          "Action already performed" },\
    { R_EOD,              "End of data" },\
    { R_BAD_ARGS,         "Invalid argument" },\
    { R_BAD_DATA,         "Invalid data" },\
    { R_WOULDBLOCK,       "Operation would block" },\
    { R_QUEUED,           "Operation queued" },\
    { R_FAILED,           "Operation failed" },\
    { R_REJECTED,         "Operation rejected" },\
    { R_INTERRUPTED,      "Operation interrupted" },\
    { R_IO_ERROR,         "I/O error" },\
    { R_NOT_PERMITTED,    "Permission Denied" },\
    { R_RETRY,            "Retry may be possible" },\
    }

int nr_verr_exit(char *fmt,...);

char *nr_strerror(int errnum);
int   nr_strerror_r(int errnum, char *strerrbuf, size_t buflen);

#endif

