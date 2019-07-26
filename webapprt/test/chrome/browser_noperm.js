Cu.import("resource://webapprt/modules/WebappRT.jsm");
let { AllPossiblePermissions } =
  Cu.import("resource://gre/modules/PermissionsInstaller.jsm", {});
let { AppsUtils } = Cu.import("resource://gre/modules/AppsUtils.jsm", {});
let { DOMApplicationRegistry } =
  Cu.import("resource://gre/modules/Webapps.jsm", {});

function test() {
  waitForExplicitFinish();

  loadWebapp("noperm.webapp", undefined, function onLoad() {
    let app = WebappRT.config.app;

    
    is(AppsUtils.getAppManifestStatus(app.manifest), Ci.nsIPrincipal.APP_STATUS_INSTALLED, "The app is not privileged");

    
    let principal = document.getElementById("content").contentDocument.defaultView.document.nodePrincipal;
    is(DOMApplicationRegistry.getAppLocalIdByManifestURL(app.manifestURL), principal.appId, "Principal app ID correct");

    
    for (let permName of AllPossiblePermissions) {
      
      let permValue = Services.perms.testExactPermissionFromPrincipal(principal, permName);

      is(permValue, Ci.nsIPermissionManager.UNKNOWN_ACTION, "Permission " + permName + " unknown.");
    }

    finish();
  });
}
