



"use strict";

let Ci = Components.interfaces;
let Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

this.EXPORTED_SYMBOLS = ["ThirdPartyCookieProbe"];

const MILLISECONDS_PER_DAY = 1000 * 60 * 60 * 24;







this.ThirdPartyCookieProbe = function() {
  














  this._thirdPartyCookies = new Map();
  


  this._latestFlush = Date.now();
};

this.ThirdPartyCookieProbe.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),
  init: function() {
    Services.obs.addObserver(this, "profile-before-change", false);
    Services.obs.addObserver(this, "third-party-cookie-accepted", false);
    Services.obs.addObserver(this, "third-party-cookie-rejected", false);
  },
  dispose: function() {
    Services.obs.removeObserver(this, "profile-before-change");
    Services.obs.removeObserver(this, "third-party-cookie-accepted");
    Services.obs.removeObserver(this, "third-party-cookie-rejected");
  },
  






  observe: function(docURI, topic, referrer) {
    try {
      if (topic == "profile-before-change") {
        
        this.flush();
        this.dispose();
      }
      if (topic != "third-party-cookie-accepted"
          && topic != "third-party-cookie-rejected") {
        
        return;
      }
      
      let firstParty = normalizeHost(referrer);
      let thirdParty = normalizeHost(docURI.QueryInterface(Ci.nsIURI).host);
      let data = this._thirdPartyCookies.get(thirdParty);
      if (!data) {
        data = new RejectStats();
        this._thirdPartyCookies.set(thirdParty, data);
      }
      if (topic == "third-party-cookie-accepted") {
        data.addAccepted(firstParty);
      } else {
        data.addRejected(firstParty);
      }
    } catch (ex) {
      
      Services.console.logStringMessage("ThirdPartyCookieProbe: Uncaught error " + ex + "\n" + ex.stack);
    }
  },

  





  flush: function(aNow = Date.now()) {
    let updays = (aNow - this._latestFlush) / MILLISECONDS_PER_DAY;
    if (updays <= 0) {
      
      
      return;
    }
    this._latestFlush = aNow;
    let acceptedSites = Services.telemetry.getHistogramById("COOKIES_3RDPARTY_NUM_SITES_ACCEPTED");
    let rejectedSites = Services.telemetry.getHistogramById("COOKIES_3RDPARTY_NUM_SITES_BLOCKED");
    let acceptedRequests = Services.telemetry.getHistogramById("COOKIES_3RDPARTY_NUM_ATTEMPTS_ACCEPTED");
    let rejectedRequests = Services.telemetry.getHistogramById("COOKIES_3RDPARTY_NUM_ATTEMPTS_BLOCKED");
    for (let [k, data] of this._thirdPartyCookies) {
      acceptedSites.add(data.countAcceptedSites / updays);
      rejectedSites.add(data.countRejectedSites / updays);
      acceptedRequests.add(data.countAcceptedRequests / updays);
      rejectedRequests.add(data.countRejectedRequests / updays);
    }
    this._thirdPartyCookies.clear();
  }
};









let RejectStats = function() {
  


  this._acceptedSites = new Set();
  


  this._rejectedSites = new Set();
  




  this._acceptedRequests = 0;
  




  this._rejectedRequests = 0;
};
RejectStats.prototype = {
  addAccepted: function(firstParty) {
    this._acceptedSites.add(firstParty);
    this._acceptedRequests++;
  },
  addRejected: function(firstParty) {
    this._rejectedSites.add(firstParty);
    this._rejectedRequests++;
  },
  get countAcceptedSites() {
    return this._acceptedSites.size;
  },
  get countRejectedSites() {
    return this._rejectedSites.size;
  },
  get countAcceptedRequests() {
    return this._acceptedRequests;
  },
  get countRejectedRequests() {
    return this._rejectedRequests;
  }
};




function normalizeHost(host) {
  return Services.eTLD.getBaseDomainFromHost(host);
};
