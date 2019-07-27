
























#ifndef AVUTIL_EVAL_H
#define AVUTIL_EVAL_H

#include "avutil.h"

typedef struct AVExpr AVExpr;



















int av_expr_parse_and_eval(double *res, const char *s,
                           const char * const *const_names, const double *const_values,
                           const char * const *func1_names, double (* const *funcs1)(void *, double),
                           const char * const *func2_names, double (* const *funcs2)(void *, double, double),
                           void *opaque, int log_offset, void *log_ctx);


















int av_expr_parse(AVExpr **expr, const char *s,
                  const char * const *const_names,
                  const char * const *func1_names, double (* const *funcs1)(void *, double),
                  const char * const *func2_names, double (* const *funcs2)(void *, double, double),
                  int log_offset, void *log_ctx);








double av_expr_eval(AVExpr *e, const double *const_values, void *opaque);




void av_expr_free(AVExpr *e);


















double av_strtod(const char *numstr, char **tail);

#endif 
