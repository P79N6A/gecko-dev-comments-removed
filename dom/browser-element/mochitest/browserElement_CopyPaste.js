



"use strict";

SimpleTest.waitForExplicitFinish();
browserElementTestHelpers.setEnabledPref(true);
browserElementTestHelpers.setSelectionChangeEnabledPref(true);
browserElementTestHelpers.addPermission();
const { Services } = SpecialPowers.Cu.import('resource://gre/modules/Services.jsm');
var gTextarea = null;
var mm;
var iframe;
var state = 0;
var stateMeaning;
var defaultData;
var pasteData;
var focusScript;

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
  iframe = document.createElement('iframe');
  SpecialPowers.wrap(iframe).mozbrowser = true;
  document.body.appendChild(iframe);

  gTextarea = document.createElement('textarea');
  document.body.appendChild(gTextarea);

  mm = SpecialPowers.getBrowserFrameMessageManager(iframe);

  iframe.addEventListener("mozbrowserloadend", function onloadend(e) {
    iframe.removeEventListener("mozbrowserloadend", onloadend);
    dispatchTest(e);
  });
}

function doCommand(cmd) {
  Services.obs.notifyObservers({wrappedJSObject: iframe},
                               'copypaste-docommand', cmd);
}

function dispatchTest(e) {
  iframe.addEventListener("mozbrowserloadend", function onloadend2(e) {
    iframe.removeEventListener("mozbrowserloadend", onloadend2);
    SimpleTest.executeSoon(function() { testSelectAll(e); });
  });

  switch (state) {
    case 0: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframe.src = "data:text/html,<html><body>" +
                   "<textarea id='text'>" + defaultData + "</textarea>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: textarea)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();elt.select();";
      break;
    case 1: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframe.src = "data:text/html,<html><body>" +
                   "<input type='text' id='text' value='" + defaultData + "'>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: <input type=text>)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();elt.select();";
      break;
    case 2: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframe.src = "data:text/html,<html><body>" +
                   "<input type='password' id='text' value='" + defaultData + "'>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: <input type=password>)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();elt.select();";
      break;
    case 3: 
      defaultData = "12345";
      pasteData = "67890";
      iframe.src = "data:text/html,<html><body>" +
                   "<input type='number' id='text' value='" + defaultData + "'>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: <input type=number>)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();elt.select();";
      break;
    case 4: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframe.src = "data:text/html,<html><body>" +
                   "<div contenteditable='true' id='text'>" + defaultData + "</div>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: content editable div)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();";
      break;
    case 5: 
      SimpleTest.finish();
      return;
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframe.src = "data:text/html,<html><body>" +
                   "<div id='text'>" + defaultData + "</div>" +
                   "</body>" +
                   "</html>";
      stateMeaning = " (test: normal div)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();";
      break;
    case 6: 
      defaultData = "Test for selection change event";
      pasteData = "from parent ";
      iframe.src = "data:text/html,<html><body id='text'>" +
                   defaultData +
                   "</body>" +
                   "<script>document.designMode='on';</script>" +
                   "</html>";
      stateMeaning = " (test: normal div with designMode:on)";
      focusScript = "var elt=content.document.getElementById('text');elt.focus();";
      break;
    default:
      SimpleTest.finish();
      break;
  }
}

function testSelectAll(e) {
  iframe.addEventListener("mozbrowserselectionchange", function selectchangeforselectall(e) {
    iframe.removeEventListener("mozbrowserselectionchange", selectchangeforselectall, true);
    ok(true, "got mozbrowserselectionchange event." + stateMeaning);
    ok(e.detail, "event.detail is not null." + stateMeaning);
    ok(e.detail.width != 0, "event.detail.width is not zero" + stateMeaning);
    ok(e.detail.height != 0, "event.detail.height is not zero" + stateMeaning);
    SimpleTest.executeSoon(function() { testCopy1(e); });
  }, true);

  mm.addMessageListener('content-focus', function messageforfocus(msg) {
    mm.removeMessageListener('content-focus', messageforfocus);
    
    doCommand('selectall');
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
  if (state == 2) {
    
    
    compareData = function(clipboardText) {
      return clipboardText.length == defaultData.length;
    };
  }

  SimpleTest.waitForClipboard(compareData, setup, success, fail);
}

function testPaste1(e) {
  
  
  copyToClipboard(pasteData);

  doCommand("paste");
  SimpleTest.executeSoon(function() { testPaste2(e); });
}

function testPaste2(e) {
  mm.addMessageListener('content-text', function messageforpaste(msg) {
    mm.removeMessageListener('content-text', messageforpaste);
    if (state == 5) {
      
      ok(SpecialPowers.wrap(msg).json === defaultData, "paste command works" + stateMeaning);
    } else if (state == 4 && browserElementTestHelpers.getOOPByDefaultPref()) {
      
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
    if (state == 4 && browserElementTestHelpers.getOOPByDefaultPref()) {
      
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
  if (state == 2) {
    
    
    compareData = function(clipboardText) {
      return clipboardText.length == pasteData.length;
    };
  } else if (state == 4 && browserElementTestHelpers.getOOPByDefaultPref()) {
    
    
    compareData = function() { return true; }
  }

  SimpleTest.waitForClipboard(compareData, setup, success, fail);
}

function testCut2(e) {
  mm.addMessageListener('content-text', function messageforcut(msg) {
    mm.removeMessageListener('content-text', messageforcut);
    
    if (state == 5) {
      ok(SpecialPowers.wrap(msg).json !== "", "cut command works" + stateMeaning);
    } else if (state == 4 && browserElementTestHelpers.getOOPByDefaultPref()) {
      
      todo(false, "cut command works" + stateMeaning);
    } else {
      ok(SpecialPowers.wrap(msg).json === "", "cut command works" + stateMeaning);
    }

    state++;
    dispatchTest(e);
  });

  mm.loadFrameScript(getScriptForGetContent(), false);
}

addEventListener('testready', runTest);
