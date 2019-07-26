





let doc;
let h1;
let div;

function createDocument()
{
  let div = doc.createElement("div");
  let h1 = doc.createElement("h1");
  let p1 = doc.createElement("p");
  let p2 = doc.createElement("p");
  let div2 = doc.createElement("div");
  let p3 = doc.createElement("p");
  doc.title = "Inspector Highlighter Meatballs";
  h1.textContent = "Inspector Tree Selection Test";
  p1.textContent = "This is some example text";
  p2.textContent = "Lorem ipsum dolor sit amet, consectetur adipisicing " +
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna " +
    "aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco " +
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure " +
    "dolor in reprehenderit in voluptate velit esse cillum dolore eu " +
    "fugiat nulla pariatur. Excepteur sint occaecat cupidatat non " +
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  p3.textContent = "Lorem ipsum dolor sit amet, consectetur adipisicing " +
    "elit, sed do eiusmod tempor incididunt ut labore et dolore magna " +
    "aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco " +
    "laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure " +
    "dolor in reprehenderit in voluptate velit esse cillum dolore eu " +
    "fugiat nulla pariatur. Excepteur sint occaecat cupidatat non " +
    "proident, sunt in culpa qui officia deserunt mollit anim id est laborum.";
  let div3 = doc.createElement("div");
  div3.id = "checkOutThisWickedSpread";
  div3.setAttribute("style", "position: absolute; top: 20px; right: 20px; height: 20px; width: 20px; background-color: yellow; border: 1px dashed black;");
  let p4 = doc.createElement("p");
  p4.setAttribute("style", "font-weight: 200; font-size: 8px; text-align: center;");
  p4.textContent = "Smörgåsbord!";
  div.appendChild(h1);
  div.appendChild(p1);
  div.appendChild(p2);
  div2.appendChild(p3);
  div3.appendChild(p4);
  doc.body.appendChild(div);
  doc.body.appendChild(div2);
  doc.body.appendChild(div3);

  openInspector(setupHighlighterTests);
}

function setupHighlighterTests()
{
  h1 = doc.querySelector("h1");
  ok(h1, "we have the header");

  let i = getActiveInspector();
  i.selection.setNode(div);
  i.highlighter.unlockAndFocus();
  i.highlighter.outline.setAttribute("disable-transitions", "true");

  executeSoon(function() {
    i.selection.once("new-node", performToggleComparisons);
    EventUtils.synthesizeMouse(h1, 2, 2, {type: "mousemove"}, content);
  });
}

function performToggleComparisons(evt)
{
  let i = getActiveInspector();

  i.highlighter.toggleLockState();
  ok(i.highlighter.locked, "highlighter locks");
  is(i.selection.node, div);
  i.highlighter.toggleLockState();
  ok(!i.highlighter.locked, "highlighter unlocks");

  i.highlighter.toggleLockState();
  ok(i.highlighter.locked, "highlighter locks if selection is unchanged");
  i.highlighter.toggleLockState();

  executeSoon(function() {
    i.selection.once("new-node", performTestComparisons);
    EventUtils.synthesizeMouse(h1, 2, 2, {type: "mousemove"}, content);
  });
}

function performTestComparisons(evt)
{
  let i = getActiveInspector();
  i.highlighter.lock();
  ok(isHighlighting(), "highlighter is highlighting");
  is(getHighlitNode(), h1, "highlighter matches selection")
  is(i.selection.node, h1, "selection matches node");
  is(i.selection.node, getHighlitNode(), "selection matches highlighter");


  div = doc.querySelector("div#checkOutThisWickedSpread");

  executeSoon(function() {
    i.selection.once("new-node", finishTestComparisons);
    i.selection.setNode(div);
  });
}

function finishTestComparisons()
{
  let i = getActiveInspector();

  
  let divDims = div.getBoundingClientRect();
  let divWidth = divDims.width;
  let divHeight = divDims.height;

  
  let outlineDims = i.highlighter.outline.getBoundingClientRect();
  let outlineWidth = outlineDims.width;
  let outlineHeight = outlineDims.height;

  
  
  

  
  let contentViewer = gBrowser.selectedBrowser.docShell.contentViewer
                             .QueryInterface(Ci.nsIMarkupDocumentViewer);
  contentViewer.fullZoom = 2;

  
  

  window.setTimeout(function() {
    
    let zoom = i.highlighter.zoom;
    is(zoom, 2, "zoom is 2?");

    
    let divDims = div.getBoundingClientRect();
    let divWidth = divDims.width * zoom;
    let divHeight = divDims.height * zoom;

    
    let outlineDims = i.highlighter.outline.getBoundingClientRect();
    let outlineWidth = outlineDims.width;
    let outlineHeight = outlineDims.height;

    
    
    

    doc = h1 = div = null;
    executeSoon(finishUp);
  }, 500);
}

function finishUp() {
  let target = TargetFactory.forTab(gBrowser.selectedTab);
  gDevTools.closeToolbox(target);
  gBrowser.removeCurrentTab();
  finish();
}

function test()
{
  waitForExplicitFinish();
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
    doc = content.document;
    waitForFocus(createDocument, content);
  }, true);

  content.location = "data:text/html,basic tests for inspector";
}

