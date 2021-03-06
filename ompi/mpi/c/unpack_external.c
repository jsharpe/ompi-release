/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2008 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2013      Los Alamos National Security, LLC.  All rights
 *                         reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#include "ompi_config.h"

#include "ompi/mpi/c/bindings.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/datatype/ompi_datatype.h"
#include "opal/datatype/opal_convertor.h"
#include "ompi/memchecker.h"

#if OPAL_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Unpack_external = PMPI_Unpack_external
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Unpack_external";


int MPI_Unpack_external (const char datarep[], const void *inbuf, MPI_Aint insize,
                         MPI_Aint *position, void *outbuf, int outcount,
                         MPI_Datatype datatype) 
{
    int rc;
    opal_convertor_t local_convertor;
    struct iovec outvec;
    unsigned int iov_count;
    size_t size;

    MEMCHECKER(
        memchecker_datatype(datatype);
        memchecker_call(&opal_memchecker_base_isdefined, outbuf, outcount, datatype);
    );

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if ((NULL == inbuf) || (NULL == position)) {  /* outbuf can be MPI_BOTTOM */
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_ARG, FUNC_NAME);
        } else if (outcount < 0) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COUNT, FUNC_NAME);
        } else if (MPI_DATATYPE_NULL == datatype || NULL == datatype) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_TYPE, FUNC_NAME);
        }
    }

    OPAL_CR_ENTER_LIBRARY();

    OBJ_CONSTRUCT(&local_convertor, opal_convertor_t);

    /* the resulting convertor will be set to the position ZERO */
    opal_convertor_copy_and_prepare_for_recv( ompi_mpi_external32_convertor,
                                              &(datatype->super), outcount, outbuf, 0, &local_convertor );

    /* Check for truncation */
    opal_convertor_get_packed_size( &local_convertor, &size );
    if( (*position + size) > (unsigned int)insize ) {
        OBJ_DESTRUCT( &local_convertor );
        OPAL_CR_EXIT_LIBRARY();
        return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_TRUNCATE, FUNC_NAME);
    }

    /* Prepare the iovec with all informations */
    outvec.iov_base = (char*) inbuf + (*position);
    outvec.iov_len = size;

    /* Do the actual unpacking */
    iov_count = 1;
    rc = opal_convertor_unpack( &local_convertor, &outvec, &iov_count, &size );
    *position += size;
    OBJ_DESTRUCT( &local_convertor );

    /* All done.  Note that the convertor returns 1 upon success, not
       OMPI_SUCCESS. */
    OMPI_ERRHANDLER_RETURN((rc == 1) ? OMPI_SUCCESS : OMPI_ERROR,
                           MPI_COMM_WORLD, MPI_ERR_UNKNOWN, FUNC_NAME);
}
