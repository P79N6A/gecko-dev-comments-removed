







function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload(evt) {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(startTests, content);
  }, true);

  content.location = "data:text/html,<p>It's going to be legen....</p>";
}

function startTests(aInspector, aToolbox) {
  let p = content.document.querySelector("p");
  Task.spawn(function() {
    info("opening the inspector tool");
    let {inspector, toolbox} = yield openInspector();

    info("hovering over the <p> line in the markup-view");
    yield hoverContainer(p, inspector);
    ok(isHighlighterVisible(), "the highlighter is still visible");

    info("selecting the <p> line by clicking in the markup-view");
    yield clickContainer(p, inspector);

    p.textContent = "wait for it ....";
    info("wait and see if the highlighter stays visible even after the node was selected");
    yield waitForTheBrieflyShowBoxModelTimeout();

    p.textContent = "dary!!!!";
    ok(isHighlighterVisible(), "the highlighter is still visible");
  }).then(null, ok.bind(null, false)).then(endTests);
}

function endTests() {
  gBrowser.removeCurrentTab();
  finish();
}

function waitForTheBrieflyShowBoxModelTimeout() {
  let deferred = promise.defer();
  
  
  content.setTimeout(deferred.resolve, 1500);
  return deferred.promise;
}
