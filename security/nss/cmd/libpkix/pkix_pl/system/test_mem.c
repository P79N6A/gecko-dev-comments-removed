









#include "testutil.h"
#include "testutil_nss.h"

static void *plContext = NULL;

static
void testMalloc(PKIX_UInt32 **array)
{
        PKIX_UInt32 i, arraySize = 10;
        PKIX_TEST_STD_VARS();

        
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Malloc(
                            (PKIX_UInt32)(arraySize*sizeof (unsigned int)),
                            (void **) array, plContext));

        
        (void) printf ("Setting array[i] = i...\n");
        for (i = 0; i < arraySize; i++) {
                (*array)[i] = i;
                if ((*array)[i] != i)
                        testError("Array has incorrect contents");
        }

        
        (void) printf("\tArray: a[0] = %d, a[5] = %d, a[7] = %d.\n",
                    (*array[0]), (*array)[5], (*array)[7]);

cleanup:
        PKIX_TEST_RETURN();
}

static
void testRealloc(PKIX_UInt32 **array)
{
        PKIX_UInt32 i, arraySize = 20;
        PKIX_TEST_STD_VARS();

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Realloc(*array,
                            (PKIX_UInt32)(arraySize*sizeof (unsigned int)),
                            (void **) array, plContext));

        
        (void) printf ("Setting new portion of array to a[i] = i...\n");
        for (i = arraySize/2; i < arraySize; i++) {
                (*array)[i] = i;
                if ((*array)[i] != i)
                        testError("Array has incorrect contents");
        }

        
        (void) printf("\tArray: a[0] = %d, a[15] = %d, a[17] = %d.\n",
                      (*array)[0], (*array)[15], (*array)[17]);

cleanup:
        PKIX_TEST_RETURN();
}

static
void testFree(PKIX_UInt32 *array)
{

        PKIX_TEST_STD_VARS();
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Free(array, plContext));

cleanup:
        PKIX_TEST_RETURN();
}

int test_mem(int argc, char *argv[]) {

        unsigned int *array = NULL;
        int arraySize = 10;
        PKIX_UInt32 actualMinorVersion;
        PKIX_UInt32 j = 0;

        PKIX_TEST_STD_VARS();

        startTests("Memory Allocation");

        PKIX_TEST_EXPECT_NO_ERROR(
            PKIX_PL_NssContext_Create(0, PKIX_FALSE, NULL, &plContext));

        subTest("PKIX_PL_Malloc");
        testMalloc(&array);

        subTest("PKIX_PL_Realloc");
        testRealloc(&array);

        subTest("PKIX_PL_Free");
        testFree(array);

        
        
        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Malloc(
                            (PKIX_UInt32)(arraySize*sizeof (unsigned int)),
                            (void **) &array, plContext));

        (void) printf("Attempting to reallocate 0 sized memory...\n");

        PKIX_TEST_EXPECT_NO_ERROR
                (PKIX_PL_Realloc(array, 0, (void **) &array, plContext));

        (void) printf("Attempting to allocate to null pointer...\n");

        PKIX_TEST_EXPECT_ERROR(PKIX_PL_Malloc(10, NULL, plContext));

        (void) printf("Attempting to reallocate to null pointer...\n");

        PKIX_TEST_EXPECT_ERROR(PKIX_PL_Realloc(NULL, 10, NULL, plContext));

        PKIX_TEST_EXPECT_NO_ERROR(PKIX_PL_Free(array, plContext));

cleanup:

        PKIX_Shutdown(plContext);

        endTests("Memory Allocation");

        return (0);
}
