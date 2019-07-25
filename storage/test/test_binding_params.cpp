






































#include "storage_test_harness.h"

#include "mozStorageHelper.h"
  





void
test_ASCIIString()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE test (str STRING)"
  ));

  
  nsCOMPtr<mozIStorageStatement> insert, select;
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO test (str) VALUES (?1)"
  ), getter_AddRefs(insert));
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT str FROM test"
  ), getter_AddRefs(select));

  
  nsCAutoString inserted("I'm an ASCII string");
  {
    mozStorageStatementScoper scoper(insert);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(insert->BindUTF8StringByIndex(0, inserted)));
    do_check_true(NS_SUCCEEDED(insert->ExecuteStep(&hasResult)));
    do_check_false(hasResult);
  }

  nsCAutoString result;
  {
    mozStorageStatementScoper scoper(select);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(select->ExecuteStep(&hasResult)));
    do_check_true(hasResult);
    do_check_true(NS_SUCCEEDED(select->GetUTF8String(0, result)));
  }

  do_check_true(result == inserted);

  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM test"));
}

void
test_CString()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE test (str STRING)"
  ));

  
  nsCOMPtr<mozIStorageStatement> insert, select;
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO test (str) VALUES (?1)"
  ), getter_AddRefs(insert));
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT str FROM test"
  ), getter_AddRefs(select));

  
  static const char sCharArray[] =
    "I'm not a \xff\x00\xac\xde\xbb ASCII string!";
  nsCAutoString inserted(sCharArray, NS_ARRAY_LENGTH(sCharArray) - 1);
  do_check_true(inserted.Length() == NS_ARRAY_LENGTH(sCharArray) - 1);
  {
    mozStorageStatementScoper scoper(insert);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(insert->BindUTF8StringByIndex(0, inserted)));
    do_check_true(NS_SUCCEEDED(insert->ExecuteStep(&hasResult)));
    do_check_false(hasResult);
  }

  {
    nsCAutoString result;

    mozStorageStatementScoper scoper(select);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(select->ExecuteStep(&hasResult)));
    do_check_true(hasResult);
    do_check_true(NS_SUCCEEDED(select->GetUTF8String(0, result)));

    do_check_true(result == inserted);
  }

  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM test"));
}

void
test_UTFStrings()
{
  nsCOMPtr<mozIStorageConnection> db(getMemoryDatabase());

  
  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING(
    "CREATE TABLE test (str STRING)"
  ));

  
  nsCOMPtr<mozIStorageStatement> insert, select;
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "INSERT INTO test (str) VALUES (?1)"
  ), getter_AddRefs(insert));
  (void)db->CreateStatement(NS_LITERAL_CSTRING(
    "SELECT str FROM test"
  ), getter_AddRefs(select));

  
  static const char sCharArray[] =
    "I'm a \xc3\xbb\xc3\xbc\xc3\xa2\xc3\xa4\xc3\xa7 UTF8 string!";
  nsCAutoString insertedUTF8(sCharArray, NS_ARRAY_LENGTH(sCharArray) - 1);
  do_check_true(insertedUTF8.Length() == NS_ARRAY_LENGTH(sCharArray) - 1);
  NS_ConvertUTF8toUTF16 insertedUTF16(insertedUTF8);
  do_check_true(insertedUTF8 == NS_ConvertUTF16toUTF8(insertedUTF16));
  {
    mozStorageStatementScoper scoper(insert);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(insert->BindUTF8StringByIndex(0, insertedUTF8)));
    do_check_true(NS_SUCCEEDED(insert->ExecuteStep(&hasResult)));
    do_check_false(hasResult);
  }

  {
    nsCAutoString result;

    mozStorageStatementScoper scoper(select);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(select->ExecuteStep(&hasResult)));
    do_check_true(hasResult);
    do_check_true(NS_SUCCEEDED(select->GetUTF8String(0, result)));

    do_check_true(result == insertedUTF8);
  }

  
  {
    nsAutoString result;

    mozStorageStatementScoper scoper(select);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(select->ExecuteStep(&hasResult)));
    do_check_true(hasResult);
    do_check_true(NS_SUCCEEDED(select->GetString(0, result)));

    do_check_true(result == insertedUTF16);
  }

  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM test"));

  
  {
    mozStorageStatementScoper scoper(insert);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(insert->BindStringByIndex(0, insertedUTF16)));
    do_check_true(NS_SUCCEEDED(insert->ExecuteStep(&hasResult)));
    do_check_false(hasResult);
  }

  {
    nsCAutoString result;

    mozStorageStatementScoper scoper(select);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(select->ExecuteStep(&hasResult)));
    do_check_true(hasResult);
    do_check_true(NS_SUCCEEDED(select->GetUTF8String(0, result)));

    do_check_true(result == insertedUTF8);
  }

  
  {
    nsAutoString result;

    mozStorageStatementScoper scoper(select);
    bool hasResult;
    do_check_true(NS_SUCCEEDED(select->ExecuteStep(&hasResult)));
    do_check_true(hasResult);
    do_check_true(NS_SUCCEEDED(select->GetString(0, result)));

    do_check_true(result == insertedUTF16);
  }

  (void)db->ExecuteSimpleSQL(NS_LITERAL_CSTRING("DELETE FROM test"));
}

void (*gTests[])(void) = {
  test_ASCIIString,
  test_CString,
  test_UTFStrings,
};

const char *file = __FILE__;
#define TEST_NAME "binding string params"
#define TEST_FILE file
#include "storage_test_harness_tail.h"
