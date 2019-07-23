



































#include <cstdio>
#include "nsStringAPI.h"

#define CHECK(x) \
  _doCheck(x, #x, __LINE__)

int _doCheck(bool cond, const char* msg, int line) {
  if (cond) return 0;
  fprintf(stderr, "FAIL: line %d: %s\n", line, msg);
  return 1;
}

int testEmpty() {
  nsString s;
  return CHECK(0 == s.Length()) +
         CHECK(s.IsEmpty());
}

int testAccess() {
  nsString s;
  s.Assign(NS_LITERAL_STRING("hello"));
  int res = CHECK(5 == s.Length()) +
            CHECK(s.EqualsLiteral("hello"));
  const PRUnichar *it, *end;
  int len = s.BeginReading(&it, &end);
  res += CHECK(5 == len);
  res += CHECK(PRUnichar('h') == it[0]) +
         CHECK(PRUnichar('e') == it[1]) +
         CHECK(PRUnichar('l') == it[2]) +
         CHECK(PRUnichar('l') == it[3]) +
         CHECK(PRUnichar('o') == it[4]) +
         CHECK(it + len == end);
  res += CHECK(s[0] == s.First());
  for (int i = 0; i < len; ++i) {
    res += CHECK(s[i] == it[i]);
    res += CHECK(s[i] == s.CharAt(i));
  }
  res += CHECK(it == s.BeginReading());
  res += CHECK(end == s.EndReading());
  return res;
}

int testWrite() {
  nsString s(NS_LITERAL_STRING("xyzz"));
  PRUnichar *begin, *end;
  int res = CHECK(4 == s.Length());
  PRUint32 len = s.BeginWriting(&begin, &end, 5);
  res += CHECK(5 == s.Length()) +
         CHECK(5 == len) +
         CHECK(end == begin + 5) +
         CHECK(begin == s.BeginWriting()) +
         CHECK(end == s.EndWriting());
  begin[4] = PRUnichar('y');
  res += CHECK(s.Equals(NS_LITERAL_STRING("xyzzy")));
  s.SetLength(4);
  res += CHECK(4 == s.Length()) +
         CHECK(s.Equals(NS_LITERAL_STRING("xyzz"))) +
         CHECK(!s.Equals(NS_LITERAL_STRING("xyzzy"))) +
         CHECK(!s.IsEmpty());
  s.Truncate();
  res += CHECK(0 == s.Length()) +
         CHECK(s.IsEmpty());
  const PRUnichar sample[] = { 's', 'a', 'm', 'p', 'l', 'e', '\0' };
  s.Assign(sample);
  res += CHECK(s.EqualsLiteral("sample"));
  s.Assign(sample, 4);
  res += CHECK(s.EqualsLiteral("samp"));
  s.Assign(PRUnichar('q'));
  res += CHECK(s.EqualsLiteral("q"));
  return res;
}

int testVoid() {
  nsString s;
  int ret = CHECK(!s.IsVoid());
  s.SetIsVoid(PR_FALSE);
  ret += CHECK(!s.IsVoid());
  s.SetIsVoid(PR_TRUE);
  ret += CHECK(s.IsVoid());
  s.SetIsVoid(PR_FALSE);
  ret += CHECK(!s.IsVoid());
  s.SetIsVoid(PR_TRUE);
  s.AssignLiteral("hello");
  ret += CHECK(!s.IsVoid());
  return ret;
}

int main() {
  int rv = 0;
  rv += testEmpty();
  rv += testAccess();
  rv += testWrite();
  rv += testVoid();
  if (0 == rv) {
    fprintf(stderr, "PASS: StringAPI tests\n");
  }
  return rv;
}
