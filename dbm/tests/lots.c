

























































#include <stdio.h>

#include <stdlib.h>
#ifdef STDC_HEADERS
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif
#include <string.h>
#include <assert.h>
#include "mcom_db.h"

DB *database=0;
int MsgPriority=5;

#if defined(_WINDOWS) && !defined(WIN32)
#define int32 long
#define uint32 unsigned long
#else
#define int32 int
#define uint32 unsigned int
#endif

typedef enum {
USE_LARGE_KEY,
USE_SMALL_KEY
} key_type_enum;

#define TraceMe(priority, msg) 		\
	do {							\
		if(priority <= MsgPriority)	\
		  {							\
			ReportStatus msg;		\
		  }							\
	} while(0)

int
ReportStatus(char *string, ...)
{
    va_list args;

#ifdef STDC_HEADERS
    va_start(args, string);
#else
    va_start(args);
#endif
    vfprintf(stderr, string, args);
    va_end(args);

	fprintf (stderr, "\n");

	return(0);
}

int
ReportError(char *string, ...)
{
    va_list args;

#ifdef STDC_HEADERS
    va_start(args, string);
#else
    va_start(args);
#endif
	fprintf (stderr, "\n	");
    vfprintf(stderr, string, args);
	fprintf (stderr, "\n");
    va_end(args);

	return(0);
}

DBT * MakeLargeKey(int32 num)
{
	int32 low_bits;
	static DBT rv;
	static char *string_rv=0;
	int rep_char;
	size_t size;

	if(string_rv)
		free(string_rv);

	


	low_bits = (num % 10000) + 1;

	
	rep_char = (char) ((low_bits % 26) + 'a');

	
	size = low_bits*sizeof(char);
	string_rv = (char *)malloc(size);

	memset(string_rv, rep_char, size);

	rv.data = string_rv;
	rv.size = size;

	return(&rv);
}

DBT * MakeSmallKey(int32 num)
{
	static DBT rv;
	static char data_string[64];

	rv.data = data_string;

	sprintf(data_string, "%ld", (long)num);
	rv.size = strlen(data_string);

	return(&rv);

}

DBT * GenKey(int32 num, key_type_enum key_type)
{
	DBT *key;

	switch(key_type)
	  {
		case USE_LARGE_KEY:
			key = MakeLargeKey(num);
			break;
		case USE_SMALL_KEY:
			key = MakeSmallKey(num);
			break;
		default:
			abort();
			break;
	  }

	return(key);
}

int
SeqDatabase()
{
	int status;
	DBT key, data;

	ReportStatus("SEQuencing through database...");

	
    if(!(status = (*database->seq)(database, &key, &data, R_FIRST)))
	  {
        while(!(status = (database->seq) (database, &key, &data, R_NEXT)))
			; 
	  }

	if(status < 0)
		ReportError("Error seq'ing database");

	return(status);
}

int 
VerifyData(DBT *data, int32 num, key_type_enum key_type)
{
	int32 count, compare_num;
	size_t size;
	int32 *int32_array;

	



	if(data->size < sizeof(int32))
	  {
		ReportError("Data size corrupted");
		return -1;
	  }

	memcpy(&count, data->data, sizeof(int32));

	size = sizeof(int32)*(count+1);

	if(size != data->size)
	  {
		ReportError("Data size corrupted");
		return -1;
	  }

	int32_array = (int32*)data->data;

	for(;count > 0; count--)
	  {
		memcpy(&compare_num, &int32_array[count], sizeof(int32));

		if(compare_num != num)
	      {
		    ReportError("Data corrupted");
		    return -1;
	      }
	  }

	return(0);
}





#define SHOULD_EXIST 1
#define SHOULD_NOT_EXIST 0
int
VerifyRange(int32 low, int32 high, int32 should_exist, key_type_enum key_type)
{
	DBT *key, data;
	int32 num;
	int status;

	TraceMe(1, ("Verifying: %ld to %ld, using %s keys", 
		    low, high, key_type == USE_SMALL_KEY ? "SMALL" : "LARGE"));

	for(num = low; num <= high; num++)
	  {

		key = GenKey(num, key_type);

		status = (*database->get)(database, key, &data, 0);

		if(status == 0)
		  {
			
			if(!should_exist)
			  {
				ReportError("Item exists but shouldn't: %ld", num);
			  }
			else
			  {
			    
			    VerifyData(&data, num, key_type);
			  }
		  }
		else if(status > 0)
		  {
			
			if(should_exist)
			  {
				ReportError("Item not found but should be: %ld", num);
			  }
		  }
		else
		  {
			
			ReportError("Database error");
			return(-1);
		  }
			
	  }

	TraceMe(1, ("Correctly verified: %ld to %ld", low, high));

	return(0);

}

DBT *
GenData(int32 num)
{
	int32 n;
	static DBT *data=0;
	int32 *int32_array;
	size_t size;

	if(!data)
	  {
		data = (DBT*)malloc(sizeof(DBT));
		data->size = 0;
		data->data = 0;
	  }
	else if(data->data)
	  {
		free(data->data);
	  }

	n = rand();

	n = n % 512;  

	
	size = sizeof(int32)*(n+1);
	int32_array = (int32 *) malloc(size);

	memcpy(&int32_array[0], &n, sizeof(int32));

	for(; n > 0; n--)
	  {
		memcpy(&int32_array[n], &num, sizeof(int32));
	  }

	data->data = (void*)int32_array;
	data->size = size;

	return(data);
}

#define ADD_RANGE 1
#define DELETE_RANGE 2

int
AddOrDelRange(int32 low, int32 high, int action, key_type_enum key_type)
{
	DBT *key, *data;
#if 0 
	DBT tmp_data;
#endif 
	int32 num;
	int status;

	if(action != ADD_RANGE && action != DELETE_RANGE)
		assert(0);

	if(action == ADD_RANGE)
	  {
		TraceMe(1, ("Adding: %ld to %ld: %s keys", low, high,
		    	key_type == USE_SMALL_KEY ? "SMALL" : "LARGE"));
	  }
	else
	  {
		TraceMe(1, ("Deleting: %ld to %ld: %s keys", low, high,
		    	key_type == USE_SMALL_KEY ? "SMALL" : "LARGE"));
	  }

	for(num = low; num <= high; num++)
	  {

		key = GenKey(num, key_type);

		if(action == ADD_RANGE)
		  {
			data = GenData(num);
			status = (*database->put)(database, key, data, 0);
		  }
		else
		  {
			status = (*database->del)(database, key, 0);
		  }

		if(status < 0)
		  {
			ReportError("Database error %s item: %ld",
							action == ADD_RANGE ? "ADDING" : "DELETING", 
							num);
		  }
		else if(status > 0)
		  {
			ReportError("Could not %s item: %ld", 
							action == ADD_RANGE ? "ADD" : "DELETE", 
							num);
		  }
		else if(action == ADD_RANGE)
		  {
#define SYNC_EVERY_TIME
#ifdef SYNC_EVERY_TIME
			status = (*database->sync)(database, 0);
			if(status != 0)
				ReportError("Database error syncing after add");
#endif

#if 0 
	 
			

			status = (*database->get)(database, key, &tmp_data, 0);

			if(status != 0)
			  {
				ReportError("Database error checking item just added: %d",
							num);
			  }
			else
			  {
				


				VerifyRange(low, num, SHOULD_EXIST, key_type);
			  }
#endif
			
		  }
	  }


	if(action == ADD_RANGE)
	  {
		TraceMe(1, ("Successfully added: %ld to %ld", low, high));
	  }
	else
	  {
		TraceMe(1, ("Successfully deleted: %ld to %ld", low, high));
	  }

	return(0);
}

int
TestRange(int32 low, int32 range, key_type_enum key_type)
{
	int status; int32 low_of_range1, high_of_range1; int32 low_of_range2, high_of_range2;
	int32 low_of_range3, high_of_range3;

	status = AddOrDelRange(low, low+range, ADD_RANGE, key_type);
	status = VerifyRange(low, low+range, SHOULD_EXIST, key_type);

	TraceMe(1, ("Finished with sub test 1"));

	SeqDatabase();

	low_of_range1 = low;
	high_of_range1 = low+(range/2);
	low_of_range2 = high_of_range1+1;
	high_of_range2 = low+range;
	status = AddOrDelRange(low_of_range1, high_of_range1, DELETE_RANGE, key_type);
	status = VerifyRange(low_of_range1, high_of_range1, SHOULD_NOT_EXIST, key_type);
	status = VerifyRange(low_of_range2, low_of_range2, SHOULD_EXIST, key_type);

	TraceMe(1, ("Finished with sub test 2"));

	SeqDatabase();

	status = AddOrDelRange(low_of_range1, high_of_range1, ADD_RANGE, key_type);
	
	status = VerifyRange(low, low+range, SHOULD_EXIST, key_type);

	TraceMe(1, ("Finished with sub test 3"));

	SeqDatabase();

	status = AddOrDelRange(low_of_range2, high_of_range2, DELETE_RANGE, key_type);
	status = VerifyRange(low_of_range1, high_of_range1, SHOULD_EXIST, key_type);
	status = VerifyRange(low_of_range2, high_of_range2, SHOULD_NOT_EXIST, key_type);

	TraceMe(1, ("Finished with sub test 4"));

	SeqDatabase();

	status = AddOrDelRange(low_of_range2, high_of_range2, ADD_RANGE, key_type);
	
	status = VerifyRange(low, low+range, SHOULD_EXIST, key_type);

	TraceMe(1, ("Finished with sub test 5"));

	SeqDatabase();

	low_of_range1 = low;
	high_of_range1 = low+(range/3);
	low_of_range2 = high_of_range1+1;
	high_of_range2 = high_of_range1+(range/3);
	low_of_range3 = high_of_range2+1;
	high_of_range3 = low+range;
	
	status = AddOrDelRange(low_of_range2, high_of_range2, DELETE_RANGE, key_type);
	status = VerifyRange(low_of_range1, high_of_range1, SHOULD_EXIST, key_type);
	status = VerifyRange(low_of_range2, low_of_range2, SHOULD_NOT_EXIST, key_type);
	status = VerifyRange(low_of_range3, low_of_range2, SHOULD_EXIST, key_type);

	TraceMe(1, ("Finished with sub test 6"));

	SeqDatabase();

	status = AddOrDelRange(low_of_range2, high_of_range2, ADD_RANGE, key_type);
	
	status = VerifyRange(low, low+range, SHOULD_EXIST, key_type);

	TraceMe(1, ("Finished with sub test 7"));

	return(0);
}

#define START_RANGE 109876
int
main(int argc, char **argv)
{
	int32 i, j=0;
    int quick_exit = 0;
    int large_keys = 0;
    HASHINFO hash_info = {
        16*1024,
        0,
        0,
        0,
        0,
        0};


    if(argc > 1)
      {
        while(argc > 1)
	  {
            if(!strcmp(argv[argc-1], "-quick"))
                quick_exit = 1;
            else if(!strcmp(argv[argc-1], "-large"))
			  {
                large_keys = 1;
			  }
            argc--;
          }
      }

	database = dbopen("test.db", O_RDWR | O_CREAT, 0644, DB_HASH, &hash_info);

	if(!database)
	  {
		ReportError("Could not open database");
#ifdef unix
		perror("");
#endif
		exit(1);
	  }

	if(quick_exit)
	  {
		if(large_keys)
			TestRange(START_RANGE, 200, USE_LARGE_KEY);
		else
			TestRange(START_RANGE, 200, USE_SMALL_KEY);

		(*database->sync)(database, 0);
		(*database->close)(database);
        exit(0);
	  }

	for(i=100; i < 10000000; i+=200)
	  {
		if(1 || j)
		  {
			TestRange(START_RANGE, i, USE_LARGE_KEY);
			j = 0;
		  }
		else
		  {
			TestRange(START_RANGE, i, USE_SMALL_KEY);
			j = 1;
		  }

		if(1 == rand() % 3)
		  {
			(*database->sync)(database, 0);
		  }
		
		if(1 == rand() % 3)
	 	  {
			
			(*database->close)(database);
			database = dbopen("test.db", O_RDWR | O_CREAT, 0644, DB_HASH, 0);
			if(!database)
	  		{
				ReportError("Could not reopen database");
#ifdef unix
				perror("");
#endif
				exit(1);
	  		}
	 	  }
		else
		  {
			
			database = dbopen("test.db", O_RDWR | O_CREAT, 0644, DB_HASH, 0);
			if(!database)
	  		{
				ReportError("Could not reopen database "
							"after not closing the other");
#ifdef unix
				perror("");
#endif
				exit(1);
	  		}
	 	  }
	  }

	return(0);
}
