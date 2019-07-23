







































#ifndef __PANGO_COVERAGE_H__
#define __PANGO_COVERAGE_H__

#include <glib.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _PangoliteCoverage PangoliteCoverage;

typedef enum {
  PANGO_COVERAGE_NONE,
  PANGO_COVERAGE_FALLBACK,
  PANGO_COVERAGE_APPROXIMATE,
  PANGO_COVERAGE_EXACT
} PangoliteCoverageLevel;

PangoliteCoverage *    pangolite_coverage_new     (void);
PangoliteCoverage *    pangolite_coverage_ref     (PangoliteCoverage      *coverage);
void               pangolite_coverage_unref   (PangoliteCoverage      *coverage);
PangoliteCoverage *    pangolite_coverage_copy    (PangoliteCoverage      *coverage);
PangoliteCoverageLevel pangolite_coverage_get     (PangoliteCoverage      *coverage,
					   int                 index);
void               pangolite_coverage_set     (PangoliteCoverage      *coverage,
					   int                 index,
					   PangoliteCoverageLevel  level);
void               pangolite_coverage_max     (PangoliteCoverage      *coverage,
					   PangoliteCoverage      *other);

void           pangolite_coverage_to_bytes   (PangoliteCoverage  *coverage,
					  guchar        **bytes,
					  int            *n_bytes);
PangoliteCoverage *pangolite_coverage_from_bytes (guchar         *bytes,
					  int             n_bytes);

#ifdef __cplusplus
}
#endif 

#endif




