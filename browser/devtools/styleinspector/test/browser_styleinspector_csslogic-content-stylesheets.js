



"use strict";






const TEST_URI_HTML = TEST_URL_ROOT + "doc_content_stylesheet.html";
const TEST_URI_XUL = TEST_URL_ROOT + "doc_content_stylesheet.xul";
const XUL_URI = Cc["@mozilla.org/network/io-service;1"]
                .getService(Ci.nsIIOService)
                .newURI(TEST_URI_XUL, null, null);
const XUL_PRINCIPAL = Components.classes["@mozilla.org/scriptsecuritymanager;1"]
                        .getService(Ci.nsIScriptSecurityManager)
                        .getNoAppCodebasePrincipal(XUL_URI);

let test = asyncTest(function*() {
  info("Checking stylesheets on HTML document");
  yield addTab(TEST_URI_HTML);
  let target = getNode("#target");

  let {toolbox, inspector, view} = yield openRuleView();
  yield selectNode("#target", inspector);

  info("Checking stylesheets");
  yield checkSheets(target);

  info("Checking stylesheets on XUL document");
  info("Allowing XUL content");
  allowXUL();
  yield addTab(TEST_URI_XUL);

  ({toolbox, inspector, view} = yield openRuleView());
  target = getNode("#target");
  yield selectNode("#target", inspector);

  yield checkSheets(target);
  info("Disallowing XUL content");
  disallowXUL();
});

function allowXUL() {
  Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager)
    .addFromPrincipal(XUL_PRINCIPAL, 'allowXULXBL', Ci.nsIPermissionManager.ALLOW_ACTION);
}

function disallowXUL() {
  Cc["@mozilla.org/permissionmanager;1"].getService(Ci.nsIPermissionManager)
    .addFromPrincipal(XUL_PRINCIPAL, 'allowXULXBL', Ci.nsIPermissionManager.DENY_ACTION);
}

function* checkSheets(target) {
  let sheets = yield executeInContent("Test:GetStyleSheetsInfoForNode", {}, {target});

  for (let sheet of sheets) {
    if (!sheet.href ||
        /doc_content_stylesheet_/.test(sheet.href)) {
      ok(sheet.isContentSheet, sheet.href + " identified as content stylesheet");
    } else {
      ok(!sheet.isContentSheet, sheet.href + " identified as non-content stylesheet");
    }
  }
}
