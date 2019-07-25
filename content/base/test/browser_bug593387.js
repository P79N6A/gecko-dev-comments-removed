






var newBrowser;

function test() {
  waitForExplicitFinish();

  var newTab = gBrowser.addTab();
  gBrowser.selectedTab = newTab;
  newBrowser = gBrowser.getBrowserForTab(newTab);
  

  newBrowser.addEventListener("load", testXFOFrameInChrome, true);
  newBrowser.contentWindow.location = "chrome://global/content/mozilla.xhtml";
}

function testXFOFrameInChrome() {
  newBrowser.removeEventListener("load", testXFOFrameInChrome, true);

  
  
  var frame = newBrowser.contentDocument.createElement("iframe");
  frame.src = "http://mochi.test:8888/tests/content/base/test/file_x-frame-options_page.sjs?testid=deny&xfo=deny";
  frame.addEventListener("load", function() {
    frame.removeEventListener("load", arguments.callee, true);

    
    var test = this.contentDocument.getElementById("test");
    is(test.tagName, "H1", "wrong element type");
    is(test.textContent, "deny", "wrong textContent");
    
    
    newBrowser.addEventListener("load", testXFOFrameInContent, true);
    newBrowser.contentWindow.location = "http://example.com/";  
  }, true);

  newBrowser.contentDocument.body.appendChild(frame);
}

function testXFOFrameInContent() {
  newBrowser.removeEventListener("load", testXFOFrameInContent, true);

  
  
  var frame = newBrowser.contentDocument.createElement("iframe");
  frame.src = "http://mochi.test:8888/tests/content/base/test/file_x-frame-options_page.sjs?testid=deny&xfo=deny";
  frame.addEventListener("load", function() {
    frame.removeEventListener("load", arguments.callee, true);

    
    var test = this.contentDocument.getElementById("test");
    is(test, undefined, "should be about:blank");

    
    gBrowser.removeCurrentTab();
    finish();
  }, true);

  newBrowser.contentDocument.body.appendChild(frame);
}
