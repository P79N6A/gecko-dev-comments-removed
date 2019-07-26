
































#include "utilities.h"

#include <iostream>
#include <fstream>
#include <string>
#include "glog/logging.h"
#include "demangle.h"
#include "googletest.h"
#include "config.h"

GLOG_DEFINE_bool(demangle_filter, false,
                 "Run demangle_unittest in filter mode");

using namespace std;
using namespace GOOGLE_NAMESPACE;


static const char *DemangleIt(const char * const mangled) {
  static char demangled[4096];
  if (Demangle(mangled, demangled, sizeof(demangled))) {
    return demangled;
  } else {
    return mangled;
  }
}


TEST(Demangle, CornerCases) {
  char tmp[10];
  EXPECT_TRUE(Demangle("_Z6foobarv", tmp, sizeof(tmp)));
  
  EXPECT_STREQ("foobar()", tmp);
  EXPECT_TRUE(Demangle("_Z6foobarv", tmp, 9));
  EXPECT_STREQ("foobar()", tmp);
  EXPECT_FALSE(Demangle("_Z6foobarv", tmp, 8));  
  EXPECT_FALSE(Demangle("_Z6foobarv", tmp, 1));
  EXPECT_FALSE(Demangle("_Z6foobarv", tmp, 0));
  EXPECT_FALSE(Demangle("_Z6foobarv", NULL, 0));  
}





TEST(Demangle, Clones) {
  char tmp[20];
  EXPECT_TRUE(Demangle("_ZL3Foov", tmp, sizeof(tmp)));
  EXPECT_STREQ("Foo()", tmp);
  EXPECT_TRUE(Demangle("_ZL3Foov.clone.3", tmp, sizeof(tmp)));
  EXPECT_STREQ("Foo()", tmp);
  EXPECT_TRUE(Demangle("_ZL3Foov.constprop.80", tmp, sizeof(tmp)));
  EXPECT_STREQ("Foo()", tmp);
  EXPECT_TRUE(Demangle("_ZL3Foov.isra.18", tmp, sizeof(tmp)));
  EXPECT_STREQ("Foo()", tmp);
  EXPECT_TRUE(Demangle("_ZL3Foov.isra.2.constprop.18", tmp, sizeof(tmp)));
  EXPECT_STREQ("Foo()", tmp);
  
  EXPECT_FALSE(Demangle("_ZL3Foov.clo", tmp, sizeof(tmp)));
  
  EXPECT_FALSE(Demangle("_ZL3Foov.clone.", tmp, sizeof(tmp)));
  
  EXPECT_FALSE(Demangle("_ZL3Foov.clone.foo", tmp, sizeof(tmp)));
  
  EXPECT_FALSE(Demangle("_ZL3Foov.isra.2.constprop.", tmp, sizeof(tmp)));
}

TEST(Demangle, FromFile) {
  string test_file = FLAGS_test_srcdir + "/src/demangle_unittest.txt";
  ifstream f(test_file.c_str());  
  EXPECT_FALSE(f.fail());

  string line;
  while (getline(f, line)) {
    
    if (line.empty() || line[0] == '#') {
      continue;
    }
    
    
    string::size_type tab_pos = line.find('\t');
    EXPECT_NE(string::npos, tab_pos);
    string mangled = line.substr(0, tab_pos);
    string demangled = line.substr(tab_pos + 1);
    EXPECT_EQ(demangled, DemangleIt(mangled.c_str()));
  }
}

int main(int argc, char **argv) {
#ifdef HAVE_LIB_GFLAGS
  ParseCommandLineFlags(&argc, &argv, true);
#endif
  InitGoogleTest(&argc, argv);

  FLAGS_logtostderr = true;
  InitGoogleLogging(argv[0]);
  if (FLAGS_demangle_filter) {
    
    string line;
    while (getline(cin, line, '\n')) {
      cout << DemangleIt(line.c_str()) << endl;
    }
    return 0;
  } else if (argc > 1) {
    cout << DemangleIt(argv[1]) << endl;
    return 0;
  } else {
    return RUN_ALL_TESTS();
  }
}
