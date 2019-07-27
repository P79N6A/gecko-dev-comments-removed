


"use strict";

const URL = ROOT + "browser_frametree_sample.html";
const URL_FRAMESET = ROOT + "browser_frametree_sample_frameset.html";








add_task(function test_frametree() {
  const FRAME_TREE_SINGLE = { href: URL };
  const FRAME_TREE_FRAMESET = {
    href: URL_FRAMESET,
    children: [{href: URL}, {href: URL}, {href: URL}]
  };

  
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseNewFrameTree(browser);
  yield checkFrameTree(browser, FRAME_TREE_SINGLE,
    "loading a page resets and creates the frame tree correctly");

  
  
  yield sendMessage(browser, "ss-test:createDynamicFrames", {id: "frames", url: URL});
  browser.loadURI(URL_FRAMESET);
  yield promiseNewFrameTree(browser);
  yield checkFrameTree(browser, FRAME_TREE_FRAMESET,
    "dynamic frames created on or after the load event are ignored");

  
  
  
  browser.goBack();
  yield promiseNewFrameTree(browser);
  yield checkFrameTree(browser, FRAME_TREE_SINGLE,
    "loading from bfache resets and creates the frame tree correctly");

  
  
  browser.loadURI(URL_FRAMESET);
  executeSoon(() => browser.stop());
  yield promiseNewFrameTree(browser);

  
  yield sendMessage(browser, "ss-test:createDynamicFrames", {id: "frames", url: URL});
  browser.loadURI(URL_FRAMESET);
  yield promiseNewFrameTree(browser);
  yield checkFrameTree(browser, FRAME_TREE_FRAMESET,
    "reloading a page resets and creates the frame tree correctly");

  
  gBrowser.removeTab(tab);
});






add_task(function test_frametree_dynamic() {
  
  
  const FRAME_TREE = {
    href: URL_FRAMESET,
    children: [{href: URL}, {href: URL}, {href: URL}]
  };
  const FRAME_TREE_REMOVED = {
    href: URL_FRAMESET,
    children: [{href: URL}, {href: URL}]
  };

  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield sendMessage(browser, "ss-test:createDynamicFrames", {id: "frames", url: URL});
  browser.loadURI(URL_FRAMESET);
  yield promiseNewFrameTree(browser);

  
  
  
  yield checkFrameTree(browser, FRAME_TREE,
    "frame tree contains first four frames");

  
  yield sendMessage(browser, "ss-test:removeLastFrame", {id: "frames"});
  
  yield checkFrameTree(browser, FRAME_TREE,
    "frame tree contains first four frames");

  
  yield sendMessage(browser, "ss-test:removeLastFrame", {id: "frames"});
  
  yield checkFrameTree(browser, FRAME_TREE_REMOVED,
    "frame tree contains first three frames");

  
  gBrowser.removeTab(tab);
});





function checkFrameTree(browser, expected, msg) {
  return sendMessage(browser, "ss-test:mapFrameTree").then(tree => {
    is(JSON.stringify(tree), JSON.stringify(expected), msg);
  });
}






function promiseNewFrameTree(browser) {
  let reset = promiseContentMessage(browser, "ss-test:onFrameTreeCollected");
  let collect = promiseContentMessage(browser, "ss-test:onFrameTreeCollected");
  return Promise.all([reset, collect]);
}
