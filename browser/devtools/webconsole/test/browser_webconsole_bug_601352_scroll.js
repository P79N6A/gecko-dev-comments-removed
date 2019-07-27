






"use strict";

function test() {
  Task.spawn(runner).then(finishTest);

  function* runner() {
    let {tab} = yield loadTab("data:text/html;charset=utf-8,Web Console test " +
                              "for bug 601352");
    let hud = yield openConsole(tab);
    hud.jsterm.clearOutput();

    let longMessage = "";
    for (let i = 0; i < 50; i++) {
      longMessage += "LongNonwrappingMessage";
    }

    for (let i = 0; i < 50; i++) {
      content.console.log("test1 message " + i);
    }

    content.console.log(longMessage);

    for (let i = 0; i < 50; i++) {
      content.console.log("test2 message " + i);
    }

    yield waitForMessages({
      webconsole: hud,
      messages: [{
        text: "test1 message 0",
      }, {
        text: "test1 message 49",
      }, {
        text: "LongNonwrappingMessage",
      }, {
        text: "test2 message 0",
      }, {
        text: "test2 message 49",
      }],
    });

    let node = yield hud.jsterm.execute("1+1");

    let scrollNode = hud.outputNode.parentNode;
    let rectNode = node.getBoundingClientRect();
    let rectOutput = scrollNode.getBoundingClientRect();
    console.debug("rectNode", rectNode, "rectOutput", rectOutput);
    console.log("scrollNode scrollHeight", scrollNode.scrollHeight,
                "scrollTop", scrollNode.scrollTop, "clientHeight",
                scrollNode.clientHeight);

    isnot(scrollNode.scrollTop, 0, "scroll location is not at the top");

    
    

    
    let height = rectOutput.height;

    
    
    let top = rectNode.top - rectOutput.top;
    let bottom = top + rectNode.height;
    info("node top " + top + " node bottom " + bottom + " node clientHeight " +
         node.clientHeight);

    ok(top >= 0 && bottom <= height, "last message is visible");
  }
}
