typedef int PRUint32;
typedef PRUint32 nsresult;

char *pseudomalloc();

nsresult foo(char **result)
{
  *result = pseudomalloc();
  if (!*result)
    return 1;

  

  return 0;
}
