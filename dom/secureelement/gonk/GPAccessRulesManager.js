





"use strict";

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/systemlibs.js");

XPCOMUtils.defineLazyServiceGetter(this, "UiccConnector",
                                   "@mozilla.org/secureelement/connector/uicc;1",
                                   "nsISecureElementConnector");

XPCOMUtils.defineLazyModuleGetter(this, "SEUtils",
                                  "resource://gre/modules/SEUtils.jsm");

XPCOMUtils.defineLazyGetter(this, "SE", function() {
  let obj = {};
  Cu.import("resource://gre/modules/se_consts.js", obj);
  return obj;
});

XPCOMUtils.defineLazyGetter(this, "GP", function() {
  let obj = {};
  Cu.import("resource://gre/modules/gp_consts.js", obj);
  return obj;
});

let DEBUG = SE.DEBUG_ACE;
function debug(msg) {
  if (DEBUG) {
    dump("-*- GPAccessRulesManager " + msg);
  }
}










function GPAccessRulesManager() {}

GPAccessRulesManager.prototype = {
  
  PKCS_AID: "a000000063504b43532d3135",

  
  
  
  READ_BINARY:   [GP.CLA_SM, GP.INS_RB, GP.P1_RB, GP.P2_RB],
  GET_RESPONSE:  [GP.CLA_SM, GP.INS_GR, GP.P1_GR, GP.P2_GR],
  SELECT_BY_DF:  [GP.CLA_SM, GP.INS_SF, GP.P1_SF_DF, GP.P2_SF_FCP],

  
  channel: null,

  
  
  REFRESH_TAG_PATH: [GP.TAG_SEQUENCE, GP.TAG_OCTETSTRING],
  refreshTag: null,

  
  rules: [],

  
  getAccessRules: function getAccessRules() {
    debug("getAccessRules");

    return new Promise((resolve, reject) => {
      this._readAccessRules(() => resolve(this.rules));
    });
  },

  _readAccessRules: Task.async(function*(done) {
    try {
      yield this._openChannel(this.PKCS_AID);

      let odf = yield this._readODF();
      let dodf = yield this._readDODF(odf);

      let acmf = yield this._readACMF(dodf);
      let refreshTag = acmf[this.REFRESH_TAG_PATH[0]]
                           [this.REFRESH_TAG_PATH[1]];

      
      if (SEUtils.arraysEqual(this.refreshTag, refreshTag)) {
        debug("_readAccessRules: refresh tag equals to the one saved.");
        yield this._closeChannel();
        return done();
      }

      this.refreshTag = refreshTag;
      debug("_readAccessRules: refresh tag saved: " + this.refreshTag);

      let acrf = yield this._readACRules(acmf);
      let accf = yield this._readACConditions(acrf);
      this.rules = yield this._parseRules(acrf, accf);

      DEBUG && debug("_readAccessRules: " + JSON.stringify(this.rules, 0, 2));

      yield this._closeChannel();
      done();
    } catch (error) {
      debug("_readAccessRules: " + error);
      this.rules = [];
      yield this._closeChannel();
      done();
    }
  }),

  _openChannel: function _openChannel(aid) {
    if (this.channel !== null) {
      debug("_openChannel: Channel already opened, rejecting.");
      return Promise.reject();
    }

    return new Promise((resolve, reject) => {
      UiccConnector.openChannel(aid, {
        notifyOpenChannelSuccess: (channel, openResponse) => {
          debug("_openChannel/notifyOpenChannelSuccess: Channel " + channel +
                " opened, open response: " + openResponse);
          this.channel = channel;
          resolve();
        },
        notifyError: (error) => {
          debug("_openChannel/notifyError: failed to open channel, error: " +
                error);
          reject(error);
        }
      });
    });
  },

  _closeChannel: function _closeChannel() {
    if (this.channel === null) {
      debug("_closeChannel: Channel not opened, rejecting.");
      return Promise.reject();
    }

    return new Promise((resolve, reject) => {
      UiccConnector.closeChannel(this.channel, {
        notifyCloseChannelSuccess: () => {
          debug("_closeChannel/notifyCloseChannelSuccess: chanel " +
                this.channel + " closed");
          this.channel = null;
          resolve();
        },
        notifyError: (error) => {
          debug("_closeChannel/notifyError: error closing channel, error" +
                error);
          reject(error);
        }
      });
    });
  },

  _exchangeAPDU: function _exchangeAPDU(bytes) {
    DEBUG && debug("apdu " + JSON.stringify(bytes));

    let apdu = this._bytesToAPDU(bytes);
    return new Promise((resolve, reject) => {
      UiccConnector.exchangeAPDU(this.channel, apdu.cla,
        apdu.ins, apdu.p1, apdu.p2, apdu.data, apdu.le,
        {
          notifyExchangeAPDUResponse: (sw1, sw2, data) => {
            debug("APDU response is " + sw1.toString(16) + sw2.toString(16) +
                  " data: " + data);

            
            if (sw1 !== 0x90 && sw2 !== 0x00) {
              debug("rejecting APDU response");
              reject(new Error("Response " + sw1 + "," + sw2));
              return;
            }

            resolve(this._parseTLV(data));
          },

          notifyError: (error) => {
            debug("_exchangeAPDU/notifyError " + error);
            reject(error);
          }
        }
      );
    });
  },

  _readBinaryFile: function _readBinaryFile(selectResponse) {
    DEBUG && debug("Select response: " + JSON.stringify(selectResponse));
    
    
    let fileLength = selectResponse[GP.TAG_FCP][0x80];

    
    if (fileLength[0] === 0 && fileLength[1] === 0) {
      return Promise.resolve(null);
    }

    
    
    return this._exchangeAPDU(this.READ_BINARY);
  },

  _selectAndRead: function _selectAndRead(df) {
    return this._exchangeAPDU(this.SELECT_BY_DF.concat(df.length & 0xFF, df))
           .then((resp) => this._readBinaryFile(resp));
  },

  _readODF: function _readODF() {
    debug("_readODF");
    return this._selectAndRead(GP.ODF_DF);
  },

  _readDODF: function _readDODF(odfFile) {
    debug("_readDODF, ODF file: " + odfFile);

    
    
    
    
    
    
    
    let DODF_DF = odfFile[GP.TAG_EF_ODF][GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
    return this._selectAndRead(DODF_DF);
  },

  _readACMF: function _readACMF(dodfFile) {
    debug("_readACMF, DODF file: " + dodfFile);

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    let gpdOid = [0x2A,                 
                  0x86, 0x48,           
                  0x86, 0xFC, 0x6B,     
                  0x81, 0x48,           
                  0x01,                 
                  0x01];                

    let records = SEUtils.ensureIsArray(dodfFile[GP.TAG_EXTERNALDO]);

    
    let gpdRecords = records.filter((record) => {
      let oid = record[GP.TAG_EXTERNALDO][GP.TAG_SEQUENCE][GP.TAG_OID];
      return SEUtils.arraysEqual(oid, gpdOid);
    });

    
    
    
    
    if (gpdRecords.length !== 1) {
      return Promise.reject(new Error(gpdRecords.length + " ACMF files found"));
    }

    let ACMain_DF = gpdRecords[0][GP.TAG_EXTERNALDO][GP.TAG_SEQUENCE]
                                 [GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
    return this._selectAndRead(ACMain_DF);
  },

  _readACRules: function _readACRules(acMainFile) {
    debug("_readACRules, ACMain file: " + acMainFile);

    
    
    
    
    
    
    
    
    

    let ACRules_DF = acMainFile[GP.TAG_SEQUENCE][GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
    return this._selectAndRead(ACRules_DF);
  },

  _readACConditions: function _readACConditions(acRulesFile) {
    debug("_readACCondition, ACRules file: " + acRulesFile);

    let acRules = SEUtils.ensureIsArray(acRulesFile[GP.TAG_SEQUENCE]);
    if (acRules.length === 0) {
      debug("No rules found in ACRules file.");
      return Promise.reject(new Error("No rules found in ACRules file"));
    }

    
    
    
    let acReadQueue = Promise.resolve({});

    acRules.forEach((ruleEntry) => {
      let df = ruleEntry[GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];

      
      let readAcCondition = (acConditionFiles) => {
        if (acConditionFiles[df] !== undefined) {
          debug("Skipping previously read acCondition df: " + df);
          return acConditionFiles;
        }

        return this._selectAndRead(df)
               .then((acConditionFileContents) => {
                 acConditionFiles[df] = acConditionFileContents;
                 return acConditionFiles;
               });
      }

      acReadQueue = acReadQueue.then(readAcCondition);
    });

    return acReadQueue;
  },

  _parseRules: function _parseRules(acRulesFile, acConditionFiles) {
    DEBUG && debug("_parseRules: acConditionFiles " + JSON.stringify(acConditionFiles));
    let rules = [];

    let acRules = SEUtils.ensureIsArray(acRulesFile[GP.TAG_SEQUENCE]);
    acRules.forEach((ruleEntry) => {
      DEBUG && debug("Parsing one rule: " + JSON.stringify(ruleEntry));
      let rule = {};

      
      
      
      let oneApplet = ruleEntry[GP.TAG_GPD_AID];
      let allApplets = ruleEntry[GP.TAG_GPD_ALL];

      if (oneApplet) {
        rule.applet = oneApplet[GP.TAG_OCTETSTRING];
      } else if (allApplets) {
        rule.applet = Ci.nsIAccessRulesManager.ALL_APPLET;
      } else {
        throw Error("Unknown applet definition");
      }

      let df = ruleEntry[GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING];
      let condition = acConditionFiles[df];
      if (condition === null) {
        rule.application = Ci.nsIAccessRulesManager.DENY_ALL;
      } else if (condition[GP.TAG_SEQUENCE]) {
        if (!Array.isArray(condition[GP.TAG_SEQUENCE]) &&
            !condition[GP.TAG_SEQUENCE][GP.TAG_OCTETSTRING]) {
          rule.application = Ci.nsIAccessRulesManager.ALLOW_ALL;
        } else {
          rule.application = SEUtils.ensureIsArray(condition[GP.TAG_SEQUENCE])
                             .map((conditionEntry) => {
            return conditionEntry[GP.TAG_OCTETSTRING];
          });
        }
      } else {
        throw Error("Unknown application definition");
      }

      DEBUG && debug("Rule parsed, adding to the list: " + JSON.stringify(rule));
      rules.push(rule);
    });

    DEBUG && debug("All rules parsed, we have those in total: " + JSON.stringify(rules));
    return rules;
  },

  _parseTLV: function _parseTLV(bytes) {
    let containerTags = [
      GP.TAG_SEQUENCE,
      GP.TAG_FCP,
      GP.TAG_GPD_AID,
      GP.TAG_EXTERNALDO,
      GP.TAG_INDIRECT,
      GP.TAG_EF_ODF
    ];
    return SEUtils.parseTLV(bytes, containerTags);
  },

  
  
  _bytesToAPDU: function _bytesToAPDU(arr) {
    let apdu = {
      cla: arr[0] & 0xFF,
      ins: arr[1] & 0xFF,
      p1: arr[2] & 0xFF,
      p2: arr[3] & 0xFF,
      p3: arr[4] & 0xFF,
      le: 0
    };

    let data = (apdu.p3 > 0) ? (arr.slice(5)) : [];
    apdu.data = (data.length) ? SEUtils.byteArrayToHexString(data) : null;
    return apdu;
  },

  classID: Components.ID("{3e046b4b-9e66-439a-97e0-98a69f39f55f}"),
  contractID: "@mozilla.org/secureelement/access-control/rules-manager;1",
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIAccessRulesManager])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([GPAccessRulesManager]);
