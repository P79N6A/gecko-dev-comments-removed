











































#include <stdlib.h>
#include <string.h>

#include "nsIDNKitInterface.h"

#define UCS_MAX		0x7fffffff
#define UNICODE_MAX	0x10ffff





#include "nameprepdata.c"





#define VERSION id11
#include "nameprep_template.c"
#undef VERSION

typedef const char	*(*nameprep_mapproc)(PRUint32 v);
typedef int		(*nameprep_checkproc)(PRUint32 v);
typedef idn_biditype_t	(*nameprep_biditypeproc)(PRUint32 v);

static struct idn_nameprep {
	char *version;
	nameprep_mapproc map_proc;
	nameprep_checkproc prohibited_proc;
	nameprep_checkproc unassigned_proc;
	nameprep_biditypeproc biditype_proc;
} nameprep_versions[] = {
#define MAKE_NAMEPREP_HANDLE(version, id) \
	{ version, \
	  compose_sym2(nameprep_map_, id), \
	  compose_sym2(nameprep_prohibited_, id), \
	  compose_sym2(nameprep_unassigned_, id), \
	  compose_sym2(nameprep_biditype_, id), }
	MAKE_NAMEPREP_HANDLE("nameprep-11", id11),
	{ NULL, NULL, NULL, NULL, NULL },
};

static idn_result_t	idn_nameprep_check(nameprep_checkproc proc,
					   const PRUint32 *str,
					   const PRUint32 **found);

idn_result_t
idn_nameprep_create(const char *version, idn_nameprep_t *handlep) {
	idn_nameprep_t handle;

	assert(handlep != NULL);

	TRACE(("idn_nameprep_create(version=%-.50s)\n",
	       version == NULL ? "<NULL>" : version));

	if (version == NULL)
		version = IDN_NAMEPREP_CURRENT;

	




	for (handle = nameprep_versions; handle->version != NULL; handle++) {
		if (strcmp(handle->version, version) == 0) {
			*handlep = handle;
			return (idn_success);
		}
	}
	return (idn_notfound);
}

void
idn_nameprep_destroy(idn_nameprep_t handle) {
	assert(handle != NULL);

	TRACE(("idn_nameprep_destroy()\n"));

	
}

idn_result_t
idn_nameprep_map(idn_nameprep_t handle, const PRUint32 *from,
		 PRUint32 *to, size_t tolen) {
	assert(handle != NULL && from != NULL && to != NULL);

	TRACE(("idn_nameprep_map(ctx=%s, from=\"%s\")\n",
	       handle->version, idn__debug_ucs4xstring(from, 50)));

	while (*from != '\0') {
		PRUint32 v = *from;
		const char *mapped;

		if (v > UCS_MAX) {
			
			return (idn_invalid_codepoint);
		} else if (v > UNICODE_MAX) {
			
			mapped = NULL;
		} else {
			
			mapped = (*handle->map_proc)(v);
		}

		if (mapped == NULL) {
			
			if (tolen < 1)
				return (idn_buffer_overflow);
			*to++ = v;
			tolen--;
		} else {
			const unsigned char *mappeddata;
			size_t mappedlen;

			mappeddata = (const unsigned char *)mapped + 1;
			mappedlen = *mapped;

			if (tolen < (mappedlen + 3) / 4)
				return (idn_buffer_overflow);
			tolen -= (mappedlen + 3) / 4;
			while (mappedlen >= 4) {
				*to  = *mappeddata++;
				*to |= *mappeddata++ <<  8;
				*to |= *mappeddata++ << 16;
				*to |= *mappeddata++ << 24;
				mappedlen -= 4;
				to++;
			}
			if (mappedlen > 0) {
				*to  = *mappeddata++;
				*to |= (mappedlen >= 2) ?
				       *mappeddata++ <<  8: 0;
				*to |= (mappedlen >= 3) ?
				       *mappeddata++ << 16: 0;
				to++;
			}
		}
		from++;
	}
	if (tolen == 0)
		return (idn_buffer_overflow);
	*to = '\0';
	return (idn_success);
}

idn_result_t
idn_nameprep_isprohibited(idn_nameprep_t handle, const PRUint32 *str,
			  const PRUint32 **found) {
	assert(handle != NULL && str != NULL && found != NULL);

	TRACE(("idn_nameprep_isprohibited(ctx=%s, str=\"%s\")\n",
	       handle->version, idn__debug_ucs4xstring(str, 50)));

	return (idn_nameprep_check(handle->prohibited_proc, str, found));
}
		
idn_result_t
idn_nameprep_isunassigned(idn_nameprep_t handle, const PRUint32 *str,
			  const PRUint32 **found) {
	assert(handle != NULL && str != NULL && found != NULL);

	TRACE(("idn_nameprep_isunassigned(handle->version, str=\"%s\")\n",
	       handle->version, idn__debug_ucs4xstring(str, 50)));

	return (idn_nameprep_check(handle->unassigned_proc, str, found));
}
		
static idn_result_t
idn_nameprep_check(nameprep_checkproc proc, const PRUint32 *str,
		   const PRUint32 **found) {
	PRUint32 v;

	while (*str != '\0') {
		v = *str;

		if (v > UCS_MAX) {
			
			return (idn_invalid_codepoint);
		} else if (v > UNICODE_MAX) {
			
			*found = str;
			return (idn_success);
		} else if ((*proc)(v)) {
			*found = str;
			return (idn_success);
		}
		str++;
	}
	*found = NULL;
	return (idn_success);
}

idn_result_t
idn_nameprep_isvalidbidi(idn_nameprep_t handle, const PRUint32 *str,
			 const PRUint32 **found) {
	PRUint32 v;
	idn_biditype_t first_char;
	idn_biditype_t last_char;
	int found_r_al;

	assert(handle != NULL && str != NULL && found != NULL);

	TRACE(("idn_nameprep_isvalidbidi(ctx=%s, str=\"%s\")\n",
	       handle->version, idn__debug_ucs4xstring(str, 50)));

	if (*str == '\0') {
		*found = NULL;
		return (idn_success);
	}

	


	found_r_al = 0;
	if (*str > UCS_MAX) {
		
		return (idn_invalid_codepoint);
	} else if (*str > UNICODE_MAX) {
		
		*found = str;
		return (idn_success);
	}
	first_char = last_char = (*(handle->biditype_proc))(*str);
	if (first_char == idn_biditype_r_al) {
		found_r_al = 1;
	}
	str++;

	


	while (*str != '\0') {
		v = *str;

		if (v > UCS_MAX) {
			
			return (idn_invalid_codepoint);
		} else if (v > UNICODE_MAX) {
			
			*found = str;
			return (idn_success);
		} else { 
			last_char = (*(handle->biditype_proc))(v);
			if (found_r_al && last_char == idn_biditype_l) {
				*found = str;
				return (idn_success);
			}
			if (first_char != idn_biditype_r_al && last_char == idn_biditype_r_al) {
				*found = str;
				return (idn_success);
			}
			if (last_char == idn_biditype_r_al) {
				found_r_al = 1;
			}
		}
		str++;
	}

	if (found_r_al) {
		if (last_char != idn_biditype_r_al) {
			*found = str - 1;
			return (idn_success);
		}
	}

	*found = NULL;
	return (idn_success);
}

idn_result_t
idn_nameprep_createproc(const char *parameter, void **handlep) {
	return idn_nameprep_create(parameter, (idn_nameprep_t *)handlep);
}

void
idn_nameprep_destroyproc(void *handle) {
	idn_nameprep_destroy((idn_nameprep_t)handle);
}

idn_result_t
idn_nameprep_mapproc(void *handle, const PRUint32 *from,
		      PRUint32 *to, size_t tolen) {
	return idn_nameprep_map((idn_nameprep_t)handle, from, to, tolen);
}

idn_result_t
idn_nameprep_prohibitproc(void *handle, const PRUint32 *str,
			   const PRUint32 **found) {
	return idn_nameprep_isprohibited((idn_nameprep_t)handle, str, found);
}

idn_result_t
idn_nameprep_unassignedproc(void *handle, const PRUint32 *str,
			     const PRUint32 **found) {
	return idn_nameprep_isunassigned((idn_nameprep_t)handle, str, found);
}

idn_result_t
idn_nameprep_bidiproc(void *handle, const PRUint32 *str,
		      const PRUint32 **found) {
	return idn_nameprep_isvalidbidi((idn_nameprep_t)handle, str, found);
}
