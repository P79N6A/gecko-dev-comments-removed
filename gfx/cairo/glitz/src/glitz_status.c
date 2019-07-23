


























#ifdef HAVE_CONFIG_H
#  include "../config.h"
#endif

#include "glitzint.h"

unsigned long
glitz_status_to_status_mask (glitz_status_t status)
{
    switch (status) {
    case GLITZ_STATUS_NO_MEMORY:
	return GLITZ_STATUS_NO_MEMORY_MASK;
    case GLITZ_STATUS_BAD_COORDINATE:
	return GLITZ_STATUS_BAD_COORDINATE_MASK;
    case GLITZ_STATUS_NOT_SUPPORTED:
	return GLITZ_STATUS_NOT_SUPPORTED_MASK;
    case GLITZ_STATUS_CONTENT_DESTROYED:
	return GLITZ_STATUS_CONTENT_DESTROYED_MASK;
    case GLITZ_STATUS_SUCCESS:
	break;
    }

    return 0;
}

glitz_status_t
glitz_status_pop_from_mask (unsigned long *mask)
{
    if (*mask & GLITZ_STATUS_NO_MEMORY_MASK) {
	*mask &= ~GLITZ_STATUS_NO_MEMORY_MASK;
	return GLITZ_STATUS_NO_MEMORY;
    } else if (*mask & GLITZ_STATUS_BAD_COORDINATE_MASK) {
	*mask &= ~GLITZ_STATUS_BAD_COORDINATE_MASK;
	return GLITZ_STATUS_BAD_COORDINATE;
    } else if (*mask & GLITZ_STATUS_NOT_SUPPORTED_MASK) {
	*mask &= ~GLITZ_STATUS_NOT_SUPPORTED_MASK;
	return GLITZ_STATUS_NOT_SUPPORTED;
    } else if (*mask & GLITZ_STATUS_CONTENT_DESTROYED_MASK) {
	*mask &= ~GLITZ_STATUS_CONTENT_DESTROYED_MASK;
	return GLITZ_STATUS_CONTENT_DESTROYED;
    }

    return GLITZ_STATUS_SUCCESS;
}

const char *
glitz_status_string (glitz_status_t status)
{
    switch (status) {
    case GLITZ_STATUS_SUCCESS:
	return "success";
    case GLITZ_STATUS_NO_MEMORY:
	return "out of memory";
    case GLITZ_STATUS_BAD_COORDINATE:
	return "bad coordinate";
    case GLITZ_STATUS_NOT_SUPPORTED:
	return "not supported";
    case GLITZ_STATUS_CONTENT_DESTROYED:
	return "content destroyed";
    }

    return "<unknown error status>";
}
