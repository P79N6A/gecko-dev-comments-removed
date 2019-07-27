




const { utils: Cu } = Components;

Cu.import("resource://gre/modules/ReaderMode.jsm");
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
    }
  },
  {
    url: URL_PREFIX + "not_an_article.html",
    expected: null
  },
  {
    url: URL_PREFIX + "developer.mozilla.org/en/XULRunner/Build_Instructions.html",
    expected: {
      title: "Building XULRunner",
      byline: null,
      excerpt: "XULRunner is built using basically the same process as Firefox or other applications. Please read and follow the general Build Documentation for instructions on how to get sources and set up build prerequisites.",
    }
  },
];

add_task(function* test_article_not_found() {
  let article = yield ReaderMode.getArticleFromCache(TEST_PAGES[0].url);
  do_check_eq(article, null);
});

add_task(function* test_store_article() {
  
  yield ReaderMode.storeArticleInCache({
    url: TEST_PAGES[0].url,
    content: "Lorem ipsum",
    title: TEST_PAGES[0].expected.title,
    byline: TEST_PAGES[0].expected.byline,
    excerpt: TEST_PAGES[0].expected.excerpt,
  });

  let article = yield ReaderMode.getArticleFromCache(TEST_PAGES[0].url);
  checkArticle(article, TEST_PAGES[0]);
});

add_task(function* test_remove_article() {
  yield ReaderMode.removeArticleFromCache(TEST_PAGES[0].url);
  let article = yield ReaderMode.getArticleFromCache(TEST_PAGES[0].url);
  do_check_eq(article, null);
});

add_task(function* test_parse_articles() {
  for (let testcase of TEST_PAGES) {
    let article = yield ReaderMode.downloadAndParseDocument(testcase.url);
    checkArticle(article, testcase);
  }
});

add_task(function* test_migrate_cache() {
  
  let cacheDB = yield new Promise((resolve, reject) => {
    let win = Services.wm.getMostRecentWindow("navigator:browser");
    let request = win.indexedDB.open("about:reader", 1);
    request.onerror = event => reject(request.error);

    
    request.onupgradeneeded = event => {
      let cacheDB = event.target.result;
      cacheDB.createObjectStore("articles", { keyPath: "url" });
    };

    request.onsuccess = event => resolve(event.target.result);
  });

  yield new Promise((resolve, reject) => {
    let transaction = cacheDB.transaction(["articles"], "readwrite");
    let store = transaction.objectStore("articles");

    let request = store.add({
      url: TEST_PAGES[0].url,
      content: "Lorem ipsum",
      title: TEST_PAGES[0].expected.title,
      byline: TEST_PAGES[0].expected.byline,
      excerpt: TEST_PAGES[0].expected.excerpt,
    });
    request.onerror = event => reject(request.error);
    request.onsuccess = event => resolve();
  });

  
  yield Reader.migrateCache();

  
  let article = yield ReaderMode.getArticleFromCache(TEST_PAGES[0].url);
  checkArticle(article, TEST_PAGES[0]);
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
}

run_next_test();
