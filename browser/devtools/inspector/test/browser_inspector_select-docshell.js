



"use strict";




let test = asyncTest(function*() {
  const FrameURL = "data:text/html;charset=UTF-8," + encodeURI("<div id=\"frame\">frame</div>");
  const URL = "data:text/html;charset=UTF-8," + encodeURI("<iframe src=\"" + FrameURL + "\"></iframe><div id=\"top\">top</div>");

  Services.prefs.setBoolPref("devtools.command-button-frames.enabled", true);

  let {toolbox, inspector} = yield openInspectorForURL(URL);

  
  let testNode = content.document.querySelector("#top");
  ok(testNode, "We have the test node on the top level document");

  assertMarkupViewIsLoaded();

  
  let btn = toolbox.doc.getElementById("command-button-frames");
  ok(!btn.firstChild.getAttribute("hidden"), "The frame list button is visible");
  let frameBtns = Array.slice(btn.firstChild.querySelectorAll("[data-window-id]"));
  is(frameBtns.length, 2, "We have both frames in the list");
  frameBtns.sort(function (a, b) {
    return a.getAttribute("label").localeCompare(b.getAttribute("label"));
  });
  is(frameBtns[0].getAttribute("label"), FrameURL, "Got top level document in the list");
  is(frameBtns[1].getAttribute("label"), URL, "Got iframe document in the list");

  
  let willNavigate = toolbox.target.on("will-navigate", () => {
    info("Navigation to the iframe has started, the inspector should be empty");
    assertMarkupViewIsEmpty();
  });
  let newRoot = inspector.once("new-root", () => {
    info("Navigation to the iframe is done, the inspector should be back up");

    
    
    let testNode = getNode("#frame", { document: content.frames[0].document});
    ok(testNode, "We have the test node on the iframe");

    
    assertMarkupViewIsLoaded();

    inspector.once("inspector-updated", () => {
      deferred.resolve();
    });
    selectNode(testNode, inspector);
  });

  
  
  selectNode("#top", inspector);
  inspector.once("inspector-updated", () => {
    
    frameBtns[0].click();
  });

  yield willNavigate;
  yield newRoot;


  Services.prefs.clearUserPref("devtools.command-button-frames.enabled");

  function assertMarkupViewIsLoaded() {
    let markupViewBox = inspector.panelDoc.getElementById("markup-box");
    is(markupViewBox.childNodes.length, 1, "The markup-view is loaded");
  }

  function assertMarkupViewIsEmpty() {
    let markupViewBox = inspector.panelDoc.getElementById("markup-box");
    is(markupViewBox.childNodes.length, 0, "The markup-view is unloaded");
  }
});

