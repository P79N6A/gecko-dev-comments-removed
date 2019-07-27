



"use strict";

SimpleTest.waitForExplicitFinish();
SimpleTest.requestFlakyTimeout("untriaged");
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.setSelectionChangeEnabledPref(true);
browserElementTestHelpers.addPermission();
const { Services } = SpecialPowers.Cu.import('resource://gre/modules/Services.jsm');

var gTextarea = null;
var mm;
var iframeOuter;
var iframeInner;
var state = 0;
var stateMeaning;
var defaultData;
var pasteData;
var focusScript;
var createEmbededFrame = false;

function copyToClipboard(str) {
  gTextarea.value = str;
  SpecialPowers.wrap(gTextarea).editor.selectAll();
  SpecialPowers.wrap(gTextarea).editor.copy();
}

function getScriptForGetContent() {
  var script = 'data:,\
    var elt = content.document.getElementById("text"); \
    var txt = ""; \
    if (elt) { \
      if (elt.tagName === "DIV" || elt.tagName === "BODY") { \
        txt = elt.textContent; \
      } else { \
        txt = elt.value; \
      } \
    } \
    sendAsyncMessage("content-text", txt);';
  return script;
}

function getScriptForSetFocus() {
  var script = 'data:,' + focusScript + 'sendAsyncMessage("content-focus")';
  return script;
}

function runTest() {
  iframeOuter = document.createElement('iframe');
  iframeOuter.setAttribute('mozbrowser', 'true');
  if (createEmbededFrame) {
    iframeOuter.src = "file_empty.html";
  }
  document.body.appendChild(iframeOuter);

  gTextarea = document.createElement('textarea');
  document.body.appendChild(gTextarea);

  iframeOuter.addEventListener("mozbrowserloadend", function onloadend(e) {
    iframeOuter.removeEventListener("mozbrowserloadend", onloadend);

    if (createEmbededFrame) {
      var contentWin = SpecialPowers.wrap(iframeOuter)
                             .QueryInterface(SpecialPowers.Ci.nsIFrameLoaderOwner)
                             .frameLoader.docShell.contentViewer.DOMDocument.defaultView;
      var contentDoc = contentWin.document;
      iframeInner = contentDoc.createElement('iframe');
      iframeInner.setAttribute('mozbrowser', true);
      iframeInner.setAttribute('remote', 'false');
      contentDoc.body.appendChild(iframeInner);
      iframeInner.addEventListener("mozbrowserloadend", function onloadendinner(e) {
        iframeInner.removeEventListener("mozbrowserloadend", onloadendinner);
        mm = SpecialPowers.getBrowserFrameMessageManager(iframeInner);
        dispatchTest(e);
      });
    } else {
      iframeInner = iframeOuter;
      mm = SpecialPowers.getBrowserFrameMessageManager(iframeInner);
      dispatchTest(e);
    }
  });
}

function doCommand(cmd) {
  Services.obs.notifyObservers({wrappedJSObject: SpecialPowers.unwrap(iframeInner)},
                               'copypaste-docommand', cmd);
}

function dispatchTest(e) {
  iframeInner.addEventListener("mozbrowserloadend", function onloadend2(e) {
    iframeInner.removeEventListener("mozbrowserloadend", onloadend2);
    iframeInner.focus();
    SimpleTest.executeSoon(function() { testSelectAll(e); });
  });

  switch (state) {
    case 0: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframeInner.src = "data:text/html,<html><body>" +
                   "<textarea id='text'>" + defaultData + "</textarea>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: textarea)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();elt.select();";
      break;
    case 1: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframeInner.src = "data:text/html,<html><body>" +
                   "<input type='text' id='text' value='" + defaultData + "'>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: <input type=text>)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();elt.select();";
      break;
    case 2: 
      defaultData = "12345";
      pasteData = "67890";
      iframeInner.src = "data:text/html,<html><body>" +
                   "<input type='number' id='text' value='" + defaultData + "'>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: <input type=number>)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();elt.select();";
      break;
    case 3: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframeInner.src = "data:text/html,<html><body>" +
                   "<div contenteditable='true' id='text'>" + defaultData + "</div>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: content editable div)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();";
      break;
    case 4: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframeInner.src = "data:text/html,<html><body>" +
                   "<div id='text'>" + defaultData + "</div>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: normal div)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();";
      break;
    case 5: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframeInner.src = "data:text/html,<html><body id='text'>" +
                   defaultData +
                   "</body>" +
                   "<script>document.designMode='on';</script>" +
                   "</html>";
      stateMeaning = " (test: normal div with designMode:on)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();";
      break;
    default:
      if (createEmbededFrame || browserElementTestHelpers.getOOPByDefaultPref()) {
        SimpleTest.finish();
      } else {
        createEmbededFrame = true;

        
        document.body.removeChild(iframeOuter);
        document.body.removeChild(gTextarea);
        state = 0;
        runTest();
      }
      break;
  }
}

function isChildProcess() {
  return SpecialPowers.Cc["@mozilla.org/xre/app-info;1"]
                         .getService(SpecialPowers.Ci.nsIXULRuntime)
                         .processType != SpecialPowers.Ci.nsIXULRuntime.PROCESS_TYPE_DEFAULT;
}

function testSelectAll(e) {
  
  if (!isChildProcess()) {
    iframeOuter.addEventListener("mozbrowserselectionstatechanged", function selectchangeforselectall(e) {
      if (e.detail.states.indexOf('selectall') == 0) {
        iframeOuter.removeEventListener("mozbrowserselectionstatechanged", selectchangeforselectall, true);
        ok(true, "got mozbrowserselectionstatechanged event." + stateMeaning);
        ok(e.detail, "event.detail is not null." + stateMeaning);
        ok(e.detail.width != 0, "event.detail.width is not zero" + stateMeaning);
        ok(e.detail.height != 0, "event.detail.height is not zero" + stateMeaning);
        ok(e.detail.states, "event.detail.state " + e.detail.states);
        SimpleTest.executeSoon(function() { testCopy1(e); });
      }
    }, true);
  }

  mm.addMessageListener('content-focus', function messageforfocus(msg) {
    mm.removeMessageListener('content-focus', messageforfocus);
    
    doCommand('selectall');
    if (isChildProcess()) {
      SimpleTest.executeSoon(function() { testCopy1(e); });
    }
  });

  mm.loadFrameScript(getScriptForSetFocus(), false);
}

function testCopy1(e) {
  
  
  copyToClipboard("");
  let setup = function() {
    doCommand("copy");
  };

  let nextTest = function(success) {
    ok(success, "copy command works" + stateMeaning);
    SimpleTest.executeSoon(function() { testPaste1(e); });
  };

  let success = function() {
    nextTest(true);
  }

  let fail = function() {
    nextTest(false);
  }

  let compareData = defaultData;
  SimpleTest.waitForClipboard(compareData, setup, success, fail);
}

function testPaste1(e) {
  
  
  copyToClipboard(pasteData);

  doCommand('selectall');
  doCommand("paste");
  SimpleTest.executeSoon(function() { testPaste2(e); });
}

function testPaste2(e) {
  mm.addMessageListener('content-text', function messageforpaste(msg) {
    mm.removeMessageListener('content-text', messageforpaste);
    if (state == 4) {
      
      ok(SpecialPowers.wrap(msg).json === defaultData, "paste command works" + stateMeaning);
    } else if (state == 3 && browserElementTestHelpers.getOOPByDefaultPref()) {
      
      todo(false, "paste command works" + stateMeaning);
    } else {
      ok(SpecialPowers.wrap(msg).json === pasteData, "paste command works" + stateMeaning);
    }
    SimpleTest.executeSoon(function() { testCut1(e); });
  });

  mm.loadFrameScript(getScriptForGetContent(), false);
}

function testCut1(e) {
  
  copyToClipboard("");
  let setup = function() {
    doCommand("selectall");
    doCommand("cut");
  };

  let nextTest = function(success) {
    if (state == 3 && browserElementTestHelpers.getOOPByDefaultPref()) {
      
      todo(false, "cut function works" + stateMeaning);
    } else {
      ok(success, "cut function works" + stateMeaning);
    }
    SimpleTest.executeSoon(function() { testCut2(e); });
  };

  let success = function() {
    nextTest(true);
  }

  let fail = function() {
    nextTest(false);
  }

  let compareData = pasteData;
  
  
  
  if ((state == 3 && browserElementTestHelpers.getOOPByDefaultPref()) ||
      state == 4) {
    compareData = function() { return true; }
  }

  SimpleTest.waitForClipboard(compareData, setup, success, fail);
}

function testCut2(e) {
  mm.addMessageListener('content-text', function messageforcut(msg) {
    mm.removeMessageListener('content-text', messageforcut);
    
    if (state == 4) {
      ok(SpecialPowers.wrap(msg).json !== "", "cut command works" + stateMeaning);
    } else if (state == 3 && browserElementTestHelpers.getOOPByDefaultPref()) {
      
      todo(false, "cut command works" + stateMeaning);
    } else {
      ok(SpecialPowers.wrap(msg).json === "", "cut command works" + stateMeaning);
    }

    state++;
    dispatchTest(e);
  });

  mm.loadFrameScript(getScriptForGetContent(), false);
}


var principal = SpecialPowers.wrap(document).nodePrincipal;
var context = { 'url': SpecialPowers.wrap(principal.URI).spec,
                'appId': principal.appId,
                'isInBrowserElement': true };

addEventListener('testready', function() {
  SpecialPowers.pushPermissions([
    {'type': 'browser', 'allow': 1, 'context': context}
  ], runTest);
});

