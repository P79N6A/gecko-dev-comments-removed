



"use strict";

this.EXPORTED_SYMBOLS = ["MobileIdentitySmsMoMtVerificationFlow"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/MobileIdentityCommon.jsm");
Cu.import("resource://gre/modules/MobileIdentitySmsVerificationFlow.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "smsService",
                                   "@mozilla.org/sms/smsservice;1",
                                   "nsISmsService");




function SilentSmsRequest(aDeferred) {
  this.deferred = aDeferred;
}
SilentSmsRequest.prototype = {
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIMobileMessageCallback]),

  classID: Components.ID("{ff46f1a8-e040-4ff4-98a7-d5a5b86a2c3e}"),

  notifyMessageSent: function notifyMessageSent(aMessage) {
    log.debug("Silent message successfully sent");
    this.deferred.resolve(aMessage);
  },

  notifySendMessageFailed: function notifySendMessageFailed(aError) {
    log.error("Error sending silent message " + aError);
    this.deferred.reject(aError);
  }
};

this.MobileIdentitySmsMoMtVerificationFlow = function(aOrigin,
                                                      aServiceId,
                                                      aIccId,
                                                      aMtSender,
                                                      aMoVerifier,
                                                      aUI,
                                                      aClient) {

  log.debug("MobileIdentitySmsMoMtVerificationFlow");

  MobileIdentitySmsVerificationFlow.call(this,
                                         aOrigin,
                                         null, 
                                         aIccId,
                                         aServiceId,
                                         false, 
                                         aMtSender,
                                         aMoVerifier,
                                         aUI,
                                         aClient,
                                         this.smsVerifyStrategy);
};

this.MobileIdentitySmsMoMtVerificationFlow.prototype = {

  __proto__: MobileIdentitySmsVerificationFlow.prototype,

  smsVerifyStrategy: function() {
    
    
    
    let deferred = Promise.defer();
    let silentSmsRequest = new SilentSmsRequest(deferred);

    
    
    
    let body = SMS_MO_MT_VERIFY + " " +
               this.client._deriveHawkCredentials(this.sessionToken).id;
    smsService.send(this.verificationOptions.serviceId,
                    this.verificationOptions.moVerifier,
                    body,
                    true, 
                    silentSmsRequest);
    log.debug("Sending " + body + " to " + this.verificationOptions.moVerifier);
    return deferred.promise;
  }
};
