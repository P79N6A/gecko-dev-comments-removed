addMessageListener("Browser:SSLErrorReportStatus", function(message) {
  sendSyncMessage("ssler-test:SSLErrorReportStatus", {reportStatus:message.data.reportStatus});
});

addMessageListener("ssler-test:SetAutoPref", function(message) {
  let checkbox = content.document.getElementById("automaticallyReportInFuture");

  
  if (checkbox.checked != message.data.value) {
    checkbox.click();
  }

  sendSyncMessage("ssler-test:AutoPrefUpdated", {});
});

addMessageListener("ssler-test:SendBtnClick", function(message) {
  if (message.data && message.data.forceUI) {
    content.dispatchEvent(new content.CustomEvent("AboutNetErrorOptions",
        {
          detail: "{\"enabled\": true, \"automatic\": false}"
        }));
  }
  let btn = content.document.getElementById("reportCertificateError");
  btn.click();
});
