


























































































#if !defined(ST_CMD_OPTION_BOOL)
#define ST_CMD_OPTION_BOOL(option_name, option_genre, option_help)
#endif
#if !defined(ST_WEB_OPTION_BOOL)
#define ST_WEB_OPTION_BOOL(option_name, option_genre, option_help)
#endif
#if !defined(ST_CMD_OPTION_STRING)
#define ST_CMD_OPTION_STRING(option_name, option_genre, default_value, option_help)
#endif
#if !defined(ST_WEB_OPTION_STRING)
#define ST_WEB_OPTION_STRING(option_name, option_genre, default_value, option_help)
#endif
#if !defined(ST_CMD_OPTION_STRING_ARRAY)
#define ST_CMD_OPTION_STRING_ARRAY(option_name, option_genre, array_size, option_help)
#endif
#if !defined(ST_WEB_OPTION_STRING_ARRAY)
#define ST_WEB_OPTION_STRING_ARRAY(option_name, option_genre, array_size, option_help)
#endif
#if !defined(ST_CMD_OPTION_STRING_PTR_ARRAY)
#define ST_CMD_OPTION_STRING_PTR_ARRAY(option_name, option_genre, option_help)
#endif
#if !defined(ST_WEB_OPTION_STRING_PTR_ARRAY)
#define ST_WEB_OPTION_STRING_PTR_ARRAY(option_name, option_genre, option_help)
#endif
#if !defined(ST_CMD_OPTION_UINT32)
#define ST_CMD_OPTION_UINT32(option_name, option_genre, default_value, multiplier, option_help)
#endif
#if !defined(ST_WEB_OPTION_UINT32)
#define ST_WEB_OPTION_UINT32(option_name, option_genre, default_value, multiplier, option_help)
#endif
#if !defined(ST_CMD_OPTION_UINT64)
#define ST_CMD_OPTION_UINT64(option_name, option_genre, default_value, multiplier, option_help)
#endif
#if !defined(ST_WEB_OPTION_UINT64)
#define ST_WEB_OPTION_UINT64(option_name, option_genre, default_value, multiplier, option_help)
#endif






#define ST_ALL_OPTION_BOOL(option_name, option_genre, option_help) \
    ST_CMD_OPTION_BOOL(option_name, option_genre, option_help) \
    ST_WEB_OPTION_BOOL(option_name, option_genre, option_help)
#define ST_ALL_OPTION_STRING(option_name, option_genre, default_value, option_help) \
    ST_CMD_OPTION_STRING(option_name, option_genre, default_value, option_help) \
    ST_WEB_OPTION_STRING(option_name, option_genre, default_value, option_help)
#define ST_ALL_OPTION_STRING_ARRAY(option_name, option_genre, array_size, option_help) \
    ST_CMD_OPTION_STRING_ARRAY(option_name, option_genre, array_size, option_help) \
    ST_WEB_OPTION_STRING_ARRAY(option_name, option_genre, array_size, option_help)
#define ST_ALL_OPTION_STRING_PTR_ARRAY(option_name, option_genre, option_help) \
    ST_CMD_OPTION_STRING_PTR_ARRAY(option_name, option_genre, option_help) \
    ST_WEB_OPTION_STRING_PTR_ARRAY(option_name, option_genre, option_help)
#define ST_ALL_OPTION_UINT32(option_name, option_genre, default_value, multiplier, option_help) \
    ST_CMD_OPTION_UINT32(option_name, option_genre, default_value, multiplier, option_help) \
    ST_WEB_OPTION_UINT32(option_name, option_genre, default_value, multiplier, option_help)
#define ST_ALL_OPTION_UINT64(option_name, option_genre, default_value, multiplier, option_help) \
    ST_CMD_OPTION_UINT64(option_name, option_genre, default_value, multiplier, option_help) \
    ST_WEB_OPTION_UINT64(option_name, option_genre, default_value, multiplier, option_help)












ST_ALL_OPTION_STRING(CategoryName,
                     CategoryGenre,
                     ST_ROOT_CATEGORY_NAME,
                     "Specify a category for reports to focus upon.\n"
                     "See http://lxr.mozilla.org/mozilla/source/tools/trace-malloc/rules.txt\n")

ST_ALL_OPTION_UINT32(OrderBy,
                     DataSortGenre,
                     ST_SIZE, 
                     1,
                     "Determine the sort order.\n"
                     "0 by weight (size * lifespan).\n"
                     "1 by size.\n"
                     "2 by lifespan.\n"
                     "3 by allocation count.\n"
                     "4 by performance cost.\n")

ST_ALL_OPTION_STRING_ARRAY(RestrictText,
                           DataSetGenre,
                           ST_SUBSTRING_MATCH_MAX,
                           "Exclude allocations which do not have this text in their backtrace.\n"
                           "Multiple restrictions are treated as a logical AND operation.\n")

ST_ALL_OPTION_UINT32(SizeMin,
                     DataSetGenre,
                     0,
                     1,
                     "Exclude allocations that are below this byte size.\n")

ST_ALL_OPTION_UINT32(SizeMax,
                     DataSetGenre,
                     0xFFFFFFFF,
                     1,
                     "Exclude allocations that are above this byte size.\n")

ST_ALL_OPTION_UINT32(LifetimeMin,
                     DataSetGenre,
                     ST_DEFAULT_LIFETIME_MIN,
                     ST_TIMEVAL_RESOLUTION,
                     "Allocations must live this number of seconds or be ignored.\n")

ST_ALL_OPTION_UINT32(LifetimeMax,
                     DataSetGenre,
                     ST_TIMEVAL_MAX / ST_TIMEVAL_RESOLUTION,
                     ST_TIMEVAL_RESOLUTION,
                     "Allocations living longer than this number of seconds will be ignored.\n")

ST_ALL_OPTION_UINT32(TimevalMin,
                     DataSetGenre,
                     0,
                     ST_TIMEVAL_RESOLUTION,
                     "Allocations existing solely before this second will be ignored.\n"
                     "Live allocations at this second and after can be considered.\n")

ST_ALL_OPTION_UINT32(TimevalMax,
                     DataSetGenre,
                     ST_TIMEVAL_MAX / ST_TIMEVAL_RESOLUTION,
                     ST_TIMEVAL_RESOLUTION,
                     "Allocations existing solely after this second will be ignored.\n"
                     "Live allocations at this second and before can be considered.\n")

ST_ALL_OPTION_UINT32(AllocationTimevalMin,
                     DataSetGenre,
                     0,
                     ST_TIMEVAL_RESOLUTION,
                     "Live and dead allocations created before this second will be ignored.\n")

ST_ALL_OPTION_UINT32(AllocationTimevalMax,
                     DataSetGenre,
                     ST_TIMEVAL_MAX / ST_TIMEVAL_RESOLUTION,
                     ST_TIMEVAL_RESOLUTION,
                     "Live and dead allocations created after this second will be ignored.\n")

ST_ALL_OPTION_UINT32(AlignBy,
                     DataSizeGenre,
                     ST_DEFAULT_ALIGNMENT_SIZE,
                     1,
                     "All allocation sizes are made to be a multiple of this number.\n"
                     "Closer to actual heap conditions; set to 1 for true sizes.\n")

ST_ALL_OPTION_UINT32(Overhead,
                     DataSizeGenre,
                     ST_DEFAULT_OVERHEAD_SIZE,
                     1,
                     "After alignment, all allocations are made to increase by this number.\n"
                     "Closer to actual heap conditions; set to 0 for true sizes.\n")

ST_ALL_OPTION_UINT32(ListItemMax,
                     UIGenre,
                     500,
                     1,
                     "Specifies the maximum number of list items to present in each list.\n")

ST_ALL_OPTION_UINT64(WeightMin,
                     DataSetGenre,
                     LL_INIT(0, 0),
                     LL_INIT(0, 1),
                     "Exclude allocations that are below this weight (lifespan * size).\n")

ST_ALL_OPTION_UINT64(WeightMax,
                     DataSetGenre,
                     LL_INIT(0xFFFFFFFF, 0xFFFFFFFF),
                     LL_INIT(0, 1),
                     "Exclude allocations that are above this weight (lifespan * size).\n")

ST_CMD_OPTION_STRING(FileName,
                     DataSetGenre,
                     "-",
                     "Specifies trace-malloc input file.\n"
                     "\"-\" indicates stdin will be used as input.\n")

ST_CMD_OPTION_STRING(CategoryFile,
                     CategoryGenre,
                     "rules.txt",
                     "Specifies the category rules file.\n"
                     "This file contains rules about how to categorize allocations.\n")

                     
ST_CMD_OPTION_UINT32(HttpdPort,
                     ServerGenre,
                     1969,
                     1,
                     "Specifies the default port the web server will listen on.\n")

ST_CMD_OPTION_STRING(OutputDir,
                     BatchModeGenre,
                     ".",
                     "Specifies a directory to output batch mode requests.\n"
                     "The directory must exist and must not use a trailing slash.\n")

ST_CMD_OPTION_STRING_PTR_ARRAY(BatchRequest,
                               BatchModeGenre,
                               "This implicitly turns on batch mode.\n"
                               "Save each requested file into the output dir, then exit.\n")

ST_CMD_OPTION_UINT32(Contexts,
                     ServerGenre,
                     1,
                     1,
                     "How many configurations to cache at the cost of a lot of memory.\n"
                     "Dedicated servers can cache more client configurations for performance.\n")

ST_CMD_OPTION_BOOL(Help,
                   UIGenre,
                   "Show command line help.\n"
                   "See http://www.mozilla.org/projects/footprint/spaceTrace.html\n")









#undef ST_ALL_OPTION_BOOL
#undef ST_CMD_OPTION_BOOL
#undef ST_WEB_OPTION_BOOL
#undef ST_ALL_OPTION_STRING
#undef ST_CMD_OPTION_STRING
#undef ST_WEB_OPTION_STRING
#undef ST_ALL_OPTION_STRING_ARRAY
#undef ST_CMD_OPTION_STRING_ARRAY
#undef ST_WEB_OPTION_STRING_ARRAY
#undef ST_ALL_OPTION_STRING_PTR_ARRAY
#undef ST_CMD_OPTION_STRING_PTR_ARRAY
#undef ST_WEB_OPTION_STRING_PTR_ARRAY
#undef ST_ALL_OPTION_UINT32
#undef ST_CMD_OPTION_UINT32
#undef ST_WEB_OPTION_UINT32
#undef ST_ALL_OPTION_UINT64
#undef ST_CMD_OPTION_UINT64
#undef ST_WEB_OPTION_UINT64
