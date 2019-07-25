






































let doc;
let div;
let plate;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource:///modules/domplate.jsm");

function createDocument()
{
  doc.body.innerHTML = '<div id="first">no</div>';
  doc.title = "Domplate Test";
  setupDomplateTests();
}

function setupDomplateTests()
{
  ok(domplate, "domplate is defined");
  plate = domplate({tag: domplate.DIV("Hello!")});
  ok(plate, "template is defined");
  div = doc.getElementById("first");
  ok(div, "we have our div");
  plate.tag.replace({}, div, template);
  is(div.innerText, "Hello!", "Is the div's innerText replaced?");
  finishUp();
}

function finishUp()
{
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

  content.location = "data:text/html,basic domplate tests";
}

