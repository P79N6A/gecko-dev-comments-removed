





struct TParseContext;
extern int glslang_initialize(TParseContext* context);
extern int glslang_finalize(TParseContext* context);

extern int glslang_scan(size_t count,
                        const char* const string[],
                        const int length[],
                        TParseContext* context);
extern int glslang_parse(TParseContext* context);

