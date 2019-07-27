



"use strict";

function runCodeMirrorTest(browser) {
  let mm = browser.messageManager;
  mm.addMessageListener('setStatus', function listener({data}) {
    let {statusMsg, type, customMsg} = data;
    codeMirror_setStatus(statusMsg, type, customMsg);
  });
  mm.addMessageListener('done', function listener({data}) {
    ok (!data.failed, "CodeMirror tests all passed");
    while (gBrowser.tabs.length > 1) gBrowser.removeCurrentTab();
    mm = null;
    finish();
  });

  
  
  
  
  mm.loadFrameScript('data:,' +
    'content.wrappedJSObject.mozilla_setStatus = function(statusMsg, type, customMsg) {' +
    '  sendSyncMessage("setStatus", {statusMsg: statusMsg, type: type, customMsg: customMsg});' +
    '};' +
    'function check() { ' +
    '  var doc = content.document; var out = doc.getElementById("status"); ' +
    '  if (!out || !out.classList.contains("done")) { return setTimeout(check, 100); }' +
    '  sendSyncMessage("done", { failed: content.wrappedJSObject.failed });' +
    '}' +
    'check();'
  , true);
}