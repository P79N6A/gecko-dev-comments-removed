




const PREF_EM_HOTFIX_ID               = "extensions.hotfix.id";
const PREF_EM_HOTFIX_LASTVERSION      = "extensions.hotfix.lastVersion";
const PREF_EM_HOTFIX_URL              = "extensions.hotfix.url";
const PREF_EM_CERT_CHECKATTRIBUTES    = "extensions.hotfix.cert.checkAttributes";
const PREF_EM_HOTFIX_CERTS            = "extensions.hotfix.certs.";


const GOOD_FINGERPRINT = "39:E7:2B:7A:5B:CF:37:78:F9:5D:4A:E0:53:2D:2F:3D:68:53:C5:60";
const BAD_FINGERPRINT  = "40:E7:2B:7A:5B:CF:37:78:F9:5D:4A:E0:53:2D:2F:3D:68:53:C5:60";

Components.utils.import("resource://testing-common/httpd.js");
var testserver = new HttpServer();
testserver.start(-1);
gPort = testserver.identity.primaryPort;
testserver.registerDirectory("/data/", do_get_file("data"));


Services.prefs.setBoolPref("extensions.checkUpdateSecurity", false);

Services.prefs.setBoolPref(PREF_EM_CERT_CHECKATTRIBUTES, true);
Services.prefs.setCharPref(PREF_EM_HOTFIX_URL, "http://localhost:" + gPort + "/hotfix.rdf");

let defaults = Services.prefs.getDefaultBranch("");
defaults.deleteBranch(PREF_EM_HOTFIX_CERTS);






function promiseInstallListener() {
  return new Promise((resolve, reject) => {
    let listener = {
      onDownloadFailed: ai => {
        AddonManager.removeInstallListener(listener);
        reject(ai);
      },
      onInstallEnded: ai => {
        AddonManager.removeInstallListener(listener);
        resolve(ai);
      },
      onDownloadCancelled: ai => {
        AddonManager.removeInstallListener(listener);
        reject(ai);
      }
    };
    AddonManager.addInstallListener(listener);
  });
}

function promiseSuccessfulInstall() {
  return promiseInstallListener().then(
    aInstall => {
      do_check_true(true);
      do_check_eq(aInstall.addon.id, Services.prefs.getCharPref(PREF_EM_HOTFIX_ID));
      aInstall.addon.uninstall();
      Services.prefs.clearUserPref(PREF_EM_HOTFIX_LASTVERSION);
    },
    aInstall => {
      do_throw("Install should not have failed");
    });
}

function promiseFailedInstall() {
  return promiseInstallListener().then(
    aInstall => {
      do_throw("Install should not have succeeded");
      aInstall.addon.uninstall();
      Services.prefs.clearUserPref(PREF_EM_HOTFIX_LASTVERSION);
    },
    aInstall => {
      do_check_true(true);
    });
}

let tryInstallHotfix = Task.async(function*(id, file, installListener) {
  Services.prefs.setCharPref(PREF_EM_HOTFIX_ID, id);

  testserver.registerPathHandler("/hotfix.rdf", function(request, response) {
    response.write(createUpdateRDF({
      [id]: [{
        version: "1.0",
        targetApplications: [{
          id: "xpcshell@tests.mozilla.org",
          minVersion: "0",
          maxVersion: "*",
          updateLink: "http://localhost:" + gPort + "/data/signing_checks/" + file,
        }]
      }]
    }));
  });

  yield Promise.all([
    installListener,
    AddonManagerPrivate.backgroundUpdateCheck()
  ]);

  testserver.registerPathHandler("/hotfix.rdf", null);
  Services.prefs.clearUserPref(PREF_EM_HOTFIX_ID);
});


add_task(function* amo_signed_hotfix() {
  Services.prefs.setCharPref(PREF_EM_HOTFIX_CERTS + "1.sha1Fingerprint", BAD_FINGERPRINT);

  yield tryInstallHotfix("firefox-hotfix@mozilla.org",
                         "hotfix_good.xpi",
                         promiseFailedInstall());
});


add_task(function* amo_signed_hotfix() {
  Services.prefs.setCharPref(PREF_EM_HOTFIX_CERTS + "1.sha1Fingerprint", GOOD_FINGERPRINT);

  yield tryInstallHotfix("firefox-hotfix@mozilla.org",
                         "hotfix_good.xpi",
                         promiseSuccessfulInstall());
});


add_task(function* amo_broken_hotfix() {
  yield tryInstallHotfix("firefox-hotfix@mozilla.org",
                         "hotfix_broken.xpi",
                         promiseFailedInstall());
});


add_task(function* amo_wrongID_rightcert() {
  yield tryInstallHotfix("test@tests.mozilla.org",
                         "hotfix_badid.xpi",
                         promiseFailedInstall());
});



add_task(function* amo_wrongID_rightcert2() {
  yield tryInstallHotfix("firefox-hotfix@mozilla.org",
                         "hotfix_badid.xpi",
                         promiseFailedInstall());
});


add_task(function* amo_signed_addon() {
  yield tryInstallHotfix("test@tests.mozilla.org",
                         "signed_bootstrap_1.xpi",
                         promiseFailedInstall());
});


add_task(function* unsigned() {
  yield tryInstallHotfix("test@tests.mozilla.org",
                         "unsigned_bootstrap_2.xpi",
                         promiseFailedInstall());
});

function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "2", "2");

  startupManager();

  run_next_test();
}
