



"use strict";

this.EXPORTED_SYMBOLS = [
  "AUSTLMY"
];

const {classes: Cc, interfaces: Ci, results: Cr, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm", this);

this.AUSTLMY = {
  
  
  
  
  
  
  
  
  
  

  
  
  EXTERNAL: "EXTERNAL",
  
  NOTIFY: "NOTIFY",

  



  
  CHK_NO_UPDATE_FOUND: 0,
  
  CHK_ADDON_NO_INCOMPAT: 1,
  
  
  CHK_SHOWPROMPT_SNIPPET: 2,
  
  CHK_SHOWPROMPT_PREF: 3,
  
  CHK_ADDON_PREF_DISABLED: 4,
  
  
  CHK_ADDON_SAME_APP_VER: 5,
  
  CHK_ADDON_UPDATES_FOR_INCOMPAT: 6,
  
  CHK_ADDON_HAVE_INCOMPAT: 7,
  
  CHK_HAS_ACTIVEUPDATE: 8,
  
  CHK_IS_DOWNLOADING: 9,
  
  CHK_IS_STAGED: 10,
  
  CHK_IS_DOWNLOADED: 11,
  
  CHK_PREF_DISABLED: 12,
  
  CHK_ADMIN_DISABLED: 13,
  
  CHK_NO_MUTEX: 14,
  
  
  CHK_UNABLE_TO_CHECK: 15,
  
  CHK_DISABLED_FOR_SESSION: 16,
  
  CHK_OFFLINE: 17,
  
  
  CHK_CERT_ATTR_NO_UPDATE_PROMPT: 18,
  
  
  CHK_CERT_ATTR_NO_UPDATE_SILENT: 19,
  
  
  CHK_CERT_ATTR_WITH_UPDATE_PROMPT: 20,
  
  
  CHK_CERT_ATTR_WITH_UPDATE_SILENT: 21,
  
  
  CHK_GENERAL_ERROR_PROMPT: 22,
  
  CHK_GENERAL_ERROR_SILENT: 23,
  
  CHK_NO_COMPAT_UPDATE_FOUND: 24,
  
  CHK_UPDATE_PREVIOUS_VERSION: 25,
  
  CHK_UPDATE_NEVER_PREF: 26,
  
  CHK_UPDATE_INVALID_TYPE: 27,
  
  CHK_UNSUPPORTED: 28,
  
  CHK_UNABLE_TO_APPLY: 29,
  
  CHK_NO_OS_VERSION: 30,
  
  CHK_NO_OS_ABI: 31,
  
  CHK_INVALID_DEFAULT_URL: 32,
  
  CHK_INVALID_USER_OVERRIDE_URL: 33,
  
  CHK_INVALID_DEFAULT_OVERRIDE_URL: 34,

  
















  pingCheckCode: function UT_pingCheckCode(aSuffix, aCode) {
    try {
      if (aCode == this.CHK_NO_UPDATE_FOUND) {
        let id = "UPDATE_CHECK_NO_UPDATE_" + aSuffix;
        
        Services.telemetry.getHistogramById(id).add();
      } else {
        let id = "UPDATE_CHECK_CODE_" + aSuffix;
        
        Services.telemetry.getHistogramById(id).add(aCode);
      }
    } catch (e) {
      Cu.reportError(e);
    }
  },

  











  pingCheckExError: function UT_pingCheckExError(aSuffix, aCode) {
    try {
      let id = "UPDATE_CHECK_EXTENDED_ERROR_" + aSuffix;
      let val = "AUS_CHECK_EX_ERR_" + aCode;
      
      Services.telemetry.getKeyedHistogramById(id).add(val);
    } catch (e) {
      Cu.reportError(e);
    }
  },

  
  STARTUP: "STARTUP",
  
  STAGE: "STAGE",

  
  PATCH_COMPLETE: "COMPLETE",
  
  PATCH_PARTIAL: "PARTIAL",
  
  PATCH_UNKNOWN: "UNKNOWN",

  



  DWNLD_SUCCESS: 0,
  DWNLD_RETRY_OFFLINE: 1,
  DWNLD_RETRY_NET_TIMEOUT: 2,
  DWNLD_RETRY_CONNECTION_REFUSED: 3,
  DWNLD_RETRY_NET_RESET: 4,
  DWNLD_ERR_NO_UPDATE: 5,
  DWNLD_ERR_NO_UPDATE_PATCH: 6,
  DWNLD_ERR_NO_PATCH_FILE: 7,
  DWNLD_ERR_PATCH_SIZE_LARGER: 8,
  DWNLD_ERR_PATCH_SIZE_NOT_EQUAL: 9,
  DWNLD_ERR_BINDING_ABORTED: 10,
  DWNLD_ERR_ABORT: 11,
  DWNLD_ERR_DOCUMENT_NOT_CACHED: 12,
  DWNLD_ERR_VERIFY_NO_REQUEST: 13,
  DWNLD_ERR_VERIFY_PATCH_SIZE_NOT_EQUAL: 14,
  DWNLD_ERR_VERIFY_NO_HASH_MATCH: 15,

  













  pingDownloadCode: function UT_pingDownloadCode(aIsComplete, aCode) {
    let patchType = this.PATCH_UNKNOWN;
    if (aIsComplete === true) {
      patchType = this.PATCH_COMPLETE;
    } else if (aIsComplete === false) {
      patchType = this.PATCH_PARTIAL;
    }
    try {
      let id = "UPDATE_DOWNLOAD_CODE_" + patchType;
      
      Services.telemetry.getHistogramById(id).add(aCode);
    } catch (e) {
      Cu.reportError(e);
    }
  },

  














  pingStateCode: function UT_pingStateCode(aSuffix, aCode) {
    try {
      let id = "UPDATE_STATE_CODE_" + aSuffix;
      
      Services.telemetry.getHistogramById(id).add(aCode);
    } catch(e) {
      Cu.reportError(e);
    }
  },

  














  pingStatusErrorCode: function UT_pingStatusErrorCode(aSuffix, aCode) {
    try {
      let id = "UPDATE_STATUS_ERROR_CODE_" + aSuffix;
      
      Services.telemetry.getHistogramById(id).add(aCode);
    } catch(e) {
      Cu.reportError(e);
    }
  },

  










  pingLastUpdateTime: function UT_pingLastUpdateTime(aSuffix) {
    const PREF_APP_UPDATE_LASTUPDATETIME = "app.update.lastUpdateTime.background-update-timer";
    if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_LASTUPDATETIME)) {
      let lastUpdateTimeSeconds = Services.prefs.getIntPref(PREF_APP_UPDATE_LASTUPDATETIME);
      if (lastUpdateTimeSeconds) {
        let currentTimeSeconds = Math.round(Date.now() / 1000);
        if (lastUpdateTimeSeconds > currentTimeSeconds) {
          try {
            let id = "UPDATE_INVALID_LASTUPDATETIME_" + aSuffix;
            
            Services.telemetry.getHistogramById().add();
          } catch(e) {
            Cu.reportError(e);
          }
        } else {
          let intervalDays = (currentTimeSeconds - lastUpdateTimeSeconds) /
                             (60 * 60 * 24);
          try {
            let id = "UPDATE_LAST_NOTIFY_INTERVAL_DAYS_" + aSuffix;
            
            Services.telemetry.getHistogramById(id).add(intervalDays);
          } catch(e) {
            Cu.reportError(e);
          }
        }
      }
    }
  },

  





  pingWizLastPageCode: function UT_pingWizLastPageCode(aPageID) {
    let pageMap = { invalid: 0,
                    dummy: 1,
                    checking: 2,
                    pluginupdatesfound: 3,
                    noupdatesfound: 4,
                    manualUpdate: 5,
                    unsupported: 6,
                    incompatibleCheck: 7,
                    updatesfoundbasic: 8,
                    updatesfoundbillboard: 9,
                    license: 10,
                    incompatibleList: 11,
                    downloading: 12,
                    errors: 13,
                    errorextra: 14,
                    errorpatching: 15,
                    finished: 16,
                    finishedBackground: 17,
                    installed: 18 };
    try {
      let id = "UPDATE_WIZ_LAST_PAGE_CODE";
      
      Services.telemetry.getHistogramById(id).add(pageMap[aPageID] ||
                                                  pageMap.invalid);
    } catch (e) {
      Cu.reportError(e);
    }
  },

  














  pingServiceInstallStatus: function UT_PSIS(aSuffix, aInstalled) {
    
    
    if (!("@mozilla.org/windows-registry-key;1" in Cc)) {
      Cu.reportError(Cr.NS_ERROR_NOT_AVAILABLE);
      return;
    }

    try {
      let id = "UPDATE_SERVICE_INSTALLED_" + aSuffix;
      
      Services.telemetry.getHistogramById(id).add(aInstalled);
    } catch(e) {
      Cu.reportError(e);
    }

    let attempted = 0;
    try {
      let wrk = Cc["@mozilla.org/windows-registry-key;1"].
                createInstance(Ci.nsIWindowsRegKey);
      wrk.open(wrk.ROOT_KEY_LOCAL_MACHINE,
               "SOFTWARE\\Mozilla\\MaintenanceService",
               wrk.ACCESS_READ | wrk.WOW64_64);
      
      attempted = wrk.readIntValue("Attempted");
      wrk.close();
    } catch(e) {
      
      
    }

    try {
      let id = "UPDATE_SERVICE_MANUALLY_UNINSTALLED_" + aSuffix;
      if (!aInstalled && attempted) {
        
        Services.telemetry.getHistogramById(id).add();
      }
    } catch(e) {
      Cu.reportError(e);
    }
  },

  















  pingBoolPref: function UT_pingBoolPref(aID, aPref, aDefault, aExpected) {
    try {
      let val = aDefault;
      if (Services.prefs.getPrefType(aPref) != Ci.nsIPrefBranch.PREF_INVALID) {
        val = Services.prefs.getBoolPref(aPref);
      }
      if (val != aExpected) {
        
        Services.telemetry.getHistogramById(aID).add();
      }
    } catch(e) {
      Cu.reportError(e);
    }
  },

  















  pingIntPref: function UT_pingIntPref(aID, aPref, aDefault, aExpected) {
    try {
      let val = aDefault;
      if (Services.prefs.getPrefType(aPref) != Ci.nsIPrefBranch.PREF_INVALID) {
        val = Services.prefs.getIntPref(aPref);
      }
      if (aExpected === undefined || val != aExpected) {
        
        Services.telemetry.getHistogramById(aID).add(val);
      }
    } catch(e) {
      Cu.reportError(e);
    }
  },

  















  pingGeneric: function UT_pingGeneric(aID, aValue, aExpected) {
    try {
      if (aExpected === undefined) {
        Services.telemetry.getHistogramById(aID).add(aValue);
      } else if (aValue != aExpected) {
        
        Services.telemetry.getHistogramById(aID).add();
      }
    } catch(e) {
      Cu.reportError(e);
    }
  }
};
Object.freeze(AUSTLMY);
