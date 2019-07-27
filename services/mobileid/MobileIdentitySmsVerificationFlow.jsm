



"use strict";

this.EXPORTED_SYMBOLS = ["MobileIdentitySmsVerificationFlow"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/MobileIdentityCommon.jsm");
Cu.import("resource://gre/modules/MobileIdentityVerificationFlow.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

#ifdef MOZ_B2G_RIL
XPCOMUtils.defineLazyServiceGetter(this, "smsService",
                                   "@mozilla.org/sms/smsservice;1",
                                   "nsISmsService");
#endif

this.MobileIdentitySmsVerificationFlow = function(aVerificationOptions,
                                                  aUI,
                                                  aClient,
                                                  aVerifyStrategy) {

  
  this.smsVerifyStrategy = aVerifyStrategy;

  log.debug("aVerificationOptions ${}", aVerificationOptions);
  MobileIdentityVerificationFlow.call(this, aVerificationOptions, aUI, aClient,
                                      this._verifyStrategy, this._cleanupStrategy);
};

this.MobileIdentitySmsVerificationFlow.prototype = {

  __proto__: MobileIdentityVerificationFlow.prototype,

  observedSilentNumber: null,

  onSilentSms: null,

  _verifyStrategy: function() {
    if (!this.smsVerifyStrategy) {
      return Promise.reject(ERROR_INTERNAL_UNEXPECTED);
    }

    
    
    
    
    

#ifdef MOZ_B2G_RIL
    this.observedSilentNumber = this.verificationOptions.mtSender;
    try {
      smsService.addSilentNumber(this.observedSilentNumber);
    } catch (e) {
      log.warn("We are already listening for that number");
    }

    this.onSilentSms = (function(aSubject, aTopic, aData) {
      log.debug("Got silent message " + aSubject.sender + " - " + aSubject.body);
      
      
      if (aSubject.sender != this.observedSilentNumber) {
        return;
      }

      

      
      
      
      
      
      
      
      let verificationCode = aSubject.body;
      if (this.verificationOptions.external) {
        
        verificationCode = aSubject.body.replace(/[^0-9]/g,'');
      }

      log.debug("Verification code: " + verificationCode);

      this.verificationCodeDeferred.resolve(verificationCode);
    }).bind(this);

    Services.obs.addObserver(this.onSilentSms,
                             SILENT_SMS_RECEIVED_TOPIC,
                             false);
    log.debug("Observing messages from " + this.observedSilentNumber);
#endif

    return this.smsVerifyStrategy();
  },

  _cleanupStrategy: function() {
#ifdef MOZ_B2G_RIL
    smsService.removeSilentNumber(this.observedSilentNumber);
    Services.obs.removeObserver(this.onSilentSms,
                                SILENT_SMS_RECEIVED_TOPIC);
    this.observedSilentNumber = null;
    this.onSilentSms = null;
#endif
  }
};
