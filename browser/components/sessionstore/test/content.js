








addEventListener("MozStorageChanged", function () {
  sendSyncMessage("ss-test:MozStorageChanged");
});

addMessageListener("ss-test:modifySessionStorage", function (msg) {
  for (let key of Object.keys(msg.data)) {
    content.sessionStorage[key] = msg.data[key];
  }
});

addMessageListener("ss-test:notifyObservers", function ({data: {topic, data}}) {
  Services.obs.notifyObservers(null, topic, data || "");
});

addMessageListener("ss-test:getStyleSheets", function (msg) {
  let sheets = content.document.styleSheets;
  let titles = Array.map(sheets, ss => [ss.title, ss.disabled]);
  sendSyncMessage("ss-test:getStyleSheets", titles);
});

addMessageListener("ss-test:enableStyleSheetsForSet", function (msg) {
  content.document.enableStyleSheetsForSet(msg.data);
  sendSyncMessage("ss-test:enableStyleSheetsForSet");
});

addMessageListener("ss-test:enableSubDocumentStyleSheetsForSet", function (msg) {
  let iframe = content.document.getElementById(msg.data.id);
  iframe.contentDocument.enableStyleSheetsForSet(msg.data.set);
  sendSyncMessage("ss-test:enableSubDocumentStyleSheetsForSet");
});

addMessageListener("ss-test:getAuthorStyleDisabled", function (msg) {
  let {authorStyleDisabled} =
    docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
  sendSyncMessage("ss-test:getAuthorStyleDisabled", authorStyleDisabled);
});

addMessageListener("ss-test:setAuthorStyleDisabled", function (msg) {
  let markupDocumentViewer =
    docShell.contentViewer.QueryInterface(Ci.nsIMarkupDocumentViewer);
  markupDocumentViewer.authorStyleDisabled = msg.data;
  sendSyncMessage("ss-test:setAuthorStyleDisabled");
});
