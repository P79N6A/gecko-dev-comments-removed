


let tempScope = {};
Cu.import("resource:///modules/devtools/Target.jsm", tempScope);
let TargetFactory = tempScope.TargetFactory;

function test() {
  waitForExplicitFinish();

  let doc;
  let node;
  let view;
  let inspector;

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    doc = content.document;
    waitForFocus(setupTest, content);
  }, true);

  content.location = "http://mochi.test:8888/browser/browser/devtools/fontinspector/test/browser_fontinspector.html";

  function setupTest() {
    let target = TargetFactory.forTab(gBrowser.selectedTab);
    gDevTools.showToolbox(target, "inspector").then(function(toolbox) {
      openFontInspector(toolbox.getCurrentPanel());
    });
  }

  function openFontInspector(aInspector) {
    inspector = aInspector;

    info("Inspector open");

    inspector.selection.setNode(doc.body);
    inspector.sidebar.select("fontinspector");
    inspector.sidebar.once("fontinspector-ready", viewReady);
  }

  function viewReady() {
    info("Font Inspector ready");

    view = inspector.sidebar.getWindowForTab("fontinspector");

    ok(!!view.fontInspector, "Font inspector document is alive.");

    let d = view.document;

    let s = d.querySelectorAll("#all-fonts > section");
    is(s.length, 2, "Found 2 fonts");

    is(s[0].querySelector(".font-name").textContent,
       "DeLarge Bold", "font 0: Right font name");
    ok(s[0].classList.contains("is-remote"),
       "font 0: is remote");
    is(s[0].querySelector(".font-url").value,
       "http://mochi.test:8888/browser/browser/devtools/fontinspector/test/browser_font.woff",
       "font 0: right url");
    is(s[0].querySelector(".font-format").textContent,
       "woff", "font 0: right font format");
    is(s[0].querySelector(".font-css-name").textContent,
       "bar", "font 0: right css name");


    let font1Name = s[1].querySelector(".font-name").textContent;
    let font1CssName = s[1].querySelector(".font-css-name").textContent;

    
    

    ok((font1Name == "Arial") || (font1Name == "Liberation Sans"),
       "font 1: Right font name");
    ok(s[1].classList.contains("is-local"), "font 1: is local");
    ok((font1CssName == "Arial") || (font1CssName == "Liberation Sans"),
       "Arial", "font 1: right css name");

    executeSoon(function() {
      gDevTools.once("toolbox-destroyed", finishUp);
      inspector._toolbox.destroy();
    });
  }

  function finishUp() {
    gBrowser.removeCurrentTab();
    finish();
  }
}
