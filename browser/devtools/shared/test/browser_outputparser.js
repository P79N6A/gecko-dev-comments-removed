


let {Services} = Cu.import("resource://gre/modules/Services.jsm", {});
let {Loader} = Cu.import("resource://gre/modules/commonjs/toolkit/loader.js", {});
let {OutputParser} = devtools.require("devtools/output-parser");

let parser;
let doc;

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onload() {
    gBrowser.selectedBrowser.removeEventListener("load", onload, true);
    waitForFocus(init, content);
    doc = content.document;
  }, true);

  content.location = "data:text/html,<h1>browser_outputParser.js</h1>" +
                     "<div></div>";
}

function init() {
  parser = new OutputParser();
  testParseCssProperty();
}

function testParseCssProperty() {
  let frag = parser.parseCssProperty("border", "1px solid red", {
    colorSwatchClass: "test-colorswatch"
  });

  let target = doc.querySelector("div");
  ok(target, "captain, we have the div");
  target.appendChild(frag);

  is(target.innerHTML,
     '1px solid <span style="background-color:red" class="test-colorswatch"></span>#F00',
     "CSS property correctly parsed");

  target.innerHTML = "";

  testParseHTMLAttribute();
}

function testParseHTMLAttribute() {
  let attrib = "color:red; font-size: 12px; background-image: " +
               "url(chrome://branding/content/about-logo.png)";
  let frag = parser.parseHTMLAttribute(attrib, {
    colorSwatchClass: "test-colorswatch",
    urlClass: "theme-link"
  });

  let target = doc.querySelector("div");
  ok(target, "captain, we have the div");
  target.appendChild(frag);

  let expected = 'color:<span style="background-color:red" ' +
                 'class="test-colorswatch"></span>#F00; font-size: 12px; ' +
                 'background-image: url(\'<a href="chrome://branding/content/about-logo.png" ' +
                 'class="theme-link" ' +
                 'target="_blank">chrome://branding/content/about-logo.png</a>\')';

  is(target.innerHTML, expected, "HTML Attribute correctly parsed");
  finishUp();
}

function finishUp() {
  Services = Loader = OutputParser = parser = doc = null;
  gBrowser.removeCurrentTab();
  finish();
}
