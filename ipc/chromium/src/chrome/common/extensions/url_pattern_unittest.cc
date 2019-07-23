



#include "chrome/common/extensions/url_pattern.h"
#include "testing/gtest/include/gtest/gtest.h"



TEST(URLPatternTest, ParseInvalid) {
  const char* kInvalidPatterns[] = {
    "http",  
    "http://",  
    "http://foo",  
    "http://*foo/bar",  
    "http://foo.*.bar/baz",  
    "http:/bar",  
    "foo://*",  
  };

  for (size_t i = 0; i < arraysize(kInvalidPatterns); ++i) {
    URLPattern pattern;
    EXPECT_FALSE(pattern.Parse(kInvalidPatterns[i]));
  }
};


TEST(URLPatternTest, Match1) {
  URLPattern pattern;
  EXPECT_TRUE(pattern.Parse("http://*/*"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesUrl(GURL("http://google.com")));
  EXPECT_TRUE(pattern.MatchesUrl(GURL("http://yahoo.com")));
  EXPECT_TRUE(pattern.MatchesUrl(GURL("http://google.com/foo")));
  EXPECT_FALSE(pattern.MatchesUrl(GURL("https://google.com")));
}


TEST(URLPatternTest, Match2) {
  URLPattern pattern;
  EXPECT_TRUE(pattern.Parse("https://*/foo*"));
  EXPECT_EQ("https", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_EQ("/foo*", pattern.path());
  EXPECT_TRUE(pattern.MatchesUrl(GURL("https://www.google.com/foo")));
  EXPECT_TRUE(pattern.MatchesUrl(GURL("https://www.google.com/foobar")));
  EXPECT_FALSE(pattern.MatchesUrl(GURL("http://www.google.com/foo")));
  EXPECT_FALSE(pattern.MatchesUrl(GURL("https://www.google.com/")));
}


TEST(URLPatternTest, Match3) {
  URLPattern pattern;
  EXPECT_TRUE(pattern.Parse("http://*.google.com/foo*bar"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("google.com", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_EQ("/foo*bar", pattern.path());
  EXPECT_TRUE(pattern.MatchesUrl(GURL("http://google.com/foobar")));
  EXPECT_TRUE(pattern.MatchesUrl(GURL("http://www.google.com/foo?bar")));
  EXPECT_TRUE(pattern.MatchesUrl(
      GURL("http://monkey.images.google.com/foooobar")));
  EXPECT_FALSE(pattern.MatchesUrl(GURL("http://yahoo.com/foobar")));
}


TEST(URLPatternTest, Match4) {
  URLPattern pattern;
  EXPECT_TRUE(pattern.Parse("chrome://thinger/*"));
  EXPECT_EQ("chrome", pattern.scheme());
  EXPECT_EQ("thinger", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesUrl(GURL("chrome://thinger/foobar")));
  EXPECT_TRUE(pattern.MatchesUrl(GURL("CHROME://thinger/")));
  EXPECT_FALSE(pattern.MatchesUrl(GURL("http://thinger/")));
}


TEST(URLPatternTest, Match5) {
  URLPattern pattern;
  EXPECT_TRUE(pattern.Parse("file:///foo?bar\\*baz"));
  EXPECT_EQ("file", pattern.scheme());
  EXPECT_EQ("", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_EQ("/foo?bar\\*baz", pattern.path());
  EXPECT_TRUE(pattern.MatchesUrl(GURL("file:///foo?bar\\hellobaz")));
  EXPECT_FALSE(pattern.MatchesUrl(GURL("file:///fooXbar\\hellobaz")));
}


TEST(URLPatternTest, Match6) {
  URLPattern pattern;
  EXPECT_TRUE(pattern.Parse("http://127.0.0.1/*"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("127.0.0.1", pattern.host());
  EXPECT_FALSE(pattern.match_subdomains());
  EXPECT_EQ("/*", pattern.path());
  EXPECT_TRUE(pattern.MatchesUrl(GURL("http://127.0.0.1")));
}


TEST(URLPatternTest, Match7) {
  URLPattern pattern;
  EXPECT_TRUE(pattern.Parse("http://*.0.0.1/*")); 
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("0.0.1", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_EQ("/*", pattern.path());
  
  EXPECT_FALSE(pattern.MatchesUrl(GURL("http://127.0.0.1")));
};


TEST(URLPatternTest, Match8) {
  URLPattern pattern;
  
  
  EXPECT_TRUE(pattern.Parse("http://*.xn--gkd/a%C2%81%E1*"));
  EXPECT_EQ("http", pattern.scheme());
  EXPECT_EQ("xn--gkd", pattern.host());
  EXPECT_TRUE(pattern.match_subdomains());
  EXPECT_EQ("/a%C2%81%E1*", pattern.path());
  EXPECT_TRUE(pattern.MatchesUrl(
      GURL("http://abc.\xe1\x80\xbf/a\xc2\x81\xe1xyz")));
  EXPECT_TRUE(pattern.MatchesUrl(
      GURL("http://\xe1\x80\xbf/a\xc2\x81\xe1\xe1")));
};
