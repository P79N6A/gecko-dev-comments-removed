






































const TEST_URI = "http://example.com/browser/browser/devtools/styleinspector/test/browser/browser_styleinspector_webconsole.htm";

let doc;
let jsterm;
let hudBox;
let stylePanels = [];

function test() {
  addTab(TEST_URI);
  browser.addEventListener("DOMContentLoaded", prepConsole, false);
}

function prepConsole() {
  browser.removeEventListener("DOMContentLoaded", prepConsole, false);
  doc = content.document;
  openConsole();
 
  ok(window.StyleInspector, "StyleInspector exists");
 
  let hud = HUDService.getHudByWindow(content);
  ok(hud, "we have a console");
 
  hudBox = hud.HUDBox;
  ok(hudBox, "we have the console display");
 
  jsterm = hud.jsterm;
  ok(jsterm, "we have a jsterm");
 
  openStyleInspector1();
}

function openStyleInspector1() {
  info("opening style inspector instance 1");
  Services.obs.addObserver(openStyleInspector2, "StyleInspector-opened", false);
  jsterm.execute("inspectstyle($('text'))");
}

function openStyleInspector2() {
  Services.obs.removeObserver(openStyleInspector2, "StyleInspector-opened", false);
  info("opening style inspector instance 2");
  Services.obs.addObserver(openStyleInspector3, "StyleInspector-opened", false);
  jsterm.execute("inspectstyle($('text2'))");
}

function openStyleInspector3() {
  Services.obs.removeObserver(openStyleInspector3, "StyleInspector-opened", false);
  info("opening style inspector instance 3");
  Services.obs.addObserver(teststylePanels, "StyleInspector-opened", false);
  jsterm.execute("inspectstyle($('container'))");
}

function teststylePanels() {
  Services.obs.removeObserver(teststylePanels, "StyleInspector-opened", false);

  info("adding style inspector instances to stylePanels array");
  let popupSet = document.getElementById("mainPopupSet");
  let len = popupSet.childNodes.length - 3;
  stylePanels.push(popupSet.childNodes[len++]);
  stylePanels.push(popupSet.childNodes[len++]);
  stylePanels.push(popupSet.childNodes[len++]);

  let eltArray = [
    doc.getElementById("text"),
    doc.getElementById("text2"),
    doc.getElementById("container")
  ];

  
  
  
  
  
  
  
  info("looping through array to check initialization");
  for (let i = 0, max = stylePanels.length; i < max; i++) {
    ok(stylePanels[i], "style inspector instance " + i +
       " correctly initialized");
    is(stylePanels[i].state, "open", "style inspector " + i + " is open");




































  }

  info("hiding stylePanels[1]");
  Services.obs.addObserver(styleInspectorClosedByHide,
                           "StyleInspector-closed", false);
  stylePanels[1].hidePopup();
}

function styleInspectorClosedByHide()
{
  Services.obs.removeObserver(styleInspectorClosedByHide, "StyleInspector-closed", false);
  is(stylePanels[0].state, "open", "instance stylePanels[0] is still open");
  is(stylePanels[1].state, "closed", "instance stylePanels[1] is hidden");
  is(stylePanels[2].state, "open", "instance stylePanels[2] is still open");

  info("closing web console");
  Services.obs.addObserver(styleInspectorClosedFromConsole1,
                           "StyleInspector-closed", false);
  closeConsole();
}

function styleInspectorClosedFromConsole1()
{
  Services.obs.removeObserver(styleInspectorClosedFromConsole1,
                              "StyleInspector-closed", false);
  info("Style Inspector 1 and 2 closed");
  executeSoon(cleanUp);
}

function cleanUp()
{
  let panels = document.querySelector("panel[hudToolId]");
  ok(!panels,
     "all style inspector panels are now detached and ready for garbage collection");

  info("cleaning up");

  doc = hudBox = stylePanels = jsterm = null;
  finishTest();
}
