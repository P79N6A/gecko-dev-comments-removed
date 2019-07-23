







































#include <stdio.h>
#include <glib.h>

#include "pango-types.h"

char    *pangolite_trim_string(const char *str);
char   **pangolite_split_file_list(const char *str);

gint     pangolite_read_line(FILE *stream, GString *str);

gboolean pangolite_skip_space(const char **pos);
gboolean pangolite_scan_word(const char **pos, GString *out);
gboolean pangolite_scan_string(const char **pos, GString *out);
gboolean pangolite_scan_int(const char **pos, int *out);

char *   pangolite_config_key_get(const char *key);







const char *pangolite_get_sysconf_subdirectory(void);





const char *pangolite_get_lib_subdirectory (void);
