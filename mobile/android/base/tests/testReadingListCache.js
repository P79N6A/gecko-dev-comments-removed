




const { utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Task.jsm");

let Reader = Services.wm.getMostRecentWindow("navigator:browser").Reader;

const URL_PREFIX = "http://mochi.test:8888/tests/robocop/reader_mode_pages/";

let TEST_PAGES = [
  {
    url: URL_PREFIX + "basic_article.html",
    expected: {
      title: "Article title",
      byline: "by Jane Doe",
      excerpt: "This is the article description.",
      length: 1931
    }
  },
  {
    url: URL_PREFIX + "addons.mozilla.org/en-US/firefox/index.html",
    expected: null
  },
  {
    url: URL_PREFIX + "developer.mozilla.org/en/XULRunner/Build_Instructions.html",
    expected: {
      title: "Building XULRunner | MDN",
      byline: null,
      excerpt: "XULRunner is built using basically the same process as Firefox or other applications. Please read and follow the general Build Documentation for instructions on how to get sources and set up build prerequisites.",
      length: 2300
    }
  },
];

add_task(function* test_article_not_found() {
  let uri = Services.io.newURI(TEST_PAGES[0].url, null, null);
  let article = yield Reader.getArticleFromCache(uri);
  do_check_eq(article, null);
});

add_task(function* test_store_article() {
  
  yield Reader.storeArticleInCache({
    url: TEST_PAGES[0].url,
    content: "Lorem ipsum",
    title: TEST_PAGES[0].expected.title,
    byline: TEST_PAGES[0].expected.byline,
    excerpt: TEST_PAGES[0].expected.excerpt,
    length: TEST_PAGES[0].expected.length
  });

  let uri = Services.io.newURI(TEST_PAGES[0].url, null, null);
  let article = yield Reader.getArticleFromCache(uri);
  checkArticle(article, TEST_PAGES[0]);
});

add_task(function* test_remove_article() {
  let uri = Services.io.newURI(TEST_PAGES[0].url, null, null);
  yield Reader.removeArticleFromCache(uri);
  let article = yield Reader.getArticleFromCache(uri);
  do_check_eq(article, null);
});

add_task(function* test_parse_articles() {
  for (let testcase of TEST_PAGES) {
    let article = yield Reader._downloadAndParseDocument(testcase.url);
    checkArticle(article, testcase);
  }
});

function checkArticle(article, testcase) {
  if (testcase.expected == null) {
    do_check_eq(article, null);
    return;
  }

  do_check_neq(article, null);
  do_check_eq(!!article.content, true); 
  do_check_eq(article.url, testcase.url);
  do_check_eq(article.title, testcase.expected.title);
  do_check_eq(article.byline, testcase.expected.byline);
  do_check_eq(article.excerpt, testcase.expected.excerpt);
  do_check_eq(article.length, testcase.expected.length);
}

run_next_test();
