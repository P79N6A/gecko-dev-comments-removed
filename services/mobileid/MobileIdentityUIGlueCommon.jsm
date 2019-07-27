



"use strict";

this.EXPORTED_SYMBOLS = ["MobileIdentityUIGluePhoneInfo",
                         "MobileIdentityUIGluePromptResult"];

this.MobileIdentityUIGluePhoneInfo = function (aMsisdn, aOperator, aServiceId,
                                               aIccId, aPrimary) {
  this.msisdn = aMsisdn;
  this.operator = aOperator;
  this.serviceId = aServiceId;
  this.iccId = aIccId;
  
  
  this.external = !!aIccId;
  this.primary = aPrimary;
}

this.MobileIdentityUIGluePhoneInfo.prototype = {};

this.MobileIdentityUIGluePromptResult = function (aPhoneNumber, aPrefix, aMcc,
                                                  aServiceId) {
  this.phoneNumber = aPhoneNumber;
  this.prefix = aPrefix;
  this.mcc = aMcc;
  this.serviceId = aServiceId;
}

this.MobileIdentityUIGluePromptResult.prototype = {};
