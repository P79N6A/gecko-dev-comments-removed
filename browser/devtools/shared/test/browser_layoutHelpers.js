



let {LayoutHelpers} = Cu.import("resource://gre/modules/devtools/LayoutHelpers.jsm", {});


const TEST_URI = TEST_URI_ROOT + "browser_layoutHelpers.html";

add_task(function*() {
  let browser = yield promiseTab(TEST_URI);
  let doc = browser.contentDocument;
  yield runTest(doc.defaultView);
  gBrowser.removeCurrentTab();
});

function runTest(win) {
  let lh = new LayoutHelpers(win);
  let some = win.document.getElementById('some');

  some.style.top = win.innerHeight + 'px';
  some.style.left = win.innerWidth + 'px';
  
  

  let xPos = Math.floor(win.innerWidth / 2);
  win.scroll(xPos, win.innerHeight + 2);  
  lh.scrollIntoViewIfNeeded(some);
  is(win.scrollY, Math.floor(win.innerHeight / 2) + 1,
     'Element completely hidden above should appear centered.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');

  win.scroll(win.innerWidth / 2, win.innerHeight + 1);  
  lh.scrollIntoViewIfNeeded(some);
  is(win.scrollY, win.innerHeight,
     'Element partially visible above should appear above.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');

  win.scroll(win.innerWidth / 2, 0);  
  lh.scrollIntoViewIfNeeded(some);
  is(win.scrollY, Math.floor(win.innerHeight / 2) + 1,
     'Element completely hidden below should appear centered.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');

  win.scroll(win.innerWidth / 2, 1);  
  lh.scrollIntoViewIfNeeded(some);
  is(win.scrollY, 2,
     'Element partially visible below should appear below.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');

  win.scroll(win.innerWidth / 2, win.innerHeight + 2);  
  lh.scrollIntoViewIfNeeded(some, false);
  is(win.scrollY, win.innerHeight,
     'Element completely hidden above should appear above ' +
     'if parameter is false.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');

  win.scroll(win.innerWidth / 2, win.innerHeight + 1);  
  lh.scrollIntoViewIfNeeded(some, false);
  is(win.scrollY, win.innerHeight,
     'Element partially visible above should appear above ' +
     'if parameter is false.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');

  win.scroll(win.innerWidth / 2, 0);  
  lh.scrollIntoViewIfNeeded(some, false);
  is(win.scrollY, 2,
     'Element completely hidden below should appear below ' +
     'if parameter is false.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');

  win.scroll(win.innerWidth / 2, 1);  
  lh.scrollIntoViewIfNeeded(some, false);
  is(win.scrollY, 2,
     'Element partially visible below should appear below ' +
     'if parameter is false.');
  is(win.scrollX, xPos,
     'scrollX position has not changed.');
}
