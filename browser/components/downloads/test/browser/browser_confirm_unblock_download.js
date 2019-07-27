


"use strict";



registerCleanupFunction(() => {});

function addDialogOpenObserver(buttonAction) {
  Services.ww.registerNotification(function onOpen(subj, topic, data) {
    if (topic == "domwindowopened" && subj instanceof Ci.nsIDOMWindow) {
      
      
      
      subj.addEventListener("load", function onLoad() {
        subj.removeEventListener("load", onLoad);
        if (subj.document.documentURI ==
            "chrome://global/content/commonDialog.xul") {
          Services.ww.unregisterNotification(onOpen);

          let dialog = subj.document.getElementById("commonDialog");
          ok(dialog.classList.contains("alert-dialog"),
             "The dialog element should contain an alert class.");

          let doc = subj.document.documentElement;
          doc.getButton(buttonAction).click();
        }
      });
    }
  });
}

add_task(function* test_confirm_unblock_dialog_unblock() {
  addDialogOpenObserver("cancel");
  let result = yield DownloadsCommon.confirmUnblockDownload(DownloadsCommon.BLOCK_VERDICT_MALWARE,
                                                            window);
  ok(result, "Should return true when the user clicks on `Unblock` button.");
});

add_task(function* test_confirm_unblock_dialog_keep_safe() {
  addDialogOpenObserver("accept");
  let result = yield DownloadsCommon.confirmUnblockDownload(DownloadsCommon.BLOCK_VERDICT_MALWARE,
                                                            window);
  ok(!result, "Should return false when the user clicks on `Keep me safe` button.");
});
