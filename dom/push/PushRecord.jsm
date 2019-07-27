



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Preferences.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PlacesUtils",
                                  "resource://gre/modules/PlacesUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils",
                                  "resource://gre/modules/PrivateBrowsingUtils.jsm");

this.EXPORTED_SYMBOLS = ["PushRecord"];

const prefs = new Preferences("dom.push.");





const QUOTA_REFRESH_TRANSITIONS_SQL = [
  Ci.nsINavHistoryService.TRANSITION_LINK,
  Ci.nsINavHistoryService.TRANSITION_TYPED,
  Ci.nsINavHistoryService.TRANSITION_BOOKMARK,
  Ci.nsINavHistoryService.TRANSITION_REDIRECT_PERMANENT,
  Ci.nsINavHistoryService.TRANSITION_REDIRECT_TEMPORARY
].join(",");

function PushRecord(props) {
  this.pushEndpoint = props.pushEndpoint;
  this.scope = props.scope;
  this.origin = Services.io.newURI(this.scope, null, null).prePath;
  this.originAttributes = props.originAttributes;
  this.pushCount = props.pushCount || 0;
  this.lastPush = props.lastPush || 0;
  this.setQuota(props.quota);
}

PushRecord.prototype = {
  setQuota(suggestedQuota) {
    this.quota = (!isNaN(suggestedQuota) && suggestedQuota >= 0) ?
                 suggestedQuota : prefs.get("maxQuotaPerSubscription");
  },

  updateQuota(lastVisit) {
    if (this.isExpired() || !this.quotaApplies()) {
      
      
      return;
    }
    if (lastVisit < 0) {
      
      
      this.quota = 0;
      return;
    }
    let currentQuota;
    if (lastVisit > this.lastPush) {
      
      
      let daysElapsed = (Date.now() - lastVisit) / 24 / 60 / 60 / 1000;
      currentQuota = Math.min(
        Math.round(8 * Math.pow(daysElapsed, -0.8)),
        prefs.get("maxQuotaPerSubscription")
      );
    } else {
      
      currentQuota = this.quota;
    }
    this.quota = Math.max(currentQuota - 1, 0);
  },

  receivedPush(lastVisit) {
    this.updateQuota(lastVisit);
    this.pushCount++;
    this.lastPush = Date.now();
  },

  







  getLastVisit() {
    if (!this.quotaApplies() || this.isTabOpen()) {
      
      
      return Promise.resolve(Date.now());
    }
    return PlacesUtils.withConnectionWrapper("PushRecord.getLastVisit", db => {
      
      
      
      
      
      return db.executeCached(
        `SELECT MAX(p.last_visit_date)
         FROM moz_places p
         INNER JOIN moz_historyvisits h ON p.id = h.place_id
         WHERE (
           p.url >= :urlLowerBound AND p.url <= :urlUpperBound AND
           h.visit_type IN (${QUOTA_REFRESH_TRANSITIONS_SQL})
         )
        `,
        {
          
          urlLowerBound: this.origin,
          urlUpperBound: this.origin + "\x7f"
        }
      );
    }).then(rows => {
      if (!rows.length) {
        return -Infinity;
      }
      
      let lastVisit = rows[0].getResultByIndex(0);
      return lastVisit / 1000;
    });
  },

  isTabOpen() {
    let windows = Services.wm.getEnumerator("navigator:browser");
    while (windows.hasMoreElements()) {
      let window = windows.getNext();
      if (window.closed || PrivateBrowsingUtils.isWindowPrivate(window)) {
        continue;
      }
      
      let tabs = window.gBrowser ? window.gBrowser.tabContainer.children :
                 window.BrowserApp.tabs;
      for (let tab of tabs) {
        
        let tabURI = (tab.linkedBrowser || tab.browser).currentURI;
        if (tabURI.prePath == this.origin) {
          return true;
        }
      }
    }
    return false;
  },

  quotaApplies() {
    return Number.isFinite(this.quota);
  },

  isExpired() {
    return this.quota === 0;
  },

  toRegistration() {
    return {
      pushEndpoint: this.pushEndpoint,
      lastPush: this.lastPush,
      pushCount: this.pushCount,
    };
  },

  toRegister() {
    return {
      pushEndpoint: this.pushEndpoint,
    };
  },
};



Object.defineProperty(PushRecord.prototype, "origin", {
  configurable: true,
  enumerable: false,
  writable: true,
});
