



"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

this.EXPORTED_SYMBOLS = [
  "TrustedRootCertificate"
];

const APP_TRUSTED_ROOTS= ["AppMarketplaceProdPublicRoot",
                          "AppMarketplaceProdReviewersRoot",
                          "AppMarketplaceDevPublicRoot",
                          "AppMarketplaceDevReviewersRoot",
                          "AppMarketplaceStageRoot",
                          "AppXPCShellRoot"];

this.TrustedRootCertificate = {
  _index: Ci.nsIX509CertDB.AppMarketplaceProdPublicRoot,
  get index() {
    return this._index;
  },
  set index(aIndex) {
    
    
    let found = APP_TRUSTED_ROOTS.some((trustRoot) => {
      return Ci.nsIX509CertDB[trustRoot] === aIndex;
    });
    if (found) {
      this._index = aIndex;
    }
  }
};

