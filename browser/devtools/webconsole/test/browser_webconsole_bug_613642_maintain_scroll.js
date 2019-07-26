








let hud, testDriver;

function testNext() {
  testDriver.next();
}

function testGen() {
  hud.jsterm.clearOutput();
  let outputNode = hud.outputNode;
  let scrollBox = outputNode.parentNode;

  for (let i = 0; i < 150; i++) {
    content.console.log("test message " + i);
  }

  waitForMessages({
    webconsole: hud,
    messages: [{
      text: "test message 149",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(testNext);

  yield undefined;

  ok(scrollBox.scrollTop > 0, "scroll location is not at the top");

  
  outputNode.focus();

  scrollBox.onscroll = () => {
    info("onscroll top " + scrollBox.scrollTop);
    if (scrollBox.scrollTop != 0) {
      
      return;
    }
    scrollBox.onscroll = null;
    is(scrollBox.scrollTop, 0, "scroll location updated (moved to top)");
    testNext();
  };
  EventUtils.synthesizeKey("VK_HOME", {}, hud.iframeWindow);

  yield undefined;

  
  content.console.log("test message 150");

  waitForMessages({
    webconsole: hud,
    messages: [{
      text: "test message 150",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(testNext);

  yield undefined;

  scrollBox.onscroll = () => {
    if (scrollBox.scrollTop != 0) {
      
      return;
    }
    scrollBox.onscroll = null;
    is(scrollBox.scrollTop, 0, "scroll location is still at the top");
    testNext();
  };

  
  
  executeSoon(scrollBox.onscroll);

  yield undefined;

  
  outputNode.lastChild.focus();

  scrollBox.onscroll = () => {
    if (scrollBox.scrollTop == 0) {
      
      return;
    }
    scrollBox.onscroll = null;
    isnot(scrollBox.scrollTop, 0, "scroll location updated (moved to bottom)");
    testNext();
  };
  EventUtils.synthesizeKey("VK_END", {});
  yield;

  let oldScrollTop = scrollBox.scrollTop;

  content.console.log("test message 151");

  waitForMessages({
    webconsole: hud,
    messages: [{
      text: "test message 151",
      category: CATEGORY_WEBDEV,
      severity: SEVERITY_LOG,
    }],
  }).then(() => {
    scrollBox.onscroll = () => {
      if (scrollBox.scrollTop == oldScrollTop) {
        
        return;
      }
      scrollBox.onscroll = null;
      isnot(scrollBox.scrollTop, oldScrollTop, "scroll location updated (moved to bottom again)");
      testNext();
    };
  });

  yield undefined;

  hud = testDriver = null;
  finishTest();
  
  yield undefined;
}

function test() {
  addTab("data:text/html;charset=utf-8,Web Console test for bug 613642: remember scroll location");
  browser.addEventListener("load", function tabLoad(aEvent) {
    browser.removeEventListener(aEvent.type, tabLoad, true);
    openConsole(null, function(aHud) {
      hud = aHud;
      testDriver = testGen();
      testDriver.next();
    });
  }, true);
}
