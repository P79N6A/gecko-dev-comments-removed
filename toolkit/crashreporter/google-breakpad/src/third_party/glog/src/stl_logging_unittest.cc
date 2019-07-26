




























#include "utilities.h"
#include "config.h"

#ifdef HAVE_USING_OPERATOR

#include "glog/stl_logging.h"

#include <iostream>
#include <map>
#include <string>
#include <strstream>
#include <vector>

#ifdef __GNUC__
# include <ext/hash_map>
# include <ext/hash_set>
#endif

#include "glog/logging.h"
#include "googletest.h"

using namespace std;
#ifdef __GNUC__
using namespace __gnu_cxx;
#endif

struct user_hash {
  size_t operator()(int x) const { return x; }
};

void TestSTLLogging() {
  {
    
    vector<int> v;
    v.push_back(10);
    v.push_back(20);
    v.push_back(30);
    char ss_buf[1000];
    ostrstream ss(ss_buf, sizeof(ss_buf));
    
    ss << v << ends;
    CHECK_STREQ(ss.str(), "10 20 30");
    vector<int> copied_v(v);
    CHECK_EQ(v, copied_v);  
  }

  {
    
    map< int, string > m;
    m[20] = "twenty";
    m[10] = "ten";
    m[30] = "thirty";
    char ss_buf[1000];
    ostrstream ss(ss_buf, sizeof(ss_buf));
    ss << m << ends;
    CHECK_STREQ(ss.str(), "(10, ten) (20, twenty) (30, thirty)");
    map< int, string > copied_m(m);
    CHECK_EQ(m, copied_m);  
  }

#ifdef __GNUC__
  {
    
    hash_set<int> hs;
    hs.insert(10);
    hs.insert(20);
    hs.insert(30);
    char ss_buf[1000];
    ostrstream ss(ss_buf, sizeof(ss_buf));
    ss << hs << ends;
    CHECK_STREQ(ss.str(), "10 20 30");
    hash_set<int> copied_hs(hs);
    CHECK_EQ(hs, copied_hs);  
  }
#endif

#ifdef __GNUC__
  {
    
    hash_map<int, string> hm;
    hm[10] = "ten";
    hm[20] = "twenty";
    hm[30] = "thirty";
    char ss_buf[1000];
    ostrstream ss(ss_buf, sizeof(ss_buf));
    ss << hm << ends;
    CHECK_STREQ(ss.str(), "(10, ten) (20, twenty) (30, thirty)");
    hash_map<int, string> copied_hm(hm);
    CHECK_EQ(hm, copied_hm);  
  }
#endif

  {
    
    vector<int> v;
    string expected;
    for (int i = 0; i < 100; i++) {
      v.push_back(i);
      if (i > 0) expected += ' ';
      char buf[256];
      sprintf(buf, "%d", i);
      expected += buf;
    }
    v.push_back(100);
    expected += " ...";
    char ss_buf[1000];
    ostrstream ss(ss_buf, sizeof(ss_buf));
    ss << v << ends;
    CHECK_STREQ(ss.str(), expected.c_str());
  }

  {
    
    
    map< int, string, greater<int> > m;
    m[20] = "twenty";
    m[10] = "ten";
    m[30] = "thirty";
    char ss_buf[1000];
    ostrstream ss(ss_buf, sizeof(ss_buf));
    ss << m << ends;
    CHECK_STREQ(ss.str(), "(30, thirty) (20, twenty) (10, ten)");
    map< int, string, greater<int> > copied_m(m);
    CHECK_EQ(m, copied_m);  
  }

#ifdef __GNUC__
  {
    
    
    hash_set<int, user_hash> hs;
    hs.insert(10);
    hs.insert(20);
    hs.insert(30);
    char ss_buf[1000];
    ostrstream ss(ss_buf, sizeof(ss_buf));
    ss << hs << ends;
    CHECK_STREQ(ss.str(), "10 20 30");
    hash_set<int, user_hash> copied_hs(hs);
    CHECK_EQ(hs, copied_hs);  
  }
#endif
}

int main(int argc, char** argv) {
  TestSTLLogging();
  std::cout << "PASS\n";
  return 0;
}

#else

#include <iostream>

int main(int argc, char** argv) {
  std::cout << "We don't support stl_logging for this compiler.\n"
            << "(we need compiler support of 'using ::operator<<' "
            << "for this feature.)\n";
  return 0;
}

#endif  
