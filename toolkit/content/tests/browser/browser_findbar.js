XPCOMUtils.defineLazyModuleGetter(this, "Promise",
  "resource://gre/modules/Promise.jsm");
Components.utils.import("resource://gre/modules/Timer.jsm", this);

add_task(function* test_not_found() {
  info("Check correct 'Phrase not found' on new tab");

  let tab = yield promiseTestPageLoad();

  
  yield promiseFindFinished("--- THIS SHOULD NEVER MATCH ---", false);
  let findbar = gBrowser.getFindBar();
  is(findbar._findStatusDesc.textContent, findbar._notFoundStr,
     "Findbar status text should be 'Phrase not found'");

  gBrowser.removeTab(tab);
});

add_task(function* test_found() {
  let tab = yield promiseTestPageLoad();

  
  yield promiseFindFinished("S", true);
  ok(!gBrowser.getFindBar()._findStatusDesc.textContent,
     "Findbar status should be empty");

  gBrowser.removeTab(tab);
});



add_task(function* test_tabwise_case_sensitive() {
  let tab1 = yield promiseTestPageLoad();
  let findbar1 = gBrowser.getFindBar();

  let tab2 = yield promiseTestPageLoad();
  let findbar2 = gBrowser.getFindBar();

  
  findbar1.getElement("find-case-sensitive").click();

  gBrowser.selectedTab = tab1;

  
  yield promiseFindFinished("S", true);
  is(findbar1._findStatusDesc.textContent, findbar1._notFoundStr,
     "Findbar status text should be 'Phrase not found'");

  gBrowser.selectedTab = tab2;

  
  yield promiseFindFinished("S", true);
  ok(!findbar2._findStatusDesc.textContent, "Findbar status should be empty");

  gBrowser.removeTab(tab1);
  gBrowser.removeTab(tab2);
});

function promiseTestPageLoad() {
  let deferred = Promise.defer();

  let tab = gBrowser.selectedTab = gBrowser.addTab("data:text/html;charset=utf-8,The letter s.");
  let browser = gBrowser.selectedBrowser;
  browser.addEventListener("load", function listener() {
    if (browser.currentURI.spec == "about:blank")
      return;
    info("Page loaded: " + browser.currentURI.spec);
    browser.removeEventListener("load", listener, true);

    deferred.resolve(tab);
  }, true);

  return deferred.promise;
}

function promiseFindFinished(searchText, highlightOn) {
  let deferred = Promise.defer();

  let findbar = gBrowser.getFindBar();
  findbar.startFind(findbar.FIND_NORMAL);
  let highlightElement = findbar.getElement("highlight");
  if (highlightElement.checked != highlightOn)
    highlightElement.click();
  executeSoon(() => {
    findbar._findField.value = searchText;

    let resultListener;
    let findTimeout = setTimeout(() => foundOrTimedout(null), 2000);
    let foundOrTimedout = function(aData) {
      if (aData === null)
        info("Result listener not called, timeout reached.");
      clearTimeout(findTimeout);
      findbar.browser.finder.removeResultListener(resultListener);
      deferred.resolve();
    }

    resultListener = {
      onFindResult: foundOrTimedout
    };
    findbar.browser.finder.addResultListener(resultListener);
    findbar._find();
  });

  return deferred.promise;
}
