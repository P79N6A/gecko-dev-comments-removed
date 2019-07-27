







"use strict";

const TEST_URI = "http://example.com/browser/browser/devtools/webconsole/" +
                 "test/test-bug-952277-highlight-nodes-in-vview.html";

let gWebConsole, gJSTerm, gVariablesView, gToolbox;

function test() {
  loadTab(TEST_URI).then(() => {
    openConsole().then(hud => {
      consoleOpened(hud);
    });
  });
}

function consoleOpened(hud) {
  gWebConsole = hud;
  gJSTerm = hud.jsterm;
  gToolbox = gDevTools.getToolbox(hud.target);
  gJSTerm.execute("document.querySelectorAll('p')").then(onQSAexecuted);
}

function onQSAexecuted(msg) {
  ok(msg, "output message found");
  let anchor = msg.querySelector("a");
  ok(anchor, "object link found");

  gJSTerm.once("variablesview-fetched", onNodeListVviewFetched);

  executeSoon(() =>
    EventUtils.synthesizeMouse(anchor, 2, 2, {}, gWebConsole.iframeWindow)
  );
}

function onNodeListVviewFetched(aEvent, aVar) {
  gVariablesView = aVar._variablesView;
  ok(gVariablesView, "variables view object");

  
  let props = [...aVar].map(([id, prop]) => [id, prop]);

  
  props = props.filter(v => v[0].match(/[0-9]+/));

  function hoverOverDomNodeVariableAndAssertHighlighter(index) {
    if (props[index]) {
      let prop = props[index][1];

      gToolbox.once("node-highlight", () => {
        ok(true, "The highlighter was shown on hover of the DOMNode");
        gToolbox.highlighterUtils.unhighlight().then(() => {
          clickOnDomNodeVariableAndAssertInspectorSelected(index);
        });
      });

      
      
      prop.highlightDomNode();
    } else {
      finishUp();
    }
  }

  function clickOnDomNodeVariableAndAssertInspectorSelected(index) {
    let prop = props[index][1];

    
    gToolbox.initInspector().then(() => {
      
      
      
      prop.openNodeInInspector().then(() => {
        is(gToolbox.currentToolId, "inspector",
           "The toolbox switched over the inspector on DOMNode click");
        gToolbox.selectTool("webconsole").then(() => {
          hoverOverDomNodeVariableAndAssertHighlighter(index + 1);
        });
      });
    });
  }

  hoverOverDomNodeVariableAndAssertHighlighter(0);
}

function finishUp() {
  gWebConsole = gJSTerm = gVariablesView = gToolbox = null;

  finishTest();
}
