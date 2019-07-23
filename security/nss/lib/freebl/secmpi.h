



































#include "mpi.h"

#define CHECK_SEC_OK(func) if (SECSuccess != (rv = func)) goto cleanup

#define CHECK_MPI_OK(func) if (MP_OKAY > (err = func)) goto cleanup

#define OCTETS_TO_MPINT(oc, mp, len) \
    CHECK_MPI_OK(mp_read_unsigned_octets((mp), oc, len))

#define SECITEM_TO_MPINT(it, mp) \
    CHECK_MPI_OK(mp_read_unsigned_octets((mp), (it).data, (it).len))

#define MPINT_TO_SECITEM(mp, it, arena)                         \
    SECITEM_AllocItem(arena, (it), mp_unsigned_octet_size(mp)); \
    if ((it)->data == NULL) {err = MP_MEM; goto cleanup;}       \
    err = mp_to_unsigned_octets(mp, (it)->data, (it)->len);     \
    if (err < 0) goto cleanup; else err = MP_OKAY;

#define MP_TO_SEC_ERROR(err)                                          \
    switch (err) {                                                    \
    case MP_MEM:    PORT_SetError(SEC_ERROR_NO_MEMORY);       break;  \
    case MP_RANGE:  PORT_SetError(SEC_ERROR_BAD_DATA);        break;  \
    case MP_BADARG: PORT_SetError(SEC_ERROR_INVALID_ARGS);    break;  \
    default:        PORT_SetError(SEC_ERROR_LIBRARY_FAILURE); break;  \
    }
