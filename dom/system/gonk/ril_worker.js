








































"use strict";

importScripts("ril_consts.js");
importScripts("resource://gre/modules/workers/require.js");
importScripts("ril_worker_buf_object.js");
importScripts("ril_worker_telephony_request_queue.js");


let DEBUG = DEBUG_WORKER;
let GLOBAL = this;

if (!this.debug) {
  
  this.debug = function debug(message) {
    dump("RIL Worker: " + message + "\n");
  };
}


const EMERGENCY_CB_MODE_TIMEOUT_MS = 300000;  

const ICC_MAX_LINEAR_FIXED_RECORDS = 0xfe;

const GET_CURRENT_CALLS_RETRY_MAX = 3;

let RILQUIRKS_CALLSTATE_EXTRA_UINT32;


let RILQUIRKS_V5_LEGACY;
let RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL;
let RILQUIRKS_SIM_APP_STATE_EXTRA_FIELDS;

let RILQUIRKS_EXTRA_UINT32_2ND_CALL;

let RILQUIRKS_HAVE_QUERY_ICC_LOCK_RETRY_COUNT;


let RILQUIRKS_SEND_STK_PROFILE_DOWNLOAD;


let RILQUIRKS_DATA_REGISTRATION_ON_DEMAND;


let RILQUIRKS_SUBSCRIPTION_CONTROL;

let RILQUIRKS_SIGNAL_EXTRA_INT32;


let RILQUIRKS_SMSC_ADDRESS_FORMAT;








function RilObject(aContext) {
  this.context = aContext;

  this.telephonyRequestQueue = new TelephonyRequestQueue(this);
  this.currentConferenceState = CALL_STATE_UNKNOWN;
  this._pendingSentSmsMap = {};
  this.pendingNetworkType = {};
  this._receivedSmsCbPagesMap = {};
  this._getCurrentCallsRetryCount = 0;

  
  this.v5Legacy = RILQUIRKS_V5_LEGACY;
}
RilObject.prototype = {
  context: null,

  


  version: null,
  v5Legacy: null,

  


  currentConferenceState: null,

  


  _pendingSentSmsMap: null,

  


  pendingNetworkType: null,

  


  cellBroadcastDisabled: false,

  



  cellBroadcastConfigs: null,
  mergedCellBroadcastConfig: null,

  _receivedSmsCbPagesMap: null,

  initRILState: function() {
    


    this.radioState = GECKO_RADIOSTATE_UNKNOWN;

    


    this._isCdma = false;

    


    this._isInEmergencyCbMode = false;

    



    this._waitingRadioTech = false;

    


    this.cardState = GECKO_CARDSTATE_UNINITIALIZED;

    


    this.IMEI = null;
    this.IMEISV = null;
    this.ESN = null;
    this.MEID = null;
    this.SMSC = null;

    


    this.iccInfoPrivate = {};

    


    this.iccInfo = {};

    


    this.cdmaHome = null;

    


    this.aid = null;

    


    this.appType = null;

    this.networkSelectionMode = GECKO_NETWORK_SELECTION_UNKNOWN;

    this.voiceRegistrationState = {};
    this.dataRegistrationState = {};

    


    this.operator = null;

    


    this.basebandVersion = null;

    
    this.sendChromeMessage({
      rilMessageType: "currentCalls",
      calls: {}
    });

    
    

    



    this._processingNetworkInfo = false;

    





    this._needRepollNetworkInfo = false;

    


    this._pendingNetworkInfo = {rilMessageType: "networkinfochanged"};

    







    this._ussdSession = null;

    


    let cbmmi = this.cellBroadcastConfigs && this.cellBroadcastConfigs.MMI;
    this.cellBroadcastConfigs = {
      MMI: cbmmi || null
    };
    this.mergedCellBroadcastConfig = null;

    


    this.pendingToReportSmsMemoryStatus = false;
    this.smsStorageAvailable = true;
  },

  











  parseInt: function(string, defaultValue, radix) {
    let number = parseInt(string, radix || 10);
    if (!isNaN(number)) {
      return number;
    }
    if (defaultValue === undefined) {
      defaultValue = null;
    }
    return defaultValue;
  },


  












  


  getICCStatus: function() {
    this.context.Buf.simpleRequest(REQUEST_GET_SIM_STATUS);
  },

  


  iccUnlockCardLock: function(options) {
    switch (options.lockType) {
      case GECKO_CARDLOCK_PIN:
        this.enterICCPIN(options);
        break;
      case GECKO_CARDLOCK_PIN2:
        this.enterICCPIN2(options);
        break;
      case GECKO_CARDLOCK_PUK:
        this.enterICCPUK(options);
        break;
      case GECKO_CARDLOCK_PUK2:
        this.enterICCPUK2(options);
        break;
      case GECKO_CARDLOCK_NCK:
      case GECKO_CARDLOCK_NSCK:
      case GECKO_CARDLOCK_NCK1:
      case GECKO_CARDLOCK_NCK2:
      case GECKO_CARDLOCK_HNCK:
      case GECKO_CARDLOCK_CCK:
      case GECKO_CARDLOCK_SPCK:
      case GECKO_CARDLOCK_PCK:
      case GECKO_CARDLOCK_RCCK:
      case GECKO_CARDLOCK_RSPCK:
      case GECKO_CARDLOCK_NCK_PUK:
      case GECKO_CARDLOCK_NSCK_PUK:
      case GECKO_CARDLOCK_NCK1_PUK:
      case GECKO_CARDLOCK_NCK2_PUK:
      case GECKO_CARDLOCK_HNCK_PUK:
      case GECKO_CARDLOCK_CCK_PUK:
      case GECKO_CARDLOCK_SPCK_PUK:
      case GECKO_CARDLOCK_PCK_PUK:
      case GECKO_CARDLOCK_RCCK_PUK: 
      case GECKO_CARDLOCK_RSPCK_PUK:
        this.enterDepersonalization(options);
        break;
      default:
        options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.sendChromeMessage(options);
    }
  },

  







  enterICCPIN: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_ENTER_SIM_PIN, options);
    Buf.writeInt32(this.v5Legacy ? 1 : 2);
    Buf.writeString(options.password);
    if (!this.v5Legacy) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  







  enterICCPIN2: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_ENTER_SIM_PIN2, options);
    Buf.writeInt32(this.v5Legacy ? 1 : 2);
    Buf.writeString(options.password);
    if (!this.v5Legacy) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  







  enterDepersonalization: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE, options);
    Buf.writeInt32(1);
    Buf.writeString(options.password);
    Buf.sendParcel();
  },

  









  changeICCPIN: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN, options);
    Buf.writeInt32(this.v5Legacy ? 2 : 3);
    Buf.writeString(options.password);
    Buf.writeString(options.newPassword);
    if (!this.v5Legacy) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  









  changeICCPIN2: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_CHANGE_SIM_PIN2, options);
    Buf.writeInt32(this.v5Legacy ? 2 : 3);
    Buf.writeString(options.password);
    Buf.writeString(options.newPassword);
    if (!this.v5Legacy) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  









   enterICCPUK: function(options) {
     let Buf = this.context.Buf;
     Buf.newParcel(REQUEST_ENTER_SIM_PUK, options);
     Buf.writeInt32(this.v5Legacy ? 2 : 3);
     Buf.writeString(options.password);
     Buf.writeString(options.newPin);
     if (!this.v5Legacy) {
       Buf.writeString(options.aid || this.aid);
     }
     Buf.sendParcel();
   },

  









   enterICCPUK2: function(options) {
     let Buf = this.context.Buf;
     Buf.newParcel(REQUEST_ENTER_SIM_PUK2, options);
     Buf.writeInt32(this.v5Legacy ? 2 : 3);
     Buf.writeString(options.password);
     Buf.writeString(options.newPin);
     if (!this.v5Legacy) {
       Buf.writeString(options.aid || this.aid);
     }
     Buf.sendParcel();
   },

  


  iccChangeCardLockPassword: function(options) {
    switch (options.lockType) {
      case GECKO_CARDLOCK_PIN:
        this.changeICCPIN(options);
        break;
      case GECKO_CARDLOCK_PIN2:
        this.changeICCPIN2(options);
        break;
      default:
        options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.sendChromeMessage(options);
    }
  },

  


  iccSetCardLockEnabled: function(options) {
    switch (options.lockType) {
      case GECKO_CARDLOCK_PIN: 
      case GECKO_CARDLOCK_FDN:
        options.facility = GECKO_CARDLOCK_TO_FACILITY[options.lockType];
        break;
      default:
        options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.sendChromeMessage(options);
        return;
    }

    options.serviceClass = ICC_SERVICE_CLASS_VOICE |
                           ICC_SERVICE_CLASS_DATA  |
                           ICC_SERVICE_CLASS_FAX;
    this.setICCFacilityLock(options);
  },

  


  iccGetCardLockEnabled: function(options) {
    switch (options.lockType) {
      case GECKO_CARDLOCK_PIN: 
      case GECKO_CARDLOCK_FDN:
        options.facility = GECKO_CARDLOCK_TO_FACILITY[options.lockType];
        break;
      default:
        options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.sendChromeMessage(options);
        return;
    }

    options.password = ""; 
    options.serviceClass = ICC_SERVICE_CLASS_VOICE |
                           ICC_SERVICE_CLASS_DATA  |
                           ICC_SERVICE_CLASS_FAX;
    this.queryICCFacilityLock(options);
  },

  





  iccGetCardLockRetryCount: function(options) {
    if (!RILQUIRKS_HAVE_QUERY_ICC_LOCK_RETRY_COUNT) {
      
      options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
      this.sendChromeMessage(options);
      return;
    }

    switch (options.lockType) {
      case GECKO_CARDLOCK_PIN:
      case GECKO_CARDLOCK_PIN2:
      case GECKO_CARDLOCK_PUK:
      case GECKO_CARDLOCK_PUK2:
      case GECKO_CARDLOCK_NCK:
      case GECKO_CARDLOCK_NSCK:
      case GECKO_CARDLOCK_CCK: 
      case GECKO_CARDLOCK_SPCK:
      
      
        options.selCode = GECKO_CARDLOCK_TO_SEL_CODE[options.lockType];
        break;
      default:
        options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        this.sendChromeMessage(options);
        return;
    }

    this.queryICCLockRetryCount(options);
  },

  







  queryICCLockRetryCount: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_GET_UNLOCK_RETRY_COUNT, options);
    Buf.writeInt32(1);
    Buf.writeString(options.selCode);
    Buf.sendParcel();
  },

  











  queryICCFacilityLock: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_QUERY_FACILITY_LOCK, options);
    Buf.writeInt32(this.v5Legacy ? 3 : 4);
    Buf.writeString(options.facility);
    Buf.writeString(options.password);
    Buf.writeString(options.serviceClass.toString());
    if (!this.v5Legacy) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  













  setICCFacilityLock: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_FACILITY_LOCK, options);
    Buf.writeInt32(this.v5Legacy ? 4 : 5);
    Buf.writeString(options.facility);
    Buf.writeString(options.enabled ? "1" : "0");
    Buf.writeString(options.password);
    Buf.writeString(options.serviceClass.toString());
    if (!this.v5Legacy) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  





















  iccIO: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SIM_IO, options);
    Buf.writeInt32(options.command);
    Buf.writeInt32(options.fileId);
    Buf.writeString(options.pathId);
    Buf.writeInt32(options.p1);
    Buf.writeInt32(options.p2);
    Buf.writeInt32(options.p3);

    
    if (options.command == ICC_COMMAND_UPDATE_RECORD &&
        options.dataWriter) {
      options.dataWriter(options.p3);
    } else {
      Buf.writeString(null);
    }

    
    if (options.command == ICC_COMMAND_UPDATE_RECORD &&
        options.pin2) {
      Buf.writeString(options.pin2);
    } else {
      Buf.writeString(null);
    }

    if (!this.v5Legacy) {
      Buf.writeString(options.aid || this.aid);
    }
    Buf.sendParcel();
  },

  





  getIMSI: function(aid) {
    let Buf = this.context.Buf;
    if (this.v5Legacy) {
      Buf.simpleRequest(REQUEST_GET_IMSI);
      return;
    }
    Buf.newParcel(REQUEST_GET_IMSI);
    Buf.writeInt32(1);
    Buf.writeString(aid || this.aid);
    Buf.sendParcel();
  },

  


  getGID1: function(options) {
    options.gid1 = this.iccInfoPrivate.gid1;
    this.sendChromeMessage(options);
  },

  







  readICCContacts: function(options) {
    if (!this.appType) {
      options.errorMsg = CONTACT_ERR_REQUEST_NOT_SUPPORTED;
      this.sendChromeMessage(options);
      return;
    }

    this.context.ICCContactHelper.readICCContacts(
      this.appType,
      options.contactType,
      function onsuccess(contacts) {
        for (let i = 0; i < contacts.length; i++) {
          let contact = contacts[i];
          let pbrIndex = contact.pbrIndex || 0;
          let recordIndex = pbrIndex * ICC_MAX_LINEAR_FIXED_RECORDS + contact.recordId;
          contact.contactId = this.iccInfo.iccid + recordIndex;
        }
        
        options.contacts = contacts;
        this.sendChromeMessage(options);
      }.bind(this),
      function onerror(errorMsg) {
        options.errorMsg = errorMsg;
        this.sendChromeMessage(options);
      }.bind(this));
  },

  







  updateICCContact: function(options) {
    let onsuccess = function onsuccess() {
      let recordIndex =
        contact.pbrIndex * ICC_MAX_LINEAR_FIXED_RECORDS + contact.recordId;
      contact.contactId = this.iccInfo.iccid + recordIndex;
      
      this.sendChromeMessage(options);
    }.bind(this);

    let onerror = function onerror(errorMsg) {
      options.errorMsg = errorMsg;
      this.sendChromeMessage(options);
    }.bind(this);

    if (!this.appType || !options.contact) {
      onerror(CONTACT_ERR_REQUEST_NOT_SUPPORTED );
      return;
    }

    let contact = options.contact;
    let iccid = this.iccInfo.iccid;
    let isValidRecordId = false;
    if (typeof contact.contactId === "string" &&
        contact.contactId.startsWith(iccid)) {
      let recordIndex = contact.contactId.substring(iccid.length);
      contact.pbrIndex = Math.floor(recordIndex / ICC_MAX_LINEAR_FIXED_RECORDS);
      contact.recordId = recordIndex % ICC_MAX_LINEAR_FIXED_RECORDS;
      isValidRecordId = contact.recordId > 0 && contact.recordId < 0xff;
    }

    if (DEBUG) {
      this.context.debug("Update ICC Contact " + JSON.stringify(contact));
    }

    let ICCContactHelper = this.context.ICCContactHelper;
    
    
    if (isValidRecordId) {
      ICCContactHelper.updateICCContact(
        this.appType, options.contactType, contact, options.pin2, onsuccess, onerror);
    } else {
      ICCContactHelper.addICCContact(
        this.appType, options.contactType, contact, options.pin2, onsuccess, onerror);
    }
  },

  






  overrideICCNetworkName: function() {
    if (!this.operator) {
      return false;
    }

    
    
    if (!this.voiceRegistrationState.cell ||
        this.voiceRegistrationState.cell.gsmLocationAreaCode == -1) {
      return false;
    }

    let ICCUtilsHelper = this.context.ICCUtilsHelper;
    let networkName = ICCUtilsHelper.getNetworkNameFromICC(
      this.operator.mcc,
      this.operator.mnc,
      this.voiceRegistrationState.cell.gsmLocationAreaCode);

    if (!networkName) {
      return false;
    }

    if (DEBUG) {
      this.context.debug("Operator names will be overriden: " +
                         "longName = " + networkName.fullName + ", " +
                         "shortName = " + networkName.shortName);
    }

    this.operator.longName = networkName.fullName;
    this.operator.shortName = networkName.shortName;

    this._sendNetworkInfoMessage(NETWORK_INFO_OPERATOR, this.operator);
    return true;
  },

  





  setRadioEnabled: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_RADIO_POWER, options);
    Buf.writeInt32(1);
    Buf.writeInt32(options.enabled ? 1 : 0);
    Buf.sendParcel();
  },

  


  _handleQueryMMICallWaiting: function(options) {
    let Buf = this.context.Buf;

    function callback(options) {
      options.length = Buf.readInt32();
      options.enabled = (Buf.readInt32() === 1);
      let services = Buf.readInt32();
      if (options.enabled) {
        options.statusMessage = MMI_SM_KS_SERVICE_ENABLED_FOR;
        let serviceClass = [];
        for (let serviceClassMask = 1;
             serviceClassMask <= ICC_SERVICE_CLASS_MAX;
             serviceClassMask <<= 1) {
          if ((serviceClassMask & services) !== 0) {
            serviceClass.push(MMI_KS_SERVICE_CLASS_MAPPING[serviceClassMask]);
          }
        }
        options.additionalInformation = serviceClass;
      } else {
        options.statusMessage = MMI_SM_KS_SERVICE_DISABLED;
      }

      
      delete options.callback;
      this.sendChromeMessage(options);
    }

    options.callback = callback;
    this.queryCallWaiting(options);
  },

  


  _handleSetMMICallWaiting: function(options) {
    function callback(options) {
      if (options.enabled) {
        options.statusMessage = MMI_SM_KS_SERVICE_ENABLED;
      } else {
        options.statusMessage = MMI_SM_KS_SERVICE_DISABLED;
      }

      
      delete options.callback;
      this.sendChromeMessage(options);
    }

    options.callback = callback;
    this.setCallWaiting(options);
  },

  



  queryCallWaiting: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_QUERY_CALL_WAITING, options);
    Buf.writeInt32(1);
    
    
    Buf.writeInt32(ICC_SERVICE_CLASS_NONE);
    Buf.sendParcel();
  },

  





  setCallWaiting: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_CALL_WAITING, options);
    Buf.writeInt32(2);
    Buf.writeInt32(options.enabled ? 1 : 0);
    Buf.writeInt32(options.serviceClass !== undefined ?
                    options.serviceClass : ICC_SERVICE_CLASS_VOICE);
    Buf.sendParcel();
  },

  





  queryCLIP: function(options) {
    this.context.Buf.simpleRequest(REQUEST_QUERY_CLIP, options);
  },

  



  getCLIR: function(options) {
    this.context.Buf.simpleRequest(REQUEST_GET_CLIR, options);
  },

  






  setCLIR: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_CLIR, options);
    Buf.writeInt32(1);
    Buf.writeInt32(options.clirMode);
    Buf.sendParcel();
  },

  





  setScreenState: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SCREEN_STATE);
    Buf.writeInt32(1);
    Buf.writeInt32(options.on ? 1 : 0);
    Buf.sendParcel();
  },

  getVoiceRegistrationState: function() {
    this.context.Buf.simpleRequest(REQUEST_VOICE_REGISTRATION_STATE);
  },

  getVoiceRadioTechnology: function() {
    this.context.Buf.simpleRequest(REQUEST_VOICE_RADIO_TECH);
  },

  getDataRegistrationState: function() {
    this.context.Buf.simpleRequest(REQUEST_DATA_REGISTRATION_STATE);
  },

  getOperator: function() {
    this.context.Buf.simpleRequest(REQUEST_OPERATOR);
  },

  





  setPreferredNetworkType: function(options) {
    let networkType = options.type;
    if (networkType < 0 || networkType >= RIL_PREFERRED_NETWORK_TYPE_TO_GECKO.length) {
      options.errorMsg = GECKO_ERROR_INVALID_PARAMETER;
      this.sendChromeMessage(options);
      return;
    }

    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_PREFERRED_NETWORK_TYPE, options);
    Buf.writeInt32(1);
    Buf.writeInt32(networkType);
    Buf.sendParcel();
  },

  


  getPreferredNetworkType: function(options) {
    this.context.Buf.simpleRequest(REQUEST_GET_PREFERRED_NETWORK_TYPE, options);
  },

  


  getNeighboringCellIds: function(options) {
    this.context.Buf.simpleRequest(REQUEST_GET_NEIGHBORING_CELL_IDS, options);
  },

  


  getCellInfoList: function(options) {
    this.context.Buf.simpleRequest(REQUEST_GET_CELL_INFO_LIST, options);
  },

  


  requestNetworkInfo: function() {
    if (this._processingNetworkInfo) {
      if (DEBUG) {
        this.context.debug("Network info requested, but we're already " +
                           "requesting network info.");
      }
      this._needRepollNetworkInfo = true;
      return;
    }

    if (DEBUG) this.context.debug("Requesting network info");

    this._processingNetworkInfo = true;
    this.getVoiceRegistrationState();
    this.getDataRegistrationState(); 
    this.getOperator();
    this.getNetworkSelectionMode();
    this.getSignalStrength();
  },

  


  getAvailableNetworks: function(options) {
    if (DEBUG) this.context.debug("Getting available networks");
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_QUERY_AVAILABLE_NETWORKS, options);
    Buf.sendParcel();
  },

  


  getNetworkSelectionMode: function() {
    if (DEBUG) this.context.debug("Getting network selection mode");
    this.context.Buf.simpleRequest(REQUEST_QUERY_NETWORK_SELECTION_MODE);
  },

  


  selectNetworkAuto: function(options) {
    if (DEBUG) this.context.debug("Setting automatic network selection");
    this.context.Buf.simpleRequest(REQUEST_SET_NETWORK_SELECTION_AUTOMATIC, options);
  },

  


  setRoamingPreference: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_CDMA_SET_ROAMING_PREFERENCE, options);
    Buf.writeInt32(1);
    Buf.writeInt32(options.mode);
    Buf.sendParcel();
  },

  


  queryRoamingPreference: function(options) {
    this.context.Buf.simpleRequest(REQUEST_CDMA_QUERY_ROAMING_PREFERENCE, options);
  },

  


  setVoicePrivacyMode: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE, options);
    Buf.writeInt32(1);
    Buf.writeInt32(options.enabled ? 1 : 0);
    Buf.sendParcel();
  },

  


  queryVoicePrivacyMode: function(options) {
    this.context.Buf.simpleRequest(REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE, options);
  },

  


  iccOpenChannel: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SIM_OPEN_CHANNEL, options);
    Buf.writeString(options.aid);
    Buf.sendParcel();
  },

  


  iccExchangeAPDU: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SIM_TRANSMIT_APDU_CHANNEL, options);
    Buf.writeInt32(options.channel);
    Buf.writeInt32(options.apdu.cla);
    Buf.writeInt32(options.apdu.command);
    Buf.writeInt32(options.apdu.p1);
    Buf.writeInt32(options.apdu.p2);
    Buf.writeInt32(options.apdu.p3);
    Buf.writeString(options.apdu.data);
    Buf.sendParcel();
  },

  


  iccCloseChannel: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SIM_CLOSE_CHANNEL, options);
    Buf.writeInt32(1);
    Buf.writeInt32(options.channel);
    Buf.sendParcel();
  },

  


  getIccServiceState: function(options) {
    switch (options.service) {
      case GECKO_CARDSERVICE_FDN:
        let ICCUtilsHelper = this.context.ICCUtilsHelper;
        options.result = ICCUtilsHelper.isICCServiceAvailable("FDN");
        break;
      default:
        options.errorMsg = GECKO_ERROR_REQUEST_NOT_SUPPORTED;
        break;
    }
    this.sendChromeMessage(options);
  },

  


  setUiccSubscription: function(options) {
    if (DEBUG) {
      this.context.debug("setUiccSubscription: " + JSON.stringify(options));
    }

    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_UICC_SUBSCRIPTION, options);
    Buf.writeInt32(this.context.clientId);
    Buf.writeInt32(options.appIndex);
    Buf.writeInt32(this.context.clientId);
    Buf.writeInt32(options.enabled ? 1 : 0);
    Buf.sendParcel();
  },

  


  selectNetwork: function(options) {
    if (DEBUG) {
      this.context.debug("Setting manual network selection: " +
                         options.mcc + ", " + options.mnc);
    }

    let numeric = (options.mcc && options.mnc) ? options.mcc + options.mnc : null;
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_NETWORK_SELECTION_MANUAL, options);
    Buf.writeString(numeric);
    Buf.sendParcel();
  },

  


  getSignalStrength: function() {
    this.context.Buf.simpleRequest(REQUEST_SIGNAL_STRENGTH);
  },

  getIMEI: function(options) {
    this.context.Buf.simpleRequest(REQUEST_GET_IMEI, options);
  },

  getIMEISV: function() {
    this.context.Buf.simpleRequest(REQUEST_GET_IMEISV);
  },

  getDeviceIdentity: function() {
    this.context.Buf.simpleRequest(REQUEST_DEVICE_IDENTITY);
  },

  getBasebandVersion: function() {
    this.context.Buf.simpleRequest(REQUEST_BASEBAND_VERSION);
  },

  sendExitEmergencyCbModeRequest: function(options) {
    this.context.Buf.simpleRequest(REQUEST_EXIT_EMERGENCY_CALLBACK_MODE, options);
  },

  getCdmaSubscription: function() {
    this.context.Buf.simpleRequest(REQUEST_CDMA_SUBSCRIPTION);
  },

  exitEmergencyCbMode: function(options) {
    
    
    
    if (!options) {
      options = {internal: true};
    }
    this._cancelEmergencyCbModeTimeout();
    this.sendExitEmergencyCbModeRequest(options);
  },

  











  dial: function(options) {
    if (options.isEmergency) {
      options.request = RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL ?
                        REQUEST_DIAL_EMERGENCY_CALL : REQUEST_DIAL;

    } else {
      options.request = REQUEST_DIAL;

      
      if (this._isInEmergencyCbMode) {
        this.exitEmergencyCbMode();
      }
    }

    this.telephonyRequestQueue.push(options.request, () => {
      let Buf = this.context.Buf;
      Buf.newParcel(options.request, options);
      Buf.writeString(options.number);
      Buf.writeInt32(options.clirMode || 0);
      Buf.writeInt32(options.uusInfo || 0);
      
      
      Buf.writeInt32(0);
      Buf.sendParcel();
    });
  },

  





  cdmaFlash: function(options) {
    let Buf = this.context.Buf;
    options.request = REQUEST_CDMA_FLASH;
    Buf.newParcel(options.request, options);
    Buf.writeString(options.featureStr || "");
    Buf.sendParcel();
  },

  





  hangUpCall: function(options) {
    this.telephonyRequestQueue.push(REQUEST_HANGUP, () => {
      let Buf = this.context.Buf;
      Buf.newParcel(REQUEST_HANGUP, options);
      Buf.writeInt32(1);
      Buf.writeInt32(options.callIndex);
      Buf.sendParcel();
    });
  },

  hangUpForeground: function(options) {
    this.telephonyRequestQueue.push(REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND, () => {
      this.context.Buf.simpleRequest(REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND,
                                     options);
    });
  },

  hangUpBackground: function(options) {
    this.telephonyRequestQueue.push(REQUEST_HANGUP_WAITING_OR_BACKGROUND, () => {
      this.context.Buf.simpleRequest(REQUEST_HANGUP_WAITING_OR_BACKGROUND,
                                     options);
    });
  },

  switchActiveCall: function(options) {
    this.telephonyRequestQueue.push(REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE, () => {
      this.context.Buf.simpleRequest(REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE,
                                     options);
    });
  },

  udub: function(options) {
    this.telephonyRequestQueue.push(REQUEST_UDUB, () => {
      this.context.Buf.simpleRequest(REQUEST_UDUB, options);
    });
  },

  answerCall: function(options) {
    this.telephonyRequestQueue.push(REQUEST_ANSWER, () => {
      this.context.Buf.simpleRequest(REQUEST_ANSWER, options);
    });
  },

  conferenceCall: function(options) {
    this.telephonyRequestQueue.push(REQUEST_CONFERENCE, () => {
      this.context.Buf.simpleRequest(REQUEST_CONFERENCE, options);
    });
  },

  separateCall: function(options) {
    this.telephonyRequestQueue.push(REQUEST_SEPARATE_CONNECTION, () => {
      let Buf = this.context.Buf;
      Buf.newParcel(REQUEST_SEPARATE_CONNECTION, options);
      Buf.writeInt32(1);
      Buf.writeInt32(options.callIndex);
      Buf.sendParcel();
    });
  },

  


  getCurrentCalls: function(options) {
    this.telephonyRequestQueue.push(REQUEST_GET_CURRENT_CALLS, () => {
      this.context.Buf.simpleRequest(REQUEST_GET_CURRENT_CALLS, options);
    });
  },

  





  setMute: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_MUTE);
    Buf.writeInt32(1);
    Buf.writeInt32(options.muted ? 1 : 0);
    Buf.sendParcel();
  },

  











  sendSMS: function(options) {
    options.langIndex = options.langIndex || PDU_NL_IDENTIFIER_DEFAULT;
    options.langShiftIndex = options.langShiftIndex || PDU_NL_IDENTIFIER_DEFAULT;

    if (!options.retryCount) {
      options.retryCount = 0;
    }

    if (!options.segmentSeq) {
      
      options.segmentSeq = 1;
      options.body = options.segments[0].body;
      options.encodedBodyLength = options.segments[0].encodedBodyLength;
    }

    let Buf = this.context.Buf;
    if (this._isCdma) {
      Buf.newParcel(REQUEST_CDMA_SEND_SMS, options);
      this.context.CdmaPDUHelper.writeMessage(options);
    } else {
      Buf.newParcel(REQUEST_SEND_SMS, options);
      Buf.writeInt32(2);
      Buf.writeString(options.SMSC);
      this.context.GsmPDUHelper.writeMessage(options);
    }
    Buf.sendParcel();
  },

  







  acknowledgeGsmSms: function(success, cause) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SMS_ACKNOWLEDGE);
    Buf.writeInt32(2);
    Buf.writeInt32(success ? 1 : 0);
    Buf.writeInt32(cause);
    Buf.sendParcel();
  },

  





  ackSMS: function(options) {
    if (options.result == PDU_FCS_RESERVED) {
      return;
    }
    if (this._isCdma) {
      this.acknowledgeCdmaSms(options.result == PDU_FCS_OK, options.result);
    } else {
      this.acknowledgeGsmSms(options.result == PDU_FCS_OK, options.result);
    }
  },

  







  acknowledgeCdmaSms: function(success, cause) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_CDMA_SMS_ACKNOWLEDGE);
    Buf.writeInt32(success ? 0 : 1);
    Buf.writeInt32(cause);
    Buf.sendParcel();
  },

  


  updateMwis: function(options) {
    if (this.context.ICCUtilsHelper.isICCServiceAvailable("MWIS")) {
      this.context.SimRecordHelper.updateMWIS(options.mwi);
    }
  },

  


  _updateSmsMemoryStatus: function() {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_REPORT_SMS_MEMORY_STATUS);
    Buf.writeInt32(1);
    Buf.writeInt32(this.smsStorageAvailable ? 1 : 0);
    Buf.sendParcel();
  },

  reportSmsMemoryStatus: function(options) {
    this.pendingToReportSmsMemoryStatus = true;
    this.smsStorageAvailable = options.isAvailable;
    this._updateSmsMemoryStatus();
  },

  setCellBroadcastDisabled: function(options) {
    this.cellBroadcastDisabled = options.disabled;

    
    
    
    
    
    if (this.mergedCellBroadcastConfig) {
      this.updateCellBroadcastConfig();
    }
  },

  setCellBroadcastSearchList: function(options) {
    let getSearchListStr = function(aSearchList) {
      if (typeof aSearchList === "string" || aSearchList instanceof String) {
        return aSearchList;
      }

      
      let prop = this._isCdma ? "cdma" : "gsm";

      return aSearchList && aSearchList[prop];
    }.bind(this);

    try {
      let str = getSearchListStr(options.searchList);
      this.cellBroadcastConfigs.MMI = this._convertCellBroadcastSearchList(str);
    } catch (e) {
      if (DEBUG) {
        this.context.debug("Invalid Cell Broadcast search list: " + e);
      }
      options.errorMsg = GECKO_ERROR_UNSPECIFIED_ERROR;
    }

    this.sendChromeMessage(options);
    if (options.errorMsg) {
      return;
    }

    this._mergeAllCellBroadcastConfigs();
  },

  updateCellBroadcastConfig: function() {
    let activate = !this.cellBroadcastDisabled &&
                   (this.mergedCellBroadcastConfig != null) &&
                   (this.mergedCellBroadcastConfig.length > 0);
    if (activate) {
      this.setSmsBroadcastConfig(this.mergedCellBroadcastConfig);
    } else {
      
      this.setSmsBroadcastActivation(false);
    }
  },

  setGsmSmsBroadcastConfig: function(config) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_GSM_SET_BROADCAST_SMS_CONFIG);

    let numConfigs = config ? config.length / 2 : 0;
    Buf.writeInt32(numConfigs);
    for (let i = 0; i < config.length;) {
      
      Buf.writeInt32(config[i++]);
      Buf.writeInt32(config[i++] - 1);
      Buf.writeInt32(0x00);
      Buf.writeInt32(0xFF);
      Buf.writeInt32(1);
    }

    Buf.sendParcel();
  },

  




  setCdmaSmsBroadcastConfig: function(config) {
    let Buf = this.context.Buf;
    
    
    Buf.newParcel(REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG);

    let numConfigs = 0;
    for (let i = 0; i < config.length; i += 2) {
      numConfigs += (config[i+1] - config[i]);
    }

    Buf.writeInt32(numConfigs);
    for (let i = 0; i < config.length;) {
      let begin = config[i++];
      let end = config[i++];

      for (let j = begin; j < end; ++j) {
        Buf.writeInt32(j);
        Buf.writeInt32(0);  
        Buf.writeInt32(1);
      }
    }

    Buf.sendParcel();
  },

  setSmsBroadcastConfig: function(config) {
    if (this._isCdma) {
      this.setCdmaSmsBroadcastConfig(config);
    } else {
      this.setGsmSmsBroadcastConfig(config);
    }
  },

  setSmsBroadcastActivation: function(activate) {
    let parcelType = this._isCdma ? REQUEST_CDMA_SMS_BROADCAST_ACTIVATION :
                                    REQUEST_GSM_SMS_BROADCAST_ACTIVATION;
    let Buf = this.context.Buf;
    Buf.newParcel(parcelType);
    Buf.writeInt32(1);
    
    Buf.writeInt32(activate ? 0 : 1);
    Buf.sendParcel();
  },

  





  startTone: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_DTMF_START, options);
    Buf.writeString(options.dtmfChar);
    Buf.sendParcel();
  },

  stopTone: function() {
    this.context.Buf.simpleRequest(REQUEST_DTMF_STOP);
  },

  





  sendTone: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_DTMF);
    Buf.writeString(options.dtmfChar);
    Buf.sendParcel();
  },

  


  getSmscAddress: function(options) {
    if (!this.SMSC) {
      this.context.Buf.simpleRequest(REQUEST_GET_SMSC_ADDRESS, options);
      return;
    }

    if (!options || options.rilMessageType !== "getSmscAddress") {
      return;
    }

    options.smscAddress = this.SMSC;
    this.sendChromeMessage(options);
  },

  











  setSmscAddress: function(options) {
    let ton = options.typeOfNumber;
    let npi = CALLED_PARTY_BCD_NPI[options.numberPlanIdentification];

    
    
    if (ton === undefined || npi === undefined || !options.smscAddress) {
      options.errorMsg = GECKO_ERROR_INVALID_PARAMETER;
      this.sendChromeMessage(options);
      return;
    }

    
    
    let numStart = options.smscAddress[0] === "+" ? 1 : 0;
    let number = options.smscAddress.substring(0, numStart) +
                 options.smscAddress.substring(numStart)
                                    .replace(/[^0-9*#abc]/ig, "");

    
    if (number.length === 0) {
      options.errorMsg = GECKO_ERROR_INVALID_PARAMETER;
      this.sendChromeMessage(options);
      return;
    }

    
    this.SMSC = null;
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_SMSC_ADDRESS, options);

    
    
    
    let tosca = (0x1 << 7) + (ton << 4) + npi;
    if (RILQUIRKS_SMSC_ADDRESS_FORMAT === "pdu") {
      let pduHelper = this.context.GsmPDUHelper;

      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      let pureNumber = number.substring(numStart)
                             .replace(/c/ig, "e")
                             .replace(/b/ig, "d")
                             .replace(/a/ig, "c")
                             .replace(/\#/g, "b")
                             .replace(/\*/g, "a");

      
      let length = Math.ceil(pureNumber.length / 2) + 1; 
      let strlen = length * 2 + 2; 

      Buf.writeInt32(strlen);
      pduHelper.writeHexOctet(length);
      pduHelper.writeHexOctet(tosca);
      pduHelper.writeSwappedNibbleBCD(pureNumber);
      Buf.writeStringDelimiter(strlen);
    } else  {
      let sca;
      sca = '"' + number + '"' + ',' + tosca;
      Buf.writeString(sca);
    }

    Buf.sendParcel();
  },

  





















  setupDataCall: function(options) {
    
    
    
    
    
    
    
    
    
    let radioTech;
    if (this.v5Legacy) {
      radioTech = this._isCdma ? DATACALL_RADIOTECHNOLOGY_CDMA
                               : DATACALL_RADIOTECHNOLOGY_GSM;
    } else {
      radioTech = options.radioTech + 2;
    }
    let Buf = this.context.Buf;
    let token = Buf.newParcel(REQUEST_SETUP_DATA_CALL, options);
    Buf.writeInt32(7);
    Buf.writeString(radioTech.toString());
    Buf.writeString(DATACALL_PROFILE_DEFAULT.toString());
    Buf.writeString(options.apn);
    Buf.writeString(options.user);
    Buf.writeString(options.passwd);
    Buf.writeString(options.chappap.toString());
    Buf.writeString(options.pdptype);
    Buf.sendParcel();
    return token;
  },

  







  deactivateDataCall: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_DEACTIVATE_DATA_CALL, options);
    Buf.writeInt32(2);
    Buf.writeString(options.cid);
    Buf.writeString(options.reason || DATACALL_DEACTIVATE_NO_REASON);
    Buf.sendParcel();
  },

  


  getDataCallList: function(options) {
    this.context.Buf.simpleRequest(REQUEST_DATA_CALL_LIST, options);
  },

  _attachDataRegistration: false,
  





  setDataRegistration: function(options) {
    this._attachDataRegistration = options.attach;

    if (RILQUIRKS_DATA_REGISTRATION_ON_DEMAND) {
      let request = options.attach ? RIL_REQUEST_GPRS_ATTACH :
                                     RIL_REQUEST_GPRS_DETACH;
      this.context.Buf.simpleRequest(request, options);
      return;
    } else if (RILQUIRKS_SUBSCRIPTION_CONTROL && options.attach) {
      this.context.Buf.simpleRequest(REQUEST_SET_DATA_SUBSCRIPTION, options);
      return;
    }

    
    
    this.sendChromeMessage(options);
  },

  


  getFailCause: function(options) {
    this.context.Buf.simpleRequest(REQUEST_LAST_CALL_FAIL_CAUSE, options);
  },

  sendMMI: function(options) {
    if (DEBUG) {
      this.context.debug("SendMMI " + JSON.stringify(options));
    }

    let _sendMMIError = (function(errorMsg) {
      options.errorMsg = errorMsg;
      this.sendChromeMessage(options);
    }).bind(this);

    
    let mmi = options.mmi;
    if (!mmi && !this._ussdSession) {
      _sendMMIError(MMI_ERROR_KS_ERROR);
      return;
    }

    function _isValidPINPUKRequest() {
      
      
      if (mmi.procedure != MMI_PROCEDURE_REGISTRATION ) {
        _sendMMIError(MMI_ERROR_KS_INVALID_ACTION);
        return false;
      }

      if (!mmi.sia || !mmi.sib || !mmi.sic) {
        _sendMMIError(MMI_ERROR_KS_ERROR);
        return false;
      }

      if (mmi.sia.length < 4 || mmi.sia.length > 8 ||
          mmi.sib.length < 4 || mmi.sib.length > 8 ||
          mmi.sic.length < 4 || mmi.sic.length > 8) {
        _sendMMIError(MMI_ERROR_KS_INVALID_PIN);
        return false;
      }

      if (mmi.sib != mmi.sic) {
        _sendMMIError(MMI_ERROR_KS_MISMATCH_PIN);
        return false;
      }

      return true;
    }

    function _isValidChangePasswordRequest() {
      if (mmi.procedure !== MMI_PROCEDURE_REGISTRATION &&
          mmi.procedure !== MMI_PROCEDURE_ACTIVATION) {
        _sendMMIError(MMI_ERROR_KS_INVALID_ACTION);
        return false;
      }

      if (mmi.sia !== "" && mmi.sia !== MMI_ZZ_BARRING_SERVICE) {
        _sendMMIError(MMI_ERROR_KS_NOT_SUPPORTED);
        return false;
      }

      let validPassword = si => /^[0-9]{4}$/.test(si);
      if (!validPassword(mmi.sib) || !validPassword(mmi.sic) ||
          !validPassword(mmi.pwd)) {
        _sendMMIError(MMI_ERROR_KS_INVALID_PASSWORD);
        return false;
      }

      if (mmi.sic != mmi.pwd) {
        _sendMMIError(MMI_ERROR_KS_MISMATCH_PASSWORD);
        return false;
      }

      return true;
    }

    let _isRadioAvailable = (function() {
      if (this.radioState !== GECKO_RADIOSTATE_ENABLED) {
        _sendMMIError(GECKO_ERROR_RADIO_NOT_AVAILABLE);
        return false;
      }
      return true;
    }).bind(this);

    
    
    let sc = mmi.serviceCode;
    switch (sc) {
      
      case MMI_SC_CFU:
      case MMI_SC_CF_BUSY:
      case MMI_SC_CF_NO_REPLY:
      case MMI_SC_CF_NOT_REACHABLE:
      case MMI_SC_CF_ALL:
      case MMI_SC_CF_ALL_CONDITIONAL:
        if (!_isRadioAvailable()) {
          return;
        }
        
        
        
        
        options.action = MMI_PROC_TO_CF_ACTION[mmi.procedure];
        options.reason = MMI_SC_TO_CF_REASON[sc];
        options.number = mmi.sia;
        options.serviceClass = this._siToServiceClass(mmi.sib);
        if (options.action == CALL_FORWARD_ACTION_QUERY_STATUS) {
          this.queryCallForwardStatus(options);
          return;
        }

        options.isSetCallForward = true;
        options.timeSeconds = mmi.sic;
        this.setCallForward(options);
        return;

      
      case MMI_SC_PIN:
        
        
        
        
        if (!_isRadioAvailable() || !_isValidPINPUKRequest()) {
          return;
        }

        options.password = mmi.sia;
        options.newPassword = mmi.sib;
        this.changeICCPIN(options);
        return;

      
      case MMI_SC_PIN2:
        
        
        
        
        if (!_isRadioAvailable() || !_isValidPINPUKRequest()) {
          return;
        }

        options.password = mmi.sia;
        options.newPassword = mmi.sib;
        this.changeICCPIN2(options);
        return;

      
      case MMI_SC_PUK:
        
        
        
        
        if (!_isRadioAvailable() || !_isValidPINPUKRequest()) {
          return;
        }

        options.password = mmi.sia;
        options.newPin = mmi.sib;
        this.enterICCPUK(options);
        return;

      
      case MMI_SC_PUK2:
        
        
        
        
        if (!_isRadioAvailable() || !_isValidPINPUKRequest()) {
          return;
        }

        options.password = mmi.sia;
        options.newPin = mmi.sib;
        this.enterICCPUK2(options);
        return;

      
      case MMI_SC_IMEI:
        
        if (this.IMEI == null) {
          this.getIMEI(options);
          return;
        }
        
        options.statusMessage = this.IMEI;
        this.sendChromeMessage(options);
        return;

      
      case MMI_SC_CLIP:
        options.procedure = mmi.procedure;
        if (options.procedure === MMI_PROCEDURE_INTERROGATION) {
          this.queryCLIP(options);
        } else {
          _sendMMIError(MMI_ERROR_KS_NOT_SUPPORTED);
        }
        return;

      
      
      
      
      case MMI_SC_CLIR:
        options.procedure = mmi.procedure;
        switch (options.procedure) {
          case MMI_PROCEDURE_INTERROGATION:
            this.getCLIR(options);
            return;
          case MMI_PROCEDURE_ACTIVATION:
            options.clirMode = CLIR_INVOCATION;
            break;
          case MMI_PROCEDURE_DEACTIVATION:
            options.clirMode = CLIR_SUPPRESSION;
            break;
          default:
            _sendMMIError(MMI_ERROR_KS_NOT_SUPPORTED);
            return;
        }
        options.isSetCLIR = true;
        this.setCLIR(options);
        return;

      
      case MMI_SC_CHANGE_PASSWORD:
        if (!_isRadioAvailable() || !_isValidChangePasswordRequest()) {
          return;
        }

        options.pin = mmi.sib;
        options.newPin = mmi.sic;
        this.changeCallBarringPassword(options);
        return;

      
      case MMI_SC_BAOC:
      case MMI_SC_BAOIC:
      case MMI_SC_BAOICxH:
      case MMI_SC_BAIC:
      case MMI_SC_BAICr:
      case MMI_SC_BA_ALL:
      case MMI_SC_BA_MO:
      case MMI_SC_BA_MT:
        options.password = mmi.sia || "";
        options.serviceClass = this._siToServiceClass(mmi.sib);
        options.facility = MMI_SC_TO_CB_FACILITY[sc];
        options.procedure = mmi.procedure;
        if (mmi.procedure === MMI_PROCEDURE_INTERROGATION) {
          this.queryICCFacilityLock(options);
          return;
        }
        if (mmi.procedure === MMI_PROCEDURE_ACTIVATION) {
          options.enabled = 1;
        } else if (mmi.procedure === MMI_PROCEDURE_DEACTIVATION) {
          options.enabled = 0;
        } else {
          _sendMMIError(MMI_ERROR_KS_NOT_SUPPORTED);
          return;
        }
        this.setICCFacilityLock(options);
        return;

      
      case MMI_SC_CALL_WAITING:
        if (!_isRadioAvailable()) {
          return;
        }


        if (mmi.procedure === MMI_PROCEDURE_INTERROGATION) {
          this._handleQueryMMICallWaiting(options);
          return;
        }

        if (mmi.procedure === MMI_PROCEDURE_ACTIVATION) {
          options.enabled = true;
        } else if (mmi.procedure === MMI_PROCEDURE_DEACTIVATION) {
          options.enabled = false;
        } else {
          _sendMMIError(MMI_ERROR_KS_NOT_SUPPORTED);
          return;
        }

        options.serviceClass = this._siToServiceClass(mmi.sia);
        this._handleSetMMICallWaiting(options);
        return;
    }

    
    if (!_isRadioAvailable()) {
      return;
    }

    options.ussd = mmi.fullMMI;

    if (this._ussdSession) {
      if (DEBUG) this.context.debug("Cancel existing ussd session.");
      this.cachedUSSDRequest = options;
      this.cancelUSSD({});
      return;
    }

    this.sendUSSD(options, false);
  },

  



  cachedUSSDRequest : null,

  





  sendUSSD: function(options, checkSession = true) {
    if (checkSession && !this._ussdSession) {
      options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
      this.sendChromeMessage(options);
      return;
    }

    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SEND_USSD, options);
    Buf.writeString(options.ussd);
    Buf.sendParcel();
  },

  


   cancelUSSD: function(options) {
     this.context.Buf.simpleRequest(REQUEST_CANCEL_USSD, options);
   },

  









  queryCallForwardStatus: function(options) {
    let Buf = this.context.Buf;
    let number = options.number || "";
    Buf.newParcel(REQUEST_QUERY_CALL_FORWARD_STATUS, options);
    Buf.writeInt32(CALL_FORWARD_ACTION_QUERY_STATUS);
    Buf.writeInt32(options.reason);
    Buf.writeInt32(options.serviceClass || ICC_SERVICE_CLASS_NONE);
    Buf.writeInt32(this._toaFromString(number));
    Buf.writeString(number);
    Buf.writeInt32(0);
    Buf.sendParcel();
  },

  













  setCallForward: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_SET_CALL_FORWARD, options);
    Buf.writeInt32(options.action);
    Buf.writeInt32(options.reason);
    Buf.writeInt32(options.serviceClass);
    Buf.writeInt32(this._toaFromString(options.number));
    Buf.writeString(options.number);
    Buf.writeInt32(options.timeSeconds);
    Buf.sendParcel();
  },

  







  queryCallBarringStatus: function(options) {
    options.facility = CALL_BARRING_PROGRAM_TO_FACILITY[options.program];
    options.password = ""; 

    
    
    
    options.queryServiceClass = options.serviceClass;
    options.serviceClass = 0;

    this.queryICCFacilityLock(options);
  },

  











  setCallBarring: function(options) {
    options.facility = CALL_BARRING_PROGRAM_TO_FACILITY[options.program];
    this.setICCFacilityLock(options);
  },

  







  changeCallBarringPassword: function(options) {
    let Buf = this.context.Buf;
    Buf.newParcel(REQUEST_CHANGE_BARRING_PASSWORD, options);
    Buf.writeInt32(3);
    
    
    Buf.writeString(ICC_CB_FACILITY_BA_ALL);
    Buf.writeString(options.pin);
    Buf.writeString(options.newPin);
    Buf.sendParcel();
  },

  





  stkHandleCallSetup: function(options) {
     let Buf = this.context.Buf;
     Buf.newParcel(REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM);
     Buf.writeInt32(1);
     Buf.writeInt32(options.hasConfirmed ? 1 : 0);
     Buf.sendParcel();
  },

  




  sendStkTerminalProfile: function(profile) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    Buf.newParcel(REQUEST_STK_SET_PROFILE);
    Buf.writeInt32(profile.length * 2);
    for (let i = 0; i < profile.length; i++) {
      GsmPDUHelper.writeHexOctet(profile[i]);
    }
    Buf.writeInt32(0);
    Buf.sendParcel();
  },

  













  sendStkTerminalResponse: function(response) {
    if (response.hasConfirmed !== undefined) {
      this.stkHandleCallSetup(response);
      return;
    }

    let Buf = this.context.Buf;
    let ComprehensionTlvHelper = this.context.ComprehensionTlvHelper;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let command = response.command;
    Buf.newParcel(REQUEST_STK_SEND_TERMINAL_RESPONSE);

    
    Buf.startCalOutgoingSize(function(size) {
      
      Buf.writeInt32(size / 2);
    });

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_COMMAND_DETAILS |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(3);
    if (command) {
      GsmPDUHelper.writeHexOctet(command.commandNumber);
      GsmPDUHelper.writeHexOctet(command.typeOfCommand);
      GsmPDUHelper.writeHexOctet(command.commandQualifier);
    } else {
      GsmPDUHelper.writeHexOctet(0x00);
      GsmPDUHelper.writeHexOctet(0x00);
      GsmPDUHelper.writeHexOctet(0x00);
    }

    
    
    
    
    
    
    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DEVICE_ID);
    GsmPDUHelper.writeHexOctet(2);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_ME);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_SIM);

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_RESULT |
                               COMPREHENSIONTLV_FLAG_CR);
    if ("additionalInformation" in response) {
      
      
      
      
      
      
      
      
      
      
      
      GsmPDUHelper.writeHexOctet(2);
      GsmPDUHelper.writeHexOctet(response.resultCode);
      GsmPDUHelper.writeHexOctet(response.additionalInformation);
    } else {
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(response.resultCode);
    }

    
    if (response.itemIdentifier != null) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ITEM_ID |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(response.itemIdentifier);
    }

    
    if (response.resultCode != STK_RESULT_HELP_INFO_REQUIRED) {
      let text;
      let coding = command.options.isUCS2 ?
                       STK_TEXT_CODING_UCS2 :
                       (command.options.isPacked ?
                          STK_TEXT_CODING_GSM_7BIT_PACKED :
                          STK_TEXT_CODING_GSM_8BIT);
      if (response.isYesNo !== undefined) {
        
        
        
        
        
        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TEXT_STRING |
                                   COMPREHENSIONTLV_FLAG_CR);
        
        GsmPDUHelper.writeHexOctet(2);
        
        GsmPDUHelper.writeHexOctet(coding);
        GsmPDUHelper.writeHexOctet(response.isYesNo ? 0x01 : 0x00);
      } else {
        if (response.input !== undefined) {
            ComprehensionTlvHelper.writeTextStringTlv(response.input, coding);
        }
      }
    }

    
    if (response.resultCode === STK_RESULT_NO_RESPONSE_FROM_USER) {
      
      
      
      
      
      
      
      let duration = command && command.options && command.options.duration;
      if (duration) {
        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DURATION);
        GsmPDUHelper.writeHexOctet(2);
        GsmPDUHelper.writeHexOctet(duration.timeUnit);
        GsmPDUHelper.writeHexOctet(duration.timeInterval);
      }
    }

    
    if (response.localInfo) {
      let localInfo = response.localInfo;

      
      if (localInfo.locationInfo) {
        ComprehensionTlvHelper.writeLocationInfoTlv(localInfo.locationInfo);
      }

      
      if (localInfo.imei != null) {
        let imei = localInfo.imei;
        if (imei.length == 15) {
          imei = imei + "0";
        }

        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_IMEI);
        GsmPDUHelper.writeHexOctet(8);
        for (let i = 0; i < imei.length / 2; i++) {
          GsmPDUHelper.writeHexOctet(parseInt(imei.substr(i * 2, 2), 16));
        }
      }

      
      if (localInfo.date != null) {
        ComprehensionTlvHelper.writeDateTimeZoneTlv(localInfo.date);
      }

      
      if (localInfo.language) {
        ComprehensionTlvHelper.writeLanguageTlv(localInfo.language);
      }
    }

    
    if (response.timer) {
      let timer = response.timer;

      if (timer.timerId) {
        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER);
        GsmPDUHelper.writeHexOctet(1);
        GsmPDUHelper.writeHexOctet(timer.timerId);
      }

      if (timer.timerValue) {
        ComprehensionTlvHelper.writeTimerValueTlv(timer.timerValue, false);
      }
    }

    
    Buf.stopCalOutgoingSize();

    Buf.writeInt32(0);
    Buf.sendParcel();
  },

  





  sendStkMenuSelection: function(command) {
    command.tag = BER_MENU_SELECTION_TAG;
    command.deviceId = {
      sourceId :STK_DEVICE_ID_KEYPAD,
      destinationId: STK_DEVICE_ID_SIM
    };
    this.sendICCEnvelopeCommand(command);
  },

  




  sendStkTimerExpiration: function(command) {
    command.tag = BER_TIMER_EXPIRATION_TAG;
    command.deviceId = {
      sourceId: STK_DEVICE_ID_ME,
      destinationId: STK_DEVICE_ID_SIM
    };
    command.timerId = command.timer.timerId;
    command.timerValue = command.timer.timerValue;
    this.sendICCEnvelopeCommand(command);
  },

  



  sendStkEventDownload: function(command) {
    command.tag = BER_EVENT_DOWNLOAD_TAG;
    command.eventList = command.event.eventType;
    switch (command.eventList) {
      case STK_EVENT_TYPE_LOCATION_STATUS:
        command.deviceId = {
          sourceId :STK_DEVICE_ID_ME,
          destinationId: STK_DEVICE_ID_SIM
        };
        command.locationStatus = command.event.locationStatus;
        
        if (command.locationStatus == STK_SERVICE_STATE_NORMAL) {
          command.locationInfo = command.event.locationInfo;
        }
        break;
      case STK_EVENT_TYPE_MT_CALL:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_NETWORK,
          destinationId: STK_DEVICE_ID_SIM
        };
        command.transactionId = 0;
        command.address = command.event.number;
        break;
      case STK_EVENT_TYPE_CALL_DISCONNECTED:
        command.cause = command.event.error;
        
      case STK_EVENT_TYPE_CALL_CONNECTED:
        command.deviceId = {
          sourceId: (command.event.isIssuedByRemote ?
                     STK_DEVICE_ID_NETWORK : STK_DEVICE_ID_ME),
          destinationId: STK_DEVICE_ID_SIM
        };
        command.transactionId = 0;
        break;
      case STK_EVENT_TYPE_USER_ACTIVITY:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_ME,
          destinationId: STK_DEVICE_ID_SIM
        };
        break;
      case STK_EVENT_TYPE_IDLE_SCREEN_AVAILABLE:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_DISPLAY,
          destinationId: STK_DEVICE_ID_SIM
        };
        break;
      case STK_EVENT_TYPE_LANGUAGE_SELECTION:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_ME,
          destinationId: STK_DEVICE_ID_SIM
        };
        command.language = command.event.language;
        break;
      case STK_EVENT_TYPE_BROWSER_TERMINATION:
        command.deviceId = {
          sourceId: STK_DEVICE_ID_ME,
          destinationId: STK_DEVICE_ID_SIM
        };
        command.terminationCause = command.event.terminationCause;
        break;
    }
    this.sendICCEnvelopeCommand(command);
  },

  
















  sendICCEnvelopeCommand: function(options) {
    if (DEBUG) {
      this.context.debug("Stk Envelope " + JSON.stringify(options));
    }

    let Buf = this.context.Buf;
    let ComprehensionTlvHelper = this.context.ComprehensionTlvHelper;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    Buf.newParcel(REQUEST_STK_SEND_ENVELOPE_COMMAND);

    
    Buf.startCalOutgoingSize(function(size) {
      
      Buf.writeInt32(size / 2);
    });

    
    GsmPDUHelper.writeHexOctet(options.tag);
    
    Buf.startCalOutgoingSize(function(size) {
      
      GsmPDUHelper.writeHexOctet(size / 4);
    });

    
    if (options.eventList != null) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_EVENT_LIST |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.eventList);
    }

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DEVICE_ID |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(2);
    GsmPDUHelper.writeHexOctet(options.deviceId.sourceId);
    GsmPDUHelper.writeHexOctet(options.deviceId.destinationId);

    
    if (options.itemIdentifier != null) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ITEM_ID |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.itemIdentifier);
    }

    
    if (options.helpRequested) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_HELP_REQUEST |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(0);
      
    }

    
    if (options.locationStatus != null) {
      let len = options.locationStatus.length;
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_LOCATION_STATUS |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.locationStatus);
    }

    
    if (options.locationInfo) {
      ComprehensionTlvHelper.writeLocationInfoTlv(options.locationInfo);
    }

    
    if (options.transactionId != null) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TRANSACTION_ID |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.transactionId);
    }

    
    if (options.address) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ADDRESS |
                                 COMPREHENSIONTLV_FLAG_CR);
      ComprehensionTlvHelper.writeLength(
        Math.ceil(options.address.length/2) + 1 
      );
      this.context.ICCPDUHelper.writeDiallingNumber(options.address);
    }

    
    if (options.cause != null) {
      ComprehensionTlvHelper.writeCauseTlv(options.cause);
    }

    
    if (options.timerId != null) {
        GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER |
                                   COMPREHENSIONTLV_FLAG_CR);
        GsmPDUHelper.writeHexOctet(1);
        GsmPDUHelper.writeHexOctet(options.timerId);
    }

    
    if (options.timerValue != null) {
        ComprehensionTlvHelper.writeTimerValueTlv(options.timerValue, true);
    }

    
    if (options.language) {
      ComprehensionTlvHelper.writeLanguageTlv(options.language);
    }

    
    if (options.terminationCause != null) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_BROWSER_TERMINATION_CAUSE |
                                 COMPREHENSIONTLV_FLAG_CR);
      GsmPDUHelper.writeHexOctet(1);
      GsmPDUHelper.writeHexOctet(options.terminationCause);
    }

    
    Buf.stopCalOutgoingSize();

    
    Buf.stopCalOutgoingSize();

    Buf.writeInt32(0);
    Buf.sendParcel();
  },

  


  reportStkServiceIsRunning: function() {
    this.context.Buf.simpleRequest(REQUEST_REPORT_STK_SERVICE_IS_RUNNING);
  },

  


  _processICCStatus: function(iccStatus) {
    
    
    
    
    if (this._waitingRadioTech) {
      return;
    }

    
    
    if (iccStatus.cardState !== CARD_STATE_PRESENT) {
      if (this.cardState !== GECKO_CARDSTATE_UNDETECTED) {
        this.operator = null;
        
        
        this.cardState = GECKO_CARDSTATE_UNDETECTED;
        this.sendChromeMessage({rilMessageType: "cardstatechange",
                                cardState: this.cardState});

        this.iccInfo = {iccType: null};
        this.context.ICCUtilsHelper.handleICCInfoChange();
      }
      return;
    }

    if (RILQUIRKS_SUBSCRIPTION_CONTROL) {
      
      
      
      let neetToActivate = iccStatus.cdmaSubscriptionAppIndex === -1 &&
                           iccStatus.gsmUmtsSubscriptionAppIndex === -1;
      if (neetToActivate &&
          
          
          
          this.radioState === GECKO_RADIOSTATE_ENABLED) {
        for (let i = 0; i < iccStatus.apps.length; i++) {
          this.setUiccSubscription({appIndex: i, enabled: true});
        }
      }
    }

    let newCardState;
    let index = this._isCdma ? iccStatus.cdmaSubscriptionAppIndex
                             : iccStatus.gsmUmtsSubscriptionAppIndex;
    let app = iccStatus.apps[index];
    if (app) {
      
      this.aid = app.aid;
      this.appType = app.app_type;
      this.iccInfo.iccType = GECKO_CARD_TYPE[this.appType];

      switch (app.app_state) {
        case CARD_APPSTATE_ILLEGAL:
          newCardState = GECKO_CARDSTATE_ILLEGAL;
          break;
        case CARD_APPSTATE_PIN:
          newCardState = GECKO_CARDSTATE_PIN_REQUIRED;
          break;
        case CARD_APPSTATE_PUK:
          newCardState = GECKO_CARDSTATE_PUK_REQUIRED;
          break;
        case CARD_APPSTATE_SUBSCRIPTION_PERSO:
          newCardState = PERSONSUBSTATE[app.perso_substate];
          break;
        case CARD_APPSTATE_READY:
          newCardState = GECKO_CARDSTATE_READY;
          break;
        case CARD_APPSTATE_UNKNOWN:
        case CARD_APPSTATE_DETECTED:
          
        default:
          newCardState = GECKO_CARDSTATE_UNKNOWN;
      }

      let pin1State = app.pin1_replaced ? iccStatus.universalPINState :
                                          app.pin1;
      if (pin1State === CARD_PINSTATE_ENABLED_PERM_BLOCKED) {
        newCardState = GECKO_CARDSTATE_PERMANENT_BLOCKED;
      }
    } else {
      
      newCardState = GECKO_CARDSTATE_UNKNOWN;
    }

    let ICCRecordHelper = this.context.ICCRecordHelper;
    
    if (iccStatus.cardState === CARD_STATE_PRESENT &&
        (this.cardState === GECKO_CARDSTATE_UNINITIALIZED ||
         this.cardState === GECKO_CARDSTATE_UNDETECTED)) {
      ICCRecordHelper.readICCID();
    }

    if (this.cardState == newCardState) {
      return;
    }

    
    this.requestNetworkInfo();
    if (newCardState == GECKO_CARDSTATE_READY) {
      
      
      if (this.appType == CARD_APPTYPE_SIM) {
        this.context.SimRecordHelper.readSimPhase();
      } else if (RILQUIRKS_SEND_STK_PROFILE_DOWNLOAD) {
        this.sendStkTerminalProfile(STK_SUPPORTED_TERMINAL_PROFILE);
      }

      ICCRecordHelper.fetchICCRecords();
    }

    this.cardState = newCardState;
    this.sendChromeMessage({rilMessageType: "cardstatechange",
                            cardState: this.cardState});
  },

   


  _processEnterAndChangeICCResponses: function(length, options) {
    options.retryCount = length ? this.context.Buf.readInt32List()[0] : -1;
    if (options.rilMessageType != "sendMMI") {
      this.sendChromeMessage(options);
      return;
    }

    let serviceCode = options.mmi.serviceCode;

    if (!options.errorMsg) {
      switch (serviceCode) {
        case MMI_SC_PIN:
          options.statusMessage = MMI_SM_KS_PIN_CHANGED;
          break;
        case MMI_SC_PIN2:
          options.statusMessage = MMI_SM_KS_PIN2_CHANGED;
          break;
        case MMI_SC_PUK:
          options.statusMessage = MMI_SM_KS_PIN_UNBLOCKED;
          break;
        case MMI_SC_PUK2:
          options.statusMessage = MMI_SM_KS_PIN2_UNBLOCKED;
          break;
      }
    } else {
      if (options.retryCount <= 0) {
        if (serviceCode === MMI_SC_PUK) {
          options.errorMsg = MMI_ERROR_KS_SIM_BLOCKED;
        } else if (serviceCode === MMI_SC_PIN) {
          options.errorMsg = MMI_ERROR_KS_NEEDS_PUK;
        }
      } else {
        if (serviceCode === MMI_SC_PIN || serviceCode === MMI_SC_PIN2) {
          options.errorMsg = MMI_ERROR_KS_BAD_PIN;
        } else if (serviceCode === MMI_SC_PUK || serviceCode === MMI_SC_PUK2) {
          options.errorMsg = MMI_ERROR_KS_BAD_PUK;
        }
        if (options.retryCount !== undefined) {
          options.additionalInformation = options.retryCount;
        }
      }
    }

    this.sendChromeMessage(options);
  },

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  _sendNetworkInfoMessage: function(type, message) {
    if (!this._processingNetworkInfo) {
      
      
      this.sendChromeMessage(message);
      return;
    }

    if (DEBUG) {
      this.context.debug("Queuing " + type + " network info message: " +
                         JSON.stringify(message));
    }
    this._pendingNetworkInfo[type] = message;
  },

  _receivedNetworkInfo: function(type) {
    if (DEBUG) this.context.debug("Received " + type + " network info.");
    if (!this._processingNetworkInfo) {
      return;
    }

    let pending = this._pendingNetworkInfo;

    
    if (!(type in pending)) {
      pending[type] = this.pendingNetworkType;
    }

    
    
    for (let i = 0; i < NETWORK_INFO_MESSAGE_TYPES.length; i++) {
      let msgType = NETWORK_INFO_MESSAGE_TYPES[i];
      if (!(msgType in pending)) {
        if (DEBUG) {
          this.context.debug("Still missing some more network info, not " +
                             "notifying main thread.");
        }
        return;
      }
    }

    
    
    
    for (let key in pending) {
      if (pending[key] == this.pendingNetworkType) {
        delete pending[key];
      }
    }

    if (DEBUG) {
      this.context.debug("All pending network info has been received: " +
                         JSON.stringify(pending));
    }

    
    
    setTimeout(this._sendPendingNetworkInfo.bind(this), 0);
  },

  _sendPendingNetworkInfo: function() {
    this.sendChromeMessage(this._pendingNetworkInfo);

    this._processingNetworkInfo = false;
    for (let i = 0; i < NETWORK_INFO_MESSAGE_TYPES.length; i++) {
      delete this._pendingNetworkInfo[NETWORK_INFO_MESSAGE_TYPES[i]];
    }

    if (this._needRepollNetworkInfo) {
      this._needRepollNetworkInfo = false;
      this.requestNetworkInfo();
    }
  },

  












  _processSignalLevel: function(signal, min, max) {
    if (signal <= min) {
      return 0;
    }

    if (signal >= max) {
      return 100;
    }

    return Math.floor((signal - min) * 100 / (max - min));
  },

  












  _processLteSignal: function(signal) {
    let info = {
      voice: {
        signalStrength:    null,
        relSignalStrength: null
      },
      data: {
        signalStrength:    null,
        relSignalStrength: null
      }
    };

    
    let signalStrength = (signal.lteRSRP === undefined || signal.lteRSRP === 0x7FFFFFFF) ?
                         null : signal.lteRSRP;
    info.voice.signalStrength = info.data.signalStrength = signalStrength;

    
    
    let rsrpLevel = -1;
    let rssnrLevel = -1;
    if (signal.lteRSRP !== undefined &&
        signal.lteRSRP !== 0x7FFFFFFF &&
        signal.lteRSRP >= 44 &&
        signal.lteRSRP <= 140) {
      rsrpLevel = this._processSignalLevel(signal.lteRSRP * -1, -115, -85);
    }

    if (signal.lteRSSNR !== undefined &&
        signal.lteRSSNR !== 0x7FFFFFFF &&
        signal.lteRSSNR >= -200 &&
        signal.lteRSSNR <= 300) {
      rssnrLevel = this._processSignalLevel(signal.lteRSSNR, -30, 130);
    }

    if (rsrpLevel !== -1 && rssnrLevel !== -1) {
      info.voice.relSignalStrength = info.data.relSignalStrength =
        Math.min(rsrpLevel, rssnrLevel);
      return info;
    }

    let level = Math.max(rsrpLevel, rssnrLevel);
    if (level !== -1) {
      info.voice.relSignalStrength = info.data.relSignalStrength = level;
      return info;
    }

    
    if (signal.lteSignalStrength !== undefined &&
        signal.lteSignalStrength >= 0 &&
        signal.lteSignalStrength <= 63) {
      level = this._processSignalLevel(signal.lteSignalStrength, 0, 12);
      info.voice.relSignalStrength = info.data.relSignalStrength = level;
      return info;
    }

    return null;
  },

  _processSignalStrength: function(signal) {
    let info = {
      voice: {
        signalStrength:    null,
        relSignalStrength: null
      },
      data: {
        signalStrength:    null,
        relSignalStrength: null
      }
    };

    
    
    if (("radioTech" in this.voiceRegistrationState) &&
        !this._isGsmTechGroup(this.voiceRegistrationState.radioTech)) {
      
      
      
      
      if (signal.cdmaDBM && signal.cdmaDBM > 0) {
        let signalStrength = -1 * signal.cdmaDBM;
        info.voice.signalStrength = signalStrength;

        
        
        let signalLevel = this._processSignalLevel(signalStrength, -105, -70);
        info.voice.relSignalStrength = signalLevel;
      }

      
      
      
      
      if (signal.evdoDBM && signal.evdoDBM > 0) {
        let signalStrength = -1 * signal.evdoDBM;
        info.data.signalStrength = signalStrength;

        
        
        let signalLevel = this._processSignalLevel(signalStrength, -105, -70);
        info.data.relSignalStrength = signalLevel;
      }
    } else {
      
      
      let lteInfo = this._processLteSignal(signal);
      if (lteInfo) {
        info = lteInfo;
      } else {
        
        
        
        
        
        
        if (signal.gsmSignalStrength &&
            signal.gsmSignalStrength >= 0 &&
            signal.gsmSignalStrength <= 31) {
          let signalStrength = -113 + 2 * signal.gsmSignalStrength;
          info.voice.signalStrength = info.data.signalStrength = signalStrength;

          
          
          let signalLevel = this._processSignalLevel(signalStrength, -110, -85);
          info.voice.relSignalStrength = info.data.relSignalStrength = signalLevel;
        }
      }
    }

    info.rilMessageType = "signalstrengthchange";
    this._sendNetworkInfoMessage(NETWORK_INFO_SIGNAL, info);
  },

  




  _processCREG: function(curState, newState) {
    let changed = false;

    let regState = this.parseInt(newState[0], NETWORK_CREG_STATE_UNKNOWN);
    if (curState.regState === undefined || curState.regState !== regState) {
      changed = true;
      curState.regState = regState;

      curState.state = NETWORK_CREG_TO_GECKO_MOBILE_CONNECTION_STATE[regState];
      curState.connected = regState == NETWORK_CREG_STATE_REGISTERED_HOME ||
                           regState == NETWORK_CREG_STATE_REGISTERED_ROAMING;
      curState.roaming = regState == NETWORK_CREG_STATE_REGISTERED_ROAMING;
      curState.emergencyCallsOnly = !curState.connected;
    }

    if (!curState.cell) {
      curState.cell = {};
    }

    
    
    let lac = this.parseInt(newState[1], -1, 16);
    if (curState.cell.gsmLocationAreaCode === undefined ||
        curState.cell.gsmLocationAreaCode !== lac) {
      curState.cell.gsmLocationAreaCode = lac;
      changed = true;
    }

    let cid = this.parseInt(newState[2], -1, 16);
    if (curState.cell.gsmCellId === undefined ||
        curState.cell.gsmCellId !== cid) {
      curState.cell.gsmCellId = cid;
      changed = true;
    }

    let radioTech = (newState[3] === undefined ?
                     NETWORK_CREG_TECH_UNKNOWN :
                     this.parseInt(newState[3], NETWORK_CREG_TECH_UNKNOWN));
    if (curState.radioTech === undefined || curState.radioTech !== radioTech) {
      changed = true;
      curState.radioTech = radioTech;
      curState.type = GECKO_RADIO_TECH[radioTech] || null;
    }
    return changed;
  },

  _processVoiceRegistrationState: function(state) {
    let rs = this.voiceRegistrationState;
    let stateChanged = this._processCREG(rs, state);
    if (stateChanged && rs.connected) {
      this.getSmscAddress();
    }

    let cell = rs.cell;
    if (this._isCdma) {
      
      
      let cdmaBaseStationId = this.parseInt(state[4], -1);
      let cdmaBaseStationLatitude = this.parseInt(state[5], -2147483648);
      let cdmaBaseStationLongitude = this.parseInt(state[6], -2147483648);
      
      let cdmaSystemId = this.parseInt(state[8], -1);
      let cdmaNetworkId = this.parseInt(state[9], -1);
      
      
      
      

      if (cell.cdmaBaseStationId !== cdmaBaseStationId ||
          cell.cdmaBaseStationLatitude !== cdmaBaseStationLatitude ||
          cell.cdmaBaseStationLongitude !== cdmaBaseStationLongitude ||
          cell.cdmaSystemId !== cdmaSystemId ||
          cell.cdmaNetworkId !== cdmaNetworkId) {
        stateChanged = true;
        cell.cdmaBaseStationId = cdmaBaseStationId;
        cell.cdmaBaseStationLatitude = cdmaBaseStationLatitude;
        cell.cdmaBaseStationLongitude = cdmaBaseStationLongitude;
        cell.cdmaSystemId = cdmaSystemId;
        cell.cdmaNetworkId = cdmaNetworkId;
      }
    }

    if (stateChanged) {
      rs.rilMessageType = "voiceregistrationstatechange";
      this._sendNetworkInfoMessage(NETWORK_INFO_VOICE_REGISTRATION_STATE, rs);
    }
  },

  _processDataRegistrationState: function(state) {
    let rs = this.dataRegistrationState;
    let stateChanged = this._processCREG(rs, state);
    if (stateChanged) {
      rs.rilMessageType = "dataregistrationstatechange";
      this._sendNetworkInfoMessage(NETWORK_INFO_DATA_REGISTRATION_STATE, rs);
    }
  },

  _processOperator: function(operatorData) {
    if (operatorData.length < 3) {
      if (DEBUG) {
        this.context.debug("Expected at least 3 strings for operator.");
      }
    }

    if (!this.operator) {
      this.operator = {
        rilMessageType: "operatorchange",
        longName: null,
        shortName: null
      };
    }

    let [longName, shortName, networkTuple] = operatorData;
    let thisTuple = (this.operator.mcc || "") + (this.operator.mnc || "");

    if (this.operator.longName !== longName ||
        this.operator.shortName !== shortName ||
        thisTuple !== networkTuple) {

      this.operator.mcc = null;
      this.operator.mnc = null;

      if (networkTuple) {
        try {
          this._processNetworkTuple(networkTuple, this.operator);
        } catch (e) {
          if (DEBUG) this.context.debug("Error processing operator tuple: " + e);
        }
      } else {
        
        
        
        if (DEBUG) {
          this.context.debug("Operator is currently unregistered");
        }
      }

      this.operator.longName = longName;
      this.operator.shortName = shortName;

      let ICCUtilsHelper = this.context.ICCUtilsHelper;
      if (ICCUtilsHelper.updateDisplayCondition()) {
        ICCUtilsHelper.handleICCInfoChange();
      }

      
      
      
      if (!this.overrideICCNetworkName()) {
        this._sendNetworkInfoMessage(NETWORK_INFO_OPERATOR, this.operator);
      }
    }
  },

  _setDataCallGeckoState: function(datacall) {
    switch (datacall.active) {
      case DATACALL_INACTIVE:
        datacall.state = GECKO_NETWORK_STATE_DISCONNECTED;
        break;
      case DATACALL_ACTIVE_DOWN:
      case DATACALL_ACTIVE_UP:
        datacall.state = GECKO_NETWORK_STATE_CONNECTED;
        break;
    }
  },

  _processSuppSvcNotification: function(info) {
    if (DEBUG) {
      this.context.debug("handle supp svc notification: " + JSON.stringify(info));
    }

    if (info.notificationType !== 1) {
      
      
      
      
      return;
    }

    let notification = null;

    switch (info.code) {
      case SUPP_SVC_NOTIFICATION_CODE2_PUT_ON_HOLD:
      case SUPP_SVC_NOTIFICATION_CODE2_RETRIEVED:
        notification = GECKO_SUPP_SVC_NOTIFICATION_FROM_CODE2[info.code];
        break;
      default:
        
        return;
    }

    let message = {rilMessageType: "suppSvcNotification",
                   number: info.number,  
                   notification: notification};
    this.sendChromeMessage(message);
  },

  _cancelEmergencyCbModeTimeout: function() {
    if (this._exitEmergencyCbModeTimeoutID) {
      clearTimeout(this._exitEmergencyCbModeTimeoutID);
      this._exitEmergencyCbModeTimeoutID = null;
    }
  },

  _handleChangedEmergencyCbMode: function(active) {
    this._isInEmergencyCbMode = active;

    
    this._cancelEmergencyCbModeTimeout();

    
    if (active) {
      this._exitEmergencyCbModeTimeoutID = setTimeout(
          this.exitEmergencyCbMode.bind(this), EMERGENCY_CB_MODE_TIMEOUT_MS);
    }

    let message = {rilMessageType: "emergencyCbModeChange",
                   active: active,
                   timeoutMs: EMERGENCY_CB_MODE_TIMEOUT_MS};
    this.sendChromeMessage(message);
  },

  _updateNetworkSelectionMode: function(mode) {
    if (this.networkSelectionMode === mode) {
      return;
    }

    let options = {
      rilMessageType: "networkselectionmodechange",
      mode: mode
    };
    this.networkSelectionMode = mode;
    this._sendNetworkInfoMessage(NETWORK_INFO_NETWORK_SELECTION_MODE, options);
  },

  _processNetworks: function() {
    let strings = this.context.Buf.readStringList();
    let networks = [];

    for (let i = 0; i < strings.length; i += 4) {
      let network = {
        longName: strings[i],
        shortName: strings[i + 1],
        mcc: null,
        mnc: null,
        state: null
      };

      let networkTuple = strings[i + 2];
      try {
        this._processNetworkTuple(networkTuple, network);
      } catch (e) {
        if (DEBUG) this.context.debug("Error processing operator tuple: " + e);
      }

      let state = strings[i + 3];
      network.state = RIL_QAN_STATE_TO_GECKO_STATE[state];

      networks.push(network);
    }
    return networks;
  },

  





  _processNetworkTuple: function(networkTuple, network) {
    let tupleLen = networkTuple.length;

    if (tupleLen == 5 || tupleLen == 6) {
      network.mcc = networkTuple.substr(0, 3);
      network.mnc = networkTuple.substr(3);
    } else {
      network.mcc = null;
      network.mnc = null;

      throw new Error("Invalid network tuple (should be 5 or 6 digits): " + networkTuple);
    }
  },

  


  _isGsmTechGroup: function(radioTech) {
    if (!radioTech) {
      return true;
    }

    switch(radioTech) {
      case NETWORK_CREG_TECH_GPRS:
      case NETWORK_CREG_TECH_EDGE:
      case NETWORK_CREG_TECH_UMTS:
      case NETWORK_CREG_TECH_HSDPA:
      case NETWORK_CREG_TECH_HSUPA:
      case NETWORK_CREG_TECH_HSPA:
      case NETWORK_CREG_TECH_LTE:
      case NETWORK_CREG_TECH_HSPAP:
      case NETWORK_CREG_TECH_GSM:
      case NETWORK_CREG_TECH_DCHSPAP_1:
      case NETWORK_CREG_TECH_DCHSPAP_2:
        return true;
    }

    return false;
  },

  


  _processRadioTech: function(radioTech) {
    let isCdma = !this._isGsmTechGroup(radioTech);
    this.radioTech = radioTech;

    if (DEBUG) {
      this.context.debug("Radio tech is set to: " + GECKO_RADIO_TECH[radioTech] +
                         ", it is a " + (isCdma?"cdma":"gsm") + " technology");
    }

    
    
    
    if (this._waitingRadioTech || isCdma != this._isCdma) {
      this._isCdma = isCdma;
      this._waitingRadioTech = false;
      if (this._isCdma) {
        this.getDeviceIdentity();
      } else {
        this.getIMEI();
        this.getIMEISV();
      }
      this.getICCStatus();
    }
  },

  


  _toaFromString: function(number) {
    let toa = TOA_UNKNOWN;
    if (number && number.length > 0 && number[0] == '+') {
      toa = TOA_INTERNATIONAL;
    }
    return toa;
  },

  



  _siToServiceClass: function(si) {
    if (!si) {
      return ICC_SERVICE_CLASS_NONE;
    }

    let serviceCode = parseInt(si, 10);
    switch (serviceCode) {
      case 10:
        return ICC_SERVICE_CLASS_SMS + ICC_SERVICE_CLASS_FAX  + ICC_SERVICE_CLASS_VOICE;
      case 11:
        return ICC_SERVICE_CLASS_VOICE;
      case 12:
        return ICC_SERVICE_CLASS_SMS + ICC_SERVICE_CLASS_FAX;
      case 13:
        return ICC_SERVICE_CLASS_FAX;
      case 16:
        return ICC_SERVICE_CLASS_SMS;
      case 19:
        return ICC_SERVICE_CLASS_FAX + ICC_SERVICE_CLASS_VOICE;
      case 21:
        return ICC_SERVICE_CLASS_PAD + ICC_SERVICE_CLASS_DATA_ASYNC;
      case 22:
        return ICC_SERVICE_CLASS_PACKET + ICC_SERVICE_CLASS_DATA_SYNC;
      case 25:
        return ICC_SERVICE_CLASS_DATA_ASYNC;
      case 26:
        return ICC_SERVICE_CLASS_DATA_SYNC + SERVICE_CLASS_VOICE;
      case 99:
        return ICC_SERVICE_CLASS_PACKET;
      default:
        return ICC_SERVICE_CLASS_NONE;
    }
  },

  




  dataDownloadViaSMSPP: function(message) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let options = {
      pid: message.pid,
      dcs: message.dcs,
      encoding: message.encoding,
    };
    Buf.newParcel(REQUEST_STK_SEND_ENVELOPE_WITH_STATUS, options);

    Buf.seekIncoming(-1 * (Buf.getCurrentParcelSize() - Buf.getReadAvailable()
                           - 2 * Buf.UINT32_SIZE)); 
    let messageStringLength = Buf.readInt32(); 
    let smscLength = GsmPDUHelper.readHexOctet(); 
    let tpduLength = (messageStringLength / 2) - (smscLength + 1); 

    
    
    
    let berLen = 4 +
                 (smscLength ? (2 + smscLength) : 0) +
                 (tpduLength <= 127 ? 2 : 3) + tpduLength; 

    let parcelLength = (berLen <= 127 ? 2 : 3) + berLen; 
    Buf.writeInt32(parcelLength * 2); 

    
    GsmPDUHelper.writeHexOctet(BER_SMS_PP_DOWNLOAD_TAG);
    if (berLen > 127) {
      GsmPDUHelper.writeHexOctet(0x81);
    }
    GsmPDUHelper.writeHexOctet(berLen);

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DEVICE_ID |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(0x02);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_NETWORK);
    GsmPDUHelper.writeHexOctet(STK_DEVICE_ID_SIM);

    
    if (smscLength) {
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_ADDRESS);
      GsmPDUHelper.writeHexOctet(smscLength);
      Buf.copyIncomingToOutgoing(Buf.PDU_HEX_OCTET_SIZE * smscLength);
    }

    
    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_SMS_TPDU |
                               COMPREHENSIONTLV_FLAG_CR);
    if (tpduLength > 127) {
      GsmPDUHelper.writeHexOctet(0x81);
    }
    GsmPDUHelper.writeHexOctet(tpduLength);
    Buf.copyIncomingToOutgoing(Buf.PDU_HEX_OCTET_SIZE * tpduLength);

    
    Buf.writeStringDelimiter(0);

    Buf.sendParcel();
  },

  








  acknowledgeIncomingGsmSmsWithPDU: function(success, responsePduLen, options) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    Buf.newParcel(REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU);

    
    Buf.writeInt32(2);

    
    Buf.writeString(success ? "1" : "0");

    
    Buf.writeInt32(2 * (responsePduLen + (success ? 5 : 6))); 
    
    GsmPDUHelper.writeHexOctet(PDU_MTI_SMS_DELIVER);
    if (!success) {
      
      GsmPDUHelper.writeHexOctet(PDU_FCS_USIM_DATA_DOWNLOAD_ERROR);
    }
    
    GsmPDUHelper.writeHexOctet(PDU_PI_USER_DATA_LENGTH |
                               PDU_PI_DATA_CODING_SCHEME |
                               PDU_PI_PROTOCOL_IDENTIFIER);
    
    GsmPDUHelper.writeHexOctet(options.pid);
    
    GsmPDUHelper.writeHexOctet(options.dcs);
    
    if (options.encoding == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      GsmPDUHelper.writeHexOctet(Math.floor(responsePduLen * 8 / 7));
    } else {
      GsmPDUHelper.writeHexOctet(responsePduLen);
    }
    
    Buf.copyIncomingToOutgoing(Buf.PDU_HEX_OCTET_SIZE * responsePduLen);
    
    Buf.writeStringDelimiter(0);

    Buf.sendParcel();
  },

  


  writeSmsToSIM: function(message) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    Buf.newParcel(REQUEST_WRITE_SMS_TO_SIM);

    
    Buf.writeInt32(EFSMS_STATUS_FREE);

    Buf.seekIncoming(-1 * (Buf.getCurrentParcelSize() - Buf.getReadAvailable()
                           - 2 * Buf.UINT32_SIZE)); 
    let messageStringLength = Buf.readInt32(); 
    let smscLength = GsmPDUHelper.readHexOctet(); 
    let pduLength = (messageStringLength / 2) - (smscLength + 1); 

    
    if (smscLength > 0) {
      Buf.seekIncoming(smscLength * Buf.PDU_HEX_OCTET_SIZE);
    }
    
    Buf.writeInt32(2 * pduLength); 
    if (pduLength) {
      Buf.copyIncomingToOutgoing(Buf.PDU_HEX_OCTET_SIZE * pduLength);
    }
    
    Buf.writeStringDelimiter(0);

    
    
    Buf.writeInt32(2 * (smscLength + 1)); 
    
    GsmPDUHelper.writeHexOctet(smscLength);
    
    if (smscLength) {
      Buf.seekIncoming(-1 * (Buf.getCurrentParcelSize() - Buf.getReadAvailable()
                             - 2 * Buf.UINT32_SIZE 
                             - 2 * Buf.PDU_HEX_OCTET_SIZE)); 
      Buf.copyIncomingToOutgoing(Buf.PDU_HEX_OCTET_SIZE * smscLength);
    }
    
    Buf.writeStringDelimiter(0);

    Buf.sendParcel();
  },

  







  _processSmsMultipart: function(message) {
    message.rilMessageType = "sms-received";

    this.sendChromeMessage(message);

    return MOZ_FCS_WAIT_FOR_EXPLICIT_ACK;
  },

  







  _processSmsStatusReport: function(length) {
    let [message, result] = this.context.GsmPDUHelper.processReceivedSms(length);
    if (!message) {
      if (DEBUG) this.context.debug("invalid SMS-STATUS-REPORT");
      return PDU_FCS_UNSPECIFIED;
    }

    let options = this._pendingSentSmsMap[message.messageRef];
    if (!options) {
      if (DEBUG) this.context.debug("no pending SMS-SUBMIT message");
      return PDU_FCS_OK;
    }

    let status = message.status;

    
    
    if ((status >= 0x80)
        || ((status >= PDU_ST_0_RESERVED_BEGIN)
            && (status < PDU_ST_0_SC_SPECIFIC_BEGIN))
        || ((status >= PDU_ST_1_RESERVED_BEGIN)
            && (status < PDU_ST_1_SC_SPECIFIC_BEGIN))
        || ((status >= PDU_ST_2_RESERVED_BEGIN)
            && (status < PDU_ST_2_SC_SPECIFIC_BEGIN))
        || ((status >= PDU_ST_3_RESERVED_BEGIN)
            && (status < PDU_ST_3_SC_SPECIFIC_BEGIN))
        ) {
      status = PDU_ST_3_SERVICE_REJECTED;
    }

    
    if ((status >>> 5) == 0x01) {
      if (DEBUG) this.context.debug("SMS-STATUS-REPORT: delivery still pending");
      return PDU_FCS_OK;
    }

    delete this._pendingSentSmsMap[message.messageRef];

    let deliveryStatus = ((status >>> 5) === 0x00)
                       ? GECKO_SMS_DELIVERY_STATUS_SUCCESS
                       : GECKO_SMS_DELIVERY_STATUS_ERROR;
    this.sendChromeMessage({
      rilMessageType: options.rilMessageType,
      rilMessageToken: options.rilMessageToken,
      deliveryStatus: deliveryStatus
    });

    return PDU_FCS_OK;
  },

  







  _processCdmaSmsStatusReport: function(message) {
    let options = this._pendingSentSmsMap[message.msgId];
    if (!options) {
      if (DEBUG) this.context.debug("no pending SMS-SUBMIT message");
      return PDU_FCS_OK;
    }

    if (message.errorClass === 2) {
      if (DEBUG) {
        this.context.debug("SMS-STATUS-REPORT: delivery still pending, " +
                           "msgStatus: " + message.msgStatus);
      }
      return PDU_FCS_OK;
    }

    delete this._pendingSentSmsMap[message.msgId];

    if (message.errorClass === -1 && message.body) {
      
      
      return this._processSmsMultipart(message);
    }

    let deliveryStatus = (message.errorClass === 0)
                       ? GECKO_SMS_DELIVERY_STATUS_SUCCESS
                       : GECKO_SMS_DELIVERY_STATUS_ERROR;
    this.sendChromeMessage({
      rilMessageType: options.rilMessageType,
      rilMessageToken: options.rilMessageToken,
      deliveryStatus: deliveryStatus
    });

    return PDU_FCS_OK;
  },

  







  _processCdmaSmsWapPush: function(message) {
    if (!message.data) {
      if (DEBUG) this.context.debug("no data inside WAP Push message.");
      return PDU_FCS_OK;
    }

    
    
    
    
    
    
    
    
    let index = 0;
    if (message.data[index++] !== 0) {
      if (DEBUG) this.context.debug("Ignore a WAP Message which is not WDP.");
      return PDU_FCS_OK;
    }

    
    
    
    message.header = {
      segmentRef:     message.msgId,
      segmentMaxSeq:  message.data[index++],
      segmentSeq:     message.data[index++] + 1 
    };

    if (message.header.segmentSeq > message.header.segmentMaxSeq) {
      if (DEBUG) this.context.debug("Wrong WDP segment info.");
      return PDU_FCS_OK;
    }

    
    if (message.header.segmentSeq == 1) {
      message.header.originatorPort = message.data[index++] << 8;
      message.header.originatorPort |= message.data[index++];
      message.header.destinationPort = message.data[index++] << 8;
      message.header.destinationPort |= message.data[index++];
    }

    message.data = message.data.subarray(index);

    return this._processSmsMultipart(message);
  },

  


  _processSentSmsSegment: function(options) {
    
    let next = options.segmentSeq;
    options.body = options.segments[next].body;
    options.encodedBodyLength = options.segments[next].encodedBodyLength;
    options.segmentSeq = next + 1;

    this.sendSMS(options);
  },

  







  _processSmsSendResult: function(length, options) {
    if (options.errorMsg) {
      if (DEBUG) {
        this.context.debug("_processSmsSendResult: errorMsg = " +
                           options.errorMsg);
      }

      if (options.errorMsg === GECKO_ERROR_SMS_SEND_FAIL_RETRY &&
          options.retryCount < SMS_RETRY_MAX) {
        options.retryCount++;
        
        this.sendSMS(options);
        return;
      }

      
      this.sendChromeMessage({
        rilMessageType: options.rilMessageType,
        rilMessageToken: options.rilMessageToken,
        errorMsg: options.errorMsg,
      });
      return;
    }

    let Buf = this.context.Buf;
    options.messageRef = Buf.readInt32();
    options.ackPDU = Buf.readString();
    options.errorCode = Buf.readInt32();

    if ((options.segmentMaxSeq > 1)
        && (options.segmentSeq < options.segmentMaxSeq)) {
      
      this._processSentSmsSegment(options);
    } else {
      
      if (options.requestStatusReport) {
        if (DEBUG) {
          this.context.debug("waiting SMS-STATUS-REPORT for messageRef " +
                             options.messageRef);
        }
        this._pendingSentSmsMap[options.messageRef] = options;
      }

      this.sendChromeMessage({
        rilMessageType: options.rilMessageType,
        rilMessageToken: options.rilMessageToken,
      });
    }
  },

  _processReceivedSmsCbPage: function(original) {
    if (original.numPages <= 1) {
      if (original.body) {
        original.fullBody = original.body;
        delete original.body;
      } else if (original.data) {
        original.fullData = original.data;
        delete original.data;
      }
      return original;
    }

    
    let hash = original.serial + ":" + this.iccInfo.mcc + ":"
               + this.iccInfo.mnc + ":";
    switch (original.geographicalScope) {
      case CB_GSM_GEOGRAPHICAL_SCOPE_CELL_WIDE_IMMEDIATE:
      case CB_GSM_GEOGRAPHICAL_SCOPE_CELL_WIDE:
        hash += this.voiceRegistrationState.cell.gsmLocationAreaCode + ":"
             + this.voiceRegistrationState.cell.gsmCellId;
        break;
      case CB_GSM_GEOGRAPHICAL_SCOPE_LOCATION_AREA_WIDE:
        hash += this.voiceRegistrationState.cell.gsmLocationAreaCode + ":";
        break;
      default:
        hash += ":";
        break;
    }

    let index = original.pageIndex;

    let options = this._receivedSmsCbPagesMap[hash];
    if (!options) {
      options = original;
      this._receivedSmsCbPagesMap[hash] = options;

      options.receivedPages = 0;
      options.pages = [];
    } else if (options.pages[index]) {
      
      if (DEBUG) {
        this.context.debug("Got duplicated page no." + index +
                           " of a multipage SMSCB: " + JSON.stringify(original));
      }
      return null;
    }

    if (options.encoding == PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      options.pages[index] = original.data;
      delete original.data;
    } else {
      options.pages[index] = original.body;
      delete original.body;
    }
    options.receivedPages++;
    if (options.receivedPages < options.numPages) {
      if (DEBUG) {
        this.context.debug("Got page no." + index + " of a multipage SMSCB: " +
                           JSON.stringify(options));
      }
      return null;
    }

    
    delete this._receivedSmsCbPagesMap[hash];

    
    if (options.encoding == PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      
      let fullDataLen = 0;
      for (let i = 1; i <= options.numPages; i++) {
        fullDataLen += options.pages[i].length;
      }

      options.fullData = new Uint8Array(fullDataLen);
      for (let d= 0, i = 1; i <= options.numPages; i++) {
        let data = options.pages[i];
        for (let j = 0; j < data.length; j++) {
          options.fullData[d++] = data[j];
        }
      }
    } else {
      options.fullBody = options.pages.join("");
    }

    if (DEBUG) {
      this.context.debug("Got full multipage SMSCB: " + JSON.stringify(options));
    }

    return options;
  },

  _mergeCellBroadcastConfigs: function(list, from, to) {
    if (!list) {
      return [from, to];
    }

    for (let i = 0, f1, t1; i < list.length;) {
      f1 = list[i++];
      t1 = list[i++];
      if (to == f1) {
        
        list[i - 2] = from;
        return list;
      }

      if (to < f1) {
        
        if (i > 2) {
          
          return list.slice(0, i - 2).concat([from, to]).concat(list.slice(i - 2));
        } else {
          return [from, to].concat(list);
        }
      }

      if (from > t1) {
        
        continue;
      }

      
      

      let changed = false;
      if (from < f1) {
        
        
        list[i - 2] = from;
        changed = true;
      }

      if (to <= t1) {
        
        
        return list;
      }

      
      let j = i;
      for (let f2, t2; j < list.length;) {
        f2 = list[j++];
        t2 = list[j++];
        if (to > t2) {
          
          
          continue;
        }

        if (to < t2) {
          if (to < f2) {
            
            
            j -= 2;
          } else if (to < t2) {
            
            
            to = t2;
          }
        }

        break;
      }

      
      list[i - 1] = to;

      if (j != i) {
        
        let ret = list.slice(0, i);
        if (j < list.length) {
          ret = ret.concat(list.slice(j));
        }
        return ret;
      }

      return list;
    }

    
    list.push(from);
    list.push(to);

    return list;
  },

  _isCellBroadcastConfigReady: function() {
    if (!("MMI" in this.cellBroadcastConfigs)) {
      return false;
    }

    
    if (!this._isCdma &&
        (!("CBMI" in this.cellBroadcastConfigs) ||
         !("CBMID" in this.cellBroadcastConfigs) ||
         !("CBMIR" in this.cellBroadcastConfigs))) {
      return false;
    }

    return true;
  },

  


  _mergeAllCellBroadcastConfigs: function() {
    if (!this._isCellBroadcastConfigReady()) {
      if (DEBUG) {
        this.context.debug("cell broadcast configs not ready, waiting ...");
      }
      return;
    }

    
    let usedCellBroadcastConfigs = {MMI: this.cellBroadcastConfigs.MMI};
    if (!this._isCdma) {
      usedCellBroadcastConfigs.CBMI = this.cellBroadcastConfigs.CBMI;
      usedCellBroadcastConfigs.CBMID = this.cellBroadcastConfigs.CBMID;
      usedCellBroadcastConfigs.CBMIR = this.cellBroadcastConfigs.CBMIR;
    }

    if (DEBUG) {
      this.context.debug("Cell Broadcast search lists: " +
                         JSON.stringify(usedCellBroadcastConfigs));
    }

    let list = null;
    for each (let ll in usedCellBroadcastConfigs) {
      if (ll == null) {
        continue;
      }

      for (let i = 0; i < ll.length; i += 2) {
        list = this._mergeCellBroadcastConfigs(list, ll[i], ll[i + 1]);
      }
    }

    if (DEBUG) {
      this.context.debug("Cell Broadcast search lists(merged): " +
                         JSON.stringify(list));
    }
    this.mergedCellBroadcastConfig = list;
    this.updateCellBroadcastConfig();
  },

  



  _checkCellBroadcastMMISettable: function(from, to) {
    if ((to <= from) || (from >= 65536) || (from < 0)) {
      return false;
    }

    if (!this._isCdma) {
      
      for (let i = 0, f, t; i < CB_NON_MMI_SETTABLE_RANGES.length;) {
        f = CB_NON_MMI_SETTABLE_RANGES[i++];
        t = CB_NON_MMI_SETTABLE_RANGES[i++];
        if ((from < t) && (to > f)) {
          
          return false;
        }
      }
    }

    return true;
  },

  


  _convertCellBroadcastSearchList: function(searchListStr) {
    let parts = searchListStr && searchListStr.split(",");
    if (!parts) {
      return null;
    }

    let list = null;
    let result, from, to;
    for (let range of parts) {
      
      
      result = range.match(/^(\d+)(?:-(\d+))?$/);
      if (!result) {
        throw "Invalid format";
      }

      from = parseInt(result[1], 10);
      to = (result[2]) ? parseInt(result[2], 10) + 1 : from + 1;
      if (!this._checkCellBroadcastMMISettable(from, to)) {
        throw "Invalid range";
      }

      if (list == null) {
        list = [];
      }
      list.push(from);
      list.push(to);
    }

    return list;
  },

  





  handleChromeMessage: function(message) {
    if (DEBUG) {
      this.context.debug("Received chrome message " + JSON.stringify(message));
    }
    let method = this[message.rilMessageType];
    if (typeof method != "function") {
      if (DEBUG) {
        this.context.debug("Don't know what to do with message " +
                           JSON.stringify(message));
      }
      return;
    }
    method.call(this, message);
  },

  


  processStkProactiveCommand: function() {
    let Buf = this.context.Buf;
    let length = Buf.readInt32();
    let berTlv;
    try {
      berTlv = this.context.BerTlvHelper.decode(length / 2);
    } catch (e) {
      if (DEBUG) this.context.debug("processStkProactiveCommand : " + e);
      this.sendStkTerminalResponse({
        resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      return;
    }

    Buf.readStringDelimiter(length);

    let ctlvs = berTlv.value;
    let ctlv = this.context.StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_COMMAND_DETAILS, ctlvs);
    if (!ctlv) {
      this.sendStkTerminalResponse({
        resultCode: STK_RESULT_CMD_DATA_NOT_UNDERSTOOD});
      throw new Error("Can't find COMMAND_DETAILS ComprehensionTlv");
    }

    let cmdDetails = ctlv.value;
    if (DEBUG) {
      this.context.debug("commandNumber = " + cmdDetails.commandNumber +
                         " typeOfCommand = " + cmdDetails.typeOfCommand.toString(16) +
                         " commandQualifier = " + cmdDetails.commandQualifier);
    }

    
    if (cmdDetails.typeOfCommand == STK_CMD_MORE_TIME) {
      this.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_OK});
      return;
    }

    this.context.StkCommandParamsFactory.createParam(cmdDetails,
                                                     ctlvs,
                                                     (aResult) => {
      cmdDetails.options = aResult;
      cmdDetails.rilMessageType = "stkcommand";
      this.sendChromeMessage(cmdDetails);
    });
  },

  sendDefaultResponse: function(options) {
    if (!options.rilMessageType) {
      return;
    }

    this.sendChromeMessage(options);
  },

  


  sendChromeMessage: function(message) {
    message.rilMessageClientId = this.context.clientId;
    postMessage(message);
  },

  





  handleParcel: function(request_type, length, options) {
    let method = this[request_type];
    if (typeof method == "function") {
      if (DEBUG) this.context.debug("Handling parcel as " + method.name);
      method.call(this, length, options);
    }

    if (this.telephonyRequestQueue.isValidRequest(request_type)) {
      this.telephonyRequestQueue.pop(request_type);
    }
  }
};

RilObject.prototype[REQUEST_GET_SIM_STATUS] = function REQUEST_GET_SIM_STATUS(length, options) {
  if (options.errorMsg) {
    return;
  }

  let iccStatus = {};
  let Buf = this.context.Buf;
  iccStatus.cardState = Buf.readInt32(); 
  iccStatus.universalPINState = Buf.readInt32(); 
  iccStatus.gsmUmtsSubscriptionAppIndex = Buf.readInt32();
  iccStatus.cdmaSubscriptionAppIndex = Buf.readInt32();
  if (!this.v5Legacy) {
    iccStatus.imsSubscriptionAppIndex = Buf.readInt32();
  }

  let apps_length = Buf.readInt32();
  if (apps_length > CARD_MAX_APPS) {
    apps_length = CARD_MAX_APPS;
  }

  iccStatus.apps = [];
  for (let i = 0 ; i < apps_length ; i++) {
    iccStatus.apps.push({
      app_type:       Buf.readInt32(), 
      app_state:      Buf.readInt32(), 
      perso_substate: Buf.readInt32(), 
      aid:            Buf.readString(),
      app_label:      Buf.readString(),
      pin1_replaced:  Buf.readInt32(),
      pin1:           Buf.readInt32(),
      pin2:           Buf.readInt32()
    });
    if (RILQUIRKS_SIM_APP_STATE_EXTRA_FIELDS) {
      Buf.readInt32();
      Buf.readInt32();
      Buf.readInt32();
      Buf.readInt32();
    }
  }

  if (DEBUG) this.context.debug("iccStatus: " + JSON.stringify(iccStatus));
  this._processICCStatus(iccStatus);
};
RilObject.prototype[REQUEST_ENTER_SIM_PIN] = function REQUEST_ENTER_SIM_PIN(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RilObject.prototype[REQUEST_ENTER_SIM_PUK] = function REQUEST_ENTER_SIM_PUK(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RilObject.prototype[REQUEST_ENTER_SIM_PIN2] = function REQUEST_ENTER_SIM_PIN2(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RilObject.prototype[REQUEST_ENTER_SIM_PUK2] = function REQUEST_ENTER_SIM_PUK(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RilObject.prototype[REQUEST_CHANGE_SIM_PIN] = function REQUEST_CHANGE_SIM_PIN(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RilObject.prototype[REQUEST_CHANGE_SIM_PIN2] = function REQUEST_CHANGE_SIM_PIN2(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RilObject.prototype[REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE] =
  function REQUEST_ENTER_NETWORK_DEPERSONALIZATION_CODE(length, options) {
  this._processEnterAndChangeICCResponses(length, options);
};
RilObject.prototype[REQUEST_GET_CURRENT_CALLS] = function REQUEST_GET_CURRENT_CALLS(length, options) {
  
  if (options.errorMsg) {
    if (this._getCurrentCallsRetryCount < GET_CURRENT_CALLS_RETRY_MAX) {
      this._getCurrentCallsRetryCount++;
      this.getCurrentCalls(options);
    } else {
      this.sendDefaultResponse(options);
    }
    return;
  }

  this._getCurrentCallsRetryCount = 0;

  let Buf = this.context.Buf;
  let calls_length = 0;
  
  
  if (length) {
    calls_length = Buf.readInt32();
  }

  let calls = {};
  for (let i = 0; i < calls_length; i++) {
    let call = {};

    
    
    if (RILQUIRKS_EXTRA_UINT32_2ND_CALL && i > 0) {
      Buf.readInt32();
    }

    call.state          = Buf.readInt32(); 
    call.callIndex      = Buf.readInt32(); 
    call.toa            = Buf.readInt32();
    call.isMpty         = Boolean(Buf.readInt32());
    call.isMT           = Boolean(Buf.readInt32());
    call.als            = Buf.readInt32();
    call.isVoice        = Boolean(Buf.readInt32());
    call.isVoicePrivacy = Boolean(Buf.readInt32());
    if (RILQUIRKS_CALLSTATE_EXTRA_UINT32) {
      Buf.readInt32();
    }
    call.number             = Buf.readString();
    call.numberPresentation = Buf.readInt32(); 
    call.name               = Buf.readString();
    call.namePresentation   = Buf.readInt32();

    call.uusInfo = null;
    let uusInfoPresent = Buf.readInt32();
    if (uusInfoPresent == 1) {
      call.uusInfo = {
        type:     Buf.readInt32(),
        dcs:      Buf.readInt32(),
        userData: null 
      };
    }

    if (call.isVoice) {
      calls[call.callIndex] = call;
    }
  }

  options.calls = calls;
  options.rilMessageType = options.rilMessageType || "currentCalls";
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_DIAL] = function REQUEST_DIAL(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_DIAL_EMERGENCY_CALL] = function REQUEST_DIAL_EMERGENCY_CALL(length, options) {
  RilObject.prototype[REQUEST_DIAL].call(this, length, options);
};
RilObject.prototype[REQUEST_GET_IMSI] = function REQUEST_GET_IMSI(length, options) {
  if (options.errorMsg) {
    return;
  }

  this.iccInfoPrivate.imsi = this.context.Buf.readString();
  if (DEBUG) {
    this.context.debug("IMSI: " + this.iccInfoPrivate.imsi);
  }

  options.rilMessageType = "iccimsi";
  options.imsi = this.iccInfoPrivate.imsi;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_HANGUP] = function REQUEST_HANGUP(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_HANGUP_WAITING_OR_BACKGROUND] = function REQUEST_HANGUP_WAITING_OR_BACKGROUND(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND] = function REQUEST_HANGUP_FOREGROUND_RESUME_BACKGROUND(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE] = function REQUEST_SWITCH_WAITING_OR_HOLDING_AND_ACTIVE(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_CONFERENCE] = function REQUEST_CONFERENCE(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_UDUB] = function REQUEST_UDUB(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_LAST_CALL_FAIL_CAUSE] = function REQUEST_LAST_CALL_FAIL_CAUSE(length, options) {
  
  let failCause = CALL_FAIL_ERROR_UNSPECIFIED;

  if (!options.errorMsg) {
    let Buf = this.context.Buf;
    let num = length ? Buf.readInt32() : 0;

    if (num) {
      let causeNum = Buf.readInt32();
      failCause = RIL_CALL_FAILCAUSE_TO_GECKO_CALL_ERROR[causeNum] || failCause;
    }
    if (DEBUG) this.context.debug("Last call fail cause: " + failCause);
  }

  options.failCause = failCause;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SIGNAL_STRENGTH] = function REQUEST_SIGNAL_STRENGTH(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_SIGNAL);

  if (options.errorMsg) {
    return;
  }

  let Buf = this.context.Buf;
  let signal = {};

  signal.gsmSignalStrength = Buf.readInt32();
  signal.gsmBitErrorRate = Buf.readInt32();
  if (RILQUIRKS_SIGNAL_EXTRA_INT32) {
    Buf.readInt32();
  }
  signal.cdmaDBM = Buf.readInt32();
  signal.cdmaECIO = Buf.readInt32();
  signal.evdoDBM = Buf.readInt32();
  signal.evdoECIO = Buf.readInt32();
  signal.evdoSNR = Buf.readInt32();

  if (!this.v5Legacy) {
    signal.lteSignalStrength = Buf.readInt32();
    signal.lteRSRP =           Buf.readInt32();
    signal.lteRSRQ =           Buf.readInt32();
    signal.lteRSSNR =          Buf.readInt32();
    signal.lteCQI =            Buf.readInt32();
  }

  if (DEBUG) this.context.debug("signal strength: " + JSON.stringify(signal));

  this._processSignalStrength(signal);
};
RilObject.prototype[REQUEST_VOICE_REGISTRATION_STATE] = function REQUEST_VOICE_REGISTRATION_STATE(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_VOICE_REGISTRATION_STATE);

  if (options.errorMsg) {
    return;
  }

  let state = this.context.Buf.readStringList();
  if (DEBUG) this.context.debug("voice registration state: " + state);

  this._processVoiceRegistrationState(state);
};
RilObject.prototype[REQUEST_DATA_REGISTRATION_STATE] = function REQUEST_DATA_REGISTRATION_STATE(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_DATA_REGISTRATION_STATE);

  if (options.errorMsg) {
    return;
  }

  let state = this.context.Buf.readStringList();
  this._processDataRegistrationState(state);
};
RilObject.prototype[REQUEST_OPERATOR] = function REQUEST_OPERATOR(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_OPERATOR);

  if (options.errorMsg) {
    return;
  }

  let operatorData = this.context.Buf.readStringList();
  if (DEBUG) this.context.debug("Operator: " + operatorData);
  this._processOperator(operatorData);
};
RilObject.prototype[REQUEST_RADIO_POWER] = function REQUEST_RADIO_POWER(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_DTMF] = null;
RilObject.prototype[REQUEST_SEND_SMS] = function REQUEST_SEND_SMS(length, options) {
  this._processSmsSendResult(length, options);
};
RilObject.prototype[REQUEST_SEND_SMS_EXPECT_MORE] = null;

RilObject.prototype.readSetupDataCall_v5 = function readSetupDataCall_v5(options) {
  if (!options) {
    options = {};
  }
  let [cid, ifname, addresses, dnses, gateways] = this.context.Buf.readStringList();
  options.cid = cid;
  options.ifname = ifname;
  options.addresses = addresses ? [addresses] : [];
  options.dnses = dnses ? [dnses] : [];
  options.gateways = gateways ? [gateways] : [];
  options.active = DATACALL_ACTIVE_UNKNOWN;
  options.state = GECKO_NETWORK_STATE_CONNECTING;
  return options;
};

RilObject.prototype[REQUEST_SETUP_DATA_CALL] = function REQUEST_SETUP_DATA_CALL(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  if (this.v5Legacy) {
    
    
    this.readSetupDataCall_v5(options);
    this.sendChromeMessage(options);
    
    
    this.getDataCallList();
    return;
  }

  let Buf = this.context.Buf;
  
  Buf.readInt32();
  
  Buf.readInt32();

  this.readDataCall_v6(options);
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SIM_IO] = function REQUEST_SIM_IO(length, options) {
  if (options.errorMsg) {
    if (options.onerror) {
      options.onerror(options.errorMsg);
    }
    return;
  }

  let Buf = this.context.Buf;
  options.sw1 = Buf.readInt32();
  options.sw2 = Buf.readInt32();

  
  if (options.sw1 !== ICC_STATUS_NORMAL_ENDING &&
      options.sw1 !== ICC_STATUS_NORMAL_ENDING_WITH_EXTRA &&
      options.sw1 !== ICC_STATUS_WITH_SIM_DATA &&
      options.sw1 !== ICC_STATUS_WITH_RESPONSE_DATA) {
    if (DEBUG) {
      this.context.debug("ICC I/O Error EF id = 0x" + options.fileId.toString(16) +
                         ", command = 0x" + options.command.toString(16) +
                         ", sw1 = 0x" + options.sw1.toString(16) +
                         ", sw2 = 0x" + options.sw2.toString(16));
    }
    if (options.onerror) {
      
      
      
      options.onerror(GECKO_ERROR_GENERIC_FAILURE);
    }
    return;
  }
  this.context.ICCIOHelper.processICCIO(options);
};
RilObject.prototype[REQUEST_SEND_USSD] = function REQUEST_SEND_USSD(length, options) {
  if (DEBUG) {
    this.context.debug("REQUEST_SEND_USSD " + JSON.stringify(options));
  }
  this._ussdSession = !options.errorMsg;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_CANCEL_USSD] = function REQUEST_CANCEL_USSD(length, options) {
  if (DEBUG) {
    this.context.debug("REQUEST_CANCEL_USSD" + JSON.stringify(options));
  }

  this._ussdSession = !!options.errorMsg;

  
  if (this.cachedUSSDRequest) {
    if (DEBUG) this.context.debug("Send out the cached ussd request");
    this.sendUSSD(this.cachedUSSDRequest);
    this.cachedUSSDRequest = null;
    return;
  }

  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_GET_CLIR] = function REQUEST_GET_CLIR(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  let Buf = this.context.Buf;
  let bufLength = Buf.readInt32();
  if (!bufLength || bufLength < 2) {
    options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
    this.sendChromeMessage(options);
    return;
  }

  options.n = Buf.readInt32(); 
  options.m = Buf.readInt32(); 

  if (options.rilMessageType === "sendMMI") {
    
    switch (options.m) {
      
      case 0:
        options.statusMessage = MMI_SM_KS_SERVICE_NOT_PROVISIONED;
        break;
      
      case 1:
        options.statusMessage = MMI_SM_KS_CLIR_PERMANENT;
        break;
      
      case 2:
        options.errorMsg = MMI_ERROR_KS_ERROR;
        break;
      
      case 3:
        
        switch (options.n) {
          
          case 0:
          
          case 1:
            options.statusMessage = MMI_SM_KS_CLIR_DEFAULT_ON_NEXT_CALL_ON;
            break;
          
          case 2:
            options.statusMessage = MMI_SM_KS_CLIR_DEFAULT_ON_NEXT_CALL_OFF;
            break;
          default:
            options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
            break;
        }
        break;
      
      case 4:
        
        switch (options.n) {
          
          case 0:
          
          case 2:
            options.statusMessage = MMI_SM_KS_CLIR_DEFAULT_OFF_NEXT_CALL_OFF;
            break;
          
          case 1:
            options.statusMessage = MMI_SM_KS_CLIR_DEFAULT_OFF_NEXT_CALL_ON;
            break;
          default:
            options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
            break;
        }
        break;
      default:
        options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
        break;
    }
  }

  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SET_CLIR] = function REQUEST_SET_CLIR(length, options) {
  if (options.rilMessageType == null) {
    
    return;
  }

  if (!options.errorMsg && options.rilMessageType === "sendMMI") {
    switch (options.procedure) {
      case MMI_PROCEDURE_ACTIVATION:
        options.statusMessage = MMI_SM_KS_SERVICE_ENABLED;
        break;
      case MMI_PROCEDURE_DEACTIVATION:
        options.statusMessage = MMI_SM_KS_SERVICE_DISABLED;
        break;
    }
  }
  this.sendChromeMessage(options);
};

RilObject.prototype[REQUEST_QUERY_CALL_FORWARD_STATUS] =
  function REQUEST_QUERY_CALL_FORWARD_STATUS(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  let Buf = this.context.Buf;
  let rulesLength = 0;
  if (length) {
    rulesLength = Buf.readInt32();
  }
  if (!rulesLength) {
    options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
    this.sendChromeMessage(options);
    return;
  }
  let rules = new Array(rulesLength);
  for (let i = 0; i < rulesLength; i++) {
    let rule = {};
    rule.active       = Buf.readInt32() == 1; 
    rule.reason       = Buf.readInt32(); 
    rule.serviceClass = Buf.readInt32();
    rule.toa          = Buf.readInt32();
    rule.number       = Buf.readString();
    rule.timeSeconds  = Buf.readInt32();
    rules[i] = rule;
  }
  options.rules = rules;
  if (options.rilMessageType === "sendMMI") {
    options.statusMessage = MMI_SM_KS_SERVICE_INTERROGATED;
    
    
    
    options.additionalInformation = rules;
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SET_CALL_FORWARD] =
    function REQUEST_SET_CALL_FORWARD(length, options) {
  if (!options.errorMsg && options.rilMessageType === "sendMMI") {
    switch (options.action) {
      case CALL_FORWARD_ACTION_ENABLE:
        options.statusMessage = MMI_SM_KS_SERVICE_ENABLED;
        break;
      case CALL_FORWARD_ACTION_DISABLE:
        options.statusMessage = MMI_SM_KS_SERVICE_DISABLED;
        break;
      case CALL_FORWARD_ACTION_REGISTRATION:
        options.statusMessage = MMI_SM_KS_SERVICE_REGISTERED;
        break;
      case CALL_FORWARD_ACTION_ERASURE:
        options.statusMessage = MMI_SM_KS_SERVICE_ERASED;
        break;
    }
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_QUERY_CALL_WAITING] =
  function REQUEST_QUERY_CALL_WAITING(length, options) {
  if (options.errorMsg) {
    if (options.callback) {
      
      delete options.callback;
    }

    this.sendChromeMessage(options);
    return;
  }

  if (options.callback) {
    options.callback.call(this, options);
    return;
  }

  let Buf = this.context.Buf;
  options.length = Buf.readInt32();
  options.enabled = ((Buf.readInt32() == 1) &&
                     ((Buf.readInt32() & ICC_SERVICE_CLASS_VOICE) == 0x01));
  this.sendChromeMessage(options);
};

RilObject.prototype[REQUEST_SET_CALL_WAITING] = function REQUEST_SET_CALL_WAITING(length, options) {
  if (options.errorMsg) {
    if (options.callback) {
      
      delete options.callback;
    }

    this.sendChromeMessage(options);
    return;
  }

  if (options.callback) {
    options.callback.call(this, options);
    return;
  }

  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SMS_ACKNOWLEDGE] = null;
RilObject.prototype[REQUEST_GET_IMEI] = function REQUEST_GET_IMEI(length, options) {
  this.IMEI = this.context.Buf.readString();
  let rilMessageType = options.rilMessageType;
  
  if (rilMessageType !== "sendMMI") {
    return;
  }

  if (!options.errorMsg && this.IMEI == null) {
    options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
  }
  options.statusMessage = this.IMEI;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_GET_IMEISV] = function REQUEST_GET_IMEISV(length, options) {
  if (options.errorMsg) {
    return;
  }

  this.IMEISV = this.context.Buf.readString();
};
RilObject.prototype[REQUEST_ANSWER] = function REQUEST_ANSWER(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_DEACTIVATE_DATA_CALL] = function REQUEST_DEACTIVATE_DATA_CALL(length, options) {
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_QUERY_FACILITY_LOCK] = function REQUEST_QUERY_FACILITY_LOCK(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  if (!length) {
    options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
    this.sendChromeMessage(options);
    return;
  }

  
  let services = this.context.Buf.readInt32List()[0];

  if (options.queryServiceClass) {
    options.enabled = (services & options.queryServiceClass) ? true : false;
    options.serviceClass = options.queryServiceClass;
  } else {
    options.enabled = services ? true : false;
  }

  if (options.rilMessageType === "sendMMI") {
    if (!options.enabled) {
      options.statusMessage = MMI_SM_KS_SERVICE_DISABLED;
    } else {
      options.statusMessage = MMI_SM_KS_SERVICE_ENABLED_FOR;
      let serviceClass = [];
      for (let serviceClassMask = 1;
           serviceClassMask <= ICC_SERVICE_CLASS_MAX;
           serviceClassMask <<= 1) {
        if ((serviceClassMask & services) !== 0) {
          serviceClass.push(MMI_KS_SERVICE_CLASS_MAPPING[serviceClassMask]);
        }
      }

      options.additionalInformation = serviceClass;
    }
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SET_FACILITY_LOCK] = function REQUEST_SET_FACILITY_LOCK(length, options) {
  options.retryCount = length ? this.context.Buf.readInt32List()[0] : -1;

  if (!options.errorMsg && (options.rilMessageType === "sendMMI")) {
    switch (options.procedure) {
      case MMI_PROCEDURE_ACTIVATION:
        options.statusMessage = MMI_SM_KS_SERVICE_ENABLED;
        break;
      case MMI_PROCEDURE_DEACTIVATION:
        options.statusMessage = MMI_SM_KS_SERVICE_DISABLED;
        break;
    }
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_CHANGE_BARRING_PASSWORD] =
  function REQUEST_CHANGE_BARRING_PASSWORD(length, options) {
  if (options.rilMessageType != "sendMMI") {
    this.sendChromeMessage(options);
    return;
  }

  options.statusMessage = MMI_SM_KS_PASSWORD_CHANGED;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_QUERY_NETWORK_SELECTION_MODE] = function REQUEST_QUERY_NETWORK_SELECTION_MODE(length, options) {
  this._receivedNetworkInfo(NETWORK_INFO_NETWORK_SELECTION_MODE);

  if (options.errorMsg) {
    return;
  }

  let mode = this.context.Buf.readInt32List();
  let selectionMode;

  switch (mode[0]) {
    case NETWORK_SELECTION_MODE_AUTOMATIC:
      selectionMode = GECKO_NETWORK_SELECTION_AUTOMATIC;
      break;
    case NETWORK_SELECTION_MODE_MANUAL:
      selectionMode = GECKO_NETWORK_SELECTION_MANUAL;
      break;
    default:
      selectionMode = GECKO_NETWORK_SELECTION_UNKNOWN;
      break;
  }

  this._updateNetworkSelectionMode(selectionMode);
};
RilObject.prototype[REQUEST_SET_NETWORK_SELECTION_AUTOMATIC] = function REQUEST_SET_NETWORK_SELECTION_AUTOMATIC(length, options) {
  if (!options.errorMsg) {
    this._updateNetworkSelectionMode(GECKO_NETWORK_SELECTION_AUTOMATIC);
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SET_NETWORK_SELECTION_MANUAL] = function REQUEST_SET_NETWORK_SELECTION_MANUAL(length, options) {
  if (!options.errorMsg) {
    this._updateNetworkSelectionMode(GECKO_NETWORK_SELECTION_MANUAL);
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_QUERY_AVAILABLE_NETWORKS] = function REQUEST_QUERY_AVAILABLE_NETWORKS(length, options) {
  if (!options.errorMsg) {
    options.networks = this._processNetworks();
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_DTMF_START] = function REQUEST_DTMF_START(length, options) {
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_DTMF_STOP] = null;
RilObject.prototype[REQUEST_BASEBAND_VERSION] = function REQUEST_BASEBAND_VERSION(length, options) {
  if (options.errorMsg) {
    return;
  }

  this.basebandVersion = this.context.Buf.readString();
  if (DEBUG) this.context.debug("Baseband version: " + this.basebandVersion);
};
RilObject.prototype[REQUEST_SEPARATE_CONNECTION] = function REQUEST_SEPARATE_CONNECTION(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_SET_MUTE] = null;
RilObject.prototype[REQUEST_GET_MUTE] = null;
RilObject.prototype[REQUEST_QUERY_CLIP] = function REQUEST_QUERY_CLIP(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  let Buf = this.context.Buf;
  let bufLength = Buf.readInt32();
  if (!bufLength) {
    options.errorMsg = GECKO_ERROR_GENERIC_FAILURE;
    this.sendChromeMessage(options);
    return;
  }

  
  
  
  
  
  options.provisioned = Buf.readInt32();
  if (options.rilMessageType === "sendMMI") {
    switch (options.provisioned) {
      case 0:
        options.statusMessage = MMI_SM_KS_SERVICE_DISABLED;
        break;
      case 1:
        options.statusMessage = MMI_SM_KS_SERVICE_ENABLED;
        break;
      default:
        options.errorMsg = MMI_ERROR_KS_ERROR;
        break;
    }
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_LAST_DATA_CALL_FAIL_CAUSE] = null;


















RilObject.prototype.readDataCall_v5 = function(options) {
  if (!options) {
    options = {};
  }
  let Buf = this.context.Buf;
  options.cid = Buf.readInt32().toString();
  options.active = Buf.readInt32(); 
  options.type = Buf.readString();
  options.apn = Buf.readString();
  let addresses = Buf.readString();
  let dnses = Buf.readString();
  options.addresses = addresses ? addresses.split(" ") : [];
  options.dnses = dnses ? dnses.split(" ") : [];
  options.gateways = [];
  this._setDataCallGeckoState(options);
  return options;
};

RilObject.prototype.readDataCall_v6 = function(options) {
  if (!options) {
    options = {};
  }
  let Buf = this.context.Buf;
  options.status = Buf.readInt32();  
  options.suggestedRetryTime = Buf.readInt32();
  options.cid = Buf.readInt32().toString();
  options.active = Buf.readInt32();  
  options.type = Buf.readString();
  options.ifname = Buf.readString();
  let addresses = Buf.readString();
  let dnses = Buf.readString();
  let gateways = Buf.readString();
  options.addresses = addresses ? addresses.split(" ") : [];
  options.dnses = dnses ? dnses.split(" ") : [];
  options.gateways = gateways ? gateways.split(" ") : [];
  this._setDataCallGeckoState(options);
  return options;
};

RilObject.prototype[REQUEST_DATA_CALL_LIST] = function REQUEST_DATA_CALL_LIST(length, options) {
  if (options.errorMsg) {
    if (options.rilMessageType) {
      this.sendChromeMessage(options);
    }
    return;
  }

  if (!options.rilMessageType) {
    
    options.rilMessageType = "datacalllistchanged";
  }

  if (!length) {
    options.datacalls = [];
    this.sendChromeMessage(options);
    return;
  }

  let Buf = this.context.Buf;
  let version = 0;
  if (!this.v5Legacy) {
    version = Buf.readInt32();
  }
  let num = Buf.readInt32();
  let datacalls = [];
  for (let i = 0; i < num; i++) {
    let datacall;
    if (version < 6) {
      datacall = this.readDataCall_v5();
    } else {
      datacall = this.readDataCall_v6();
    }
    datacalls.push(datacall);
  }

  options.datacalls = datacalls;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_RESET_RADIO] = null;
RilObject.prototype[REQUEST_OEM_HOOK_RAW] = null;
RilObject.prototype[REQUEST_OEM_HOOK_STRINGS] = null;
RilObject.prototype[REQUEST_SCREEN_STATE] = null;
RilObject.prototype[REQUEST_SET_SUPP_SVC_NOTIFICATION] = null;
RilObject.prototype[REQUEST_WRITE_SMS_TO_SIM] = function REQUEST_WRITE_SMS_TO_SIM(length, options) {
  if (options.errorMsg) {
    
    
    
    
    this.acknowledgeGsmSms(false, PDU_FCS_PROTOCOL_ERROR);
  } else {
    this.acknowledgeGsmSms(true, PDU_FCS_OK);
  }
};
RilObject.prototype[REQUEST_DELETE_SMS_ON_SIM] = null;
RilObject.prototype[REQUEST_SET_BAND_MODE] = null;
RilObject.prototype[REQUEST_QUERY_AVAILABLE_BAND_MODE] = null;
RilObject.prototype[REQUEST_STK_GET_PROFILE] = null;
RilObject.prototype[REQUEST_STK_SET_PROFILE] = null;
RilObject.prototype[REQUEST_STK_SEND_ENVELOPE_COMMAND] = null;
RilObject.prototype[REQUEST_STK_SEND_TERMINAL_RESPONSE] = null;
RilObject.prototype[REQUEST_STK_HANDLE_CALL_SETUP_REQUESTED_FROM_SIM] = null;
RilObject.prototype[REQUEST_EXPLICIT_CALL_TRANSFER] = null;
RilObject.prototype[REQUEST_SET_PREFERRED_NETWORK_TYPE] = function REQUEST_SET_PREFERRED_NETWORK_TYPE(length, options) {
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_GET_PREFERRED_NETWORK_TYPE] = function REQUEST_GET_PREFERRED_NETWORK_TYPE(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  options.type = this.context.Buf.readInt32List()[0];
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_GET_NEIGHBORING_CELL_IDS] = function REQUEST_GET_NEIGHBORING_CELL_IDS(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  let radioTech = this.voiceRegistrationState.radioTech;
  if (radioTech == undefined || radioTech == NETWORK_CREG_TECH_UNKNOWN) {
    options.errorMsg = "RadioTechUnavailable";
    this.sendChromeMessage(options);
    return;
  }
  if (!this._isGsmTechGroup(radioTech) || radioTech == NETWORK_CREG_TECH_LTE) {
    options.errorMsg = "UnsupportedRadioTech";
    this.sendChromeMessage(options);
    return;
  }

  let Buf = this.context.Buf;
  let neighboringCellIds = [];
  let num = Buf.readInt32();

  for (let i = 0; i < num; i++) {
    let cellId = {};
    cellId.networkType = GECKO_RADIO_TECH[radioTech];
    cellId.signalStrength = Buf.readInt32();

    let cid = Buf.readString();
    
    let length = cid.length;
    if (length > 8) {
      continue;
    }
    if (length < 8) {
      for (let j = 0; j < (8-length); j++) {
        cid = "0" + cid;
      }
    }

    switch (radioTech) {
      case NETWORK_CREG_TECH_GPRS:
      case NETWORK_CREG_TECH_EDGE:
      case NETWORK_CREG_TECH_GSM:
        cellId.gsmCellId = this.parseInt(cid.substring(4), -1, 16);
        cellId.gsmLocationAreaCode = this.parseInt(cid.substring(0, 4), -1, 16);
        break;
      case NETWORK_CREG_TECH_UMTS:
      case NETWORK_CREG_TECH_HSDPA:
      case NETWORK_CREG_TECH_HSUPA:
      case NETWORK_CREG_TECH_HSPA:
      case NETWORK_CREG_TECH_HSPAP:
      case NETWORK_CREG_TECH_DCHSPAP_1:
      case NETWORK_CREG_TECH_DCHSPAP_2:
        cellId.wcdmaPsc = this.parseInt(cid, -1, 16);
        break;
    }

    neighboringCellIds.push(cellId);
  }

  options.result = neighboringCellIds;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_GET_CELL_INFO_LIST] = function REQUEST_GET_CELL_INFO_LIST(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  let Buf = this.context.Buf;
  let cellInfoList = [];
  let num = Buf.readInt32();
  for (let i = 0; i < num; i++) {
    let cellInfo = {};
    cellInfo.type = Buf.readInt32();
    cellInfo.registered = Buf.readInt32() ? true : false;
    cellInfo.timestampType = Buf.readInt32();
    cellInfo.timestamp = Buf.readInt64();

    switch(cellInfo.type) {
      case CELL_INFO_TYPE_GSM:
      case CELL_INFO_TYPE_WCDMA:
        cellInfo.mcc = Buf.readInt32();
        cellInfo.mnc = Buf.readInt32();
        cellInfo.lac = Buf.readInt32();
        cellInfo.cid = Buf.readInt32();
        if (cellInfo.type == CELL_INFO_TYPE_WCDMA) {
          cellInfo.psc = Buf.readInt32();
        }
        cellInfo.signalStrength = Buf.readInt32();
        cellInfo.bitErrorRate = Buf.readInt32();
        break;
      case CELL_INFO_TYPE_CDMA:
        cellInfo.networkId = Buf.readInt32();
        cellInfo.systemId = Buf.readInt32();
        cellInfo.basestationId = Buf.readInt32();
        cellInfo.longitude = Buf.readInt32();
        cellInfo.latitude = Buf.readInt32();
        cellInfo.cdmaDbm = Buf.readInt32();
        cellInfo.cdmaEcio = Buf.readInt32();
        cellInfo.evdoDbm = Buf.readInt32();
        cellInfo.evdoEcio = Buf.readInt32();
        cellInfo.evdoSnr = Buf.readInt32();
        break;
      case CELL_INFO_TYPE_LTE:
        cellInfo.mcc = Buf.readInt32();
        cellInfo.mnc = Buf.readInt32();
        cellInfo.cid = Buf.readInt32();
        cellInfo.pcid = Buf.readInt32();
        cellInfo.tac = Buf.readInt32();
        cellInfo.signalStrength = Buf.readInt32();
        cellInfo.rsrp = Buf.readInt32();
        cellInfo.rsrq = Buf.readInt32();
        cellInfo.rssnr = Buf.readInt32();
        cellInfo.cqi = Buf.readInt32();
        break;
    }
    cellInfoList.push(cellInfo);
  }
  options.result = cellInfoList;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SET_LOCATION_UPDATES] = null;
RilObject.prototype[REQUEST_CDMA_SET_SUBSCRIPTION_SOURCE] = null;
RilObject.prototype[REQUEST_CDMA_SET_ROAMING_PREFERENCE] = function REQUEST_CDMA_SET_ROAMING_PREFERENCE(length, options) {
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_CDMA_QUERY_ROAMING_PREFERENCE] = function REQUEST_CDMA_QUERY_ROAMING_PREFERENCE(length, options) {
  if (!options.errorMsg) {
    options.mode = this.context.Buf.readInt32List()[0];
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SET_TTY_MODE] = null;
RilObject.prototype[REQUEST_QUERY_TTY_MODE] = null;
RilObject.prototype[REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE] = function REQUEST_CDMA_SET_PREFERRED_VOICE_PRIVACY_MODE(length, options) {
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE] = function REQUEST_CDMA_QUERY_PREFERRED_VOICE_PRIVACY_MODE(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  let enabled = this.context.Buf.readInt32List();
  options.enabled = enabled[0] ? true : false;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_CDMA_FLASH] = function REQUEST_CDMA_FLASH(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_CDMA_BURST_DTMF] = null;
RilObject.prototype[REQUEST_CDMA_VALIDATE_AND_WRITE_AKEY] = null;
RilObject.prototype[REQUEST_CDMA_SEND_SMS] = function REQUEST_CDMA_SEND_SMS(length, options) {
  this._processSmsSendResult(length, options);
};
RilObject.prototype[REQUEST_CDMA_SMS_ACKNOWLEDGE] = null;
RilObject.prototype[REQUEST_GSM_GET_BROADCAST_SMS_CONFIG] = null;
RilObject.prototype[REQUEST_GSM_SET_BROADCAST_SMS_CONFIG] = function REQUEST_GSM_SET_BROADCAST_SMS_CONFIG(length, options) {
  if (options.errorMsg) {
    return;
  }
  this.setSmsBroadcastActivation(true);
};
RilObject.prototype[REQUEST_GSM_SMS_BROADCAST_ACTIVATION] = null;
RilObject.prototype[REQUEST_CDMA_GET_BROADCAST_SMS_CONFIG] = null;
RilObject.prototype[REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG] = function REQUEST_CDMA_SET_BROADCAST_SMS_CONFIG(length, options) {
  if (options.errorMsg) {
    return;
  }
  this.setSmsBroadcastActivation(true);
};
RilObject.prototype[REQUEST_CDMA_SMS_BROADCAST_ACTIVATION] = null;
RilObject.prototype[REQUEST_CDMA_SUBSCRIPTION] = function REQUEST_CDMA_SUBSCRIPTION(length, options) {
  if (options.errorMsg) {
    return;
  }

  let result = this.context.Buf.readStringList();

  this.iccInfo.mdn = result[0];
  
  
  
  this.iccInfo.prlVersion = parseInt(result[4], 10);

  this.context.ICCUtilsHelper.handleICCInfoChange();
};
RilObject.prototype[REQUEST_CDMA_WRITE_SMS_TO_RUIM] = null;
RilObject.prototype[REQUEST_CDMA_DELETE_SMS_ON_RUIM] = null;
RilObject.prototype[REQUEST_DEVICE_IDENTITY] = function REQUEST_DEVICE_IDENTITY(length, options) {
  if (options.errorMsg) {
    return;
  }

  let result = this.context.Buf.readStringList();

  
  
  
  this.ESN = result[2];
  this.MEID = result[3];
};
RilObject.prototype[REQUEST_EXIT_EMERGENCY_CALLBACK_MODE] = function REQUEST_EXIT_EMERGENCY_CALLBACK_MODE(length, options) {
  if (options.internal) {
    return;
  }

  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_GET_SMSC_ADDRESS] = function REQUEST_GET_SMSC_ADDRESS(length, options) {
  this.SMSC = options.errorMsg ? null : this.context.Buf.readString();

  if (!options.rilMessageType || options.rilMessageType !== "getSmscAddress") {
    return;
  }

  options.smscAddress = this.SMSC;
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SET_SMSC_ADDRESS] = function REQUEST_SET_SMSC_ADDRESS(length, options) {
  if (!options.rilMessageType || options.rilMessageType !== "setSmscAddress") {
    return;
  }

  if (options.rilRequestError) {
    optioins.errorMsg = RIL_ERROR_TO_GECKO_ERROR[options.rilRequestError];
  }

  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_REPORT_SMS_MEMORY_STATUS] = function REQUEST_REPORT_SMS_MEMORY_STATUS(length, options) {
  this.pendingToReportSmsMemoryStatus = !!options.errorMsg;
};
RilObject.prototype[REQUEST_REPORT_STK_SERVICE_IS_RUNNING] = null;
RilObject.prototype[REQUEST_CDMA_GET_SUBSCRIPTION_SOURCE] = null;
RilObject.prototype[REQUEST_ISIM_AUTHENTICATION] = null;
RilObject.prototype[REQUEST_ACKNOWLEDGE_INCOMING_GSM_SMS_WITH_PDU] = null;
RilObject.prototype[REQUEST_STK_SEND_ENVELOPE_WITH_STATUS] = function REQUEST_STK_SEND_ENVELOPE_WITH_STATUS(length, options) {
  if (options.errorMsg) {
    this.acknowledgeGsmSms(false, PDU_FCS_UNSPECIFIED);
    return;
  }

  let Buf = this.context.Buf;
  let sw1 = Buf.readInt32();
  let sw2 = Buf.readInt32();
  if ((sw1 == ICC_STATUS_SAT_BUSY) && (sw2 === 0x00)) {
    this.acknowledgeGsmSms(false, PDU_FCS_USAT_BUSY);
    return;
  }

  let success = ((sw1 == ICC_STATUS_NORMAL_ENDING) && (sw2 === 0x00))
                || (sw1 == ICC_STATUS_NORMAL_ENDING_WITH_EXTRA);

  let messageStringLength = Buf.readInt32(); 
  let responsePduLen = messageStringLength / 2; 
  if (!responsePduLen) {
    this.acknowledgeGsmSms(success, success ? PDU_FCS_OK
                                         : PDU_FCS_USIM_DATA_DOWNLOAD_ERROR);
    return;
  }

  this.acknowledgeIncomingGsmSmsWithPDU(success, responsePduLen, options);
};
RilObject.prototype[REQUEST_VOICE_RADIO_TECH] = function REQUEST_VOICE_RADIO_TECH(length, options) {
  if (options.errorMsg) {
    if (DEBUG) {
      this.context.debug("Error when getting voice radio tech: " +
                         options.errorMsg);
    }
    return;
  }
  let radioTech = this.context.Buf.readInt32List();
  this._processRadioTech(radioTech[0]);
};
RilObject.prototype[REQUEST_GET_CELL_INFO_LIST] = null;
RilObject.prototype[REQUEST_SET_UNSOL_CELL_INFO_LIST_RATE] = null;
RilObject.prototype[REQUEST_SET_INITIAL_ATTACH_APN] = null;
RilObject.prototype[REQUEST_IMS_REGISTRATION_STATE] = null;
RilObject.prototype[REQUEST_IMS_SEND_SMS] = null;
RilObject.prototype[REQUEST_SIM_TRANSMIT_APDU_BASIC] = null;
RilObject.prototype[REQUEST_SIM_OPEN_CHANNEL] = function REQUEST_SIM_OPEN_CHANNEL(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  options.channel = this.context.Buf.readInt32List()[0];
  
  
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_SIM_CLOSE_CHANNEL] = function REQUEST_SIM_CLOSE_CHANNEL(length, options) {
  this.sendDefaultResponse(options);
};
RilObject.prototype[REQUEST_SIM_TRANSMIT_APDU_CHANNEL] = function REQUEST_SIM_TRANSMIT_APDU_CHANNEL(length, options) {
  if (options.errorMsg) {
    this.sendChromeMessage(options);
    return;
  }

  let Buf = this.context.Buf;
  options.sw1 = Buf.readInt32();
  options.sw2 = Buf.readInt32();
  options.simResponse = Buf.readString();
  if (DEBUG) {
    this.context.debug("Setting return values for RIL[REQUEST_SIM_TRANSMIT_APDU_CHANNEL]: [" +
                       options.sw1 + "," +
                       options.sw2 + ", " +
                       options.simResponse + "]");
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_NV_READ_ITEM] = null;
RilObject.prototype[REQUEST_NV_WRITE_ITEM] = null;
RilObject.prototype[REQUEST_NV_WRITE_CDMA_PRL] = null;
RilObject.prototype[REQUEST_NV_RESET_CONFIG] = null;
RilObject.prototype[REQUEST_SET_UICC_SUBSCRIPTION] = function REQUEST_SET_UICC_SUBSCRIPTION(length, options) {
  
  if (this._attachDataRegistration) {
    this.setDataRegistration({attach: true});
  }
};
RilObject.prototype[REQUEST_ALLOW_DATA] = null;
RilObject.prototype[REQUEST_GET_HARDWARE_CONFIG] = null;
RilObject.prototype[REQUEST_SIM_AUTHENTICATION] = null;
RilObject.prototype[REQUEST_GET_DC_RT_INFO] = null;
RilObject.prototype[REQUEST_SET_DC_RT_INFO_RATE] = null;
RilObject.prototype[REQUEST_SET_DATA_PROFILE] = null;
RilObject.prototype[REQUEST_SHUTDOWN] = null;
RilObject.prototype[REQUEST_SET_DATA_SUBSCRIPTION] = function REQUEST_SET_DATA_SUBSCRIPTION(length, options) {
  if (!options.rilMessageType) {
    
    return;
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[REQUEST_GET_UNLOCK_RETRY_COUNT] = function REQUEST_GET_UNLOCK_RETRY_COUNT(length, options) {
  options.retryCount = length ? this.context.Buf.readInt32List()[0] : -1;
  this.sendChromeMessage(options);
};
RilObject.prototype[RIL_REQUEST_GPRS_ATTACH] = function RIL_REQUEST_GPRS_ATTACH(length, options) {
  if (!options.rilMessageType) {
    
    return;
  }
  this.sendChromeMessage(options);
};
RilObject.prototype[RIL_REQUEST_GPRS_DETACH] = function RIL_REQUEST_GPRS_DETACH(length, options) {
  this.sendChromeMessage(options);
};
RilObject.prototype[UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED] = function UNSOLICITED_RESPONSE_RADIO_STATE_CHANGED() {
  let radioState = this.context.Buf.readInt32();
  let newState;
  switch (radioState) {
    case RADIO_STATE_UNAVAILABLE:
      newState = GECKO_RADIOSTATE_UNKNOWN;
      break;
    case RADIO_STATE_OFF:
      newState = GECKO_RADIOSTATE_DISABLED;
      break;
    default:
      newState = GECKO_RADIOSTATE_ENABLED;
  }

  if (DEBUG) {
    this.context.debug("Radio state changed from '" + this.radioState +
                       "' to '" + newState + "'");
  }
  if (this.radioState == newState) {
    return;
  }

  switch (radioState) {
    case RADIO_STATE_SIM_READY:
    case RADIO_STATE_SIM_NOT_READY:
    case RADIO_STATE_SIM_LOCKED_OR_ABSENT:
      this._isCdma = false;
      this._waitingRadioTech = false;
      break;
    case RADIO_STATE_RUIM_READY:
    case RADIO_STATE_RUIM_NOT_READY:
    case RADIO_STATE_RUIM_LOCKED_OR_ABSENT:
    case RADIO_STATE_NV_READY:
    case RADIO_STATE_NV_NOT_READY:
      this._isCdma = true;
      this._waitingRadioTech = false;
      break;
    case RADIO_STATE_ON: 
      
      
      
      this._waitingRadioTech = true;
      this.getVoiceRadioTechnology();
      break;
  }

  if ((this.radioState == GECKO_RADIOSTATE_UNKNOWN ||
       this.radioState == GECKO_RADIOSTATE_DISABLED) &&
       newState == GECKO_RADIOSTATE_ENABLED) {
    
    if (!this._waitingRadioTech) {
      if (this._isCdma) {
        this.getDeviceIdentity();
      } else {
        this.getIMEI();
        this.getIMEISV();
      }
    }
    this.getBasebandVersion();
    this.updateCellBroadcastConfig();
    if ((RILQUIRKS_DATA_REGISTRATION_ON_DEMAND ||
         RILQUIRKS_SUBSCRIPTION_CONTROL) &&
        this._attachDataRegistration) {
      this.setDataRegistration({attach: true});
    }

    if (this.pendingToReportSmsMemoryStatus) {
      this._updateSmsMemoryStatus();
    }
  }

  this.radioState = newState;
  this.sendChromeMessage({
    rilMessageType: "radiostatechange",
    radioState: newState
  });

  
  
  
  if (radioState == RADIO_STATE_UNAVAILABLE ||
      radioState == RADIO_STATE_OFF ||
      this._waitingRadioTech) {
    return;
  }
  this.getICCStatus();
};
RilObject.prototype[UNSOLICITED_RESPONSE_CALL_STATE_CHANGED] = function UNSOLICITED_RESPONSE_CALL_STATE_CHANGED() {
  this.getCurrentCalls();
};
RilObject.prototype[UNSOLICITED_RESPONSE_VOICE_NETWORK_STATE_CHANGED] = function UNSOLICITED_RESPONSE_VOICE_NETWORK_STATE_CHANGED() {
  if (DEBUG) {
    this.context.debug("Network state changed, re-requesting phone state and " +
                       "ICC status");
  }
  this.getICCStatus();
  this.requestNetworkInfo();
};
RilObject.prototype[UNSOLICITED_RESPONSE_NEW_SMS] = function UNSOLICITED_RESPONSE_NEW_SMS(length) {
  let [message, result] = this.context.GsmPDUHelper.processReceivedSms(length);

  if (message) {
    result = this._processSmsMultipart(message);
  }

  if (result == PDU_FCS_RESERVED || result == MOZ_FCS_WAIT_FOR_EXPLICIT_ACK) {
    return;
  }

  
  this.acknowledgeGsmSms(result == PDU_FCS_OK, result);
};
RilObject.prototype[UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT] = function UNSOLICITED_RESPONSE_NEW_SMS_STATUS_REPORT(length) {
  let result = this._processSmsStatusReport(length);
  this.acknowledgeGsmSms(result == PDU_FCS_OK, result);
};
RilObject.prototype[UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM] = function UNSOLICITED_RESPONSE_NEW_SMS_ON_SIM(length) {
  let recordNumber = this.context.Buf.readInt32List()[0];

  this.context.SimRecordHelper.readSMS(
    recordNumber,
    function onsuccess(message) {
      if (message && message.simStatus === 3) { 
        this._processSmsMultipart(message);
      }
    }.bind(this),
    function onerror(errorMsg) {
      if (DEBUG) {
        this.context.debug("Failed to Read NEW SMS on SIM #" + recordNumber +
                           ", errorMsg: " + errorMsg);
      }
    });
};
RilObject.prototype[UNSOLICITED_ON_USSD] = function UNSOLICITED_ON_USSD() {
  let [typeCode, message] = this.context.Buf.readStringList();
  if (DEBUG) {
    this.context.debug("On USSD. Type Code: " + typeCode + " Message: " + message);
  }

  let oldSession = this._ussdSession;

  
  this._ussdSession = typeCode == "1";

  if (!oldSession && !this._ussdSession && !message) {
    return;
  }

  this.sendChromeMessage({rilMessageType: "ussdreceived",
                          message: message,
                          sessionEnded: !this._ussdSession});
};
RilObject.prototype[UNSOLICITED_ON_USSD_REQUEST] = null;
RilObject.prototype[UNSOLICITED_NITZ_TIME_RECEIVED] = function UNSOLICITED_NITZ_TIME_RECEIVED() {
  let dateString = this.context.Buf.readString();

  
  
  
  

  if (DEBUG) this.context.debug("DateTimeZone string " + dateString);

  let now = Date.now();

  let year = parseInt(dateString.substr(0, 2), 10);
  let month = parseInt(dateString.substr(3, 2), 10);
  let day = parseInt(dateString.substr(6, 2), 10);
  let hours = parseInt(dateString.substr(9, 2), 10);
  let minutes = parseInt(dateString.substr(12, 2), 10);
  let seconds = parseInt(dateString.substr(15, 2), 10);
  
  let tz = parseInt(dateString.substr(17, 3), 10);
  
  let dst = parseInt(dateString.substr(21, 2), 10);

  let timeInMS = Date.UTC(year + PDU_TIMESTAMP_YEAR_OFFSET, month - 1, day,
                          hours, minutes, seconds);

  if (isNaN(timeInMS)) {
    if (DEBUG) this.context.debug("NITZ failed to convert date");
    return;
  }

  this.sendChromeMessage({rilMessageType: "nitzTime",
                          networkTimeInMS: timeInMS,
                          networkTimeZoneInMinutes: -(tz * 15),
                          networkDSTInMinutes: -(dst * 60),
                          receiveTimeInMS: now});
};

RilObject.prototype[UNSOLICITED_SIGNAL_STRENGTH] = function UNSOLICITED_SIGNAL_STRENGTH(length) {
  this[REQUEST_SIGNAL_STRENGTH](length, {});
};
RilObject.prototype[UNSOLICITED_DATA_CALL_LIST_CHANGED] = function UNSOLICITED_DATA_CALL_LIST_CHANGED(length) {
  if (this.v5Legacy) {
    this.getDataCallList();
    return;
  }
  this[REQUEST_DATA_CALL_LIST](length, {});
};
RilObject.prototype[UNSOLICITED_SUPP_SVC_NOTIFICATION] = function UNSOLICITED_SUPP_SVC_NOTIFICATION(length) {
  let Buf = this.context.Buf;
  let info = {};
  info.notificationType = Buf.readInt32();
  info.code = Buf.readInt32();
  info.index = Buf.readInt32();
  info.type = Buf.readInt32();
  info.number = Buf.readString();

  this._processSuppSvcNotification(info);
};

RilObject.prototype[UNSOLICITED_STK_SESSION_END] = function UNSOLICITED_STK_SESSION_END() {
  this.sendChromeMessage({rilMessageType: "stksessionend"});
};
RilObject.prototype[UNSOLICITED_STK_PROACTIVE_COMMAND] = function UNSOLICITED_STK_PROACTIVE_COMMAND() {
  this.processStkProactiveCommand();
};
RilObject.prototype[UNSOLICITED_STK_EVENT_NOTIFY] = function UNSOLICITED_STK_EVENT_NOTIFY() {
  this.processStkProactiveCommand();
};
RilObject.prototype[UNSOLICITED_STK_CALL_SETUP] = null;
RilObject.prototype[UNSOLICITED_SIM_SMS_STORAGE_FULL] = null;
RilObject.prototype[UNSOLICITED_SIM_REFRESH] = null;
RilObject.prototype[UNSOLICITED_CALL_RING] = function UNSOLICITED_CALL_RING() {
  let Buf = this.context.Buf;
  let info = {rilMessageType: "callRing"};
  let isCDMA = false; 
  if (isCDMA) {
    info.isPresent = Buf.readInt32();
    info.signalType = Buf.readInt32();
    info.alertPitch = Buf.readInt32();
    info.signal = Buf.readInt32();
  }
  
  
  
  
  this.sendChromeMessage(info);
};
RilObject.prototype[UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED] = function UNSOLICITED_RESPONSE_SIM_STATUS_CHANGED() {
  this.getICCStatus();
};
RilObject.prototype[UNSOLICITED_RESPONSE_CDMA_NEW_SMS] = function UNSOLICITED_RESPONSE_CDMA_NEW_SMS(length) {
  let [message, result] = this.context.CdmaPDUHelper.processReceivedSms(length);

  if (message) {
    if (message.teleservice === PDU_CDMA_MSG_TELESERIVCIE_ID_WAP) {
      result = this._processCdmaSmsWapPush(message);
    } else if (message.subMsgType === PDU_CDMA_MSG_TYPE_DELIVER_ACK) {
      result = this._processCdmaSmsStatusReport(message);
    } else {
      result = this._processSmsMultipart(message);
    }
  }

  if (result == PDU_FCS_RESERVED || result == MOZ_FCS_WAIT_FOR_EXPLICIT_ACK) {
    return;
  }

  
  this.acknowledgeCdmaSms(result == PDU_FCS_OK, result);
};
RilObject.prototype[UNSOLICITED_RESPONSE_NEW_BROADCAST_SMS] = function UNSOLICITED_RESPONSE_NEW_BROADCAST_SMS(length) {
  let message;
  try {
    message =
      this.context.GsmPDUHelper.readCbMessage(this.context.Buf.readInt32());
  } catch (e) {
    if (DEBUG) {
      this.context.debug("Failed to parse Cell Broadcast message: " + e);
    }
    return;
  }

  message = this._processReceivedSmsCbPage(message);
  if (!message) {
    return;
  }

  message.rilMessageType = "cellbroadcast-received";
  this.sendChromeMessage(message);
};
RilObject.prototype[UNSOLICITED_CDMA_RUIM_SMS_STORAGE_FULL] = null;
RilObject.prototype[UNSOLICITED_RESTRICTED_STATE_CHANGED] = null;
RilObject.prototype[UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE] = function UNSOLICITED_ENTER_EMERGENCY_CALLBACK_MODE() {
  this._handleChangedEmergencyCbMode(true);
};
RilObject.prototype[UNSOLICITED_CDMA_CALL_WAITING] = function UNSOLICITED_CDMA_CALL_WAITING(length) {
  let Buf = this.context.Buf;
  let call = {};
  call.number              = Buf.readString();
  call.numberPresentation  = Buf.readInt32();
  call.name                = Buf.readString();
  call.namePresentation    = Buf.readInt32();
  call.isPresent           = Buf.readInt32();
  call.signalType          = Buf.readInt32();
  call.alertPitch          = Buf.readInt32();
  call.signal              = Buf.readInt32();
  this.sendChromeMessage({rilMessageType: "cdmaCallWaiting",
                          waitingCall: call});
};
RilObject.prototype[UNSOLICITED_CDMA_OTA_PROVISION_STATUS] = function UNSOLICITED_CDMA_OTA_PROVISION_STATUS() {
  let status =
    CDMA_OTA_PROVISION_STATUS_TO_GECKO[this.context.Buf.readInt32List()[0]];
  if (!status) {
    return;
  }
  this.sendChromeMessage({rilMessageType: "otastatuschange",
                          status: status});
};
RilObject.prototype[UNSOLICITED_CDMA_INFO_REC] = function UNSOLICITED_CDMA_INFO_REC(length) {
  this.sendChromeMessage({
    rilMessageType: "cdma-info-rec-received",
    records: this.context.CdmaPDUHelper.decodeInformationRecord()
  });
};
RilObject.prototype[UNSOLICITED_OEM_HOOK_RAW] = null;
RilObject.prototype[UNSOLICITED_RINGBACK_TONE] = null;
RilObject.prototype[UNSOLICITED_RESEND_INCALL_MUTE] = null;
RilObject.prototype[UNSOLICITED_CDMA_SUBSCRIPTION_SOURCE_CHANGED] = null;
RilObject.prototype[UNSOLICITED_CDMA_PRL_CHANGED] = function UNSOLICITED_CDMA_PRL_CHANGED(length) {
  let version = this.context.Buf.readInt32List()[0];
  if (version !== this.iccInfo.prlVersion) {
    this.iccInfo.prlVersion = version;
    this.context.ICCUtilsHelper.handleICCInfoChange();
  }
};
RilObject.prototype[UNSOLICITED_EXIT_EMERGENCY_CALLBACK_MODE] = function UNSOLICITED_EXIT_EMERGENCY_CALLBACK_MODE() {
  this._handleChangedEmergencyCbMode(false);
};
RilObject.prototype[UNSOLICITED_RIL_CONNECTED] = function UNSOLICITED_RIL_CONNECTED(length) {
  
  
  if (!length) {
    return;
  }

  this.version = this.context.Buf.readInt32List()[0];
  this.v5Legacy = (this.version < 5);
  if (DEBUG) {
    this.context.debug("Detected RIL version " + this.version);
    this.context.debug("this.v5Legacy is " + this.v5Legacy);
  }

  this.initRILState();
  
  this.getDataCallList();
  
  this.exitEmergencyCbMode();
  
  this.setRadioEnabled({enabled: false});
};
RilObject.prototype[UNSOLICITED_VOICE_RADIO_TECH_CHANGED] = function UNSOLICITED_VOICE_RADIO_TECH_CHANGED(length) {
  
  
  
  
  
  this._processRadioTech(this.context.Buf.readInt32List()[0]);
};
RilObject.prototype[UNSOLICITED_CELL_INFO_LIST] = null;
RilObject.prototype[UNSOLICITED_RESPONSE_IMS_NETWORK_STATE_CHANGED] = null;
RilObject.prototype[UNSOLICITED_UICC_SUBSCRIPTION_STATUS_CHANGED] = null;
RilObject.prototype[UNSOLICITED_SRVCC_STATE_NOTIFY] = null;
RilObject.prototype[UNSOLICITED_HARDWARE_CONFIG_CHANGED] = null;
RilObject.prototype[UNSOLICITED_DC_RT_INFO_CHANGED] = null;









function GsmPDUHelperObject(aContext) {
  this.context = aContext;
}
GsmPDUHelperObject.prototype = {
  context: null,

  




  readHexNibble: function() {
    let nibble = this.context.Buf.readUint16();
    if (nibble >= 48 && nibble <= 57) {
      nibble -= 48; 
    } else if (nibble >= 65 && nibble <= 70) {
      nibble -= 55; 
    } else if (nibble >= 97 && nibble <= 102) {
      nibble -= 87; 
    } else {
      throw "Found invalid nibble during PDU parsing: " +
            String.fromCharCode(nibble);
    }
    return nibble;
  },

  





  writeHexNibble: function(nibble) {
    nibble &= 0x0f;
    if (nibble < 10) {
      nibble += 48; 
    } else {
      nibble += 55; 
    }
    this.context.Buf.writeUint16(nibble);
  },

  




  readHexOctet: function() {
    return (this.readHexNibble() << 4) | this.readHexNibble();
  },

  





  writeHexOctet: function(octet) {
    this.writeHexNibble(octet >> 4);
    this.writeHexNibble(octet);
  },

  


  readHexOctetArray: function(length) {
    let array = new Uint8Array(length);
    for (let i = 0; i < length; i++) {
      array[i] = this.readHexOctet();
    }
    return array;
  },

  








  writeWithBuffer: function(writeFunction) {
    let buf = [];
    let writeHexOctet = this.writeHexOctet;
    this.writeHexOctet = function(octet) {
      buf.push(octet);
    }

    try {
      writeFunction();
    } catch (e) {
      if (DEBUG) {
        debug("Error when writeWithBuffer: " + e);
      }
      buf = [];
    } finally {
      this.writeHexOctet = writeHexOctet;
    }

    return buf;
  },

  









  octetToBCD: function(octet) {
    return ((octet & 0xf0) <= 0x90) * ((octet >> 4) & 0x0f) +
           ((octet & 0x0f) <= 0x09) * (octet & 0x0f) * 10;
  },

  








  BCDToOctet: function(bcd) {
    bcd = Math.abs(bcd);
    return ((bcd % 10) << 4) + (Math.floor(bcd / 10) % 10);
  },

  











  bcdChars: "0123456789",
  semiOctetToBcdChar: function(semiOctet, suppressException) {
    if (semiOctet >= this.bcdChars.length) {
      if (suppressException) {
        return "";
      } else {
        throw new RangeError();
      }
    }

    return this.bcdChars.charAt(semiOctet);
  },

  











  extendedBcdChars: "0123456789*#,;",
  semiOctetToExtendedBcdChar: function(semiOctet, suppressException) {
    if (semiOctet >= this.extendedBcdChars.length) {
      if (suppressException) {
        return "";
      } else {
        throw new RangeError();
      }
    }

    return this.extendedBcdChars.charAt(semiOctet);
  },

  







  readSwappedNibbleBcdNum: function(pairs) {
    let number = 0;
    for (let i = 0; i < pairs; i++) {
      let octet = this.readHexOctet();
      
      if (octet == 0xff) {
        continue;
      }
      
      
      if ((octet & 0xf0) == 0xf0) {
        number *= 10;
        number += octet & 0x0f;
        continue;
      }
      number *= 100;
      number += this.octetToBCD(octet);
    }
    return number;
  },

  










  readSwappedNibbleBcdString: function(pairs, suppressException) {
    let str = "";
    for (let i = 0; i < pairs; i++) {
      let nibbleH = this.readHexNibble();
      let nibbleL = this.readHexNibble();
      if (nibbleL == 0x0F) {
        break;
      }

      str += this.semiOctetToBcdChar(nibbleL, suppressException);
      if (nibbleH != 0x0F) {
        str += this.semiOctetToBcdChar(nibbleH, suppressException);
      }
    }

    return str;
  },

  










  readSwappedNibbleExtendedBcdString: function(pairs, suppressException) {
    let str = "";
    for (let i = 0; i < pairs; i++) {
      let nibbleH = this.readHexNibble();
      let nibbleL = this.readHexNibble();
      if (nibbleL == 0x0F) {
        break;
      }

      str += this.semiOctetToExtendedBcdChar(nibbleL, suppressException);
      if (nibbleH != 0x0F) {
        str += this.semiOctetToExtendedBcdChar(nibbleH, suppressException);
      }
    }

    return str;
  },

  





  writeSwappedNibbleBCD: function(data) {
    data = data.toString();
    if (data.length % 2) {
      data += "F";
    }
    let Buf = this.context.Buf;
    for (let i = 0; i < data.length; i += 2) {
      Buf.writeUint16(data.charCodeAt(i + 1));
      Buf.writeUint16(data.charCodeAt(i));
    }
  },

  






  writeSwappedNibbleBCDNum: function(data) {
    data = data.toString();
    if (data.length % 2) {
      data = "0" + data;
    }
    let Buf = this.context.Buf;
    for (let i = 0; i < data.length; i += 2) {
      Buf.writeUint16(data.charCodeAt(i + 1));
      Buf.writeUint16(data.charCodeAt(i));
    }
  },

  














  readSeptetsToString: function(length, paddingBits, langIndex, langShiftIndex) {
    let ret = "";
    let byteLength = Math.ceil((length * 7 + paddingBits) / 8);

    












    let data = 0;
    let dataBits = 0;
    if (paddingBits) {
      data = this.readHexOctet() >> paddingBits;
      dataBits = 8 - paddingBits;
      --byteLength;
    }

    let escapeFound = false;
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[langIndex];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[langShiftIndex];
    do {
      
      let bytesToRead = Math.min(byteLength, dataBits ? 3 : 4);
      for (let i = 0; i < bytesToRead; i++) {
        data |= this.readHexOctet() << dataBits;
        dataBits += 8;
        --byteLength;
      }

      
      for (; dataBits >= 7; dataBits -= 7) {
        let septet = data & 0x7F;
        data >>>= 7;

        if (escapeFound) {
          escapeFound = false;
          if (septet == PDU_NL_EXTENDED_ESCAPE) {
            
            
            
            ret += " ";
          } else if (septet == PDU_NL_RESERVED_CONTROL) {
            
            
            
            ret += " ";
          } else {
            ret += langShiftTable[septet];
          }
        } else if (septet == PDU_NL_EXTENDED_ESCAPE) {
          escapeFound = true;

          
          --length;
        } else {
          ret += langTable[septet];
        }
      }
    } while (byteLength);

    if (ret.length != length) {
      















      ret = ret.slice(0, length);
    }
    return ret;
  },

  writeStringAsSeptets: function(message, paddingBits, langIndex, langShiftIndex) {
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[langIndex];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[langShiftIndex];

    let dataBits = paddingBits;
    let data = 0;
    for (let i = 0; i < message.length; i++) {
      let c = message.charAt(i);
      let septet = langTable.indexOf(c);
      if (septet == PDU_NL_EXTENDED_ESCAPE) {
        continue;
      }

      if (septet >= 0) {
        data |= septet << dataBits;
        dataBits += 7;
      } else {
        septet = langShiftTable.indexOf(c);
        if (septet == -1) {
          throw new Error("'" + c + "' is not in 7 bit alphabet "
                          + langIndex + ":" + langShiftIndex + "!");
        }

        if (septet == PDU_NL_RESERVED_CONTROL) {
          continue;
        }

        data |= PDU_NL_EXTENDED_ESCAPE << dataBits;
        dataBits += 7;
        data |= septet << dataBits;
        dataBits += 7;
      }

      for (; dataBits >= 8; dataBits -= 8) {
        this.writeHexOctet(data & 0xFF);
        data >>>= 8;
      }
    }

    if (dataBits !== 0) {
      this.writeHexOctet(data & 0xFF);
    }
  },

  writeStringAs8BitUnpacked: function(text) {
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

    let len = text ? text.length : 0;
    for (let i = 0; i < len; i++) {
      let c = text.charAt(i);
      let octet = langTable.indexOf(c);

      if (octet == -1) {
        octet = langShiftTable.indexOf(c);
        if (octet == -1) {
          
          octet = langTable.indexOf(' ');
        } else {
          this.writeHexOctet(PDU_NL_EXTENDED_ESCAPE);
        }
      }
      this.writeHexOctet(octet);
    }
  },

  







  readUCS2String: function(numOctets) {
    let str = "";
    let length = numOctets / 2;
    for (let i = 0; i < length; ++i) {
      let code = (this.readHexOctet() << 8) | this.readHexOctet();
      str += String.fromCharCode(code);
    }

    if (DEBUG) this.context.debug("Read UCS2 string: " + str);

    return str;
  },

  





  writeUCS2String: function(message) {
    for (let i = 0; i < message.length; ++i) {
      let code = message.charCodeAt(i);
      this.writeHexOctet((code >> 8) & 0xFF);
      this.writeHexOctet(code & 0xFF);
    }
  },

  







  readUserDataHeader: function(msg) {
    










    let header = {
      length: 0,
      langIndex: PDU_NL_IDENTIFIER_DEFAULT,
      langShiftIndex: PDU_NL_IDENTIFIER_DEFAULT
    };

    header.length = this.readHexOctet();
    if (DEBUG) this.context.debug("Read UDH length: " + header.length);

    let dataAvailable = header.length;
    while (dataAvailable >= 2) {
      let id = this.readHexOctet();
      let length = this.readHexOctet();
      if (DEBUG) this.context.debug("Read UDH id: " + id + ", length: " + length);

      dataAvailable -= 2;

      switch (id) {
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT: {
          let ref = this.readHexOctet();
          let max = this.readHexOctet();
          let seq = this.readHexOctet();
          dataAvailable -= 3;
          if (max && seq && (seq <= max)) {
            header.segmentRef = ref;
            header.segmentMaxSeq = max;
            header.segmentSeq = seq;
          }
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_8BIT: {
          let dstp = this.readHexOctet();
          let orip = this.readHexOctet();
          dataAvailable -= 2;
          if ((dstp < PDU_APA_RESERVED_8BIT_PORTS)
              || (orip < PDU_APA_RESERVED_8BIT_PORTS)) {
            
            
            
            break;
          }
          header.destinationPort = dstp;
          header.originatorPort = orip;
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_16BIT: {
          let dstp = (this.readHexOctet() << 8) | this.readHexOctet();
          let orip = (this.readHexOctet() << 8) | this.readHexOctet();
          dataAvailable -= 4;
          if ((dstp >= PDU_APA_VALID_16BIT_PORTS) ||
              (orip >= PDU_APA_VALID_16BIT_PORTS)) {
            
            
            
            
            
            
            
            this.context.debug("Warning: Invalid port numbers [dstp, orip]: " +
                               JSON.stringify([dstp, orip]));
          }
          header.destinationPort = dstp;
          header.originatorPort = orip;
          break;
        }
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_16BIT: {
          let ref = (this.readHexOctet() << 8) | this.readHexOctet();
          let max = this.readHexOctet();
          let seq = this.readHexOctet();
          dataAvailable -= 4;
          if (max && seq && (seq <= max)) {
            header.segmentRef = ref;
            header.segmentMaxSeq = max;
            header.segmentSeq = seq;
          }
          break;
        }
        case PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT:
          let langShiftIndex = this.readHexOctet();
          --dataAvailable;
          if (langShiftIndex < PDU_NL_SINGLE_SHIFT_TABLES.length) {
            header.langShiftIndex = langShiftIndex;
          }
          break;
        case PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT:
          let langIndex = this.readHexOctet();
          --dataAvailable;
          if (langIndex < PDU_NL_LOCKING_SHIFT_TABLES.length) {
            header.langIndex = langIndex;
          }
          break;
        case PDU_IEI_SPECIAL_SMS_MESSAGE_INDICATION:
          let msgInd = this.readHexOctet() & 0xFF;
          let msgCount = this.readHexOctet();
          dataAvailable -= 2;


          






          let storeType = msgInd & PDU_MWI_STORE_TYPE_BIT;
          let mwi = msg.mwi;
          if (!mwi) {
            mwi = msg.mwi = {};
          }

          if (storeType == PDU_MWI_STORE_TYPE_STORE) {
            
            
            mwi.discard = false;
          } else if (mwi.discard === undefined) {
            
            
            mwi.discard = true;
          }

          mwi.msgCount = msgCount & 0xFF;
          mwi.active = mwi.msgCount > 0;

          if (DEBUG) {
            this.context.debug("MWI in TP_UDH received: " + JSON.stringify(mwi));
          }

          break;
        default:
          if (DEBUG) {
            this.context.debug("readUserDataHeader: unsupported IEI(" + id +
                               "), " + length + " bytes.");
          }

          
          if (length) {
            let octets;
            if (DEBUG) octets = new Uint8Array(length);

            for (let i = 0; i < length; i++) {
              let octet = this.readHexOctet();
              if (DEBUG) octets[i] = octet;
            }
            dataAvailable -= length;

            if (DEBUG) {
              this.context.debug("readUserDataHeader: " + Array.slice(octets));
            }
          }
          break;
      }
    }

    if (dataAvailable !== 0) {
      throw new Error("Illegal user data header found!");
    }

    msg.header = header;
  },

  






  writeUserDataHeader: function(options) {
    this.writeHexOctet(options.userDataHeaderLength);

    if (options.segmentMaxSeq > 1) {
      if (options.segmentRef16Bit) {
        this.writeHexOctet(PDU_IEI_CONCATENATED_SHORT_MESSAGES_16BIT);
        this.writeHexOctet(4);
        this.writeHexOctet((options.segmentRef >> 8) & 0xFF);
      } else {
        this.writeHexOctet(PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT);
        this.writeHexOctet(3);
      }
      this.writeHexOctet(options.segmentRef & 0xFF);
      this.writeHexOctet(options.segmentMaxSeq & 0xFF);
      this.writeHexOctet(options.segmentSeq & 0xFF);
    }

    if (options.dcs == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      if (options.langIndex != PDU_NL_IDENTIFIER_DEFAULT) {
        this.writeHexOctet(PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT);
        this.writeHexOctet(1);
        this.writeHexOctet(options.langIndex);
      }

      if (options.langShiftIndex != PDU_NL_IDENTIFIER_DEFAULT) {
        this.writeHexOctet(PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT);
        this.writeHexOctet(1);
        this.writeHexOctet(options.langShiftIndex);
      }
    }
  },

  








  readAddress: function(len) {
    
    if (!len || (len < 0)) {
      if (DEBUG) {
        this.context.debug("PDU error: invalid sender address length: " + len);
      }
      return null;
    }
    if (len % 2 == 1) {
      len += 1;
    }
    if (DEBUG) this.context.debug("PDU: Going to read address: " + len);

    
    let toa = this.readHexOctet();
    let addr = "";

    if ((toa & 0xF0) == PDU_TOA_ALPHANUMERIC) {
      addr = this.readSeptetsToString(Math.floor(len * 4 / 7), 0,
          PDU_NL_IDENTIFIER_DEFAULT , PDU_NL_IDENTIFIER_DEFAULT );
      return addr;
    }
    addr = this.readSwappedNibbleExtendedBcdString(len / 2);
    if (addr.length <= 0) {
      if (DEBUG) this.context.debug("PDU error: no number provided");
      return null;
    }
    if ((toa & 0xF0) == (PDU_TOA_INTERNATIONAL)) {
      addr = '+' + addr;
    }

    return addr;
  },

  







  readProtocolIndicator: function(msg) {
    
    
    msg.pid = this.readHexOctet();

    msg.epid = msg.pid;
    switch (msg.epid & 0xC0) {
      case 0x40:
        
        switch (msg.epid) {
          case PDU_PID_SHORT_MESSAGE_TYPE_0:
          case PDU_PID_ANSI_136_R_DATA:
          case PDU_PID_USIM_DATA_DOWNLOAD:
            return;
        }
        break;
    }

    msg.epid = PDU_PID_DEFAULT;
  },

  







  readDataCodingScheme: function(msg) {
    let dcs = this.readHexOctet();
    if (DEBUG) this.context.debug("PDU: read SMS dcs: " + dcs);

    
    let messageClass = PDU_DCS_MSG_CLASS_NORMAL;
    
    let encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
    switch (dcs & PDU_DCS_CODING_GROUP_BITS) {
      case 0x40: 
      case 0x50:
      case 0x60:
      case 0x70:
        
      case 0x00: 
      case 0x10:
      case 0x20:
      case 0x30:
        if (dcs & 0x10) {
          messageClass = dcs & PDU_DCS_MSG_CLASS_BITS;
        }
        switch (dcs & 0x0C) {
          case 0x4:
            encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
            break;
          case 0x8:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            break;
        }
        break;

      case 0xE0: 
        encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
        
        
        
      case 0xC0: 
      case 0xD0: 
        
        let active = (dcs & PDU_DCS_MWI_ACTIVE_BITS) == PDU_DCS_MWI_ACTIVE_VALUE;

        
        switch (dcs & PDU_DCS_MWI_TYPE_BITS) {
          case PDU_DCS_MWI_TYPE_VOICEMAIL:
            let mwi = msg.mwi;
            if (!mwi) {
              mwi = msg.mwi = {};
            }

            mwi.active = active;
            mwi.discard = (dcs & PDU_DCS_CODING_GROUP_BITS) == 0xC0;
            mwi.msgCount = active ? GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN : 0;

            if (DEBUG) {
              this.context.debug("MWI in DCS received for voicemail: " +
                                 JSON.stringify(mwi));
            }
            break;
          case PDU_DCS_MWI_TYPE_FAX:
            if (DEBUG) this.context.debug("MWI in DCS received for fax");
            break;
          case PDU_DCS_MWI_TYPE_EMAIL:
            if (DEBUG) this.context.debug("MWI in DCS received for email");
            break;
          default:
            if (DEBUG) this.context.debug("MWI in DCS received for \"other\"");
            break;
        }
        break;

      case 0xF0: 
        if (dcs & 0x04) {
          encoding = PDU_DCS_MSG_CODING_8BITS_ALPHABET;
        }
        messageClass = dcs & PDU_DCS_MSG_CLASS_BITS;
        break;

      default:
        
        break;
    }

    msg.dcs = dcs;
    msg.encoding = encoding;
    msg.messageClass = GECKO_SMS_MESSAGE_CLASSES[messageClass];

    if (DEBUG) this.context.debug("PDU: message encoding is " + encoding + " bit.");
  },

  




  readTimestamp: function() {
    let year   = this.readSwappedNibbleBcdNum(1) + PDU_TIMESTAMP_YEAR_OFFSET;
    let month  = this.readSwappedNibbleBcdNum(1) - 1;
    let day    = this.readSwappedNibbleBcdNum(1);
    let hour   = this.readSwappedNibbleBcdNum(1);
    let minute = this.readSwappedNibbleBcdNum(1);
    let second = this.readSwappedNibbleBcdNum(1);
    let timestamp = Date.UTC(year, month, day, hour, minute, second);

    
    
    
    
    
    let tzOctet = this.readHexOctet();
    let tzOffset = this.octetToBCD(tzOctet & ~0x08) * 15 * 60 * 1000;
    tzOffset = (tzOctet & 0x08) ? -tzOffset : tzOffset;
    timestamp -= tzOffset;

    return timestamp;
  },

  




  writeTimestamp: function(date) {
    this.writeSwappedNibbleBCDNum(date.getFullYear() - PDU_TIMESTAMP_YEAR_OFFSET);

    
    
    this.writeSwappedNibbleBCDNum(date.getMonth() + 1);
    this.writeSwappedNibbleBCDNum(date.getDate());
    this.writeSwappedNibbleBCDNum(date.getHours());
    this.writeSwappedNibbleBCDNum(date.getMinutes());
    this.writeSwappedNibbleBCDNum(date.getSeconds());

    
    
    
    
    
    
    
    let zone = date.getTimezoneOffset() / 15;
    let octet = this.BCDToOctet(zone);

    
    
    
    
    
    if (zone > 0) {
      octet = octet | 0x08;
    }
    this.writeHexOctet(octet);
  },

  








  readUserData: function(msg, length) {
    if (DEBUG) {
      this.context.debug("Reading " + length + " bytes of user data.");
    }

    let paddingBits = 0;
    if (msg.udhi) {
      this.readUserDataHeader(msg);

      if (msg.encoding == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
        let headerBits = (msg.header.length + 1) * 8;
        let headerSeptets = Math.ceil(headerBits / 7);

        length -= headerSeptets;
        paddingBits = headerSeptets * 7 - headerBits;
      } else {
        length -= (msg.header.length + 1);
      }
    }

    if (DEBUG) {
      this.context.debug("After header, " + length + " septets left of user data");
    }

    msg.body = null;
    msg.data = null;

    if (length <= 0) {
      
      return;
    }

    switch (msg.encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        
        
        if (length > PDU_MAX_USER_DATA_7BIT) {
          if (DEBUG) {
            this.context.debug("PDU error: user data is too long: " + length);
          }
          break;
        }

        let langIndex = msg.udhi ? msg.header.langIndex : PDU_NL_IDENTIFIER_DEFAULT;
        let langShiftIndex = msg.udhi ? msg.header.langShiftIndex : PDU_NL_IDENTIFIER_DEFAULT;
        msg.body = this.readSeptetsToString(length, paddingBits, langIndex,
                                            langShiftIndex);
        break;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        msg.data = this.readHexOctetArray(length);
        break;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        msg.body = this.readUCS2String(length);
        break;
    }
  },

  





  readExtraParams: function(msg) {
    
    
    
    if (this.context.Buf.getReadAvailable() <= 4) {
      return;
    }

    
    let pi;
    do {
      
      
      
      
      pi = this.readHexOctet();
    } while (pi & PDU_PI_EXTENSION);

    
    
    
    msg.dcs = 0;
    msg.encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;

    
    if (pi & PDU_PI_PROTOCOL_IDENTIFIER) {
      this.readProtocolIndicator(msg);
    }
    
    if (pi & PDU_PI_DATA_CODING_SCHEME) {
      this.readDataCodingScheme(msg);
    }
    
    if (pi & PDU_PI_USER_DATA_LENGTH) {
      let userDataLength = this.readHexOctet();
      this.readUserData(msg, userDataLength);
    }
  },

  





  readMessage: function() {
    
    let msg = {
      
      
      
      
      SMSC:              null, 
      mti:               null, 
      udhi:              null, 
      sender:            null, 
      recipient:         null, 
      pid:               null, 
      epid:              null, 
      dcs:               null, 
      mwi:               null, 
      replace:          false, 
      header:            null, 
      body:              null, 
      data:              null, 
      sentTimestamp:     null, 
      status:            null, 
      scts:              null, 
      dt:                null, 
    };

    
    let smscLength = this.readHexOctet();
    if (smscLength > 0) {
      let smscTypeOfAddress = this.readHexOctet();
      
      msg.SMSC = this.readSwappedNibbleExtendedBcdString(smscLength - 1);
      if ((smscTypeOfAddress >> 4) == (PDU_TOA_INTERNATIONAL >> 4)) {
        msg.SMSC = '+' + msg.SMSC;
      }
    }

    
    let firstOctet = this.readHexOctet();
    
    msg.mti = firstOctet & 0x03;
    
    msg.udhi = firstOctet & PDU_UDHI;

    switch (msg.mti) {
      case PDU_MTI_SMS_RESERVED:
        
        
        
      case PDU_MTI_SMS_DELIVER:
        return this.readDeliverMessage(msg);
      case PDU_MTI_SMS_STATUS_REPORT:
        return this.readStatusReportMessage(msg);
      default:
        return null;
    }
  },

  







  processReceivedSms: function(length) {
    if (!length) {
      if (DEBUG) this.context.debug("Received empty SMS!");
      return [null, PDU_FCS_UNSPECIFIED];
    }

    let Buf = this.context.Buf;

    
    
    let messageStringLength = Buf.readInt32();
    if (DEBUG) this.context.debug("Got new SMS, length " + messageStringLength);
    let message = this.readMessage();
    if (DEBUG) this.context.debug("Got new SMS: " + JSON.stringify(message));

    
    Buf.readStringDelimiter(length);

    
    if (!message) {
      return [null, PDU_FCS_UNSPECIFIED];
    }

    if (message.epid == PDU_PID_SHORT_MESSAGE_TYPE_0) {
      
      
      
      return [null, PDU_FCS_OK];
    }

    if (message.messageClass == GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]) {
      let RIL = this.context.RIL;
      switch (message.epid) {
        case PDU_PID_ANSI_136_R_DATA:
        case PDU_PID_USIM_DATA_DOWNLOAD:
          let ICCUtilsHelper = this.context.ICCUtilsHelper;
          if (ICCUtilsHelper.isICCServiceAvailable("DATA_DOWNLOAD_SMS_PP")) {
            
            
            
            
            RIL.dataDownloadViaSMSPP(message);

            
            
            return [null, PDU_FCS_RESERVED];
          }

          
          
          

          
        default:
          RIL.writeSmsToSIM(message);
          break;
      }
    }

    
    if ((message.messageClass != GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_0]) && !true) {
      
      
      
      

      if (message.messageClass == GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_2]) {
        
        
        return [null, PDU_FCS_MEMORY_CAPACITY_EXCEEDED];
      }

      return [null, PDU_FCS_UNSPECIFIED];
    }

    return [message, PDU_FCS_OK];
  },

  





  readDeliverMessage: function(msg) {
    
    let senderAddressLength = this.readHexOctet();
    msg.sender = this.readAddress(senderAddressLength);
    
    this.readProtocolIndicator(msg);
    
    this.readDataCodingScheme(msg);
    
    msg.sentTimestamp = this.readTimestamp();
    
    let userDataLength = this.readHexOctet();

    
    if (userDataLength > 0) {
      this.readUserData(msg, userDataLength);
    }

    return msg;
  },

  





  readStatusReportMessage: function(msg) {
    
    msg.messageRef = this.readHexOctet();
    
    let recipientAddressLength = this.readHexOctet();
    msg.recipient = this.readAddress(recipientAddressLength);
    
    msg.scts = this.readTimestamp();
    
    msg.dt = this.readTimestamp();
    
    msg.status = this.readHexOctet();

    this.readExtraParams(msg);

    return msg;
  },

  


























  writeMessage: function(options) {
    if (DEBUG) {
      this.context.debug("writeMessage: " + JSON.stringify(options));
    }
    let Buf = this.context.Buf;
    let address = options.number;
    let body = options.body;
    let dcs = options.dcs;
    let userDataHeaderLength = options.userDataHeaderLength;
    let encodedBodyLength = options.encodedBodyLength;
    let langIndex = options.langIndex;
    let langShiftIndex = options.langShiftIndex;

    
    
    
    
    
    
    
    
    
    

    let addressFormat = PDU_TOA_ISDN; 
    if (address[0] == '+') {
      addressFormat = PDU_TOA_INTERNATIONAL | PDU_TOA_ISDN; 
      address = address.substring(1);
    }
    
    let validity = 0;

    let headerOctets = (userDataHeaderLength ? userDataHeaderLength + 1 : 0);
    let paddingBits;
    let userDataLengthInSeptets;
    let userDataLengthInOctets;
    if (dcs == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      let headerSeptets = Math.ceil(headerOctets * 8 / 7);
      userDataLengthInSeptets = headerSeptets + encodedBodyLength;
      userDataLengthInOctets = Math.ceil(userDataLengthInSeptets * 7 / 8);
      paddingBits = headerSeptets * 7 - headerOctets * 8;
    } else {
      userDataLengthInOctets = headerOctets + encodedBodyLength;
      paddingBits = 0;
    }

    let pduOctetLength = 4 + 
                         Math.ceil(address.length / 2) +
                         3 + 
                         userDataLengthInOctets;
    if (validity) {
      
    }

    
    
    Buf.writeInt32(pduOctetLength * 2);

    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    let firstOctet = PDU_MTI_SMS_SUBMIT;

    
    if (options.requestStatusReport) {
      firstOctet |= PDU_SRI_SRR;
    }

    
    if (validity) {
      
    }
    
    if (headerOctets) {
      firstOctet |= PDU_UDHI;
    }
    this.writeHexOctet(firstOctet);

    
    this.writeHexOctet(0x00);

    
    this.writeHexOctet(address.length);
    this.writeHexOctet(addressFormat);
    this.writeSwappedNibbleBCD(address);

    
    this.writeHexOctet(0x00);

    
    
    this.writeHexOctet(dcs);

    
    if (validity) {
      this.writeHexOctet(validity);
    }

    
    if (dcs == PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      this.writeHexOctet(userDataLengthInSeptets);
    } else {
      this.writeHexOctet(userDataLengthInOctets);
    }

    if (headerOctets) {
      this.writeUserDataHeader(options);
    }

    switch (dcs) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        this.writeStringAsSeptets(body, paddingBits, langIndex, langShiftIndex);
        break;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        
        break;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        this.writeUCS2String(body);
        break;
    }

    
    
    Buf.writeUint16(0);
    Buf.writeUint16(0);
  },

  







  readCbSerialNumber: function(msg) {
    let Buf = this.context.Buf;
    msg.serial = Buf.readUint8() << 8 | Buf.readUint8();
    msg.geographicalScope = (msg.serial >>> 14) & 0x03;
    msg.messageCode = (msg.serial >>> 4) & 0x03FF;
    msg.updateNumber = msg.serial & 0x0F;
  },

  







  readCbMessageIdentifier: function(msg) {
    let Buf = this.context.Buf;
    msg.messageId = Buf.readUint8() << 8 | Buf.readUint8();
  },

  






  readCbEtwsInfo: function(msg) {
    if ((msg.format != CB_FORMAT_ETWS)
        && (msg.messageId >= CB_GSM_MESSAGEID_ETWS_BEGIN)
        && (msg.messageId <= CB_GSM_MESSAGEID_ETWS_END)) {
      
      
      
      msg.etws = {
        emergencyUserAlert: msg.messageCode & 0x0200 ? true : false,
        popup:              msg.messageCode & 0x0100 ? true : false
      };

      let warningType = msg.messageId - CB_GSM_MESSAGEID_ETWS_BEGIN;
      if (warningType < CB_ETWS_WARNING_TYPE_NAMES.length) {
        msg.etws.warningType = warningType;
      }
    }
  },

  







  readCbDataCodingScheme: function(msg) {
    let dcs = this.context.Buf.readUint8();
    if (DEBUG) this.context.debug("PDU: read CBS dcs: " + dcs);

    let language = null, hasLanguageIndicator = false;
    
    
    let encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
    let messageClass = PDU_DCS_MSG_CLASS_NORMAL;

    switch (dcs & PDU_DCS_CODING_GROUP_BITS) {
      case 0x00: 
        language = CB_DCS_LANG_GROUP_1[dcs & 0x0F];
        break;

      case 0x10: 
        switch (dcs & 0x0F) {
          case 0x00:
            hasLanguageIndicator = true;
            break;
          case 0x01:
            encoding = PDU_DCS_MSG_CODING_16BITS_ALPHABET;
            hasLanguageIndicator = true;
            break;
        }
        break;

      case 0x20: 
        language = CB_DCS_LANG_GROUP_2[dcs & 0x0F];
        break;

      case 0x40: 
      case 0x50:
      
      
      case 0x90: 
        encoding = (dcs & 0x0C);
        if (encoding == 0x0C) {
          encoding = PDU_DCS_MSG_CODING_7BITS_ALPHABET;
        }
        messageClass = (dcs & PDU_DCS_MSG_CLASS_BITS);
        break;

      case 0xF0:
        encoding = (dcs & 0x04) ? PDU_DCS_MSG_CODING_8BITS_ALPHABET
                                : PDU_DCS_MSG_CODING_7BITS_ALPHABET;
        switch(dcs & PDU_DCS_MSG_CLASS_BITS) {
          case 0x01: messageClass = PDU_DCS_MSG_CLASS_USER_1; break;
          case 0x02: messageClass = PDU_DCS_MSG_CLASS_USER_2; break;
          case 0x03: messageClass = PDU_DCS_MSG_CLASS_3; break;
        }
        break;

      case 0x30: 
      case 0x80: 
      case 0xA0: 
      case 0xB0:
      case 0xC0:
        break;

      default:
        throw new Error("Unsupported CBS data coding scheme: " + dcs);
    }

    msg.dcs = dcs;
    msg.encoding = encoding;
    msg.language = language;
    msg.messageClass = GECKO_SMS_MESSAGE_CLASSES[messageClass];
    msg.hasLanguageIndicator = hasLanguageIndicator;
  },

  







  readCbPageParameter: function(msg) {
    let octet = this.context.Buf.readUint8();
    msg.pageIndex = (octet >>> 4) & 0x0F;
    msg.numPages = octet & 0x0F;
    if (!msg.pageIndex || !msg.numPages) {
      
      
      
      msg.pageIndex = msg.numPages = 1;
    }
  },

  







  readCbWarningType: function(msg) {
    let Buf = this.context.Buf;
    let word = Buf.readUint8() << 8 | Buf.readUint8();
    msg.etws = {
      warningType:        (word >>> 9) & 0x7F,
      popup:              word & 0x80 ? true : false,
      emergencyUserAlert: word & 0x100 ? true : false
    };
  },

  







  readGsmCbData: function(msg, length) {
    let Buf = this.context.Buf;
    let bufAdapter = {
      context: this.context,
      readHexOctet: function() {
        return Buf.readUint8();
      }
    };

    msg.body = null;
    msg.data = null;
    switch (msg.encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        msg.body = this.readSeptetsToString.call(bufAdapter,
                                                 Math.floor(length * 8 / 7), 0,
                                                 PDU_NL_IDENTIFIER_DEFAULT,
                                                 PDU_NL_IDENTIFIER_DEFAULT);
        if (msg.hasLanguageIndicator) {
          msg.language = msg.body.substring(0, 2);
          msg.body = msg.body.substring(3);
        }
        break;

      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        msg.data = Buf.readUint8Array(length);
        break;

      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        if (msg.hasLanguageIndicator) {
          msg.language = this.readSeptetsToString.call(bufAdapter, 2, 0,
                                                       PDU_NL_IDENTIFIER_DEFAULT,
                                                       PDU_NL_IDENTIFIER_DEFAULT);
          length -= 2;
        }
        msg.body = this.readUCS2String.call(bufAdapter, length);
        break;
    }

    if (msg.data || !msg.body) {
      return;
    }

    
    
    
    
    
    
    
    
    for (let i = msg.body.length - 1; i >= 0; i--) {
      if (msg.body.charAt(i) !== '\r') {
        msg.body = msg.body.substring(0, i + 1);
        break;
      }
    }
  },

  












  readUmtsCbData: function(msg) {
    let Buf = this.context.Buf;
    let numOfPages = Buf.readUint8();
    if (numOfPages < 0 || numOfPages > 15) {
      throw new Error("Invalid numOfPages: " + numOfPages);
    }

    let bufAdapter = {
      context: this.context,
      readHexOctet: function() {
        return Buf.readUint8();
      }
    };

    let removePaddingCharactors = function (text) {
      for (let i = text.length - 1; i >= 0; i--) {
        if (text.charAt(i) !== '\r') {
          return text.substring(0, i + 1);
        }
      }
      return text;
    };

    let totalLength = 0, length, pageLengths = [];
    for (let i = 0; i < numOfPages; i++) {
      Buf.seekIncoming(CB_MSG_PAGE_INFO_SIZE);
      length = Buf.readUint8();
      totalLength += length;
      pageLengths.push(length);
    }

    
    Buf.seekIncoming(-numOfPages * (CB_MSG_PAGE_INFO_SIZE + 1));

    switch (msg.encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET: {
        let body;
        msg.body = "";
        for (let i = 0; i < numOfPages; i++) {
          body = this.readSeptetsToString.call(bufAdapter,
                                               Math.floor(pageLengths[i] * 8 / 7),
                                               0,
                                               PDU_NL_IDENTIFIER_DEFAULT,
                                               PDU_NL_IDENTIFIER_DEFAULT);
          if (msg.hasLanguageIndicator) {
            if (!msg.language) {
              msg.language = body.substring(0, 2);
            }
            body = body.substring(3);
          }

          msg.body += removePaddingCharactors(body);

          
          Buf.seekIncoming(CB_MSG_PAGE_INFO_SIZE - pageLengths[i]);
          
          Buf.readUint8();
        }

        break;
      }

      case PDU_DCS_MSG_CODING_8BITS_ALPHABET: {
        msg.data = new Uint8Array(totalLength);
        for (let i = 0, j = 0; i < numOfPages; i++) {
          for (let pageLength = pageLengths[i]; pageLength > 0; pageLength--) {
              msg.data[j++] = Buf.readUint8();
          }

          
          Buf.seekIncoming(CB_MSG_PAGE_INFO_SIZE - pageLengths[i]);
          
          Buf.readUint8();
        }

        break;
      }

      case PDU_DCS_MSG_CODING_16BITS_ALPHABET: {
        msg.body = "";
        for (let i = 0; i < numOfPages; i++) {
          let pageLength = pageLengths[i];
          if (msg.hasLanguageIndicator) {
            if (!msg.language) {
              msg.language = this.readSeptetsToString.call(bufAdapter,
                                                           2,
                                                           0,
                                                           PDU_NL_IDENTIFIER_DEFAULT,
                                                           PDU_NL_IDENTIFIER_DEFAULT);
            } else {
              Buf.readUint16();
            }

            pageLength -= 2;
          }

          msg.body += removePaddingCharactors(
                        this.readUCS2String.call(bufAdapter, pageLength));

          
          Buf.seekIncoming(CB_MSG_PAGE_INFO_SIZE - pageLengths[i]);
          
          Buf.readUint8();
        }

        break;
      }
    }
  },

  





  readCbMessage: function(pduLength) {
    
    let msg = {
      
      serial:               null,                              
      updateNumber:         null,                              
      format:               null,                              
      dcs:                  0x0F,                              
      encoding:             PDU_DCS_MSG_CODING_7BITS_ALPHABET, 
      hasLanguageIndicator: false,                             
      data:                 null,                              
      body:                 null,                              
      pageIndex:            1,                                 
      numPages:             1,                                 

      
      geographicalScope:    null,                              
      messageCode:          null,                              
      messageId:            null,                              
      language:             null,                              
      fullBody:             null,                              
      fullData:             null,                              
      messageClass:         GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL], 
      etws:                 null                               
      




    };

    if (pduLength <= CB_MESSAGE_SIZE_ETWS) {
      msg.format = CB_FORMAT_ETWS;
      return this.readEtwsCbMessage(msg);
    }

    if (pduLength <= CB_MESSAGE_SIZE_GSM) {
      msg.format = CB_FORMAT_GSM;
      return this.readGsmCbMessage(msg, pduLength);
    }

    if (pduLength >= CB_MESSAGE_SIZE_UMTS_MIN &&
        pduLength <= CB_MESSAGE_SIZE_UMTS_MAX) {
      msg.format = CB_FORMAT_UMTS;
      return this.readUmtsCbMessage(msg);
    }

    throw new Error("Invalid PDU Length: " + pduLength);
  },

  








  readUmtsCbMessage: function(msg) {
    let Buf = this.context.Buf;
    let type = Buf.readUint8();
    if (type != CB_UMTS_MESSAGE_TYPE_CBS) {
      throw new Error("Unsupported UMTS Cell Broadcast message type: " + type);
    }

    this.readCbMessageIdentifier(msg);
    this.readCbSerialNumber(msg);
    this.readCbEtwsInfo(msg);
    this.readCbDataCodingScheme(msg);
    this.readUmtsCbData(msg);

    return msg;
  },

  









  readGsmCbMessage: function(msg, pduLength) {
    this.readCbSerialNumber(msg);
    this.readCbMessageIdentifier(msg);
    this.readCbEtwsInfo(msg);
    this.readCbDataCodingScheme(msg);
    this.readCbPageParameter(msg);

    
    this.readGsmCbData(msg, pduLength - 6);

    return msg;
  },

  







  readEtwsCbMessage: function(msg) {
    this.readCbSerialNumber(msg);
    this.readCbMessageIdentifier(msg);
    this.readCbWarningType(msg);

    
    
    

    return msg;
  },

  










  readNetworkName: function(len) {
    
    
    
    
    
    
    
    
    

    let codingInfo = this.readHexOctet();
    if (!(codingInfo & 0x80)) {
      return null;
    }

    let textEncoding = (codingInfo & 0x70) >> 4;
    let shouldIncludeCountryInitials = !!(codingInfo & 0x08);
    let spareBits = codingInfo & 0x07;
    let resultString;

    switch (textEncoding) {
    case 0:
      
      resultString = this.readSeptetsToString(
        Math.floor(((len - 1) * 8 - spareBits) / 7), 0,
        PDU_NL_IDENTIFIER_DEFAULT,
        PDU_NL_IDENTIFIER_DEFAULT);
      break;
    case 1:
      
      resultString = this.context.ICCPDUHelper.readAlphaIdentifier(len - 1);
      break;
    default:
      
      return null;
    }

    
    
    return resultString;
  }
};




function BitBufferHelperObject(aContext) {
  this.readBuffer = [];
  this.writeBuffer = [];
}
BitBufferHelperObject.prototype = {
  readCache: 0,
  readCacheSize: 0,
  readBuffer: null,
  readIndex: 0,
  writeCache: 0,
  writeCacheSize: 0,
  writeBuffer: null,

  
  
  readBits: function(length) {
    if (length <= 0 || length > 32) {
      return null;
    }

    if (length > this.readCacheSize) {
      let bytesToRead = Math.ceil((length - this.readCacheSize) / 8);
      for(let i = 0; i < bytesToRead; i++) {
        this.readCache = (this.readCache << 8) | (this.readBuffer[this.readIndex++] & 0xFF);
        this.readCacheSize += 8;
      }
    }

    let bitOffset = (this.readCacheSize - length),
        resultMask = (1 << length) - 1,
        result = 0;

    result = (this.readCache >> bitOffset) & resultMask;
    this.readCacheSize -= length;

    return result;
  },

  backwardReadPilot: function(length) {
    if (length <= 0) {
      return;
    }

    
    let bitIndexToRead = this.readIndex * 8 - this.readCacheSize - length;

    if (bitIndexToRead < 0) {
      return;
    }

    
    let readBits = bitIndexToRead % 8;
    this.readIndex = Math.floor(bitIndexToRead / 8) + ((readBits) ? 1 : 0);
    this.readCache = (readBits) ? this.readBuffer[this.readIndex - 1] : 0;
    this.readCacheSize = (readBits) ? (8 - readBits) : 0;
  },

  writeBits: function(value, length) {
    if (length <= 0 || length > 32) {
      return;
    }

    let totalLength = length + this.writeCacheSize;

    
    if (totalLength < 8) {
      let valueMask = (1 << length) - 1;
      this.writeCache = (this.writeCache << length) | (value & valueMask);
      this.writeCacheSize += length;
      return;
    }

    
    if (this.writeCacheSize) {
      let mergeLength = 8 - this.writeCacheSize,
          valueMask = (1 << mergeLength) - 1;

      this.writeCache = (this.writeCache << mergeLength) | ((value >> (length - mergeLength)) & valueMask);
      this.writeBuffer.push(this.writeCache & 0xFF);
      length -= mergeLength;
    }

    
    while (length >= 8) {
      length -= 8;
      this.writeBuffer.push((value >> length) & 0xFF);
    }

    
    this.writeCacheSize = length;
    this.writeCache = value & ((1 << length) - 1);

    return;
  },

  
  
  nextOctetAlign: function() {
    this.readCache = 0;
    this.readCacheSize = 0;
  },

  
  
  flushWithPadding: function() {
    if (this.writeCacheSize) {
      this.writeBuffer.push(this.writeCache << (8 - this.writeCacheSize));
    }
    this.writeCache = 0;
    this.writeCacheSize = 0;
  },

  startWrite: function(dataBuffer) {
    this.writeBuffer = dataBuffer;
    this.writeCache = 0;
    this.writeCacheSize = 0;
  },

  startRead: function(dataBuffer) {
    this.readBuffer = dataBuffer;
    this.readCache = 0;
    this.readCacheSize = 0;
    this.readIndex = 0;
  },

  getWriteBufferSize: function() {
    return this.writeBuffer.length;
  },

  overwriteWriteBuffer: function(position, data) {
    let writeLength = data.length;
    if (writeLength + position >= this.writeBuffer.length) {
      writeLength = this.writeBuffer.length - position;
    }
    for (let i = 0; i < writeLength; i++) {
      this.writeBuffer[i + position] = data[i];
    }
  }
};








function CdmaPDUHelperObject(aContext) {
  this.context = aContext;
}
CdmaPDUHelperObject.prototype = {
  context: null,

  
  
  dtmfChars: ".1234567890*#...",

  























  writeMessage: function(options) {
    if (DEBUG) {
      this.context.debug("cdma_writeMessage: " + JSON.stringify(options));
    }

    
    options.encoding = this.gsmDcsToCdmaEncoding(options.dcs);

    
    if (options.segmentMaxSeq > 1) {
      this.writeInt(PDU_CDMA_MSG_TELESERIVCIE_ID_WEMT);
    } else {
      this.writeInt(PDU_CDMA_MSG_TELESERIVCIE_ID_SMS);
    }

    this.writeInt(0);
    this.writeInt(PDU_CDMA_MSG_CATEGORY_UNSPEC);

    
    let addrInfo = this.encodeAddr(options.number);
    this.writeByte(addrInfo.digitMode);
    this.writeByte(addrInfo.numberMode);
    this.writeByte(addrInfo.numberType);
    this.writeByte(addrInfo.numberPlan);
    this.writeByte(addrInfo.address.length);
    for (let i = 0; i < addrInfo.address.length; i++) {
      this.writeByte(addrInfo.address[i]);
    }

    
    this.writeByte(0);  
    this.writeByte(0);  
    this.writeByte(0);  

    
    let encodeResult = this.encodeUserData(options);
    this.writeByte(encodeResult.length);
    for (let i = 0; i < encodeResult.length; i++) {
      this.writeByte(encodeResult[i]);
    }

    encodeResult = null;
  },

  


  writeInt: function(value) {
    this.context.Buf.writeInt32(value);
  },

  writeByte: function(value) {
    this.context.Buf.writeInt32(value & 0xFF);
  },

  


  gsmDcsToCdmaEncoding: function(encoding) {
    switch (encoding) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        return PDU_CDMA_MSG_CODING_7BITS_ASCII;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        return PDU_CDMA_MSG_CODING_OCTET;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        return PDU_CDMA_MSG_CODING_UNICODE;
    }
    throw new Error("gsmDcsToCdmaEncoding(): Invalid GSM SMS DCS value: " + encoding);
  },

  







  encodeAddr: function(address) {
    let result = {};

    result.numberType = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_UNKNOWN;
    result.numberPlan = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_UNKNOWN;

    if (address[0] === '+') {
      address = address.substring(1);
    }

    
    result.digitMode = PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF;
    result.numberMode = PDU_CDMA_MSG_ADDR_NUMBER_MODE_ANSI;

    result.address = [];
    for (let i = 0; i < address.length; i++) {
      let addrDigit = this.dtmfChars.indexOf(address.charAt(i));
      if (addrDigit < 0) {
        result.digitMode = PDU_CDMA_MSG_ADDR_DIGIT_MODE_ASCII;
        result.numberMode = PDU_CDMA_MSG_ADDR_NUMBER_MODE_ASCII;
        result.address = [];
        break;
      }
      result.address.push(addrDigit);
    }

    
    if (result.digitMode !== PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF) {
      if (address.indexOf("@") !== -1) {
        result.numberType = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_NATIONAL;
      }

      for (let i = 0; i < address.length; i++) {
        result.address.push(address.charCodeAt(i) & 0x7F);
      }
    }

    return result;
  },

  
























  encodeUserData: function(options) {
    let userDataBuffer = [];
    this.context.BitBufferHelper.startWrite(userDataBuffer);

    
    this.encodeUserDataMsgId(options);

    
    this.encodeUserDataMsg(options);

    
    this.encodeUserDataReplyOption(options);

    return userDataBuffer;
  },

  




  encodeUserDataMsgId: function(options) {
    let BitBufferHelper = this.context.BitBufferHelper;
    BitBufferHelper.writeBits(PDU_CDMA_MSG_USERDATA_MSG_ID, 8);
    BitBufferHelper.writeBits(3, 8);
    BitBufferHelper.writeBits(PDU_CDMA_MSG_TYPE_SUBMIT, 4);
    BitBufferHelper.writeBits(1, 16); 
    if (options.segmentMaxSeq > 1) {
      BitBufferHelper.writeBits(1, 1);
    } else {
      BitBufferHelper.writeBits(0, 1);
    }

    BitBufferHelper.flushWithPadding();
  },

  




  encodeUserDataMsg: function(options) {
    let BitBufferHelper = this.context.BitBufferHelper;
    BitBufferHelper.writeBits(PDU_CDMA_MSG_USERDATA_BODY, 8);
    
    BitBufferHelper.writeBits(0, 8);
    let lengthPosition = BitBufferHelper.getWriteBufferSize();

    BitBufferHelper.writeBits(options.encoding, 5);

    
    let msgBody = options.body,
        msgBodySize = (options.encoding === PDU_CDMA_MSG_CODING_7BITS_ASCII ?
                       options.encodedBodyLength :
                       msgBody.length);
    if (options.segmentMaxSeq > 1) {
      if (options.encoding === PDU_CDMA_MSG_CODING_7BITS_ASCII) {
          BitBufferHelper.writeBits(msgBodySize + 7, 8); 

          BitBufferHelper.writeBits(5, 8);  
          BitBufferHelper.writeBits(PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT, 8);  
          BitBufferHelper.writeBits(3, 8);  
          BitBufferHelper.writeBits(options.segmentRef & 0xFF, 8);      
          BitBufferHelper.writeBits(options.segmentMaxSeq & 0xFF, 8);   
          BitBufferHelper.writeBits(options.segmentSeq & 0xFF, 8);      
          BitBufferHelper.writeBits(0, 1);  
        } else {
          if (options.encoding === PDU_CDMA_MSG_CODING_UNICODE) {
            BitBufferHelper.writeBits(msgBodySize + 3, 8); 
          } else {
            BitBufferHelper.writeBits(msgBodySize + 6, 8); 
          }

          BitBufferHelper.writeBits(5, 8);  
          BitBufferHelper.writeBits(PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT, 8);  
          BitBufferHelper.writeBits(3, 8);  
          BitBufferHelper.writeBits(options.segmentRef & 0xFF, 8);      
          BitBufferHelper.writeBits(options.segmentMaxSeq & 0xFF, 8);   
          BitBufferHelper.writeBits(options.segmentSeq & 0xFF, 8);      
        }
    } else {
      BitBufferHelper.writeBits(msgBodySize, 8);
    }

    
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    for (let i = 0; i < msgBody.length; i++) {
      switch (options.encoding) {
        case PDU_CDMA_MSG_CODING_OCTET: {
          let msgDigit = msgBody.charCodeAt(i);
          BitBufferHelper.writeBits(msgDigit, 8);
          break;
        }
        case PDU_CDMA_MSG_CODING_7BITS_ASCII: {
          let msgDigit = msgBody.charCodeAt(i),
              msgDigitChar = msgBody.charAt(i);

          if (msgDigit >= 32) {
            BitBufferHelper.writeBits(msgDigit, 7);
          } else {
            msgDigit = langTable.indexOf(msgDigitChar);

            if (msgDigit === PDU_NL_EXTENDED_ESCAPE) {
              break;
            }
            if (msgDigit >= 0) {
              BitBufferHelper.writeBits(msgDigit, 7);
            } else {
              msgDigit = langShiftTable.indexOf(msgDigitChar);
              if (msgDigit == -1) {
                throw new Error("'" + msgDigitChar + "' is not in 7 bit alphabet "
                                + langIndex + ":" + langShiftIndex + "!");
              }

              if (msgDigit === PDU_NL_RESERVED_CONTROL) {
                break;
              }

              BitBufferHelper.writeBits(PDU_NL_EXTENDED_ESCAPE, 7);
              BitBufferHelper.writeBits(msgDigit, 7);
            }
          }
          break;
        }
        case PDU_CDMA_MSG_CODING_UNICODE: {
          let msgDigit = msgBody.charCodeAt(i);
          BitBufferHelper.writeBits(msgDigit, 16);
          break;
        }
      }
    }
    BitBufferHelper.flushWithPadding();

    
    let currentPosition = BitBufferHelper.getWriteBufferSize();
    BitBufferHelper.overwriteWriteBuffer(lengthPosition - 1, [currentPosition - lengthPosition]);
  },

  




  encodeUserDataReplyOption: function(options) {
    if (options.requestStatusReport) {
      let BitBufferHelper = this.context.BitBufferHelper;
      BitBufferHelper.writeBits(PDU_CDMA_MSG_USERDATA_REPLY_OPTION, 8);
      BitBufferHelper.writeBits(1, 8);
      BitBufferHelper.writeBits(0, 1); 
      BitBufferHelper.writeBits(1, 1); 
      BitBufferHelper.flushWithPadding();
    }
  },

  



  readMessage: function() {
    let message = {};

    
    message.teleservice = this.readInt();

    
    let isServicePresent = this.readByte();
    if (isServicePresent) {
      message.messageType = PDU_CDMA_MSG_TYPE_BROADCAST;
    } else {
      if (message.teleservice) {
        message.messageType = PDU_CDMA_MSG_TYPE_P2P;
      } else {
        message.messageType = PDU_CDMA_MSG_TYPE_ACK;
      }
    }

    
    message.service = this.readInt();

    
    let addrInfo = {};
    addrInfo.digitMode = (this.readInt() & 0x01);
    addrInfo.numberMode = (this.readInt() & 0x01);
    addrInfo.numberType = (this.readInt() & 0x01);
    addrInfo.numberPlan = (this.readInt() & 0x01);
    addrInfo.addrLength = this.readByte();
    addrInfo.address = [];
    for (let i = 0; i < addrInfo.addrLength; i++) {
      addrInfo.address.push(this.readByte());
    }
    message.sender = this.decodeAddr(addrInfo);

    
    addrInfo.Type = (this.readInt() & 0x07);
    addrInfo.Odd = (this.readByte() & 0x01);
    addrInfo.addrLength = this.readByte();
    for (let i = 0; i < addrInfo.addrLength; i++) {
      let addrDigit = this.readByte();
      message.sender += String.fromCharCode(addrDigit);
    }

    
    this.decodeUserData(message);

    
    let userData = message[PDU_CDMA_MSG_USERDATA_BODY];
    [message.header, message.body, message.encoding, message.data] =
      (userData) ? [userData.header, userData.body, userData.encoding, userData.data]
                 : [null, null, null, null];

    
    
    
    let msgStatus = message[PDU_CDMA_MSG_USER_DATA_MSG_STATUS];
    [message.errorClass, message.msgStatus] =
      (msgStatus) ? [msgStatus.errorClass, msgStatus.msgStatus]
                  : ((message.body) ? [-1, -1] : [0, 0]);

    
    let msg = {
      SMSC:             "",
      mti:              0,
      udhi:             0,
      sender:           message.sender,
      recipient:        null,
      pid:              PDU_PID_DEFAULT,
      epid:             PDU_PID_DEFAULT,
      dcs:              0,
      mwi:              null,
      replace:          false,
      header:           message.header,
      body:             message.body,
      data:             message.data,
      sentTimestamp:    message[PDU_CDMA_MSG_USERDATA_TIMESTAMP],
      language:         message[PDU_CDMA_LANGUAGE_INDICATOR],
      status:           null,
      scts:             null,
      dt:               null,
      encoding:         message.encoding,
      messageClass:     GECKO_SMS_MESSAGE_CLASSES[PDU_DCS_MSG_CLASS_NORMAL],
      messageType:      message.messageType,
      serviceCategory:  message.service,
      subMsgType:       message[PDU_CDMA_MSG_USERDATA_MSG_ID].msgType,
      msgId:            message[PDU_CDMA_MSG_USERDATA_MSG_ID].msgId,
      errorClass:       message.errorClass,
      msgStatus:        message.msgStatus,
      teleservice:      message.teleservice
    };

    return msg;
  },

  







  processReceivedSms: function(length) {
    if (!length) {
      if (DEBUG) this.context.debug("Received empty SMS!");
      return [null, PDU_FCS_UNSPECIFIED];
    }

    let message = this.readMessage();
    if (DEBUG) this.context.debug("Got new SMS: " + JSON.stringify(message));

    
    if (!message) {
      return [null, PDU_FCS_UNSPECIFIED];
    }

    return [message, PDU_FCS_OK];
  },

  


  readInt: function() {
    return this.context.Buf.readInt32();
  },

  readByte: function() {
    return (this.context.Buf.readInt32() & 0xFF);
  },

  












  decodeAddr: function(addrInfo) {
    let result = "";
    for (let i = 0; i < addrInfo.addrLength; i++) {
      if (addrInfo.digitMode === PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF) {
        result += this.dtmfChars.charAt(addrInfo.address[i]);
      } else {
        result += String.fromCharCode(addrInfo.address[i]);
      }
    }
    return result;
  },

  






  decodeUserData: function(message) {
    let userDataLength = this.readInt();

    while (userDataLength > 0) {
      let id = this.readByte(),
          length = this.readByte(),
          userDataBuffer = [];

      for (let i = 0; i < length; i++) {
          userDataBuffer.push(this.readByte());
      }

      this.context.BitBufferHelper.startRead(userDataBuffer);

      switch (id) {
        case PDU_CDMA_MSG_USERDATA_MSG_ID:
          message[id] = this.decodeUserDataMsgId();
          break;
        case PDU_CDMA_MSG_USERDATA_BODY:
          message[id] = this.decodeUserDataMsg(message[PDU_CDMA_MSG_USERDATA_MSG_ID].userHeader);
          break;
        case PDU_CDMA_MSG_USERDATA_TIMESTAMP:
          message[id] = this.decodeUserDataTimestamp();
          break;
        case PDU_CDMA_MSG_USERDATA_REPLY_OPTION:
          message[id] = this.decodeUserDataReplyOption();
          break;
        case PDU_CDMA_LANGUAGE_INDICATOR:
          message[id] = this.decodeLanguageIndicator();
          break;
        case PDU_CDMA_MSG_USERDATA_CALLBACK_NUMBER:
          message[id] = this.decodeUserDataCallbackNumber();
          break;
        case PDU_CDMA_MSG_USER_DATA_MSG_STATUS:
          message[id] = this.decodeUserDataMsgStatus();
          break;
      }

      userDataLength -= (length + 2);
      userDataBuffer = [];
    }
  },

  




  decodeUserDataMsgId: function() {
    let result = {};
    let BitBufferHelper = this.context.BitBufferHelper;
    result.msgType = BitBufferHelper.readBits(4);
    result.msgId = BitBufferHelper.readBits(16);
    result.userHeader = BitBufferHelper.readBits(1);

    return result;
  },

  







  decodeUserDataHeader: function(encoding) {
    let BitBufferHelper = this.context.BitBufferHelper;
    let header = {},
        headerSize = BitBufferHelper.readBits(8),
        userDataHeaderSize = headerSize + 1,
        headerPaddingBits = 0;

    
    if (encoding === PDU_DCS_MSG_CODING_7BITS_ALPHABET) {
      
      header.length = Math.ceil(userDataHeaderSize * 8 / 7);
      
      headerPaddingBits = (header.length * 7) - (userDataHeaderSize * 8);
    } else if (encoding === PDU_DCS_MSG_CODING_8BITS_ALPHABET) {
      header.length = userDataHeaderSize;
    } else {
      header.length = userDataHeaderSize / 2;
    }

    while (headerSize) {
      let identifier = BitBufferHelper.readBits(8),
          length = BitBufferHelper.readBits(8);

      headerSize -= (2 + length);

      switch (identifier) {
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_8BIT: {
          let ref = BitBufferHelper.readBits(8),
              max = BitBufferHelper.readBits(8),
              seq = BitBufferHelper.readBits(8);
          if (max && seq && (seq <= max)) {
            header.segmentRef = ref;
            header.segmentMaxSeq = max;
            header.segmentSeq = seq;
          }
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_8BIT: {
          let dstp = BitBufferHelper.readBits(8),
              orip = BitBufferHelper.readBits(8);
          if ((dstp < PDU_APA_RESERVED_8BIT_PORTS)
              || (orip < PDU_APA_RESERVED_8BIT_PORTS)) {
            
            
            
            break;
          }
          header.destinationPort = dstp;
          header.originatorPort = orip;
          break;
        }
        case PDU_IEI_APPLICATION_PORT_ADDRESSING_SCHEME_16BIT: {
          let dstp = (BitBufferHelper.readBits(8) << 8) | BitBufferHelper.readBits(8),
              orip = (BitBufferHelper.readBits(8) << 8) | BitBufferHelper.readBits(8);
          
          
          
          if ((dstp < PDU_APA_VALID_16BIT_PORTS)
              && (orip < PDU_APA_VALID_16BIT_PORTS)) {
            header.destinationPort = dstp;
            header.originatorPort = orip;
          }
          break;
        }
        case PDU_IEI_CONCATENATED_SHORT_MESSAGES_16BIT: {
          let ref = (BitBufferHelper.readBits(8) << 8) | BitBufferHelper.readBits(8),
              max = BitBufferHelper.readBits(8),
              seq = BitBufferHelper.readBits(8);
          if (max && seq && (seq <= max)) {
            header.segmentRef = ref;
            header.segmentMaxSeq = max;
            header.segmentSeq = seq;
          }
          break;
        }
        case PDU_IEI_NATIONAL_LANGUAGE_SINGLE_SHIFT: {
          let langShiftIndex = BitBufferHelper.readBits(8);
          if (langShiftIndex < PDU_NL_SINGLE_SHIFT_TABLES.length) {
            header.langShiftIndex = langShiftIndex;
          }
          break;
        }
        case PDU_IEI_NATIONAL_LANGUAGE_LOCKING_SHIFT: {
          let langIndex = BitBufferHelper.readBits(8);
          if (langIndex < PDU_NL_LOCKING_SHIFT_TABLES.length) {
            header.langIndex = langIndex;
          }
          break;
        }
        case PDU_IEI_SPECIAL_SMS_MESSAGE_INDICATION: {
          let msgInd = BitBufferHelper.readBits(8) & 0xFF,
              msgCount = BitBufferHelper.readBits(8);
          






          let storeType = msgInd & PDU_MWI_STORE_TYPE_BIT;
          header.mwi = {};
          mwi = header.mwi;

          if (storeType == PDU_MWI_STORE_TYPE_STORE) {
            
            
            mwi.discard = false;
          } else if (mwi.discard === undefined) {
            
            
            mwi.discard = true;
          }

          mwi.msgCount = msgCount & 0xFF;
          mwi.active = mwi.msgCount > 0;

          if (DEBUG) {
            this.context.debug("MWI in TP_UDH received: " + JSON.stringify(mwi));
          }
          break;
        }
        default:
          
          for (let i = 0; i < length; i++) {
            BitBufferHelper.readBits(8);
          }
      }
    }

    
    if (headerPaddingBits) {
      BitBufferHelper.readBits(headerPaddingBits);
    }

    return header;
  },

  getCdmaMsgEncoding: function(encoding) {
    
    switch (encoding) {
      case PDU_CDMA_MSG_CODING_7BITS_ASCII:
      case PDU_CDMA_MSG_CODING_IA5:
      case PDU_CDMA_MSG_CODING_7BITS_GSM:
        return PDU_DCS_MSG_CODING_7BITS_ALPHABET;
      case PDU_CDMA_MSG_CODING_OCTET:
      case PDU_CDMA_MSG_CODING_IS_91:
      case PDU_CDMA_MSG_CODING_LATIN_HEBREW:
      case PDU_CDMA_MSG_CODING_LATIN:
        return PDU_DCS_MSG_CODING_8BITS_ALPHABET;
      case PDU_CDMA_MSG_CODING_UNICODE:
      case PDU_CDMA_MSG_CODING_SHIFT_JIS:
      case PDU_CDMA_MSG_CODING_KOREAN:
        return PDU_DCS_MSG_CODING_16BITS_ALPHABET;
    }
    return null;
  },

  decodeCdmaPDUMsg: function(encoding, msgType, msgBodySize) {
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    let BitBufferHelper = this.context.BitBufferHelper;
    let result = "";
    let msgDigit;
    switch (encoding) {
      case PDU_CDMA_MSG_CODING_OCTET:         
        while(msgBodySize > 0) {
          msgDigit = String.fromCharCode(BitBufferHelper.readBits(8));
          result += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_IS_91:         
        
        switch (msgType) {
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_SMS:
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_SMS_FULL:
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_VOICEMAIL_STATUS:
            while(msgBodySize > 0) {
              msgDigit = String.fromCharCode(BitBufferHelper.readBits(6) + 0x20);
              result += msgDigit;
              msgBodySize--;
            }
            break;
          case PDU_CDMA_MSG_CODING_IS_91_TYPE_CLI:
            let addrInfo = {};
            addrInfo.digitMode = PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF;
            addrInfo.numberMode = PDU_CDMA_MSG_ADDR_NUMBER_MODE_ANSI;
            addrInfo.numberType = PDU_CDMA_MSG_ADDR_NUMBER_TYPE_UNKNOWN;
            addrInfo.numberPlan = PDU_CDMA_MSG_ADDR_NUMBER_PLAN_UNKNOWN;
            addrInfo.addrLength = msgBodySize;
            addrInfo.address = [];
            for (let i = 0; i < addrInfo.addrLength; i++) {
              addrInfo.address.push(BitBufferHelper.readBits(4));
            }
            result = this.decodeAddr(addrInfo);
            break;
        }
        
      case PDU_CDMA_MSG_CODING_7BITS_ASCII:
      case PDU_CDMA_MSG_CODING_IA5:           
        while(msgBodySize > 0) {
          msgDigit = BitBufferHelper.readBits(7);
          if (msgDigit >= 32) {
            msgDigit = String.fromCharCode(msgDigit);
          } else {
            if (msgDigit !== PDU_NL_EXTENDED_ESCAPE) {
              msgDigit = langTable[msgDigit];
            } else {
              msgDigit = BitBufferHelper.readBits(7);
              msgBodySize--;
              msgDigit = langShiftTable[msgDigit];
            }
          }
          result += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_UNICODE:
        while(msgBodySize > 0) {
          msgDigit = String.fromCharCode(BitBufferHelper.readBits(16));
          result += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_7BITS_GSM:     
        while(msgBodySize > 0) {
          msgDigit = BitBufferHelper.readBits(7);
          if (msgDigit !== PDU_NL_EXTENDED_ESCAPE) {
            msgDigit = langTable[msgDigit];
          } else {
            msgDigit = BitBufferHelper.readBits(7);
            msgBodySize--;
            msgDigit = langShiftTable[msgDigit];
          }
          result += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_LATIN:         
        
        while(msgBodySize > 0) {
          msgDigit = String.fromCharCode(BitBufferHelper.readBits(8));
          result += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_LATIN_HEBREW:  
        
        while(msgBodySize > 0) {
          msgDigit = BitBufferHelper.readBits(8);
          if (msgDigit === 0xDF) {
            msgDigit = String.fromCharCode(0x2017);
          } else if (msgDigit === 0xFD) {
            msgDigit = String.fromCharCode(0x200E);
          } else if (msgDigit === 0xFE) {
            msgDigit = String.fromCharCode(0x200F);
          } else if (msgDigit >= 0xE0 && msgDigit <= 0xFA) {
            msgDigit = String.fromCharCode(0x4F0 + msgDigit);
          } else {
            msgDigit = String.fromCharCode(msgDigit);
          }
          result += msgDigit;
          msgBodySize--;
        }
        break;
      case PDU_CDMA_MSG_CODING_SHIFT_JIS:
        
        
        let shift_jis_message = [];

        while (msgBodySize > 0) {
          shift_jis_message.push(BitBufferHelper.readBits(8));
          msgBodySize--;
        }

        let decoder = new TextDecoder("shift_jis");
        result = decoder.decode(new Uint8Array(shift_jis_message));
        break;
      case PDU_CDMA_MSG_CODING_KOREAN:
      case PDU_CDMA_MSG_CODING_GSM_DCS:
        
      default:
        break;
    }
    return result;
  },

  




  decodeUserDataMsg: function(hasUserHeader) {
    let BitBufferHelper = this.context.BitBufferHelper;
    let result = {},
        encoding = BitBufferHelper.readBits(5),
        msgType;

    if (encoding === PDU_CDMA_MSG_CODING_IS_91) {
      msgType = BitBufferHelper.readBits(8);
    }
    result.encoding = this.getCdmaMsgEncoding(encoding);

    let msgBodySize = BitBufferHelper.readBits(8);

    
    if (hasUserHeader) {
      result.header = this.decodeUserDataHeader(result.encoding);
      
      msgBodySize -= result.header.length;
    }

    
    if (encoding === PDU_CDMA_MSG_CODING_OCTET && msgBodySize > 0) {
      result.data = new Uint8Array(msgBodySize);
      for (let i = 0; i < msgBodySize; i++) {
        result.data[i] = BitBufferHelper.readBits(8);
      }
      BitBufferHelper.backwardReadPilot(8 * msgBodySize);
    }

    
    result.body = this.decodeCdmaPDUMsg(encoding, msgType, msgBodySize);

    return result;
  },

  decodeBcd: function(value) {
    return ((value >> 4) & 0xF) * 10 + (value & 0x0F);
  },

  




  decodeUserDataTimestamp: function() {
    let BitBufferHelper = this.context.BitBufferHelper;
    let year = this.decodeBcd(BitBufferHelper.readBits(8)),
        month = this.decodeBcd(BitBufferHelper.readBits(8)) - 1,
        date = this.decodeBcd(BitBufferHelper.readBits(8)),
        hour = this.decodeBcd(BitBufferHelper.readBits(8)),
        min = this.decodeBcd(BitBufferHelper.readBits(8)),
        sec = this.decodeBcd(BitBufferHelper.readBits(8));

    if (year >= 96 && year <= 99) {
      year += 1900;
    } else {
      year += 2000;
    }

    let result = (new Date(year, month, date, hour, min, sec, 0)).valueOf();

    return result;
  },

  




  decodeUserDataReplyOption: function() {
    let replyAction = this.context.BitBufferHelper.readBits(4),
        result = { userAck: (replyAction & 0x8) ? true : false,
                   deliverAck: (replyAction & 0x4) ? true : false,
                   readAck: (replyAction & 0x2) ? true : false,
                   report: (replyAction & 0x1) ? true : false
                 };

    return result;
  },

  




  decodeLanguageIndicator: function() {
    let language = this.context.BitBufferHelper.readBits(8);
    let result = CB_CDMA_LANG_GROUP[language];
    return result;
  },

  




  decodeUserDataCallbackNumber: function() {
    let BitBufferHelper = this.context.BitBufferHelper;
    let digitMode = BitBufferHelper.readBits(1);
    if (digitMode) {
      let numberType = BitBufferHelper.readBits(3),
          numberPlan = BitBufferHelper.readBits(4);
    }
    let numberFields = BitBufferHelper.readBits(8),
        result = "";
    for (let i = 0; i < numberFields; i++) {
      if (digitMode === PDU_CDMA_MSG_ADDR_DIGIT_MODE_DTMF) {
        let addrDigit = BitBufferHelper.readBits(4);
        result += this.dtmfChars.charAt(addrDigit);
      } else {
        let addrDigit = BitBufferHelper.readBits(8);
        result += String.fromCharCode(addrDigit);
      }
    }

    return result;
  },

  




  decodeUserDataMsgStatus: function() {
    let BitBufferHelper = this.context.BitBufferHelper;
    let result = {
      errorClass: BitBufferHelper.readBits(2),
      msgStatus: BitBufferHelper.readBits(6)
    };

    return result;
  },

  


  decodeInformationRecord: function() {
    let Buf = this.context.Buf;
    let records = [];
    let numOfRecords = Buf.readInt32();

    let type;
    let record;
    for (let i = 0; i < numOfRecords; i++) {
      record = {};
      type = Buf.readInt32();

      switch (type) {
        


        case PDU_CDMA_INFO_REC_TYPE_DISPLAY:
        case PDU_CDMA_INFO_REC_TYPE_EXTENDED_DISPLAY:
          record.display = Buf.readString();
          break;
        case PDU_CDMA_INFO_REC_TYPE_CALLED_PARTY_NUMBER:
          record.calledNumber = {};
          record.calledNumber.number = Buf.readString();
          record.calledNumber.type = Buf.readInt32();
          record.calledNumber.plan = Buf.readInt32();
          record.calledNumber.pi = Buf.readInt32();
          record.calledNumber.si = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_CALLING_PARTY_NUMBER:
          record.callingNumber = {};
          record.callingNumber.number = Buf.readString();
          record.callingNumber.type = Buf.readInt32();
          record.callingNumber.plan = Buf.readInt32();
          record.callingNumber.pi = Buf.readInt32();
          record.callingNumber.si = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_CONNECTED_NUMBER:
          record.connectedNumber = {};
          record.connectedNumber.number = Buf.readString();
          record.connectedNumber.type = Buf.readInt32();
          record.connectedNumber.plan = Buf.readInt32();
          record.connectedNumber.pi = Buf.readInt32();
          record.connectedNumber.si = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_SIGNAL:
          record.signal = {};
          if (!Buf.readInt32()) { 
            Buf.seekIncoming(3 * Buf.UINT32_SIZE);
            continue;
          }
          record.signal.type = Buf.readInt32();
          record.signal.alertPitch = Buf.readInt32();
          record.signal.signal = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_REDIRECTING_NUMBER:
          record.redirect = {};
          record.redirect.number = Buf.readString();
          record.redirect.type = Buf.readInt32();
          record.redirect.plan = Buf.readInt32();
          record.redirect.pi = Buf.readInt32();
          record.redirect.si = Buf.readInt32();
          record.redirect.reason = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_LINE_CONTROL:
          record.lineControl = {};
          record.lineControl.polarityIncluded = Buf.readInt32();
          record.lineControl.toggle = Buf.readInt32();
          record.lineControl.reverse = Buf.readInt32();
          record.lineControl.powerDenial = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_T53_CLIR:
          record.clirCause = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_T53_AUDIO_CONTROL:
          record.audioControl = {};
          record.audioControl.upLink = Buf.readInt32();
          record.audioControl.downLink = Buf.readInt32();
          break;
        case PDU_CDMA_INFO_REC_TYPE_T53_RELEASE:
          
        default:
          throw new Error("UNSOLICITED_CDMA_INFO_REC(), Unsupported information record type " + type + "\n");
      }

      records.push(record);
    }

    return records;
  }
};




function ICCPDUHelperObject(aContext) {
  this.context = aContext;
}
ICCPDUHelperObject.prototype = {
  context: null,

  






  read8BitUnpackedToString: function(numOctets) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let ret = "";
    let escapeFound = false;
    let i;
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

    for(i = 0; i < numOctets; i++) {
      let octet = GsmPDUHelper.readHexOctet();
      if (octet == 0xff) {
        i++;
        break;
      }

      if (escapeFound) {
        escapeFound = false;
        if (octet == PDU_NL_EXTENDED_ESCAPE) {
          
          
          
          ret += " ";
        } else if (octet == PDU_NL_RESERVED_CONTROL) {
          
          
          
          ret += " ";
        } else {
          ret += langShiftTable[octet];
        }
      } else if (octet == PDU_NL_EXTENDED_ESCAPE) {
        escapeFound = true;
      } else {
        ret += langTable[octet];
      }
    }

    let Buf = this.context.Buf;
    Buf.seekIncoming((numOctets - i) * Buf.PDU_HEX_OCTET_SIZE);
    return ret;
  },

  






  writeStringTo8BitUnpacked: function(numOctets, str) {
    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

    let GsmPDUHelper = this.context.GsmPDUHelper;

    
    
    let i, j;
    let len = str ? str.length : 0;
    for (i = 0, j = 0; i < len && j < numOctets; i++) {
      let c = str.charAt(i);
      let octet = langTable.indexOf(c);

      if (octet == -1) {
        
        if (j + 2 > numOctets) {
          break;
        }

        octet = langShiftTable.indexOf(c);
        if (octet == -1) {
          
          octet = langTable.indexOf(' ');
        } else {
          GsmPDUHelper.writeHexOctet(PDU_NL_EXTENDED_ESCAPE);
          j++;
        }
      }
      GsmPDUHelper.writeHexOctet(octet);
      j++;
    }

    
    while (j++ < numOctets) {
      GsmPDUHelper.writeHexOctet(0xff);
    }
  },

  











  writeICCUCS2String: function(numOctets, str) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let scheme = 0x80;
    let basePointer;

    if (str.length > 2) {
      let min = 0xFFFF;
      let max = 0;
      for (let i = 0; i < str.length; i++) {
        let code = str.charCodeAt(i);
        
        if (code & 0xFF80) {
          if (min > code) {
            min = code;
          }
          if (max < code) {
            max = code;
          }
        }
      }

      
      if ((max - min) >= 0 && (max - min) < 128) {
        
        
        
        
        if (((min & 0x7f80) == (max & 0x7f80)) &&
            ((max & 0x8000) == 0)) {
          scheme = 0x81;
          basePointer = min & 0x7f80;
        } else {
          scheme = 0x82;
          basePointer = min;
        }
      }
    }

    switch (scheme) {
      




      case 0x80: {
        
        GsmPDUHelper.writeHexOctet(0x80);
        numOctets--;
        
        if (str.length * 2 > numOctets) {
          str = str.substring(0, Math.floor(numOctets / 2));
        }
        GsmPDUHelper.writeUCS2String(str);

        
        for (let i = str.length * 2; i < numOctets; i++) {
          GsmPDUHelper.writeHexOctet(0xff);
        }
        return;
      }
      












      case 0x81: {
        GsmPDUHelper.writeHexOctet(0x81);

        if (str.length > (numOctets - 3)) {
          str = str.substring(0, numOctets - 3);
        }

        GsmPDUHelper.writeHexOctet(str.length);
        GsmPDUHelper.writeHexOctet((basePointer >> 7) & 0xff);
        numOctets -= 3;
        break;
      }
      










      case 0x82: {
        GsmPDUHelper.writeHexOctet(0x82);

        if (str.length > (numOctets - 4)) {
          str = str.substring(0, numOctets - 4);
        }

        GsmPDUHelper.writeHexOctet(str.length);
        GsmPDUHelper.writeHexOctet((basePointer >> 8) & 0xff);
        GsmPDUHelper.writeHexOctet(basePointer & 0xff);
        numOctets -= 4;
        break;
      }
    }

    if (scheme == 0x81 || scheme == 0x82) {
      for (let i = 0; i < str.length; i++) {
        let code = str.charCodeAt(i);

        
        
        if (code >> 8 == 0) {
          GsmPDUHelper.writeHexOctet(code & 0x7F);
        } else {
          
          
          GsmPDUHelper.writeHexOctet((code - basePointer) | 0x80);
        }
      }

      
      for (let i = 0; i < numOctets - str.length; i++) {
        GsmPDUHelper.writeHexOctet(0xff);
      }
    }
  },

 








  readICCUCS2String: function(scheme, numOctets) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let str = "";
    switch (scheme) {
      




      case 0x80:
        let isOdd = numOctets % 2;
        let i;
        for (i = 0; i < numOctets - isOdd; i += 2) {
          let code = (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
          if (code == 0xffff) {
            i += 2;
            break;
          }
          str += String.fromCharCode(code);
        }

        
        Buf.seekIncoming((numOctets - i) * Buf.PDU_HEX_OCTET_SIZE);
        break;
      case 0x81: 
      case 0x82:
        






















        let len = GsmPDUHelper.readHexOctet();
        let offset, headerLen;
        if (scheme == 0x81) {
          offset = GsmPDUHelper.readHexOctet() << 7;
          headerLen = 2;
        } else {
          offset = (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
          headerLen = 3;
        }

        for (let i = 0; i < len; i++) {
          let ch = GsmPDUHelper.readHexOctet();
          if (ch & 0x80) {
            
            str += String.fromCharCode((ch & 0x7f) + offset);
          } else {
            
            let count = 0, gotUCS2 = 0;
            while ((i + count + 1 < len)) {
              count++;
              if (GsmPDUHelper.readHexOctet() & 0x80) {
                gotUCS2 = 1;
                break;
              }
            }
            
            
            Buf.seekIncoming(-1 * (count + 1) * Buf.PDU_HEX_OCTET_SIZE);
            str += this.read8BitUnpackedToString(count + 1 - gotUCS2);
            i += count - gotUCS2;
          }
        }

        
        Buf.seekIncoming((numOctets - len - headerLen) * Buf.PDU_HEX_OCTET_SIZE);
        break;
    }
    return str;
  },

  




  readAlphaIdDiallingNumber: function(recordSize) {
    let Buf = this.context.Buf;
    let length = Buf.readInt32();

    let alphaLen = recordSize - ADN_FOOTER_SIZE_BYTES;
    let alphaId = this.readAlphaIdentifier(alphaLen);

    let number = this.readNumberWithLength();

    
    Buf.seekIncoming(2 * Buf.PDU_HEX_OCTET_SIZE);
    Buf.readStringDelimiter(length);

    let contact = null;
    if (alphaId || number) {
      contact = {alphaId: alphaId,
                 number: number};
    }
    return contact;
  },

  






  writeAlphaIdDiallingNumber: function(recordSize, alphaId, number) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    
    let strLen = recordSize * 2;
    Buf.writeInt32(strLen);

    let alphaLen = recordSize - ADN_FOOTER_SIZE_BYTES;
    this.writeAlphaIdentifier(alphaLen, alphaId);
    this.writeNumberWithLength(number);

    
    GsmPDUHelper.writeHexOctet(0xff);
    GsmPDUHelper.writeHexOctet(0xff);
    Buf.writeStringDelimiter(strLen);
  },

  













  readAlphaIdentifier: function(numOctets) {
    if (numOctets === 0) {
      return "";
    }

    let temp;
    
    if ((temp = this.context.GsmPDUHelper.readHexOctet()) == 0x80 ||
         temp == 0x81 ||
         temp == 0x82) {
      numOctets--;
      return this.readICCUCS2String(temp, numOctets);
    } else {
      let Buf = this.context.Buf;
      Buf.seekIncoming(-1 * Buf.PDU_HEX_OCTET_SIZE);
      return this.read8BitUnpackedToString(numOctets);
    }
  },

  










  writeAlphaIdentifier: function(numOctets, alphaId) {
    if (numOctets === 0) {
      return;
    }

    
    if (!alphaId || this.context.ICCUtilsHelper.isGsm8BitAlphabet(alphaId)) {
      this.writeStringTo8BitUnpacked(numOctets, alphaId);
    } else {
      this.writeICCUCS2String(numOctets, alphaId);
    }
  },

  




















  readDiallingNumber: function(len) {
    if (DEBUG) this.context.debug("PDU: Going to read Dialling number: " + len);
    if (len === 0) {
      return "";
    }

    let GsmPDUHelper = this.context.GsmPDUHelper;

    
    let toa = GsmPDUHelper.readHexOctet();

    let number = GsmPDUHelper.readSwappedNibbleExtendedBcdString(len - 1);
    if (number.length <= 0) {
      if (DEBUG) this.context.debug("No number provided");
      return "";
    }
    if ((toa >> 4) == (PDU_TOA_INTERNATIONAL >> 4)) {
      number = '+' + number;
    }
    return number;
  },

  




  writeDiallingNumber: function(number) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let toa = PDU_TOA_ISDN; 
    if (number[0] == '+') {
      toa = PDU_TOA_INTERNATIONAL | PDU_TOA_ISDN; 
      number = number.substring(1);
    }
    GsmPDUHelper.writeHexOctet(toa);
    GsmPDUHelper.writeSwappedNibbleBCD(number);
  },

  readNumberWithLength: function() {
    let Buf = this.context.Buf;
    let number;
    let numLen = this.context.GsmPDUHelper.readHexOctet();
    if (numLen != 0xff) {
      if (numLen > ADN_MAX_BCD_NUMBER_BYTES) {
        if (DEBUG) {
          this.context.debug(
            "Error: invalid length of BCD number/SSC contents - " + numLen);
        }
        Buf.seekIncoming(ADN_MAX_BCD_NUMBER_BYTES * Buf.PDU_HEX_OCTET_SIZE);
        return "";
      }

      number = this.readDiallingNumber(numLen);
      Buf.seekIncoming((ADN_MAX_BCD_NUMBER_BYTES - numLen) * Buf.PDU_HEX_OCTET_SIZE);
    } else {
      Buf.seekIncoming(ADN_MAX_BCD_NUMBER_BYTES * Buf.PDU_HEX_OCTET_SIZE);
    }

    return number;
  },

  writeNumberWithLength: function(number) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    if (number) {
      let numStart = number[0] == "+" ? 1 : 0;
      number = number.substring(0, numStart) +
               number.substring(numStart)
                     .replace(/[^0-9*#,]/g, "")
                     .replace(/\*/g, "a")
                     .replace(/\#/g, "b")
                     .replace(/\,/g, "c");

      let numDigits = number.length - numStart;
      if (numDigits > ADN_MAX_NUMBER_DIGITS) {
        number = number.substring(0, ADN_MAX_NUMBER_DIGITS + numStart);
        numDigits = number.length - numStart;
      }

      
      let numLen = Math.ceil(numDigits / 2) + 1;
      GsmPDUHelper.writeHexOctet(numLen);
      this.writeDiallingNumber(number);
      
      for (let i = 0; i < ADN_MAX_BCD_NUMBER_BYTES - numLen; i++) {
        GsmPDUHelper.writeHexOctet(0xff);
      }
    } else {
      
      for (let i = 0; i < ADN_MAX_BCD_NUMBER_BYTES + 1; i++) {
        GsmPDUHelper.writeHexOctet(0xff);
      }
    }
  }
};

function StkCommandParamsFactoryObject(aContext) {
  this.context = aContext;
}
StkCommandParamsFactoryObject.prototype = {
  context: null,

  createParam: function(cmdDetails, ctlvs, onComplete) {
    let method = this[cmdDetails.typeOfCommand];
    if (typeof method != "function") {
      if (DEBUG) {
        this.context.debug("Unknown proactive command " +
                           cmdDetails.typeOfCommand.toString(16));
      }
      return;
    }
    method.call(this, cmdDetails, ctlvs, onComplete);
  },

  loadIcons: function(iconIdCtlvs, callback) {
    if (!iconIdCtlvs ||
        !this.context.ICCUtilsHelper.isICCServiceAvailable("IMG")) {
      callback(null);
      return;
    }

    let onerror = (function() {
      callback(null);
    }).bind(this);

    let onsuccess = (function(aIcons) {
      callback(aIcons);
    }).bind(this);

    this.context.IconLoader.loadIcons(iconIdCtlvs.map(aCtlv => aCtlv.value.identifier),
                                      onsuccess,
                                      onerror);
  },

  appendIconIfNecessary: function(iconIdCtlvs, result, onComplete) {
    this.loadIcons(iconIdCtlvs, (aIcons) => {
      if (aIcons) {
        result.icons = aIcons[0];
        result.iconSelfExplanatory =
          iconIdCtlvs[0].value.qualifier == 0 ? true : false;
      }

      onComplete(result);
    });
  },

  









  processRefresh: function(cmdDetails, ctlvs, onComplete) {
    let refreshType = cmdDetails.commandQualifier;
    switch (refreshType) {
      case STK_REFRESH_FILE_CHANGE:
      case STK_REFRESH_NAA_INIT_AND_FILE_CHANGE:
        let ctlv = this.context.StkProactiveCmdHelper.searchForTag(
          COMPREHENSIONTLV_TAG_FILE_LIST, ctlvs);
        if (ctlv) {
          let list = ctlv.value.fileList;
          if (DEBUG) {
            this.context.debug("Refresh, list = " + list);
          }
          this.context.ICCRecordHelper.fetchICCRecords();
        }
        break;
    }

    onComplete(null);
  },

  









  processPollInterval: function(cmdDetails, ctlvs, onComplete) {
    
    let ctlv = this.context.StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_DURATION, ctlvs);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Poll Interval: Required value missing : Duration");
    }

    onComplete(ctlv.value);
  },

  









  processPollOff: function(cmdDetails, ctlvs, onComplete) {
    onComplete(null);
  },

  









  processSetUpEventList: function(cmdDetails, ctlvs, onComplete) {
    
    let ctlv = this.context.StkProactiveCmdHelper.searchForTag(
        COMPREHENSIONTLV_TAG_EVENT_LIST, ctlvs);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Event List: Required value missing : Event List");
    }

    onComplete(ctlv.value || { eventList: null });
  },

  









  processSetupMenu: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let menu = {
      
      isHelpAvailable: !!(cmdDetails.commandQualifier & 0x80)
    };

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_ALPHA_ID,
      COMPREHENSIONTLV_TAG_ITEM,
      COMPREHENSIONTLV_TAG_ITEM_ID,
      COMPREHENSIONTLV_TAG_NEXT_ACTION_IND,
      COMPREHENSIONTLV_TAG_ICON_ID,
      COMPREHENSIONTLV_TAG_ICON_ID_LIST
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ALPHA_ID);
    if (ctlv) {
      menu.title = ctlv.value.identifier;
    }

    
    let menuCtlvs = selectedCtlvs[COMPREHENSIONTLV_TAG_ITEM];
    if (!menuCtlvs) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Menu: Required value missing : items");
    }
    menu.items = menuCtlvs.map(aCtlv => aCtlv.value);

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ITEM_ID);
    if (ctlv) {
      menu.defaultItem = ctlv.value.identifier - 1;
    }

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_NEXT_ACTION_IND);
    if (ctlv) {
      menu.nextActionList = ctlv.value;
    }

    
    let iconIdCtlvs = null;
    let menuIconCtlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ICON_ID);
    if (menuIconCtlv) {
      iconIdCtlvs = [menuIconCtlv];
    }

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ICON_ID_LIST);
    if (ctlv) {
      if (!iconIdCtlvs) {
        iconIdCtlvs = [];
      };
      let iconIdList = ctlv.value;
      iconIdCtlvs = iconIdCtlvs.concat(iconIdList.identifiers.map((aId) => {
        return {
          value: { qualifier: iconIdList.qualifier, identifier: aId }
        };
      }));
    }

    this.loadIcons(iconIdCtlvs, (aIcons) => {
      if (aIcons) {
        if (menuIconCtlv) {
          menu.iconSelfExplanatory =
            (iconIdCtlvs.shift().value.qualifier == 0) ? true: false;
          menu.icons = aIcons.shift();
        }

        for (let i = 0; i < aIcons.length; i++) {
          menu.items[i].icons = aIcons[i];
          menu.items[i].iconSelfExplanatory =
            (iconIdCtlvs[i].value.qualifier == 0) ? true: false;
        }
      }

      onComplete(menu);
    });
  },

  









  processSelectItem: function(cmdDetails, ctlvs, onComplete) {
    this.processSetupMenu(cmdDetails, ctlvs, (menu) => {
      
      menu.presentationType = cmdDetails.commandQualifier & 0x03;
      onComplete(menu);
    });
  },

  









  processDisplayText: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let textMsg = {
      isHighPriority: !!(cmdDetails.commandQualifier & 0x01),
      userClear: !!(cmdDetails.commandQualifier & 0x80)
    };

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_TEXT_STRING,
      COMPREHENSIONTLV_TAG_IMMEDIATE_RESPONSE,
      COMPREHENSIONTLV_TAG_DURATION,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_TEXT_STRING);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Display Text: Required value missing : Text String");
    }
    textMsg.text = ctlv.value.textString;

    
    textMsg.responseNeeded =
      !!(selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_IMMEDIATE_RESPONSE));

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_DURATION);
    if (ctlv) {
      textMsg.duration = ctlv.value;
    }

    
    this.appendIconIfNecessary(selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null,
                               textMsg,
                               onComplete);
  },

  









  processSetUpIdleModeText: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let textMsg = {};

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_TEXT_STRING,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_TEXT_STRING);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Set Up Idle Text: Required value missing : Text String");
    }
    textMsg.text = ctlv.value.textString;

    
    this.appendIconIfNecessary(selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null,
                               textMsg,
                               onComplete);
  },

  









  processGetInkey: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let input = {
      minLength: 1,
      maxLength: 1,
      isAlphabet: !!(cmdDetails.commandQualifier & 0x01),
      isUCS2: !!(cmdDetails.commandQualifier & 0x02),
      
      
      isYesNoRequested: !!(cmdDetails.commandQualifier & 0x04),
      
      isHelpAvailable: !!(cmdDetails.commandQualifier & 0x80)
    };

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_TEXT_STRING,
      COMPREHENSIONTLV_TAG_DURATION,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_TEXT_STRING);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Get InKey: Required value missing : Text String");
    }
    input.text = ctlv.value.textString;

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_DURATION);
    if (ctlv) {
      input.duration = ctlv.value;
    }

    
    this.appendIconIfNecessary(selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null,
                               input,
                               onComplete);
  },

  









  processGetInput: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let input = {
      isAlphabet: !!(cmdDetails.commandQualifier & 0x01),
      isUCS2: !!(cmdDetails.commandQualifier & 0x02),
      
      hideInput: !!(cmdDetails.commandQualifier & 0x04),
      
      isPacked: !!(cmdDetails.commandQualifier & 0x08),
      
      isHelpAvailable: !!(cmdDetails.commandQualifier & 0x80)
    };

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_TEXT_STRING,
      COMPREHENSIONTLV_TAG_RESPONSE_LENGTH,
      COMPREHENSIONTLV_TAG_DEFAULT_TEXT,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_TEXT_STRING);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Get Input: Required value missing : Text String");
    }
    input.text = ctlv.value.textString;

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_RESPONSE_LENGTH);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Get Input: Required value missing : Response Length");
    }
    input.minLength = ctlv.value.minLength;
    input.maxLength = ctlv.value.maxLength;

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_DEFAULT_TEXT);
    if (ctlv) {
      input.defaultText = ctlv.value.textString;
    }

    
    this.appendIconIfNecessary(selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null,
                               input,
                               onComplete);
  },

  









  processEventNotify: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let textMsg = {};

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_ALPHA_ID,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ALPHA_ID);
    if (ctlv) {
      textMsg.text = ctlv.value.identifier;
    }

    
    this.appendIconIfNecessary(selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null,
                               textMsg,
                               onComplete);
  },

  









  processSetupCall: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let call = {};
    let confirmMessage = {};
    let callMessage = {};

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_ADDRESS,
      COMPREHENSIONTLV_TAG_ALPHA_ID,
      COMPREHENSIONTLV_TAG_ICON_ID,
      COMPREHENSIONTLV_TAG_DURATION
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ADDRESS);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Set Up Call: Required value missing : Address");
    }
    call.address = ctlv.value.number;

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ALPHA_ID);
    if (ctlv) {
      confirmMessage.text = ctlv.value.identifier;
      call.confirmMessage = confirmMessage;
    }

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ALPHA_ID);
    if (ctlv) {
      callMessage.text = ctlv.value.identifier;
      call.callMessage = callMessage;
    }

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_DURATION);
    if (ctlv) {
      call.duration = ctlv.value;
    }

    
    let iconIdCtlvs = selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null;
    this.loadIcons(iconIdCtlvs, (aIcons) => {
      if (aIcons) {
        confirmMessage.icons = aIcons[0];
        confirmMessage.iconSelfExplanatory =
          (iconIdCtlvs[0].value.qualifier == 0) ? true: false;
        call.confirmMessage = confirmMessage;

        if (aIcons.length > 1) {
          callMessage.icons = aIcons[1];
          callMessage.iconSelfExplanatory =
            (iconIdCtlvs[1].value.qualifier == 0) ? true: false;
          call.callMessage = callMessage;
        }
      }

      onComplete(call);
    });
  },

  









  processLaunchBrowser: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let browser = {
      mode: cmdDetails.commandQualifier & 0x03
    };
    let confirmMessage = {};

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_URL,
      COMPREHENSIONTLV_TAG_ALPHA_ID,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_URL);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Launch Browser: Required value missing : URL");
    }
    browser.url = ctlv.value.url;

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ALPHA_ID);
    if (ctlv) {
      confirmMessage.text = ctlv.value.identifier;
      browser.confirmMessage = confirmMessage;
    }

    
    let iconIdCtlvs = selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null;
    this.loadIcons(iconIdCtlvs, (aIcons) => {
       if (aIcons) {
         confirmMessage.icons = aIcons[0];
         confirmMessage.iconSelfExplanatory =
           (iconIdCtlvs[0].value.qualifier == 0) ? true: false;
         browser.confirmMessage = confirmMessage;
       }

       onComplete(browser);
    });
  },

  









  processPlayTone: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let playTone = {
      
      isVibrate: !!(cmdDetails.commandQualifier & 0x01)
    };

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_ALPHA_ID,
      COMPREHENSIONTLV_TAG_TONE,
      COMPREHENSIONTLV_TAG_DURATION,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ALPHA_ID);
    if (ctlv) {
      playTone.text = ctlv.value.identifier;
    }

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_TONE);
    if (ctlv) {
      playTone.tone = ctlv.value.tone;
    }

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_DURATION);
    if (ctlv) {
      playTone.duration = ctlv.value;
    }

    
    this.appendIconIfNecessary(selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null,
                               playTone,
                               onComplete);
  },

  









  processProvideLocalInfo: function(cmdDetails, ctlvs, onComplete) {
    let provideLocalInfo = {
      localInfoType: cmdDetails.commandQualifier
    };

    onComplete(provideLocalInfo);
  },

  









  processTimerManagement: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let timer = {
      timerAction: cmdDetails.commandQualifier
    };

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER,
      COMPREHENSIONTLV_TAG_TIMER_VALUE
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER);
    if (!ctlv) {
      this.context.RIL.sendStkTerminalResponse({
        command: cmdDetails,
        resultCode: STK_RESULT_REQUIRED_VALUES_MISSING});
      throw new Error("Stk Timer Management: Required value missing : Timer Identifier");
    }
    timer.timerId = ctlv.value.timerId;

    
    ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_TIMER_VALUE);
    if (ctlv) {
      timer.timerValue = ctlv.value.timerValue;
    }

    onComplete(timer);
  },

   









  processBipMessage: function(cmdDetails, ctlvs, onComplete) {
    let StkProactiveCmdHelper = this.context.StkProactiveCmdHelper;
    let bipMsg = {};

    let selectedCtlvs = StkProactiveCmdHelper.searchForSelectedTags(ctlvs, [
      COMPREHENSIONTLV_TAG_ALPHA_ID,
      COMPREHENSIONTLV_TAG_ICON_ID
    ]);

    
    let ctlv = selectedCtlvs.retrieve(COMPREHENSIONTLV_TAG_ALPHA_ID);
    if (ctlv) {
      bipMsg.text = ctlv.value.identifier;
    }

    
    this.appendIconIfNecessary(selectedCtlvs[COMPREHENSIONTLV_TAG_ICON_ID] || null,
                               bipMsg,
                               onComplete);
  }
};
StkCommandParamsFactoryObject.prototype[STK_CMD_REFRESH] = function STK_CMD_REFRESH(cmdDetails, ctlvs, onComplete) {
  return this.processRefresh(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_POLL_INTERVAL] = function STK_CMD_POLL_INTERVAL(cmdDetails, ctlvs, onComplete) {
  return this.processPollInterval(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_POLL_OFF] = function STK_CMD_POLL_OFF(cmdDetails, ctlvs, onComplete) {
  return this.processPollOff(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_PROVIDE_LOCAL_INFO] = function STK_CMD_PROVIDE_LOCAL_INFO(cmdDetails, ctlvs, onComplete) {
  return this.processProvideLocalInfo(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SET_UP_EVENT_LIST] = function STK_CMD_SET_UP_EVENT_LIST(cmdDetails, ctlvs, onComplete) {
  return this.processSetUpEventList(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SET_UP_MENU] = function STK_CMD_SET_UP_MENU(cmdDetails, ctlvs, onComplete) {
  return this.processSetupMenu(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SELECT_ITEM] = function STK_CMD_SELECT_ITEM(cmdDetails, ctlvs, onComplete) {
  return this.processSelectItem(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_DISPLAY_TEXT] = function STK_CMD_DISPLAY_TEXT(cmdDetails, ctlvs, onComplete) {
  return this.processDisplayText(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SET_UP_IDLE_MODE_TEXT] = function STK_CMD_SET_UP_IDLE_MODE_TEXT(cmdDetails, ctlvs, onComplete) {
  return this.processSetUpIdleModeText(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_GET_INKEY] = function STK_CMD_GET_INKEY(cmdDetails, ctlvs, onComplete) {
  return this.processGetInkey(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_GET_INPUT] = function STK_CMD_GET_INPUT(cmdDetails, ctlvs, onComplete) {
  return this.processGetInput(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SEND_SS] = function STK_CMD_SEND_SS(cmdDetails, ctlvs, onComplete) {
  return this.processEventNotify(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SEND_USSD] = function STK_CMD_SEND_USSD(cmdDetails, ctlvs, onComplete) {
  return this.processEventNotify(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SEND_SMS] = function STK_CMD_SEND_SMS(cmdDetails, ctlvs, onComplete) {
  return this.processEventNotify(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SEND_DTMF] = function STK_CMD_SEND_DTMF(cmdDetails, ctlvs, onComplete) {
  return this.processEventNotify(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SET_UP_CALL] = function STK_CMD_SET_UP_CALL(cmdDetails, ctlvs, onComplete) {
  return this.processSetupCall(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_LAUNCH_BROWSER] = function STK_CMD_LAUNCH_BROWSER(cmdDetails, ctlvs, onComplete) {
  return this.processLaunchBrowser(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_PLAY_TONE] = function STK_CMD_PLAY_TONE(cmdDetails, ctlvs, onComplete) {
  return this.processPlayTone(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_TIMER_MANAGEMENT] = function STK_CMD_TIMER_MANAGEMENT(cmdDetails, ctlvs, onComplete) {
  return this.processTimerManagement(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_OPEN_CHANNEL] = function STK_CMD_OPEN_CHANNEL(cmdDetails, ctlvs, onComplete) {
  return this.processBipMessage(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_CLOSE_CHANNEL] = function STK_CMD_CLOSE_CHANNEL(cmdDetails, ctlvs, onComplete) {
  return this.processBipMessage(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_RECEIVE_DATA] = function STK_CMD_RECEIVE_DATA(cmdDetails, ctlvs, onComplete) {
  return this.processBipMessage(cmdDetails, ctlvs, onComplete);
};
StkCommandParamsFactoryObject.prototype[STK_CMD_SEND_DATA] = function STK_CMD_SEND_DATA(cmdDetails, ctlvs, onComplete) {
  return this.processBipMessage(cmdDetails, ctlvs, onComplete);
};

function StkProactiveCmdHelperObject(aContext) {
  this.context = aContext;
}
StkProactiveCmdHelperObject.prototype = {
  context: null,

  retrieve: function(tag, length) {
    let method = this[tag];
    if (typeof method != "function") {
      if (DEBUG) {
        this.context.debug("Unknown comprehension tag " + tag.toString(16));
      }
      let Buf = this.context.Buf;
      Buf.seekIncoming(length * Buf.PDU_HEX_OCTET_SIZE);
      return null;
    }
    return method.call(this, length);
  },

  









  retrieveCommandDetails: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let cmdDetails = {
      commandNumber: GsmPDUHelper.readHexOctet(),
      typeOfCommand: GsmPDUHelper.readHexOctet(),
      commandQualifier: GsmPDUHelper.readHexOctet()
    };
    return cmdDetails;
  },

  








  retrieveDeviceId: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let deviceId = {
      sourceId: GsmPDUHelper.readHexOctet(),
      destinationId: GsmPDUHelper.readHexOctet()
    };
    return deviceId;
  },

  








  retrieveAlphaId: function(length) {
    let alphaId = {
      identifier: this.context.ICCPDUHelper.readAlphaIdentifier(length)
    };
    return alphaId;
  },

  








  retrieveDuration: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let duration = {
      timeUnit: GsmPDUHelper.readHexOctet(),
      timeInterval: GsmPDUHelper.readHexOctet(),
    };
    return duration;
  },

  









  retrieveAddress: function(length) {
    let address = {
      number : this.context.ICCPDUHelper.readDiallingNumber(length)
    };
    return address;
  },

  









  retrieveTextString: function(length) {
    if (!length) {
      
      return {textString: null};
    }

    let GsmPDUHelper = this.context.GsmPDUHelper;
    let text = {
      codingScheme: GsmPDUHelper.readHexOctet()
    };

    length--; 
    switch (text.codingScheme & 0x0c) {
      case STK_TEXT_CODING_GSM_7BIT_PACKED:
        text.textString =
          GsmPDUHelper.readSeptetsToString(Math.floor(length * 8 / 7), 0, 0, 0);
        break;
      case STK_TEXT_CODING_GSM_8BIT:
        text.textString =
          this.context.ICCPDUHelper.read8BitUnpackedToString(length);
        break;
      case STK_TEXT_CODING_UCS2:
        text.textString = GsmPDUHelper.readUCS2String(length);
        break;
    }
    return text;
  },

  







  retrieveTone: function(length) {
    let tone = {
      tone: this.context.GsmPDUHelper.readHexOctet(),
    };
    return tone;
  },

  









  retrieveItem: function(length) {
    
    
    
    
    if (!length) {
      return null;
    }
    let item = {
      identifier: this.context.GsmPDUHelper.readHexOctet(),
      text: this.context.ICCPDUHelper.readAlphaIdentifier(length - 1)
    };
    return item;
  },

  







  retrieveItemId: function(length) {
    let itemId = {
      identifier: this.context.GsmPDUHelper.readHexOctet()
    };
    return itemId;
  },

  








  retrieveResponseLength: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let rspLength = {
      minLength : GsmPDUHelper.readHexOctet(),
      maxLength : GsmPDUHelper.readHexOctet()
    };
    return rspLength;
  },

  









  retrieveFileList: function(length) {
    let num = this.context.GsmPDUHelper.readHexOctet();
    let fileList = "";
    length--; 
    for (let i = 0; i < 2 * length; i++) {
      
      
      fileList += String.fromCharCode(this.context.Buf.readUint16());
    }
    return {
      fileList: fileList
    };
  },

  




  retrieveDefaultText: function(length) {
    return this.retrieveTextString(length);
  },

  


  retrieveEventList: function(length) {
    if (!length) {
      
      
      return null;
    }

    let GsmPDUHelper = this.context.GsmPDUHelper;
    let eventList = [];
    for (let i = 0; i < length; i++) {
      eventList.push(GsmPDUHelper.readHexOctet());
    }
    return {
      eventList: eventList
    };
  },

  








  retrieveIconId: function(length) {
    if (!length) {
      return null;
    }

    let iconId = {
      qualifier: this.context.GsmPDUHelper.readHexOctet(),
      identifier: this.context.GsmPDUHelper.readHexOctet()
    };
    return iconId;
  },

  









  retrieveIconIdList: function(length) {
    if (!length) {
      return null;
    }

    let iconIdList = {
      qualifier: this.context.GsmPDUHelper.readHexOctet(),
      identifiers: []
    };
    for (let i = 0; i < length - 1; i++) {
      iconIdList.identifiers.push(this.context.GsmPDUHelper.readHexOctet());
    }
    return iconIdList;
  },

  







  retrieveTimerId: function(length) {
    let id = {
      timerId: this.context.GsmPDUHelper.readHexOctet()
    };
    return id;
  },

  









  retrieveTimerValue: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let value = {
      timerValue: (GsmPDUHelper.readSwappedNibbleBcdNum(1) * 60 * 60) +
                  (GsmPDUHelper.readSwappedNibbleBcdNum(1) * 60) +
                  (GsmPDUHelper.readSwappedNibbleBcdNum(1))
    };
    return value;
  },

  






  retrieveImmediaResponse: function(length) {
    return {};
  },

  








  retrieveUrl: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let s = "";
    for (let i = 0; i < length; i++) {
      s += String.fromCharCode(GsmPDUHelper.readHexOctet());
    }
    return {url: s};
  },

  








  retrieveNextActionList: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let nextActionList = [];
    for (let i = 0; i < length; i++) {
      nextActionList.push(GsmPDUHelper.readHexOctet());
    }
    return nextActionList;
  },

  searchForTag: function(tag, ctlvs) {
    let iter = Iterator(ctlvs);
    for (let [index, ctlv] in iter) {
      if ((ctlv.tag & ~COMPREHENSIONTLV_FLAG_CR) == tag) {
        return ctlv;
      }
    }
    return null;
  },

  searchForSelectedTags: function(ctlvs, tags) {
    let ret = {
      
      retrieve: function(aTag) {
        return (this[aTag]) ? this[aTag].shift() : null;
      }
    };

    ctlvs.forEach((aCtlv) => {
      tags.forEach((aTag) => {
        if ((aCtlv.tag & ~COMPREHENSIONTLV_FLAG_CR) == aTag) {
          if (!ret[aTag]) {
            ret[aTag] = [];
          }
          ret[aTag].push(aCtlv);
        }
      });
    });

    return ret;
  },
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_COMMAND_DETAILS] = function COMPREHENSIONTLV_TAG_COMMAND_DETAILS(length) {
  return this.retrieveCommandDetails(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_DEVICE_ID] = function COMPREHENSIONTLV_TAG_DEVICE_ID(length) {
  return this.retrieveDeviceId(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_ALPHA_ID] = function COMPREHENSIONTLV_TAG_ALPHA_ID(length) {
  return this.retrieveAlphaId(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_DURATION] = function COMPREHENSIONTLV_TAG_DURATION(length) {
  return this.retrieveDuration(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_ADDRESS] = function COMPREHENSIONTLV_TAG_ADDRESS(length) {
  return this.retrieveAddress(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_TEXT_STRING] = function COMPREHENSIONTLV_TAG_TEXT_STRING(length) {
  return this.retrieveTextString(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_TONE] = function COMPREHENSIONTLV_TAG_TONE(length) {
  return this.retrieveTone(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_ITEM] = function COMPREHENSIONTLV_TAG_ITEM(length) {
  return this.retrieveItem(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_ITEM_ID] = function COMPREHENSIONTLV_TAG_ITEM_ID(length) {
  return this.retrieveItemId(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_RESPONSE_LENGTH] = function COMPREHENSIONTLV_TAG_RESPONSE_LENGTH(length) {
  return this.retrieveResponseLength(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_FILE_LIST] = function COMPREHENSIONTLV_TAG_FILE_LIST(length) {
  return this.retrieveFileList(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_DEFAULT_TEXT] = function COMPREHENSIONTLV_TAG_DEFAULT_TEXT(length) {
  return this.retrieveDefaultText(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_EVENT_LIST] = function COMPREHENSIONTLV_TAG_EVENT_LIST(length) {
  return this.retrieveEventList(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_ICON_ID] = function COMPREHENSIONTLV_TAG_ICON_ID(length) {
  return this.retrieveIconId(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_ICON_ID_LIST] = function COMPREHENSIONTLV_TAG_ICON_ID_LIST(length) {
  return this.retrieveIconIdList(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER] = function COMPREHENSIONTLV_TAG_TIMER_IDENTIFIER(length) {
  return this.retrieveTimerId(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_TIMER_VALUE] = function COMPREHENSIONTLV_TAG_TIMER_VALUE(length) {
  return this.retrieveTimerValue(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_IMMEDIATE_RESPONSE] = function COMPREHENSIONTLV_TAG_IMMEDIATE_RESPONSE(length) {
  return this.retrieveImmediaResponse(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_URL] = function COMPREHENSIONTLV_TAG_URL(length) {
  return this.retrieveUrl(length);
};
StkProactiveCmdHelperObject.prototype[COMPREHENSIONTLV_TAG_NEXT_ACTION_IND] = function COMPREHENSIONTLV_TAG_NEXT_ACTION_IND(length) {
  return this.retrieveNextActionList(length);
};

function ComprehensionTlvHelperObject(aContext) {
  this.context = aContext;
}
ComprehensionTlvHelperObject.prototype = {
  context: null,

  


  decode: function() {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let hlen = 0; 
    let temp = GsmPDUHelper.readHexOctet();
    hlen++;

    
    let tag, cr;
    switch (temp) {
      
      case 0x0: 
      case 0xff: 
      case 0x80: 
        throw new Error("Invalid octet when parsing Comprehension TLV :" + temp);
      case 0x7f: 
        
        
        
        
        tag = (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
        hlen += 2;
        cr = (tag & 0x8000) !== 0;
        tag &= ~0x8000;
        break;
      default: 
        tag = temp;
        
        
        
        cr = (tag & 0x80) !== 0;
        tag &= ~0x80;
    }

    
    
    
    
    
    
    
    
    

    let length; 
    temp = GsmPDUHelper.readHexOctet();
    hlen++;
    if (temp < 0x80) {
      length = temp;
    } else if (temp == 0x81) {
      length = GsmPDUHelper.readHexOctet();
      hlen++;
      if (length < 0x80) {
        throw new Error("Invalid length in Comprehension TLV :" + length);
      }
    } else if (temp == 0x82) {
      length = (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
      hlen += 2;
      if (lenth < 0x0100) {
        throw new Error("Invalid length in 3-byte Comprehension TLV :" + length);
      }
    } else if (temp == 0x83) {
      length = (GsmPDUHelper.readHexOctet() << 16) |
               (GsmPDUHelper.readHexOctet() << 8)  |
                GsmPDUHelper.readHexOctet();
      hlen += 3;
      if (length < 0x010000) {
        throw new Error("Invalid length in 4-byte Comprehension TLV :" + length);
      }
    } else {
      throw new Error("Invalid octet in Comprehension TLV :" + temp);
    }

    let ctlv = {
      tag: tag,
      length: length,
      value: this.context.StkProactiveCmdHelper.retrieve(tag, length),
      cr: cr,
      hlen: hlen
    };
    return ctlv;
  },

  decodeChunks: function(length) {
    let chunks = [];
    let index = 0;
    while (index < length) {
      let tlv = this.decode();
      chunks.push(tlv);
      index += tlv.length;
      index += tlv.hlen;
    }
    return chunks;
  },

  




  writeLocationInfoTlv: function(loc) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_LOCATION_INFO |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(loc.gsmCellId > 0xffff ? 9 : 7);
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    let mcc = loc.mcc, mnc;
    if (loc.mnc.length == 2) {
      mnc = "F" + loc.mnc;
    } else {
      mnc = loc.mnc[2] + loc.mnc[0] + loc.mnc[1];
    }
    GsmPDUHelper.writeSwappedNibbleBCD(mcc + mnc);

    
    GsmPDUHelper.writeHexOctet((loc.gsmLocationAreaCode >> 8) & 0xff);
    GsmPDUHelper.writeHexOctet(loc.gsmLocationAreaCode & 0xff);

    
    if (loc.gsmCellId > 0xffff) {
      
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 24) & 0xff);
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 16) & 0xff);
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(loc.gsmCellId & 0xff);
    } else {
      
      GsmPDUHelper.writeHexOctet((loc.gsmCellId >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(loc.gsmCellId & 0xff);
    }
  },

  





  writeCauseTlv: function(geckoError) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let cause = -1;
    for (let errorNo in RIL_ERROR_TO_GECKO_ERROR) {
      if (geckoError == RIL_ERROR_TO_GECKO_ERROR[errorNo]) {
        cause = errorNo;
        break;
      }
    }
    cause = (cause == -1) ? ERROR_SUCCESS : cause;

    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_CAUSE |
                               COMPREHENSIONTLV_FLAG_CR);
    GsmPDUHelper.writeHexOctet(2);  

    
    GsmPDUHelper.writeHexOctet(0x60);

    
    
    
    
    GsmPDUHelper.writeHexOctet(0x80 | cause);
  },

  writeDateTimeZoneTlv: function(date) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_DATE_TIME_ZONE);
    GsmPDUHelper.writeHexOctet(7);
    GsmPDUHelper.writeTimestamp(date);
  },

  writeLanguageTlv: function(language) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_LANGUAGE);
    GsmPDUHelper.writeHexOctet(2);

    
    
    GsmPDUHelper.writeHexOctet(
      PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT].indexOf(language[0]));
    GsmPDUHelper.writeHexOctet(
      PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT].indexOf(language[1]));
  },

  





  writeTimerValueTlv: function(seconds, cr) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TIMER_VALUE |
                               (cr ? COMPREHENSIONTLV_FLAG_CR : 0));
    GsmPDUHelper.writeHexOctet(3);

    
    
    
    
    GsmPDUHelper.writeSwappedNibbleBCDNum(Math.floor(seconds / 60 / 60));
    GsmPDUHelper.writeSwappedNibbleBCDNum(Math.floor(seconds / 60) % 60);
    GsmPDUHelper.writeSwappedNibbleBCDNum(seconds % 60);
  },

  writeTextStringTlv: function(text, coding) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let buf = GsmPDUHelper.writeWithBuffer(() => {
      
      GsmPDUHelper.writeHexOctet(coding);

      
      switch (coding) {
        case STK_TEXT_CODING_UCS2:
          GsmPDUHelper.writeUCS2String(text);
          break;
        case STK_TEXT_CODING_GSM_7BIT_PACKED:
          GsmPDUHelper.writeStringAsSeptets(text, 0, 0, 0);
          break;
        case STK_TEXT_CODING_GSM_8BIT:
          GsmPDUHelper.writeStringAs8BitUnpacked(text);
          break;
      }
    });

    let length = buf.length;
    if (length) {
      
      GsmPDUHelper.writeHexOctet(COMPREHENSIONTLV_TAG_TEXT_STRING |
                                 COMPREHENSIONTLV_FLAG_CR);
      
      this.writeLength(length);
      
      for (let i = 0; i < length; i++) {
        GsmPDUHelper.writeHexOctet(buf[i]);
      }
    }
  },

  getSizeOfLengthOctets: function(length) {
    if (length >= 0x10000) {
      return 4; 
    } else if (length >= 0x100) {
      return 3; 
    } else if (length >= 0x80) {
      return 2; 
    } else {
      return 1; 
    }
  },

  writeLength: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    
    
    
    
    
    
    
    if (length < 0x80) {
      GsmPDUHelper.writeHexOctet(length);
    } else if (0x80 <= length && length < 0x100) {
      GsmPDUHelper.writeHexOctet(0x81);
      GsmPDUHelper.writeHexOctet(length);
    } else if (0x100 <= length && length < 0x10000) {
      GsmPDUHelper.writeHexOctet(0x82);
      GsmPDUHelper.writeHexOctet((length >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(length & 0xff);
    } else if (0x10000 <= length && length < 0x1000000) {
      GsmPDUHelper.writeHexOctet(0x83);
      GsmPDUHelper.writeHexOctet((length >> 16) & 0xff);
      GsmPDUHelper.writeHexOctet((length >> 8) & 0xff);
      GsmPDUHelper.writeHexOctet(length & 0xff);
    } else {
      throw new Error("Invalid length value :" + length);
    }
  },
};

function BerTlvHelperObject(aContext) {
  this.context = aContext;
}
BerTlvHelperObject.prototype = {
  context: null,

  





  decode: function(dataLen) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let hlen = 0;
    let tag = GsmPDUHelper.readHexOctet();
    hlen++;

    
    
    
    
    let length;
    let temp = GsmPDUHelper.readHexOctet();
    hlen++;
    if (temp < 0x80) {
      length = temp;
    } else if (temp === 0x81) {
      length = GsmPDUHelper.readHexOctet();
      hlen++;
      if (length < 0x80) {
        throw new Error("Invalid length " + length);
      }
    } else {
      throw new Error("Invalid length octet " + temp);
    }

    
    if (dataLen - hlen !== length) {
      throw new Error("Unexpected BerTlvHelper value length!!");
    }

    let method = this[tag];
    if (typeof method != "function") {
      throw new Error("Unknown Ber tag 0x" + tag.toString(16));
    }

    let value = method.call(this, length);

    return {
      tag: tag,
      length: length,
      value: value
    };
  },

  





  processFcpTemplate: function(length) {
    let tlvs = this.decodeChunks(length);
    return tlvs;
  },

  





  processProactiveCommand: function(length) {
    let ctlvs = this.context.ComprehensionTlvHelper.decodeChunks(length);
    return ctlvs;
  },

  


  decodeInnerTlv: function() {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let tag = GsmPDUHelper.readHexOctet();
    let length = GsmPDUHelper.readHexOctet();
    return {
      tag: tag,
      length: length,
      value: this.retrieve(tag, length)
    };
  },

  decodeChunks: function(length) {
    let chunks = [];
    let index = 0;
    while (index < length) {
      let tlv = this.decodeInnerTlv();
      if (tlv.value) {
        chunks.push(tlv);
      }
      index += tlv.length;
      
      index += 2;
    }
    return chunks;
  },

  retrieve: function(tag, length) {
    let method = this[tag];
    if (typeof method != "function") {
      if (DEBUG) {
        this.context.debug("Unknown Ber tag : 0x" + tag.toString(16));
      }
      let Buf = this.context.Buf;
      Buf.seekIncoming(length * Buf.PDU_HEX_OCTET_SIZE);
      return null;
    }
    return method.call(this, length);
  },

  








  retrieveFileSizeData: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let fileSizeData = 0;
    for (let i = 0; i < length; i++) {
      fileSizeData = fileSizeData << 8;
      fileSizeData += GsmPDUHelper.readHexOctet();
    }

    return {fileSizeData: fileSizeData};
  },

  










  retrieveFileDescriptor: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let fileDescriptorByte = GsmPDUHelper.readHexOctet();
    let dataCodingByte = GsmPDUHelper.readHexOctet();
    
    
    let fileStructure = fileDescriptorByte & 0x07;

    let fileDescriptor = {
      fileStructure: fileStructure
    };
    
    
    if (fileStructure === UICC_EF_STRUCTURE[EF_STRUCTURE_LINEAR_FIXED] ||
        fileStructure === UICC_EF_STRUCTURE[EF_STRUCTURE_CYCLIC]) {
      fileDescriptor.recordLength = (GsmPDUHelper.readHexOctet() << 8) +
                                     GsmPDUHelper.readHexOctet();
      fileDescriptor.numOfRecords = GsmPDUHelper.readHexOctet();
    }

    return fileDescriptor;
  },

  







  retrieveFileIdentifier: function(length) {
    let GsmPDUHelper = this.context.GsmPDUHelper;
    return {fileId : (GsmPDUHelper.readHexOctet() << 8) +
                      GsmPDUHelper.readHexOctet()};
  },

  searchForNextTag: function(tag, iter) {
    for (let [index, tlv] in iter) {
      if (tlv.tag === tag) {
        return tlv;
      }
    }
    return null;
  }
};
BerTlvHelperObject.prototype[BER_FCP_TEMPLATE_TAG] = function BER_FCP_TEMPLATE_TAG(length) {
  return this.processFcpTemplate(length);
};
BerTlvHelperObject.prototype[BER_PROACTIVE_COMMAND_TAG] = function BER_PROACTIVE_COMMAND_TAG(length) {
  return this.processProactiveCommand(length);
};
BerTlvHelperObject.prototype[BER_FCP_FILE_SIZE_DATA_TAG] = function BER_FCP_FILE_SIZE_DATA_TAG(length) {
  return this.retrieveFileSizeData(length);
};
BerTlvHelperObject.prototype[BER_FCP_FILE_DESCRIPTOR_TAG] = function BER_FCP_FILE_DESCRIPTOR_TAG(length) {
  return this.retrieveFileDescriptor(length);
};
BerTlvHelperObject.prototype[BER_FCP_FILE_IDENTIFIER_TAG] = function BER_FCP_FILE_IDENTIFIER_TAG(length) {
  return this.retrieveFileIdentifier(length);
};




function ICCFileHelperObject(aContext) {
  this.context = aContext;
}
ICCFileHelperObject.prototype = {
  context: null,

  



  getCommonEFPath: function(fileId) {
    switch (fileId) {
      case ICC_EF_ICCID:
        return EF_PATH_MF_SIM;
      case ICC_EF_ADN:
      case ICC_EF_SDN: 
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM;
      case ICC_EF_PBR:
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK;
      case ICC_EF_IMG:
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_GRAPHICS;
    }
    return null;
  },

  


  getSimEFPath: function(fileId) {
    switch (fileId) {
      case ICC_EF_FDN:
      case ICC_EF_MSISDN:
      case ICC_EF_SMS:
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM;
      case ICC_EF_AD:
      case ICC_EF_MBDN:
      case ICC_EF_MWIS:
      case ICC_EF_PLMNsel:
      case ICC_EF_SPN:
      case ICC_EF_SPDI:
      case ICC_EF_SST:
      case ICC_EF_PHASE:
      case ICC_EF_CBMI:
      case ICC_EF_CBMID:
      case ICC_EF_CBMIR:
      case ICC_EF_OPL:
      case ICC_EF_PNN:
      case ICC_EF_GID1:
      case ICC_EF_CPHS_INFO:
      case ICC_EF_CPHS_MBN:
        return EF_PATH_MF_SIM + EF_PATH_DF_GSM;
      default:
        return null;
    }
  },

  


  getUSimEFPath: function(fileId) {
    switch (fileId) {
      case ICC_EF_AD:
      case ICC_EF_FDN:
      case ICC_EF_MBDN:
      case ICC_EF_MWIS:
      case ICC_EF_UST:
      case ICC_EF_MSISDN:
      case ICC_EF_SPN:
      case ICC_EF_SPDI:
      case ICC_EF_CBMI:
      case ICC_EF_CBMID:
      case ICC_EF_CBMIR:
      case ICC_EF_OPL:
      case ICC_EF_PNN:
      case ICC_EF_SMS:
      case ICC_EF_GID1:
      
      
      
      
      case ICC_EF_CPHS_INFO:
      case ICC_EF_CPHS_MBN:
        return EF_PATH_MF_SIM + EF_PATH_ADF_USIM;
      default:
        
        
        
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM + EF_PATH_DF_PHONEBOOK;
    }
  },

  


  getRuimEFPath: function(fileId) {
    switch(fileId) {
      case ICC_EF_CSIM_IMSI_M:
      case ICC_EF_CSIM_CDMAHOME:
      case ICC_EF_CSIM_CST:
      case ICC_EF_CSIM_SPN:
        return EF_PATH_MF_SIM + EF_PATH_DF_CDMA;
      case ICC_EF_FDN:
        return EF_PATH_MF_SIM + EF_PATH_DF_TELECOM;
      default:
        return null;
    }
  },

  







  getEFPath: function(fileId) {
    let path = this.getCommonEFPath(fileId);
    if (path) {
      return path;
    }

    switch (this.context.RIL.appType) {
      case CARD_APPTYPE_SIM:
        return this.getSimEFPath(fileId);
      case CARD_APPTYPE_USIM:
        return this.getUSimEFPath(fileId);
      case CARD_APPTYPE_RUIM:
        return this.getRuimEFPath(fileId);
      default:
        return null;
    }
  }
};




function ICCIOHelperObject(aContext) {
  this.context = aContext;
}
ICCIOHelperObject.prototype = {
  context: null,

  













  loadLinearFixedEF: function(options) {
    let cb;
    let readRecord = (function(options) {
      options.command = ICC_COMMAND_READ_RECORD;
      options.p1 = options.recordNumber || 1; 
      options.p2 = READ_RECORD_ABSOLUTE_MODE;
      options.p3 = options.recordSize;
      options.callback = cb || options.callback;
      this.context.RIL.iccIO(options);
    }).bind(this);

    options.structure = EF_STRUCTURE_LINEAR_FIXED;
    options.pathId = this.context.ICCFileHelper.getEFPath(options.fileId);
    if (options.recordSize) {
      readRecord(options);
      return;
    }

    cb = options.callback;
    options.callback = readRecord;
    this.getResponse(options);
  },

  


  loadNextRecord: function(options) {
    options.p1++;
    this.context.RIL.iccIO(options);
  },

  















  updateLinearFixedEF: function(options) {
    if (!options.fileId || !options.recordNumber) {
      throw new Error("Unexpected fileId " + options.fileId +
                      " or recordNumber " + options.recordNumber);
    }

    options.structure = EF_STRUCTURE_LINEAR_FIXED;
    options.pathId = this.context.ICCFileHelper.getEFPath(options.fileId);
    let cb = options.callback;
    options.callback = function callback(options) {
      options.callback = cb;
      options.command = ICC_COMMAND_UPDATE_RECORD;
      options.p1 = options.recordNumber;
      options.p2 = READ_RECORD_ABSOLUTE_MODE;
      options.p3 = options.recordSize;
      this.context.RIL.iccIO(options);
    }.bind(this);
    this.getResponse(options);
  },

  









  loadTransparentEF: function(options) {
    options.structure = EF_STRUCTURE_TRANSPARENT;
    let cb = options.callback;
    options.callback = function callback(options) {
      options.callback = cb;
      options.command = ICC_COMMAND_READ_BINARY;
      options.p2 = 0x00;
      options.p3 = options.fileSize;
      this.context.RIL.iccIO(options);
    }.bind(this);
    this.getResponse(options);
  },

  





  getResponse: function(options) {
    options.command = ICC_COMMAND_GET_RESPONSE;
    options.pathId = options.pathId ||
                     this.context.ICCFileHelper.getEFPath(options.fileId);
    if (!options.pathId) {
      throw new Error("Unknown pathId for " + options.fileId.toString(16));
    }
    options.p1 = 0; 
    switch (this.context.RIL.appType) {
      case CARD_APPTYPE_USIM:
        options.p2 = GET_RESPONSE_FCP_TEMPLATE;
        options.p3 = 0x00;
        break;
      
      case CARD_APPTYPE_RUIM:
      case CARD_APPTYPE_CSIM:
      case CARD_APPTYPE_ISIM:
      
      case CARD_APPTYPE_SIM:
      default:
        options.p2 = 0x00;
        options.p3 = GET_RESPONSE_EF_SIZE_BYTES;
        break;
    }
    this.context.RIL.iccIO(options);
  },

  


  processICCIO: function(options) {
    let func = this[options.command];
    func.call(this, options);
  },

  


  processICCIOGetResponse: function(options) {
    let Buf = this.context.Buf;
    let strLen = Buf.readInt32();

    let peek = this.context.GsmPDUHelper.readHexOctet();
    Buf.seekIncoming(-1 * Buf.PDU_HEX_OCTET_SIZE);
    if (peek === BER_FCP_TEMPLATE_TAG) {
      this.processUSimGetResponse(options, strLen / 2);
    } else {
      this.processSimGetResponse(options);
    }
    Buf.readStringDelimiter(strLen);

    if (options.callback) {
      options.callback(options);
    }
  },

  


  processUSimGetResponse: function(options, octetLen) {
    let BerTlvHelper = this.context.BerTlvHelper;

    let berTlv = BerTlvHelper.decode(octetLen);
    
    let iter = Iterator(berTlv.value);
    let tlv = BerTlvHelper.searchForNextTag(BER_FCP_FILE_DESCRIPTOR_TAG,
                                            iter);
    if (!tlv ||
        (tlv.value.fileStructure !== UICC_EF_STRUCTURE[options.structure])) {
      throw new Error("Expected EF structure " +
                      UICC_EF_STRUCTURE[options.structure] +
                      " but read " + tlv.value.fileStructure);
    }

    if (tlv.value.fileStructure === UICC_EF_STRUCTURE[EF_STRUCTURE_LINEAR_FIXED] ||
        tlv.value.fileStructure === UICC_EF_STRUCTURE[EF_STRUCTURE_CYCLIC]) {
      options.recordSize = tlv.value.recordLength;
      options.totalRecords = tlv.value.numOfRecords;
    }

    tlv = BerTlvHelper.searchForNextTag(BER_FCP_FILE_IDENTIFIER_TAG, iter);
    if (!tlv || (tlv.value.fileId !== options.fileId)) {
      throw new Error("Expected file ID " + options.fileId.toString(16) +
                      " but read " + fileId.toString(16));
    }

    tlv = BerTlvHelper.searchForNextTag(BER_FCP_FILE_SIZE_DATA_TAG, iter);
    if (!tlv) {
      throw new Error("Unexpected file size data");
    }
    options.fileSize = tlv.value.fileSizeData;
  },

  


  processSimGetResponse: function(options) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;

    

    
    Buf.seekIncoming(2 * Buf.PDU_HEX_OCTET_SIZE);

    
    options.fileSize = (GsmPDUHelper.readHexOctet() << 8) |
                        GsmPDUHelper.readHexOctet();

    
    let fileId = (GsmPDUHelper.readHexOctet() << 8) |
                  GsmPDUHelper.readHexOctet();
    if (fileId != options.fileId) {
      throw new Error("Expected file ID " + options.fileId.toString(16) +
                      " but read " + fileId.toString(16));
    }

    
    let fileType = GsmPDUHelper.readHexOctet();
    if (fileType != TYPE_EF) {
      throw new Error("Unexpected file type " + fileType);
    }

    
    
    
    
    Buf.seekIncoming(((RESPONSE_DATA_STRUCTURE - RESPONSE_DATA_FILE_TYPE - 1) *
        Buf.PDU_HEX_OCTET_SIZE));

    
    let efStructure = GsmPDUHelper.readHexOctet();
    if (efStructure != options.structure) {
      throw new Error("Expected EF structure " + options.structure +
                      " but read " + efStructure);
    }

    
    
    if (efStructure == EF_STRUCTURE_LINEAR_FIXED ||
        efStructure == EF_STRUCTURE_CYCLIC) {
      options.recordSize = GsmPDUHelper.readHexOctet();
      options.totalRecords = options.fileSize / options.recordSize;
    } else {
      Buf.seekIncoming(1 * Buf.PDU_HEX_OCTET_SIZE);
    }
  },

  


  processICCIOReadRecord: function(options) {
    if (options.callback) {
      options.callback(options);
    }
  },

  


  processICCIOReadBinary: function(options) {
    if (options.callback) {
      options.callback(options);
    }
  },

  


  processICCIOUpdateRecord: function(options) {
    if (options.callback) {
      options.callback(options);
    }
  },
};
ICCIOHelperObject.prototype[ICC_COMMAND_SEEK] = null;
ICCIOHelperObject.prototype[ICC_COMMAND_READ_BINARY] = function ICC_COMMAND_READ_BINARY(options) {
  this.processICCIOReadBinary(options);
};
ICCIOHelperObject.prototype[ICC_COMMAND_READ_RECORD] = function ICC_COMMAND_READ_RECORD(options) {
  this.processICCIOReadRecord(options);
};
ICCIOHelperObject.prototype[ICC_COMMAND_GET_RESPONSE] = function ICC_COMMAND_GET_RESPONSE(options) {
  this.processICCIOGetResponse(options);
};
ICCIOHelperObject.prototype[ICC_COMMAND_UPDATE_BINARY] = null;
ICCIOHelperObject.prototype[ICC_COMMAND_UPDATE_RECORD] = function ICC_COMMAND_UPDATE_RECORD(options) {
  this.processICCIOUpdateRecord(options);
};




function ICCRecordHelperObject(aContext) {
  this.context = aContext;
  
  this._freeRecordIds = {};
}
ICCRecordHelperObject.prototype = {
  context: null,

  


  fetchICCRecords: function() {
    switch (this.context.RIL.appType) {
      case CARD_APPTYPE_SIM:
      case CARD_APPTYPE_USIM:
        this.context.SimRecordHelper.fetchSimRecords();
        break;
      case CARD_APPTYPE_RUIM:
        this.context.RuimRecordHelper.fetchRuimRecords();
        break;
    }
  },

  


  readICCID: function() {
    function callback() {
      let Buf = this.context.Buf;
      let RIL = this.context.RIL;
      let GsmPDUHelper = this.context.GsmPDUHelper;

      let strLen = Buf.readInt32();
      let octetLen = strLen / 2;
      RIL.iccInfo.iccid =
        GsmPDUHelper.readSwappedNibbleBcdString(octetLen, true);
      
      let unReadBuffer = this.context.Buf.getReadAvailable() -
                         this.context.Buf.PDU_HEX_OCTET_SIZE;
      if (unReadBuffer > 0) {
        this.context.Buf.seekIncoming(unReadBuffer);
      }
      Buf.readStringDelimiter(strLen);

      if (DEBUG) this.context.debug("ICCID: " + RIL.iccInfo.iccid);
      if (RIL.iccInfo.iccid) {
        this.context.ICCUtilsHelper.handleICCInfoChange();
        RIL.reportStkServiceIsRunning();
      }
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_ICCID,
      callback: callback.bind(this)
    });
  },

  






  readADNLike: function(fileId, onsuccess, onerror) {
    let ICCIOHelper = this.context.ICCIOHelper;

    function callback(options) {
      let contact =
        this.context.ICCPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (contact) {
        contact.recordId = options.p1;
        contacts.push(contact);
      }

      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (DEBUG) {
          for (let i = 0; i < contacts.length; i++) {
            this.context.debug("contact [" + i + "] " +
                               JSON.stringify(contacts[i]));
          }
        }
        if (onsuccess) {
          onsuccess(contacts);
        }
      }
    }

    let contacts = [];
    ICCIOHelper.loadLinearFixedEF({fileId: fileId,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  








  updateADNLike: function(fileId, contact, pin2, onsuccess, onerror) {
    function dataWriter(recordSize) {
      this.context.ICCPDUHelper.writeAlphaIdDiallingNumber(recordSize,
                                                           contact.alphaId,
                                                           contact.number);
    }

    function callback(options) {
      if (onsuccess) {
        onsuccess();
      }
    }

    if (!contact || !contact.recordId) {
      if (onerror) onerror(GECKO_ERROR_INVALID_PARAMETER);
      return;
    }

    this.context.ICCIOHelper.updateLinearFixedEF({
      fileId: fileId,
      recordNumber: contact.recordId,
      dataWriter: dataWriter.bind(this),
      pin2: pin2,
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  





  readPBR: function(onsuccess, onerror) {
    let Buf = this.context.Buf;
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let ICCIOHelper = this.context.ICCIOHelper;
    let ICCUtilsHelper = this.context.ICCUtilsHelper;
    let RIL = this.context.RIL;

    function callback(options) {
      let strLen = Buf.readInt32();
      let octetLen = strLen / 2, readLen = 0;

      let pbrTlvs = [];
      while (readLen < octetLen) {
        let tag = GsmPDUHelper.readHexOctet();
        if (tag == 0xff) {
          readLen++;
          Buf.seekIncoming((octetLen - readLen) * Buf.PDU_HEX_OCTET_SIZE);
          break;
        }

        let tlvLen = GsmPDUHelper.readHexOctet();
        let tlvs = ICCUtilsHelper.decodeSimTlvs(tlvLen);
        pbrTlvs.push({tag: tag,
                      length: tlvLen,
                      value: tlvs});

        readLen += tlvLen + 2; 
      }
      Buf.readStringDelimiter(strLen);

      if (pbrTlvs.length > 0) {
        let pbr = ICCUtilsHelper.parsePbrTlvs(pbrTlvs);
        
        if (!pbr.adn) {
          if (onerror) onerror("Cannot access ADN.");
          return;
        }
        pbrs.push(pbr);
      }

      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (onsuccess) {
          RIL.iccInfoPrivate.pbrs = pbrs;
          onsuccess(pbrs);
        }
      }
    }

    if (RIL.iccInfoPrivate.pbrs) {
      onsuccess(RIL.iccInfoPrivate.pbrs);
      return;
    }

    let pbrs = [];
    ICCIOHelper.loadLinearFixedEF({fileId : ICC_EF_PBR,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },

  


  _iapRecordSize: null,

  









  readIAP: function(fileId, recordNumber, onsuccess, onerror) {
    function callback(options) {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      let octetLen = strLen / 2;
      this._iapRecordSize = options.recordSize;

      let iap = this.context.GsmPDUHelper.readHexOctetArray(octetLen);
      Buf.readStringDelimiter(strLen);

      if (onsuccess) {
        onsuccess(iap);
      }
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: fileId,
      recordNumber: recordNumber,
      recordSize: this._iapRecordSize,
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  










  updateIAP: function(fileId, recordNumber, iap, onsuccess, onerror) {
    let dataWriter = function dataWriter(recordSize) {
      let Buf = this.context.Buf;
      let GsmPDUHelper = this.context.GsmPDUHelper;

      
      let strLen = recordSize * 2;
      Buf.writeInt32(strLen);

      for (let i = 0; i < iap.length; i++) {
        GsmPDUHelper.writeHexOctet(iap[i]);
      }

      Buf.writeStringDelimiter(strLen);
    }.bind(this);

    this.context.ICCIOHelper.updateLinearFixedEF({
      fileId: fileId,
      recordNumber: recordNumber,
      dataWriter: dataWriter,
      callback: onsuccess,
      onerror: onerror
    });
  },

  


  _emailRecordSize: null,

  










  readEmail: function(fileId, fileType, recordNumber, onsuccess, onerror) {
    function callback(options) {
      let Buf = this.context.Buf;
      let ICCPDUHelper = this.context.ICCPDUHelper;

      let strLen = Buf.readInt32();
      let octetLen = strLen / 2;
      let email = null;
      this._emailRecordSize = options.recordSize;

      
      
      
      
      
      
      
      
      if (fileType == ICC_USIM_TYPE1_TAG) {
        email = ICCPDUHelper.read8BitUnpackedToString(octetLen);
      } else {
        email = ICCPDUHelper.read8BitUnpackedToString(octetLen - 2);

        
        Buf.seekIncoming(2 * Buf.PDU_HEX_OCTET_SIZE); 
      }

      Buf.readStringDelimiter(strLen);

      if (onsuccess) {
        onsuccess(email);
      }
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: fileId,
      recordNumber: recordNumber,
      recordSize: this._emailRecordSize,
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  











  updateEmail: function(pbr, recordNumber, email, adnRecordId, onsuccess, onerror) {
    let fileId = pbr[USIM_PBR_EMAIL].fileId;
    let fileType = pbr[USIM_PBR_EMAIL].fileType;
    let dataWriter = function dataWriter(recordSize) {
      let Buf = this.context.Buf;
      let GsmPDUHelper = this.context.GsmPDUHelper;
      let ICCPDUHelper = this.context.ICCPDUHelper;

      
      let strLen = recordSize * 2;
      Buf.writeInt32(strLen);

      if (fileType == ICC_USIM_TYPE1_TAG) {
        ICCPDUHelper.writeStringTo8BitUnpacked(recordSize, email);
      } else {
        ICCPDUHelper.writeStringTo8BitUnpacked(recordSize - 2, email);
        GsmPDUHelper.writeHexOctet(pbr.adn.sfi || 0xff);
        GsmPDUHelper.writeHexOctet(adnRecordId);
      }

      Buf.writeStringDelimiter(strLen);
    }.bind(this);

    this.context.ICCIOHelper.updateLinearFixedEF({
      fileId: fileId,
      recordNumber: recordNumber,
      dataWriter: dataWriter,
      callback: onsuccess,
      onerror: onerror
    });
  },

  


  _anrRecordSize: null,

  










  readANR: function(fileId, fileType, recordNumber, onsuccess, onerror) {
    function callback(options) {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      let number = null;
      this._anrRecordSize = options.recordSize;

      
      Buf.seekIncoming(1 * Buf.PDU_HEX_OCTET_SIZE);

      number = this.context.ICCPDUHelper.readNumberWithLength();

      
      Buf.seekIncoming(2 * Buf.PDU_HEX_OCTET_SIZE);

      
      if (fileType == ICC_USIM_TYPE2_TAG) {
        
        Buf.seekIncoming(2 * Buf.PDU_HEX_OCTET_SIZE);
      }

      Buf.readStringDelimiter(strLen);

      if (onsuccess) {
        onsuccess(number);
      }
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: fileId,
      recordNumber: recordNumber,
      recordSize: this._anrRecordSize,
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  











  updateANR: function(pbr, recordNumber, number, adnRecordId, onsuccess, onerror) {
    let fileId = pbr[USIM_PBR_ANR0].fileId;
    let fileType = pbr[USIM_PBR_ANR0].fileType;
    let dataWriter = function dataWriter(recordSize) {
      let Buf = this.context.Buf;
      let GsmPDUHelper = this.context.GsmPDUHelper;

      
      let strLen = recordSize * 2;
      Buf.writeInt32(strLen);

      
      GsmPDUHelper.writeHexOctet(0xff);

      this.context.ICCPDUHelper.writeNumberWithLength(number);

      
      GsmPDUHelper.writeHexOctet(0xff);
      GsmPDUHelper.writeHexOctet(0xff);

      
      if (fileType == ICC_USIM_TYPE2_TAG) {
        GsmPDUHelper.writeHexOctet(pbr.adn.sfi || 0xff);
        GsmPDUHelper.writeHexOctet(adnRecordId);
      }

      Buf.writeStringDelimiter(strLen);
    }.bind(this);

    this.context.ICCIOHelper.updateLinearFixedEF({
      fileId: fileId,
      recordNumber: recordNumber,
      dataWriter: dataWriter,
      callback: onsuccess,
      onerror: onerror
    });
  },

  


  _freeRecordIds: null,

  






  findFreeRecordId: function(fileId, onsuccess, onerror) {
    let ICCIOHelper = this.context.ICCIOHelper;

    function callback(options) {
      let Buf = this.context.Buf;
      let GsmPDUHelper = this.context.GsmPDUHelper;

      let strLen = Buf.readInt32();
      let octetLen = strLen / 2;
      let readLen = 0;

      while (readLen < octetLen) {
        let octet = GsmPDUHelper.readHexOctet();
        readLen++;
        if (octet != 0xff) {
          break;
        }
      }

      let nextRecord = (options.p1 % options.totalRecords) + 1;

      if (readLen == octetLen) {
        
        this._freeRecordIds[fileId] = nextRecord;
        if (onsuccess) {
          onsuccess(options.p1);
        }
        return;
      } else {
        Buf.seekIncoming((octetLen - readLen) * Buf.PDU_HEX_OCTET_SIZE);
      }

      Buf.readStringDelimiter(strLen);

      if (nextRecord !== recordNumber) {
        options.p1 = nextRecord;
        this.context.RIL.iccIO(options);
      } else {
        
        delete this._freeRecordIds[fileId];
        if (DEBUG) {
          this.context.debug(CONTACT_ERR_NO_FREE_RECORD_FOUND);
        }
        onerror(CONTACT_ERR_NO_FREE_RECORD_FOUND);
      }
    }

    
    let recordNumber = this._freeRecordIds[fileId] || 1;
    ICCIOHelper.loadLinearFixedEF({fileId: fileId,
                                   recordNumber: recordNumber,
                                   callback: callback.bind(this),
                                   onerror: onerror});
  },
};




function SimRecordHelperObject(aContext) {
  this.context = aContext;
}
SimRecordHelperObject.prototype = {
  context: null,

  


  fetchSimRecords: function() {
    this.context.RIL.getIMSI();
    this.readAD();
    
    
    
    
    
    
    
    this.readCphsInfo(() => this.readSST(),
                      (aErrorMsg) => {
                        this.context.debug("Failed to read CPHS_INFO: " + aErrorMsg);
                        this.readSST();
                      });
  },

  



  readSimPhase: function() {
    function callback() {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();

      let GsmPDUHelper = this.context.GsmPDUHelper;
      let phase = GsmPDUHelper.readHexOctet();
      
      
      if (RILQUIRKS_SEND_STK_PROFILE_DOWNLOAD &&
          phase >= ICC_PHASE_2_PROFILE_DOWNLOAD_REQUIRED) {
        this.context.RIL.sendStkTerminalProfile(STK_SUPPORTED_TERMINAL_PROFILE);
      }

      Buf.readStringDelimiter(strLen);
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_PHASE,
      callback: callback.bind(this)
    });
  },

  


  readMSISDN: function() {
    function callback(options) {
      let RIL = this.context.RIL;

      let contact =
        this.context.ICCPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (!contact ||
          (RIL.iccInfo.msisdn !== undefined &&
           RIL.iccInfo.msisdn === contact.number)) {
        return;
      }
      RIL.iccInfo.msisdn = contact.number;
      if (DEBUG) this.context.debug("MSISDN: " + RIL.iccInfo.msisdn);
      this.context.ICCUtilsHelper.handleICCInfoChange();
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: ICC_EF_MSISDN,
      callback: callback.bind(this)
    });
  },

  


  readAD: function() {
    function callback() {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      
      let octetLen = strLen / 2;
      let ad = this.context.GsmPDUHelper.readHexOctetArray(octetLen);
      Buf.readStringDelimiter(strLen);

      if (DEBUG) {
        let str = "";
        for (let i = 0; i < ad.length; i++) {
          str += ad[i] + ", ";
        }
        this.context.debug("AD: " + str);
      }

      let ICCUtilsHelper = this.context.ICCUtilsHelper;
      let RIL = this.context.RIL;
      
      let mncLength = 0;
      if (ad && ad[3]) {
        mncLength = ad[3] & 0x0f;
        if (mncLength != 0x02 && mncLength != 0x03) {
           mncLength = 0;
        }
      }
      
      let mccMnc = ICCUtilsHelper.parseMccMncFromImsi(RIL.iccInfoPrivate.imsi,
                                                      mncLength);
      if (mccMnc) {
        RIL.iccInfo.mcc = mccMnc.mcc;
        RIL.iccInfo.mnc = mccMnc.mnc;
        ICCUtilsHelper.handleICCInfoChange();
      }
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_AD,
      callback: callback.bind(this)
    });
  },

  


  readSPN: function() {
    function callback() {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      
      let octetLen = strLen / 2;
      let spnDisplayCondition = this.context.GsmPDUHelper.readHexOctet();
      
      let spn = this.context.ICCPDUHelper.readAlphaIdentifier(octetLen - 1);
      Buf.readStringDelimiter(strLen);

      if (DEBUG) {
        this.context.debug("SPN: spn = " + spn +
                           ", spnDisplayCondition = " + spnDisplayCondition);
      }

      let RIL = this.context.RIL;
      RIL.iccInfoPrivate.spnDisplayCondition = spnDisplayCondition;
      RIL.iccInfo.spn = spn;
      let ICCUtilsHelper = this.context.ICCUtilsHelper;
      ICCUtilsHelper.updateDisplayCondition();
      ICCUtilsHelper.handleICCInfoChange();
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_SPN,
      callback: callback.bind(this)
    });
  },

  readIMG: function(recordNumber, onsuccess, onerror) {
    function callback(options) {
      let RIL = this.context.RIL;
      let Buf = this.context.Buf;
      let GsmPDUHelper = this.context.GsmPDUHelper;
      let strLen = Buf.readInt32();
      
      let octetLen = strLen / 2;

      let numInstances = GsmPDUHelper.readHexOctet();

      
      
      
      if (octetLen < (9 * numInstances + 1)) {
        Buf.seekIncoming((octetLen - 1) * Buf.PDU_HEX_OCTET_SIZE);
        Buf.readStringDelimiter(strLen);
        if (onerror) {
          onerror();
        }
        return;
      }

      let imgDescriptors = [];
      for (let i = 0; i < numInstances; i++) {
        imgDescriptors[i] = {
          width: GsmPDUHelper.readHexOctet(),
          height: GsmPDUHelper.readHexOctet(),
          codingScheme: GsmPDUHelper.readHexOctet(),
          fileId: (GsmPDUHelper.readHexOctet() << 8) |
                  GsmPDUHelper.readHexOctet(),
          offset: (GsmPDUHelper.readHexOctet() << 8) |
                  GsmPDUHelper.readHexOctet(),
          dataLen: (GsmPDUHelper.readHexOctet() << 8) |
                   GsmPDUHelper.readHexOctet()
        };
      }
      Buf.seekIncoming((octetLen - 9 * numInstances - 1) * Buf.PDU_HEX_OCTET_SIZE);
      Buf.readStringDelimiter(strLen);

      let instances = [];
      let currentInstance = 0;
      let readNextInstance = (function(img) {
        instances[currentInstance] = img;
        currentInstance++;

        if (currentInstance < numInstances) {
          let imgDescriptor = imgDescriptors[currentInstance];
          this.readIIDF(imgDescriptor.fileId,
                        imgDescriptor.offset,
                        imgDescriptor.dataLen,
                        imgDescriptor.codingScheme,
                        readNextInstance,
                        onerror);
        } else {
          if (onsuccess) {
            onsuccess(instances);
          }
        }
      }).bind(this);

      this.readIIDF(imgDescriptors[0].fileId,
                    imgDescriptors[0].offset,
                    imgDescriptors[0].dataLen,
                    imgDescriptors[0].codingScheme,
                    readNextInstance,
                    onerror);
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: ICC_EF_IMG,
      recordNumber: recordNumber,
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  readIIDF: function(fileId, offset, dataLen, codingScheme, onsuccess, onerror) {
    
    if ((fileId >> 8) != 0x4F) {
      if (onerror) {
        onerror();
      }
      return;
    }

    function callback() {
      let Buf = this.context.Buf;
      let RIL = this.context.RIL;
      let GsmPDUHelper = this.context.GsmPDUHelper;
      let strLen = Buf.readInt32();
      
      let octetLen = strLen / 2;

      if (octetLen < offset + dataLen) {
        
        
        Buf.seekIncoming(octetLen * Buf.PDU_HEX_OCTET_SIZE);
        Buf.readStringDelimiter(strLen);
        if (onerror) {
          onerror();
        }
        return;
      }

      Buf.seekIncoming(offset * Buf.PDU_HEX_OCTET_SIZE);

      let rawData = {
        width: GsmPDUHelper.readHexOctet(),
        height: GsmPDUHelper.readHexOctet(),
        codingScheme: codingScheme
      };

      switch (codingScheme) {
        case ICC_IMG_CODING_SCHEME_BASIC:
          rawData.body = GsmPDUHelper.readHexOctetArray(
            dataLen - ICC_IMG_HEADER_SIZE_BASIC);
          Buf.seekIncoming((octetLen - offset - dataLen) * Buf.PDU_HEX_OCTET_SIZE);
          break;

        case ICC_IMG_CODING_SCHEME_COLOR:
        case ICC_IMG_CODING_SCHEME_COLOR_TRANSPARENCY:
          rawData.bitsPerImgPoint = GsmPDUHelper.readHexOctet();
          let num = GsmPDUHelper.readHexOctet();
          
          rawData.numOfClutEntries = (num === 0) ? 0x100 : num;
          rawData.clutOffset = (GsmPDUHelper.readHexOctet() << 8) |
                               GsmPDUHelper.readHexOctet();
          rawData.body = GsmPDUHelper.readHexOctetArray(
              dataLen - ICC_IMG_HEADER_SIZE_COLOR);

          Buf.seekIncoming((rawData.clutOffset - offset - dataLen) *
                           Buf.PDU_HEX_OCTET_SIZE);
          let clut = GsmPDUHelper.readHexOctetArray(rawData.numOfClutEntries *
                                                    ICC_CLUT_ENTRY_SIZE);

          rawData.clut = clut;
      }

      Buf.readStringDelimiter(strLen);

      if (onsuccess) {
        onsuccess(rawData);
      }
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: fileId,
      pathId: this.context.ICCFileHelper.getEFPath(ICC_EF_IMG),
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  


  readSST: function() {
    function callback() {
      let Buf = this.context.Buf;
      let RIL = this.context.RIL;

      let strLen = Buf.readInt32();
      
      let octetLen = strLen / 2;
      let sst = this.context.GsmPDUHelper.readHexOctetArray(octetLen);
      Buf.readStringDelimiter(strLen);
      RIL.iccInfoPrivate.sst = sst;
      if (DEBUG) {
        let str = "";
        for (let i = 0; i < sst.length; i++) {
          str += sst[i] + ", ";
        }
        this.context.debug("SST: " + str);
      }

      let ICCUtilsHelper = this.context.ICCUtilsHelper;
      if (ICCUtilsHelper.isICCServiceAvailable("MSISDN")) {
        if (DEBUG) this.context.debug("MSISDN: MSISDN is available");
        this.readMSISDN();
      } else {
        if (DEBUG) this.context.debug("MSISDN: MSISDN service is not available");
      }

      
      if (ICCUtilsHelper.isICCServiceAvailable("SPN")) {
        if (DEBUG) this.context.debug("SPN: SPN is available");
        this.readSPN();
      } else {
        if (DEBUG) this.context.debug("SPN: SPN service is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("MDN")) {
        if (DEBUG) this.context.debug("MDN: MDN available.");
        this.readMBDN();
      } else {
        if (DEBUG) this.context.debug("MDN: MDN service is not available");

        if (ICCUtilsHelper.isCphsServiceAvailable("MBN")) {
          
          this.readCphsMBN();
        } else {
          if (DEBUG) this.context.debug("CPHS_MBN: CPHS_MBN service is not available");
        }
      }

      if (ICCUtilsHelper.isICCServiceAvailable("MWIS")) {
        if (DEBUG) this.context.debug("MWIS: MWIS is available");
        this.readMWIS();
      } else {
        if (DEBUG) this.context.debug("MWIS: MWIS is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("SPDI")) {
        if (DEBUG) this.context.debug("SPDI: SPDI available.");
        this.readSPDI();
      } else {
        if (DEBUG) this.context.debug("SPDI: SPDI service is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("PNN")) {
        if (DEBUG) this.context.debug("PNN: PNN is available");
        this.readPNN();
      } else {
        if (DEBUG) this.context.debug("PNN: PNN is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("OPL")) {
        if (DEBUG) this.context.debug("OPL: OPL is available");
        this.readOPL();
      } else {
        if (DEBUG) this.context.debug("OPL: OPL is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("GID1")) {
        if (DEBUG) this.context.debug("GID1: GID1 is available");
        this.readGID1();
      } else {
        if (DEBUG) this.context.debug("GID1: GID1 is not available");
      }

      if (ICCUtilsHelper.isICCServiceAvailable("CBMI")) {
        this.readCBMI();
      } else {
        RIL.cellBroadcastConfigs.CBMI = null;
      }
      if (ICCUtilsHelper.isICCServiceAvailable("DATA_DOWNLOAD_SMS_CB")) {
        this.readCBMID();
      } else {
        RIL.cellBroadcastConfigs.CBMID = null;
      }
      if (ICCUtilsHelper.isICCServiceAvailable("CBMIR")) {
        this.readCBMIR();
      } else {
        RIL.cellBroadcastConfigs.CBMIR = null;
      }
      RIL._mergeAllCellBroadcastConfigs();
    }

    
    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_SST,
      callback: callback.bind(this)
    });
  },

  




  readMBDN: function() {
    function callback(options) {
      let RIL = this.context.RIL;
      let contact =
        this.context.ICCPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if ((!contact ||
           ((!contact.alphaId || contact.alphaId == "") &&
            (!contact.number || contact.number == ""))) &&
          this.context.ICCUtilsHelper.isCphsServiceAvailable("MBN")) {
        
        this.readCphsMBN();
        return;
      }

      if (!contact ||
          (RIL.iccInfoPrivate.mbdn !== undefined &&
           RIL.iccInfoPrivate.mbdn === contact.number)) {
        return;
      }
      RIL.iccInfoPrivate.mbdn = contact.number;
      if (DEBUG) {
        this.context.debug("MBDN, alphaId=" + contact.alphaId +
                           " number=" + contact.number);
      }
      contact.rilMessageType = "iccmbdn";
      RIL.sendChromeMessage(contact);
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: ICC_EF_MBDN,
      callback: callback.bind(this)
    });
  },

  




  readMWIS: function() {
    function callback(options) {
      let Buf = this.context.Buf;
      let RIL = this.context.RIL;

      let strLen = Buf.readInt32();
      
      let octetLen = strLen / 2;
      let mwis = this.context.GsmPDUHelper.readHexOctetArray(octetLen);
      Buf.readStringDelimiter(strLen);
      if (!mwis) {
        return;
      }
      RIL.iccInfoPrivate.mwis = mwis; 

      let mwi = {};
      
      
      
      
      
      
      
      mwi.active = ((mwis[0] & 0x01) != 0);

      if (mwi.active) {
        
        
        
        
        
        
        
        
        mwi.msgCount = (mwis[1] === 0) ? GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN
                                       : mwis[1];
      } else {
        mwi.msgCount = 0;
      }

      RIL.sendChromeMessage({ rilMessageType: "iccmwis",
                              mwi: mwi });
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: ICC_EF_MWIS,
      recordNumber: 1, 
      callback: callback.bind(this)
    });
  },

  




  updateMWIS: function(mwi) {
    let RIL = this.context.RIL;
    if (!RIL.iccInfoPrivate.mwis) {
      return;
    }

    function dataWriter(recordSize) {
      let mwis = RIL.iccInfoPrivate.mwis;

      let msgCount =
          (mwi.msgCount === GECKO_VOICEMAIL_MESSAGE_COUNT_UNKNOWN) ? 0 : mwi.msgCount;

      [mwis[0], mwis[1]] = (mwi.active) ? [(mwis[0] | 0x01), msgCount]
                                        : [(mwis[0] & 0xFE), 0];

      let strLen = recordSize * 2;
      let Buf = this.context.Buf;
      Buf.writeInt32(strLen);

      let GsmPDUHelper = this.context.GsmPDUHelper;
      for (let i = 0; i < mwis.length; i++) {
        GsmPDUHelper.writeHexOctet(mwis[i]);
      }

      Buf.writeStringDelimiter(strLen);
    }

    this.context.ICCIOHelper.updateLinearFixedEF({
      fileId: ICC_EF_MWIS,
      recordNumber: 1, 
      dataWriter: dataWriter.bind(this)
    });
  },

  





  readSPDI: function() {
    function callback() {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      let octetLen = strLen / 2;
      let readLen = 0;
      let endLoop = false;

      let RIL = this.context.RIL;
      RIL.iccInfoPrivate.SPDI = null;

      let GsmPDUHelper = this.context.GsmPDUHelper;
      while ((readLen < octetLen) && !endLoop) {
        let tlvTag = GsmPDUHelper.readHexOctet();
        let tlvLen = GsmPDUHelper.readHexOctet();
        readLen += 2; 
        switch (tlvTag) {
        case SPDI_TAG_SPDI:
          
          continue;
        case SPDI_TAG_PLMN_LIST:
          
          RIL.iccInfoPrivate.SPDI = this.readPLMNEntries(tlvLen / 3);
          readLen += tlvLen;
          endLoop = true;
          break;
        default:
          
          
          endLoop = true;
          break;
        }
      }

      
      Buf.seekIncoming((octetLen - readLen) * Buf.PDU_HEX_OCTET_SIZE);
      Buf.readStringDelimiter(strLen);

      if (DEBUG) {
        this.context.debug("SPDI: " + JSON.stringify(RIL.iccInfoPrivate.SPDI));
      }
      let ICCUtilsHelper = this.context.ICCUtilsHelper;
      if (ICCUtilsHelper.updateDisplayCondition()) {
        ICCUtilsHelper.handleICCInfoChange();
      }
    }

    
    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_SPDI,
      callback: callback.bind(this)
    });
  },

  _readCbmiHelper: function(which) {
    let RIL = this.context.RIL;

    function callback() {
      let Buf = this.context.Buf;
      let strLength = Buf.readInt32();

      
      
      let numIds = strLength / 4, list = null;
      if (numIds) {
        list = [];
        let GsmPDUHelper = this.context.GsmPDUHelper;
        for (let i = 0, id; i < numIds; i++) {
          id = GsmPDUHelper.readHexOctet() << 8 | GsmPDUHelper.readHexOctet();
          
          if (id != 0xFFFF) {
            list.push(id);
            list.push(id + 1);
          }
        }
      }
      if (DEBUG) {
        this.context.debug(which + ": " + JSON.stringify(list));
      }

      Buf.readStringDelimiter(strLength);

      RIL.cellBroadcastConfigs[which] = list;
      RIL._mergeAllCellBroadcastConfigs();
    }

    function onerror() {
      RIL.cellBroadcastConfigs[which] = null;
      RIL._mergeAllCellBroadcastConfigs();
    }

    let fileId = GLOBAL["ICC_EF_" + which];
    this.context.ICCIOHelper.loadTransparentEF({
      fileId: fileId,
      callback: callback.bind(this),
      onerror: onerror.bind(this)
    });
  },

  





  readCBMI: function() {
    this._readCbmiHelper("CBMI");
  },

  





  readCBMID: function() {
    this._readCbmiHelper("CBMID");
  },

  





  readCBMIR: function() {
    let RIL = this.context.RIL;

    function callback() {
      let Buf = this.context.Buf;
      let strLength = Buf.readInt32();

      
      
      let numIds = strLength / 8, list = null;
      if (numIds) {
        list = [];
        let GsmPDUHelper = this.context.GsmPDUHelper;
        for (let i = 0, from, to; i < numIds; i++) {
          
          
          
          from = GsmPDUHelper.readHexOctet() << 8 | GsmPDUHelper.readHexOctet();
          to = GsmPDUHelper.readHexOctet() << 8 | GsmPDUHelper.readHexOctet();
          
          if ((from != 0xFFFF) && (to != 0xFFFF)) {
            list.push(from);
            list.push(to + 1);
          }
        }
      }
      if (DEBUG) {
        this.context.debug("CBMIR: " + JSON.stringify(list));
      }

      Buf.readStringDelimiter(strLength);

      RIL.cellBroadcastConfigs.CBMIR = list;
      RIL._mergeAllCellBroadcastConfigs();
    }

    function onerror() {
      RIL.cellBroadcastConfigs.CBMIR = null;
      RIL._mergeAllCellBroadcastConfigs();
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_CBMIR,
      callback: callback.bind(this),
      onerror: onerror.bind(this)
    });
  },

  





  readOPL: function() {
    let ICCIOHelper = this.context.ICCIOHelper;
    let opl = [];
    function callback(options) {
      let Buf = this.context.Buf;
      let GsmPDUHelper = this.context.GsmPDUHelper;

      let strLen = Buf.readInt32();
      
      
      
      
      
      
      
      let mccMnc = [GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet()];
      if (mccMnc[0] != 0xFF || mccMnc[1] != 0xFF || mccMnc[2] != 0xFF) {
        let oplElement = {};
        let semiOctets = [];
        for (let i = 0; i < mccMnc.length; i++) {
          semiOctets.push((mccMnc[i] & 0xf0) >> 4);
          semiOctets.push(mccMnc[i] & 0x0f);
        }
        let reformat = [semiOctets[1], semiOctets[0], semiOctets[3],
                        semiOctets[5], semiOctets[4], semiOctets[2]];
        let buf = "";
        for (let i = 0; i < reformat.length; i++) {
          if (reformat[i] != 0xF) {
            buf += GsmPDUHelper.semiOctetToExtendedBcdChar(reformat[i]);
          }
          if (i === 2) {
            
            oplElement.mcc = buf;
            buf = "";
          } else if (i === 5) {
            
            oplElement.mnc = buf;
          }
        }
        
        oplElement.lacTacStart =
          (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
        oplElement.lacTacEnd =
          (GsmPDUHelper.readHexOctet() << 8) | GsmPDUHelper.readHexOctet();
        
        oplElement.pnnRecordId = GsmPDUHelper.readHexOctet();
        if (DEBUG) {
          this.context.debug("OPL: [" + (opl.length + 1) + "]: " +
                             JSON.stringify(oplElement));
        }
        opl.push(oplElement);
      } else {
        Buf.seekIncoming(5 * Buf.PDU_HEX_OCTET_SIZE);
      }
      Buf.readStringDelimiter(strLen);

      let RIL = this.context.RIL;
      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        RIL.iccInfoPrivate.OPL = opl;
        RIL.overrideICCNetworkName();
      }
    }

    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_OPL,
                                   callback: callback.bind(this)});
  },

  





  readPNN: function() {
    let ICCIOHelper = this.context.ICCIOHelper;
    function callback(options) {
      let pnnElement;
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      let octetLen = strLen / 2;
      let readLen = 0;

      let GsmPDUHelper = this.context.GsmPDUHelper;
      while (readLen < octetLen) {
        let tlvTag = GsmPDUHelper.readHexOctet();

        if (tlvTag == 0xFF) {
          
          readLen++;
          Buf.seekIncoming((octetLen - readLen) * Buf.PDU_HEX_OCTET_SIZE);
          break;
        }

        
        pnnElement = pnnElement || {};

        let tlvLen = GsmPDUHelper.readHexOctet();

        switch (tlvTag) {
          case PNN_IEI_FULL_NETWORK_NAME:
            pnnElement.fullName = GsmPDUHelper.readNetworkName(tlvLen);
            break;
          case PNN_IEI_SHORT_NETWORK_NAME:
            pnnElement.shortName = GsmPDUHelper.readNetworkName(tlvLen);
            break;
          default:
            Buf.seekIncoming(tlvLen * Buf.PDU_HEX_OCTET_SIZE);
            break;
        }

        readLen += (tlvLen + 2); 
      }
      Buf.readStringDelimiter(strLen);

      pnn.push(pnnElement);

      let RIL = this.context.RIL;
      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (DEBUG) {
          for (let i = 0; i < pnn.length; i++) {
            this.context.debug("PNN: [" + i + "]: " + JSON.stringify(pnn[i]));
          }
        }
        RIL.iccInfoPrivate.PNN = pnn;
        RIL.overrideICCNetworkName();
      }
    }

    let pnn = [];
    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_PNN,
                                   callback: callback.bind(this)});
  },

  








  readPLMNEntries: function(length) {
    let plmnList = [];
    
    if (DEBUG) {
      this.context.debug("PLMN entries length = " + length);
    }
    let GsmPDUHelper = this.context.GsmPDUHelper;
    let index = 0;
    while (index < length) {
      
      
      try {
        let plmn = [GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet(),
                    GsmPDUHelper.readHexOctet()];
        if (DEBUG) {
          this.context.debug("Reading PLMN entry: [" + index + "]: '" + plmn + "'");
        }
        if (plmn[0] != 0xFF &&
            plmn[1] != 0xFF &&
            plmn[2] != 0xFF) {
          let semiOctets = [];
          for (let idx = 0; idx < plmn.length; idx++) {
            semiOctets.push((plmn[idx] & 0xF0) >> 4);
            semiOctets.push(plmn[idx] & 0x0F);
          }

          
          
          
          
          
          
          let reformat = [semiOctets[1], semiOctets[0], semiOctets[3],
                          semiOctets[5], semiOctets[4], semiOctets[2]];
          let buf = "";
          let plmnEntry = {};
          for (let i = 0; i < reformat.length; i++) {
            if (reformat[i] != 0xF) {
              buf += GsmPDUHelper.semiOctetToExtendedBcdChar(reformat[i]);
            }
            if (i === 2) {
              
              plmnEntry.mcc = buf;
              buf = "";
            } else if (i === 5) {
              
              plmnEntry.mnc = buf;
            }
          }
          if (DEBUG) {
            this.context.debug("PLMN = " + plmnEntry.mcc + ", " + plmnEntry.mnc);
          }
          plmnList.push(plmnEntry);
        }
      } catch (e) {
        if (DEBUG) {
          this.context.debug("PLMN entry " + index + " is invalid.");
        }
        break;
      }
      index ++;
    }
    return plmnList;
  },

  






  readSMS: function(recordNumber, onsuccess, onerror) {
    function callback(options) {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();

      
      
      
      
      
      
      let GsmPDUHelper = this.context.GsmPDUHelper;
      let status = GsmPDUHelper.readHexOctet();

      let message = GsmPDUHelper.readMessage();
      message.simStatus = status;

      
      Buf.seekIncoming(Buf.getReadAvailable() - Buf.PDU_HEX_OCTET_SIZE);

      Buf.readStringDelimiter(strLen);

      if (message) {
        onsuccess(message);
      } else {
        onerror("Failed to decode SMS on SIM #" + recordNumber);
      }
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: ICC_EF_SMS,
      recordNumber: recordNumber,
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  readGID1: function() {
    function callback() {
      let Buf = this.context.Buf;
      let RIL = this.context.RIL;

      RIL.iccInfoPrivate.gid1 = Buf.readString();
      if (DEBUG) {
        this.context.debug("GID1: " + RIL.iccInfoPrivate.gid1);
      }
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_GID1,
      callback: callback.bind(this)
    });
  },

  







  readCphsInfo: function(onsuccess, onerror) {
    function callback() {
      try {
        let Buf = this.context.Buf;
        let RIL = this.context.RIL;

        let strLen = Buf.readInt32();
        
        let octetLen = strLen / 2;
        let cphsInfo = this.context.GsmPDUHelper.readHexOctetArray(octetLen);
        Buf.readStringDelimiter(strLen);
        if (DEBUG) {
          let str = "";
          for (let i = 0; i < cphsInfo.length; i++) {
            str += cphsInfo[i] + ", ";
          }
          this.context.debug("CPHS INFO: " + str);
        }

        























        let cphsPhase = cphsInfo[0];
        if (cphsPhase == 1) {
          
          cphsInfo[1] &= 0x3F;
          
          
          if (cphsInfo.length > 2) {
            cphsInfo[2] = 0x00;
          }
        } else if (cphsPhase == 2) {
          
          cphsInfo[1] &= 0xF3;
        } else {
          throw new Error("Unknown CPHS phase: " + cphsPhase);
        }

        RIL.iccInfoPrivate.cphsSt = cphsInfo.subarray(1);
        onsuccess();
      } catch(e) {
        onerror(e.toString());
      }
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_CPHS_INFO,
      callback: callback.bind(this),
      onerror: onerror
    });
  },

  




  readCphsMBN: function() {
    function callback(options) {
      let RIL = this.context.RIL;
      let contact =
        this.context.ICCPDUHelper.readAlphaIdDiallingNumber(options.recordSize);
      if (!contact ||
          (RIL.iccInfoPrivate.mbdn !== undefined &&
           RIL.iccInfoPrivate.mbdn === contact.number)) {
        return;
      }
      RIL.iccInfoPrivate.mbdn = contact.number;
      if (DEBUG) {
        this.context.debug("CPHS_MDN, alphaId=" + contact.alphaId +
                           " number=" + contact.number);
      }
      contact.rilMessageType = "iccmbdn";
      RIL.sendChromeMessage(contact);
    }

    this.context.ICCIOHelper.loadLinearFixedEF({
      fileId: ICC_EF_CPHS_MBN,
      callback: callback.bind(this)
    });
  }
};

function RuimRecordHelperObject(aContext) {
  this.context = aContext;
}
RuimRecordHelperObject.prototype = {
  context: null,

  fetchRuimRecords: function() {
    this.getIMSI_M();
    this.readCST();
    this.readCDMAHome();
    this.context.RIL.getCdmaSubscription();
  },

  



  getIMSI_M: function() {
    function callback() {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      let encodedImsi = this.context.GsmPDUHelper.readHexOctetArray(strLen / 2);
      Buf.readStringDelimiter(strLen);

      if ((encodedImsi[CSIM_IMSI_M_PROGRAMMED_BYTE] & 0x80)) { 
        let RIL = this.context.RIL;
        RIL.iccInfoPrivate.imsi = this.decodeIMSI(encodedImsi);
        RIL.sendChromeMessage({rilMessageType: "iccimsi",
                               imsi: RIL.iccInfoPrivate.imsi});

        let ICCUtilsHelper = this.context.ICCUtilsHelper;
        let mccMnc = ICCUtilsHelper.parseMccMncFromImsi(RIL.iccInfoPrivate.imsi);
        if (mccMnc) {
          RIL.iccInfo.mcc = mccMnc.mcc;
          RIL.iccInfo.mnc = mccMnc.mnc;
          ICCUtilsHelper.handleICCInfoChange();
        }
      }
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_CSIM_IMSI_M,
      callback: callback.bind(this)
    });
  },

  








  decodeIMSI: function(encodedImsi) {
    
    let encodedMCC = ((encodedImsi[CSIM_IMSI_M_MCC_BYTE + 1] & 0x03) << 8) +
                      (encodedImsi[CSIM_IMSI_M_MCC_BYTE] & 0xff);
    let mcc = this.decodeIMSIValue(encodedMCC, 3);

    
    let encodedMNC =  encodedImsi[CSIM_IMSI_M_MNC_BYTE] & 0x7f;
    let mnc = this.decodeIMSIValue(encodedMNC, 2);

    
    let encodedMIN2 = ((encodedImsi[CSIM_IMSI_M_MIN2_BYTE + 1] & 0x03) << 8) +
                       (encodedImsi[CSIM_IMSI_M_MIN2_BYTE] & 0xff);
    let min2 = this.decodeIMSIValue(encodedMIN2, 3);

    
    let encodedMIN1First3 = ((encodedImsi[CSIM_IMSI_M_MIN1_BYTE + 2] & 0xff) << 2) +
                             ((encodedImsi[CSIM_IMSI_M_MIN1_BYTE + 1] & 0xc0) >> 6);
    let min1First3 = this.decodeIMSIValue(encodedMIN1First3, 3);

    let encodedFourthDigit = (encodedImsi[CSIM_IMSI_M_MIN1_BYTE + 1] & 0x3c) >> 2;
    if (encodedFourthDigit > 9) {
      encodedFourthDigit = 0;
    }
    let fourthDigit = encodedFourthDigit.toString();

    let encodedMIN1Last3 = ((encodedImsi[CSIM_IMSI_M_MIN1_BYTE + 1] & 0x03) << 8) +
                            (encodedImsi[CSIM_IMSI_M_MIN1_BYTE] & 0xff);
    let min1Last3 = this.decodeIMSIValue(encodedMIN1Last3, 3);

    return mcc + mnc + min2 + min1First3 + fourthDigit + min1Last3;
  },

  



  decodeIMSIValue: function(encoded, length) {
    let offset = length === 3 ? 111 : 11;
    let value = encoded + offset;

    for (let base = 10, temp = value, i = 0; i < length; i++) {
      if (temp % 10 === 0) {
        value -= base;
      }
      temp = Math.floor(value / base);
      base = base * 10;
    }

    let s = value.toString();
    while (s.length < length) {
      s = "0" + s;
    }

    return s;
  },

  



  readCDMAHome: function() {
    let ICCIOHelper = this.context.ICCIOHelper;

    function callback(options) {
      let Buf = this.context.Buf;
      let GsmPDUHelper = this.context.GsmPDUHelper;

      let strLen = Buf.readInt32();
      let tempOctet = GsmPDUHelper.readHexOctet();
      cdmaHomeSystemId.push(((GsmPDUHelper.readHexOctet() & 0x7f) << 8) | tempOctet);
      tempOctet = GsmPDUHelper.readHexOctet();
      cdmaHomeNetworkId.push(((GsmPDUHelper.readHexOctet() & 0xff) << 8) | tempOctet);

      
      Buf.seekIncoming(Buf.PDU_HEX_OCTET_SIZE);

      Buf.readStringDelimiter(strLen);
      if (options.p1 < options.totalRecords) {
        ICCIOHelper.loadNextRecord(options);
      } else {
        if (DEBUG) {
          this.context.debug("CDMAHome system id: " +
                             JSON.stringify(cdmaHomeSystemId));
          this.context.debug("CDMAHome network id: " +
                             JSON.stringify(cdmaHomeNetworkId));
        }
        this.context.RIL.cdmaHome = {
          systemId: cdmaHomeSystemId,
          networkId: cdmaHomeNetworkId
        };
      }
    }

    let cdmaHomeSystemId = [], cdmaHomeNetworkId = [];
    ICCIOHelper.loadLinearFixedEF({fileId: ICC_EF_CSIM_CDMAHOME,
                                   callback: callback.bind(this)});
  },

  



  readCST: function() {
    function callback() {
      let Buf = this.context.Buf;
      let RIL = this.context.RIL;

      let strLen = Buf.readInt32();
      
      RIL.iccInfoPrivate.cst =
        this.context.GsmPDUHelper.readHexOctetArray(strLen / 2);
      Buf.readStringDelimiter(strLen);

      if (DEBUG) {
        let str = "";
        for (let i = 0; i < RIL.iccInfoPrivate.cst.length; i++) {
          str += RIL.iccInfoPrivate.cst[i] + ", ";
        }
        this.context.debug("CST: " + str);
      }

      if (this.context.ICCUtilsHelper.isICCServiceAvailable("SPN")) {
        if (DEBUG) this.context.debug("SPN: SPN is available");
        this.readSPN();
      }
    }
    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_CSIM_CST,
      callback: callback.bind(this)
    });
  },

  readSPN: function() {
    function callback() {
      let Buf = this.context.Buf;
      let strLen = Buf.readInt32();
      let octetLen = strLen / 2;

      let GsmPDUHelper = this.context.GsmPDUHelper;
      let displayCondition = GsmPDUHelper.readHexOctet();
      let codingScheme = GsmPDUHelper.readHexOctet();
      
      Buf.seekIncoming(Buf.PDU_HEX_OCTET_SIZE);
      let readLen = 3;

      
      let userDataBuffer = [];

      while (readLen < octetLen) {
        let octet = GsmPDUHelper.readHexOctet();
        readLen++;
        if (octet == 0xff) {
          break;
        }
        userDataBuffer.push(octet);
      }

      this.context.BitBufferHelper.startRead(userDataBuffer);

      let CdmaPDUHelper = this.context.CdmaPDUHelper;
      let msgLen;
      switch (CdmaPDUHelper.getCdmaMsgEncoding(codingScheme)) {
      case PDU_DCS_MSG_CODING_7BITS_ALPHABET:
        msgLen = Math.floor(userDataBuffer.length * 8 / 7);
        break;
      case PDU_DCS_MSG_CODING_8BITS_ALPHABET:
        msgLen = userDataBuffer.length;
        break;
      case PDU_DCS_MSG_CODING_16BITS_ALPHABET:
        msgLen = Math.floor(userDataBuffer.length / 2);
        break;
      }

      let RIL = this.context.RIL;
      RIL.iccInfo.spn = CdmaPDUHelper.decodeCdmaPDUMsg(codingScheme, null, msgLen);
      if (DEBUG) {
        this.context.debug("CDMA SPN: " + RIL.iccInfo.spn +
                           ", Display condition: " + displayCondition);
      }
      RIL.iccInfoPrivate.spnDisplayCondition = displayCondition;
      Buf.seekIncoming((octetLen - readLen) * Buf.PDU_HEX_OCTET_SIZE);
      Buf.readStringDelimiter(strLen);
    }

    this.context.ICCIOHelper.loadTransparentEF({
      fileId: ICC_EF_CSIM_SPN,
      callback: callback.bind(this)
    });
  }
};




function ICCUtilsHelperObject(aContext) {
  this.context = aContext;
}
ICCUtilsHelperObject.prototype = {
  context: null,

  









  getNetworkNameFromICC: function(mcc, mnc, lac) {
    let RIL = this.context.RIL;
    let iccInfoPriv = RIL.iccInfoPrivate;
    let iccInfo = RIL.iccInfo;
    let pnnEntry;

    if (!mcc || !mnc || lac == null || lac < 0) {
      return null;
    }

    
    if (!iccInfoPriv.PNN) {
      return null;
    }

    if (!this.isICCServiceAvailable("OPL")) {
      
      
      
      
      
      
      if (mcc == iccInfo.mcc && mnc == iccInfo.mnc) {
        pnnEntry = iccInfoPriv.PNN[0];
      }
    } else {
      let GsmPDUHelper = this.context.GsmPDUHelper;
      let wildChar = GsmPDUHelper.extendedBcdChars.charAt(0x0d);
      
      
      
      
      let length = iccInfoPriv.OPL ? iccInfoPriv.OPL.length : 0;
      for (let i = 0; i < length; i++) {
        let unmatch = false;
        let opl = iccInfoPriv.OPL[i];
        
        
        
        if (opl.mcc.indexOf(wildChar) !== -1) {
          for (let j = 0; j < opl.mcc.length; j++) {
            if (opl.mcc[j] !== wildChar && opl.mcc[j] !== mcc[j]) {
              unmatch = true;
              break;
            }
          }
          if (unmatch) {
            continue;
          }
        } else {
          if (mcc !== opl.mcc) {
            continue;
          }
        }

        if (mnc.length !== opl.mnc.length) {
          continue;
        }

        if (opl.mnc.indexOf(wildChar) !== -1) {
          for (let j = 0; j < opl.mnc.length; j++) {
            if (opl.mnc[j] !== wildChar && opl.mnc[j] !== mnc[j]) {
              unmatch = true;
              break;
            }
          }
          if (unmatch) {
            continue;
          }
        } else {
          if (mnc !== opl.mnc) {
            continue;
          }
        }

        
        
        
        if ((opl.lacTacStart === 0x0 && opl.lacTacEnd == 0xFFFE) ||
            (opl.lacTacStart <= lac && opl.lacTacEnd >= lac)) {
          if (opl.pnnRecordId === 0) {
            
            
            
            return null;
          }
          pnnEntry = iccInfoPriv.PNN[opl.pnnRecordId - 1];
          break;
        }
      }
    }

    if (!pnnEntry) {
      return null;
    }

    
    return { fullName: pnnEntry.fullName || "",
             shortName: pnnEntry.shortName || "" };
  },

  





  updateDisplayCondition: function() {
    let RIL = this.context.RIL;

    
    
    
    let iccInfo = RIL.iccInfo;
    let iccInfoPriv = RIL.iccInfoPrivate;
    let displayCondition = iccInfoPriv.spnDisplayCondition;
    let origIsDisplayNetworkNameRequired = iccInfo.isDisplayNetworkNameRequired;
    let origIsDisplaySPNRequired = iccInfo.isDisplaySpnRequired;

    if (displayCondition === undefined) {
      iccInfo.isDisplayNetworkNameRequired = true;
      iccInfo.isDisplaySpnRequired = false;
    } else if (RIL._isCdma) {
      
      let cdmaHome = RIL.cdmaHome;
      let cell = RIL.voiceRegistrationState.cell;
      let sid = cell && cell.cdmaSystemId;
      let nid = cell && cell.cdmaNetworkId;

      iccInfo.isDisplayNetworkNameRequired = false;

      
      
      if (displayCondition === 0x0) {
        iccInfo.isDisplaySpnRequired = false;
      } else {
        
        
        if (!cdmaHome ||
            !cdmaHome.systemId ||
            cdmaHome.systemId.length === 0 ||
            cdmaHome.systemId.length != cdmaHome.networkId.length ||
            !sid || !nid) {
          
          
          
          iccInfo.isDisplaySpnRequired = true;
        } else {
          
          
          let inHomeArea = false;
          for (let i = 0; i < cdmaHome.systemId.length; i++) {
            let homeSid = cdmaHome.systemId[i],
                homeNid = cdmaHome.networkId[i];
            if (homeSid === 0 || homeNid === 0 
               || homeSid != sid) {
              continue;
            }
            
            
            if (homeNid == 65535 || homeNid == nid) {
              inHomeArea = true;
              break;
            }
          }
          iccInfo.isDisplaySpnRequired = inHomeArea;
        }
      }
    } else {
      
      let operatorMnc = RIL.operator ? RIL.operator.mnc : -1;
      let operatorMcc = RIL.operator ? RIL.operator.mcc : -1;

      
      
      let isOnMatchingPlmn = false;

      
      
      if (iccInfo.mcc == operatorMcc && iccInfo.mnc == operatorMnc) {
        isOnMatchingPlmn = true;
      }

      
      if (!isOnMatchingPlmn && iccInfoPriv.SPDI) {
        let iccSpdi = iccInfoPriv.SPDI; 
        for (let plmn in iccSpdi) {
          let plmnMcc = iccSpdi[plmn].mcc;
          let plmnMnc = iccSpdi[plmn].mnc;
          isOnMatchingPlmn = (plmnMcc == operatorMcc) && (plmnMnc == operatorMnc);
          if (isOnMatchingPlmn) {
            break;
          }
        }
      }

      
      
      if (isOnMatchingPlmn) {
        
        
        if (DEBUG) {
          this.context.debug("PLMN is HPLMN or PLMN " + "is in PLMN list");
        }

        
        
        
        iccInfo.isDisplaySpnRequired = true;
        iccInfo.isDisplayNetworkNameRequired = (displayCondition & 0x01) !== 0;
      } else {
        
        
        if (DEBUG) {
          this.context.debug("PLMN isn't HPLMN and PLMN isn't in PLMN list");
        }

        iccInfo.isDisplayNetworkNameRequired = true;
        iccInfo.isDisplaySpnRequired = (displayCondition & 0x02) === 0;
      }
    }

    if (DEBUG) {
      this.context.debug("isDisplayNetworkNameRequired = " +
                         iccInfo.isDisplayNetworkNameRequired);
      this.context.debug("isDisplaySpnRequired = " + iccInfo.isDisplaySpnRequired);
    }

    return ((origIsDisplayNetworkNameRequired !== iccInfo.isDisplayNetworkNameRequired) ||
            (origIsDisplaySPNRequired !== iccInfo.isDisplaySpnRequired));
  },

  decodeSimTlvs: function(tlvsLen) {
    let GsmPDUHelper = this.context.GsmPDUHelper;

    let index = 0;
    let tlvs = [];
    while (index < tlvsLen) {
      let simTlv = {
        tag : GsmPDUHelper.readHexOctet(),
        length : GsmPDUHelper.readHexOctet(),
      };
      simTlv.value = GsmPDUHelper.readHexOctetArray(simTlv.length);
      tlvs.push(simTlv);
      index += simTlv.length + 2; 
    }
    return tlvs;
  },

  


  parsePbrTlvs: function(pbrTlvs) {
    let pbr = {};
    for (let i = 0; i < pbrTlvs.length; i++) {
      let pbrTlv = pbrTlvs[i];
      let anrIndex = 0;
      for (let j = 0; j < pbrTlv.value.length; j++) {
        let tlv = pbrTlv.value[j];
        let tagName = USIM_TAG_NAME[tlv.tag];

        
        if (tlv.tag == ICC_USIM_EFANR_TAG) {
          tagName += anrIndex;
          anrIndex++;
        }
        pbr[tagName] = tlv;
        pbr[tagName].fileType = pbrTlv.tag;
        pbr[tagName].fileId = (tlv.value[0] << 8) | tlv.value[1];
        pbr[tagName].sfi = tlv.value[2];

        
        if (pbrTlv.tag == ICC_USIM_TYPE2_TAG) {
          pbr[tagName].indexInIAP = j;
        }
      }
    }

    return pbr;
  },

  


  handleICCInfoChange: function() {
    let RIL = this.context.RIL;
    RIL.iccInfo.rilMessageType = "iccinfochange";
    RIL.sendChromeMessage(RIL.iccInfo);
  },

  







  isICCServiceAvailable: function(geckoService) {
    let RIL = this.context.RIL;
    let serviceTable = RIL._isCdma ? RIL.iccInfoPrivate.cst:
                                     RIL.iccInfoPrivate.sst;
    let index, bitmask;
    if (RIL.appType == CARD_APPTYPE_SIM || RIL.appType == CARD_APPTYPE_RUIM) {
      













      let simService;
      if (RIL.appType == CARD_APPTYPE_SIM) {
        simService = GECKO_ICC_SERVICES.sim[geckoService];
      } else {
        simService = GECKO_ICC_SERVICES.ruim[geckoService];
      }
      if (!simService) {
        return false;
      }
      simService -= 1;
      index = Math.floor(simService / 4);
      bitmask = 2 << ((simService % 4) << 1);
    } else if (RIL.appType == CARD_APPTYPE_USIM) {
      











      let usimService = GECKO_ICC_SERVICES.usim[geckoService];
      if (!usimService) {
        return false;
      }
      usimService -= 1;
      index = Math.floor(usimService / 8);
      bitmask = 1 << ((usimService % 8) << 0);
    }

    return (serviceTable !== null) &&
           (index < serviceTable.length) &&
           ((serviceTable[index] & bitmask) !== 0);
  },

  







  isCphsServiceAvailable: function(geckoService) {
    let RIL = this.context.RIL;
    let serviceTable = RIL.iccInfoPrivate.cphsSt;

    if (!(serviceTable instanceof Uint8Array)) {
      return false;
    }

    













    let cphsService  = GECKO_ICC_SERVICES.cphs[geckoService];

    if (!cphsService) {
      return false;
    }
    cphsService -= 1;
    let index = Math.floor(cphsService / 4);
    let bitmask = 2 << ((cphsService % 4) << 1);

    return (index < serviceTable.length) &&
           ((serviceTable[index] & bitmask) !== 0);
  },

  





  isGsm8BitAlphabet: function(str) {
    if (!str) {
      return false;
    }

    const langTable = PDU_NL_LOCKING_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];
    const langShiftTable = PDU_NL_SINGLE_SHIFT_TABLES[PDU_NL_IDENTIFIER_DEFAULT];

    for (let i = 0; i < str.length; i++) {
      let c = str.charAt(i);
      let octet = langTable.indexOf(c);
      if (octet == -1) {
        octet = langShiftTable.indexOf(c);
        if (octet == -1) {
          return false;
        }
      }
    }

    return true;
  },

  












  parseMccMncFromImsi: function(imsi, mncLength) {
    if (!imsi) {
      return null;
    }

    
    let mcc = imsi.substr(0,3);
    if (!mncLength) {
      
      
      if (PLMN_HAVING_3DIGITS_MNC[mcc] &&
          PLMN_HAVING_3DIGITS_MNC[mcc].indexOf(imsi.substr(3, 3)) !== -1) {
        mncLength = 3;
      } else {
        
        let index = MCC_TABLE_FOR_MNC_LENGTH_IS_3.indexOf(mcc);
        mncLength = (index !== -1) ? 3 : 2;
      }
    }
    let mnc = imsi.substr(3, mncLength);
    if (DEBUG) {
      this.context.debug("IMSI: " + imsi + " MCC: " + mcc + " MNC: " + mnc);
    }

    return { mcc: mcc, mnc: mnc};
  },
};




function ICCContactHelperObject(aContext) {
  this.context = aContext;
}
ICCContactHelperObject.prototype = {
  context: null,

  


  hasDfPhoneBook: function(appType) {
    switch (appType) {
      case CARD_APPTYPE_SIM:
        return false;
      case CARD_APPTYPE_USIM:
        return true;
      case CARD_APPTYPE_RUIM:
        let ICCUtilsHelper = this.context.ICCUtilsHelper;
        return ICCUtilsHelper.isICCServiceAvailable("ENHANCED_PHONEBOOK");
      default:
        return false;
    }
  },

  







  readICCContacts: function(appType, contactType, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;
    let ICCUtilsHelper = this.context.ICCUtilsHelper;

    switch (contactType) {
      case GECKO_CARDCONTACT_TYPE_ADN:
        if (!this.hasDfPhoneBook(appType)) {
          ICCRecordHelper.readADNLike(ICC_EF_ADN, onsuccess, onerror);
        } else {
          this.readUSimContacts(onsuccess, onerror);
        }
        break;
      case GECKO_CARDCONTACT_TYPE_FDN:
        if (!ICCUtilsHelper.isICCServiceAvailable("FDN")) {
          onerror(CONTACT_ERR_CONTACT_TYPE_NOT_SUPPORTED);
          break;
        }
        ICCRecordHelper.readADNLike(ICC_EF_FDN, onsuccess, onerror);
        break;
      case GECKO_CARDCONTACT_TYPE_SDN:
        if (!ICCUtilsHelper.isICCServiceAvailable("SDN")) {
          onerror(CONTACT_ERR_CONTACT_TYPE_NOT_SUPPORTED);
          break;
        }

        ICCRecordHelper.readADNLike(ICC_EF_SDN, onsuccess, onerror);
        break;
      default:
        if (DEBUG) {
          this.context.debug("Unsupported contactType :" + contactType);
        }
        onerror(CONTACT_ERR_CONTACT_TYPE_NOT_SUPPORTED);
        break;
    }
  },

  







  findFreeICCContact: function(appType, contactType, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;

    switch (contactType) {
      case GECKO_CARDCONTACT_TYPE_ADN:
        if (!this.hasDfPhoneBook(appType)) {
          ICCRecordHelper.findFreeRecordId(ICC_EF_ADN, onsuccess.bind(null, 0), onerror);
        } else {
          let gotPbrCb = function gotPbrCb(pbrs) {
            this.findUSimFreeADNRecordId(pbrs, onsuccess, onerror);
          }.bind(this);

          ICCRecordHelper.readPBR(gotPbrCb, onerror);
        }
        break;
      case GECKO_CARDCONTACT_TYPE_FDN:
        ICCRecordHelper.findFreeRecordId(ICC_EF_FDN, onsuccess.bind(null, 0), onerror);
        break;
      default:
        if (DEBUG) {
          this.context.debug("Unsupported contactType :" + contactType);
        }
        onerror(CONTACT_ERR_CONTACT_TYPE_NOT_SUPPORTED);
        break;
    }
  },

  


  _freePbrIndex: 0,

   






  findUSimFreeADNRecordId: function(pbrs, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;

    function callback(pbrIndex, recordId) {
      
      this._freePbrIndex = pbrIndex;
      onsuccess(pbrIndex, recordId);
    }

    let nextPbrIndex = -1;
    (function findFreeRecordId(pbrIndex) {
      if (nextPbrIndex === this._freePbrIndex) {
        
        this._freePbrIndex = 0;
        if (DEBUG) {
          this.context.debug(CONTACT_ERR_NO_FREE_RECORD_FOUND);
        }
        onerror(CONTACT_ERR_NO_FREE_RECORD_FOUND);
        return;
      }

      let pbr = pbrs[pbrIndex];
      nextPbrIndex = (pbrIndex + 1) % pbrs.length;
      ICCRecordHelper.findFreeRecordId(
        pbr.adn.fileId,
        callback.bind(this, pbrIndex),
        findFreeRecordId.bind(this, nextPbrIndex));
    }).call(this, this._freePbrIndex);
  },

  









  addICCContact: function(appType, contactType, contact, pin2, onsuccess, onerror) {
    let foundFreeCb = (function foundFreeCb(pbrIndex, recordId) {
      contact.pbrIndex = pbrIndex;
      contact.recordId = recordId;
      this.updateICCContact(appType, contactType, contact, pin2, onsuccess, onerror);
    }).bind(this);

    
    this.findFreeICCContact(appType, contactType, foundFreeCb, onerror);
  },

  









  updateICCContact: function(appType, contactType, contact, pin2, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;
    let ICCUtilsHelper = this.context.ICCUtilsHelper;

    switch (contactType) {
      case GECKO_CARDCONTACT_TYPE_ADN:
        if (!this.hasDfPhoneBook(appType)) {
          ICCRecordHelper.updateADNLike(ICC_EF_ADN, contact, null, onsuccess, onerror);
        } else {
          this.updateUSimContact(contact, onsuccess, onerror);
        }
        break;
      case GECKO_CARDCONTACT_TYPE_FDN:
        if (!pin2) {
          onerror(GECKO_ERROR_SIM_PIN2);
          return;
        }
        if (!ICCUtilsHelper.isICCServiceAvailable("FDN")) {
          onerror(CONTACT_ERR_CONTACT_TYPE_NOT_SUPPORTED);
          break;
        }
        ICCRecordHelper.updateADNLike(ICC_EF_FDN, contact, pin2, onsuccess, onerror);
        break;
      default:
        if (DEBUG) {
          this.context.debug("Unsupported contactType :" + contactType);
        }
        onerror(CONTACT_ERR_CONTACT_TYPE_NOT_SUPPORTED);
        break;
    }
  },

  





  readUSimContacts: function(onsuccess, onerror) {
    let gotPbrCb = function gotPbrCb(pbrs) {
      this.readAllPhonebookSets(pbrs, onsuccess, onerror);
    }.bind(this);

    this.context.ICCRecordHelper.readPBR(gotPbrCb, onerror);
  },

  






  readAllPhonebookSets: function(pbrs, onsuccess, onerror) {
    let allContacts = [], pbrIndex = 0;
    let readPhonebook = function(contacts) {
      if (contacts) {
        allContacts = allContacts.concat(contacts);
      }

      let cLen = contacts ? contacts.length : 0;
      for (let i = 0; i < cLen; i++) {
        contacts[i].pbrIndex = pbrIndex;
      }

      pbrIndex++;
      if (pbrIndex >= pbrs.length) {
        if (onsuccess) {
          onsuccess(allContacts);
        }
        return;
      }

      this.readPhonebookSet(pbrs[pbrIndex], readPhonebook, onerror);
    }.bind(this);

    this.readPhonebookSet(pbrs[pbrIndex], readPhonebook, onerror);
  },

  






  readPhonebookSet: function(pbr, onsuccess, onerror) {
    let gotAdnCb = function gotAdnCb(contacts) {
      this.readSupportedPBRFields(pbr, contacts, onsuccess, onerror);
    }.bind(this);

    this.context.ICCRecordHelper.readADNLike(pbr.adn.fileId, gotAdnCb, onerror);
  },

  







  readSupportedPBRFields: function(pbr, contacts, onsuccess, onerror) {
    let fieldIndex = 0;
    (function readField() {
      let field = USIM_PBR_FIELDS[fieldIndex];
      fieldIndex += 1;
      if (!field) {
        if (onsuccess) {
          onsuccess(contacts);
        }
        return;
      }

      this.readPhonebookField(pbr, contacts, field, readField.bind(this), onerror);
    }).call(this);
  },

  








  readPhonebookField: function(pbr, contacts, field, onsuccess, onerror) {
    if (!pbr[field]) {
      if (onsuccess) {
        onsuccess(contacts);
      }
      return;
    }

    (function doReadContactField(n) {
      if (n >= contacts.length) {
        
        if (onsuccess) {
          onsuccess(contacts);
        }
        return;
      }

      
      this.readContactField(pbr, contacts[n], field,
                            doReadContactField.bind(this, n + 1), onerror);
    }).call(this, 0);
  },

  








  readContactField: function(pbr, contact, field, onsuccess, onerror) {
    let gotRecordIdCb = function gotRecordIdCb(recordId) {
      if (recordId == 0xff) {
        if (onsuccess) {
          onsuccess();
        }
        return;
      }

      let fileId = pbr[field].fileId;
      let fileType = pbr[field].fileType;
      let gotFieldCb = function gotFieldCb(value) {
        if (value) {
          
          if (field.startsWith(USIM_PBR_ANR)) {
            if (!contact[USIM_PBR_ANR]) {
              contact[USIM_PBR_ANR] = [];
            }
            contact[USIM_PBR_ANR].push(value);
          } else {
            contact[field] = value;
          }
        }

        if (onsuccess) {
          onsuccess();
        }
      }.bind(this);

      let ICCRecordHelper = this.context.ICCRecordHelper;
      
      let ef = field.startsWith(USIM_PBR_ANR) ? USIM_PBR_ANR : field;
      switch (ef) {
        case USIM_PBR_EMAIL:
          ICCRecordHelper.readEmail(fileId, fileType, recordId, gotFieldCb, onerror);
          break;
        case USIM_PBR_ANR:
          ICCRecordHelper.readANR(fileId, fileType, recordId, gotFieldCb, onerror);
          break;
        default:
          if (DEBUG) {
            this.context.debug("Unsupported field :" + field);
          }
          onerror(CONTACT_ERR_FIELD_NOT_SUPPORTED);
          break;
      }
    }.bind(this);

    this.getContactFieldRecordId(pbr, contact, field, gotRecordIdCb, onerror);
  },

  












  getContactFieldRecordId: function(pbr, contact, field, onsuccess, onerror) {
    if (pbr[field].fileType == ICC_USIM_TYPE1_TAG) {
      
      if (onsuccess) {
        onsuccess(contact.recordId);
      }
    } else if (pbr[field].fileType == ICC_USIM_TYPE2_TAG) {
      
      let gotIapCb = function gotIapCb(iap) {
        let indexInIAP = pbr[field].indexInIAP;
        let recordId = iap[indexInIAP];

        if (onsuccess) {
          onsuccess(recordId);
        }
      }.bind(this);

      this.context.ICCRecordHelper.readIAP(pbr.iap.fileId, contact.recordId,
                                           gotIapCb, onerror);
    } else {
      if (DEBUG) {
        this.context.debug("USIM PBR files in Type 3 format are not supported.");
      }
      onerror(CONTACT_ERR_REQUEST_NOT_SUPPORTED);
    }
  },

  






  updateUSimContact: function(contact, onsuccess, onerror) {
    let gotPbrCb = function gotPbrCb(pbrs) {
      let pbr = pbrs[contact.pbrIndex];
      if (!pbr) {
        if (DEBUG) {
          this.context.debug(CONTACT_ERR_CANNOT_ACCESS_PHONEBOOK);
        }
        onerror(CONTACT_ERR_CANNOT_ACCESS_PHONEBOOK);
        return;
      }
      this.updatePhonebookSet(pbr, contact, onsuccess, onerror);
    }.bind(this);

    this.context.ICCRecordHelper.readPBR(gotPbrCb, onerror);
  },

  






  updatePhonebookSet: function(pbr, contact, onsuccess, onerror) {
    let updateAdnCb = function() {
      this.updateSupportedPBRFields(pbr, contact, onsuccess, onerror);
    }.bind(this);

    this.context.ICCRecordHelper.updateADNLike(pbr.adn.fileId, contact, null,
                                               updateAdnCb, onerror);
  },

  







  updateSupportedPBRFields: function(pbr, contact, onsuccess, onerror) {
    let fieldIndex = 0;
    (function updateField() {
      let field = USIM_PBR_FIELDS[fieldIndex];
      fieldIndex += 1;
      if (!field) {
        if (onsuccess) {
          onsuccess();
        }
        return;
      }

      
      if (!pbr[field]) {
        updateField.call(this);
        return;
      }

      this.updateContactField(pbr, contact, field, updateField.bind(this), onerror);
    }).call(this);
  },

  








  updateContactField: function(pbr, contact, field, onsuccess, onerror) {
    if (pbr[field].fileType === ICC_USIM_TYPE1_TAG) {
      this.updateContactFieldType1(pbr, contact, field, onsuccess, onerror);
    } else if (pbr[field].fileType === ICC_USIM_TYPE2_TAG) {
      this.updateContactFieldType2(pbr, contact, field, onsuccess, onerror);
    } else {
      if (DEBUG) {
        this.context.debug("USIM PBR files in Type 3 format are not supported.");
      }
      onerror(CONTACT_ERR_REQUEST_NOT_SUPPORTED);
    }
  },

  








  updateContactFieldType1: function(pbr, contact, field, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;

    if (field === USIM_PBR_EMAIL) {
      ICCRecordHelper.updateEmail(pbr, contact.recordId, contact.email, null, onsuccess, onerror);
    } else if (field === USIM_PBR_ANR0) {
      let anr = Array.isArray(contact.anr) ? contact.anr[0] : null;
      ICCRecordHelper.updateANR(pbr, contact.recordId, anr, null, onsuccess, onerror);
    } else {
     if (DEBUG) {
       this.context.debug("Unsupported field :" + field);
     }
     onerror(CONTACT_ERR_FIELD_NOT_SUPPORTED);
    }
  },

  








  updateContactFieldType2: function(pbr, contact, field, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;

    
    
    
    
    
    
    

    let gotIapCb = function gotIapCb(iap) {
      let recordId = iap[pbr[field].indexInIAP];
      if (recordId === 0xff) {
        
        
        
        
        if ((field === USIM_PBR_EMAIL && contact.email) ||
            (field === USIM_PBR_ANR0 &&
             (Array.isArray(contact.anr) && contact.anr[0]))) {
          
          this.addContactFieldType2(pbr, contact, field, onsuccess, onerror);
        } else {
          if (onsuccess) {
            onsuccess();
          }
        }
        return;
      }

      
      if (field === USIM_PBR_EMAIL) {
        ICCRecordHelper.updateEmail(pbr, recordId, contact.email, contact.recordId, onsuccess, onerror);
      } else if (field === USIM_PBR_ANR0) {
        let anr = Array.isArray(contact.anr) ? contact.anr[0] : null;
        ICCRecordHelper.updateANR(pbr, recordId, anr, contact.recordId, onsuccess, onerror);
      } else {
        if (DEBUG) {
          this.context.debug("Unsupported field :" + field);
        }
        onerror(CONTACT_ERR_FIELD_NOT_SUPPORTED);
      }

    }.bind(this);

    ICCRecordHelper.readIAP(pbr.iap.fileId, contact.recordId, gotIapCb, onerror);
  },

  








  addContactFieldType2: function(pbr, contact, field, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;

    let successCb = function successCb(recordId) {
      let updateCb = function updateCb() {
        this.updateContactFieldIndexInIAP(pbr, contact.recordId, field, recordId, onsuccess, onerror);
      }.bind(this);

      if (field === USIM_PBR_EMAIL) {
        ICCRecordHelper.updateEmail(pbr, recordId, contact.email, contact.recordId, updateCb, onerror);
      } else if (field === USIM_PBR_ANR0) {
        ICCRecordHelper.updateANR(pbr, recordId, contact.anr[0], contact.recordId, updateCb, onerror);
      }
    }.bind(this);

    let errorCb = function errorCb(errorMsg) {
      if (DEBUG) {
        this.context.debug(errorMsg + " USIM field " + field);
      }
      onerror(errorMsg);
    }.bind(this);

    ICCRecordHelper.findFreeRecordId(pbr[field].fileId, successCb, errorCb);
  },

  










  updateContactFieldIndexInIAP: function(pbr, recordNumber, field, value, onsuccess, onerror) {
    let ICCRecordHelper = this.context.ICCRecordHelper;

    let gotIAPCb = function gotIAPCb(iap) {
      iap[pbr[field].indexInIAP] = value;
      ICCRecordHelper.updateIAP(pbr.iap.fileId, recordNumber, iap, onsuccess, onerror);
    }.bind(this);
    ICCRecordHelper.readIAP(pbr.iap.fileId, recordNumber, gotIAPCb, onerror);
  },
};

function IconLoaderObject(aContext) {
  this.context = aContext;
}
IconLoaderObject.prototype = {
  context: null,

  






  loadIcons: function(recordNumbers, onsuccess, onerror) {
    if (!recordNumbers || !recordNumbers.length) {
      if (onerror) {
        onerror();
      }
      return;
    }

    this._start({
      recordNumbers: recordNumbers,
      onsuccess: onsuccess,
      onerror: onerror});
  },

  _start: function(options) {
    let callback = (function(icons) {
      if (!options.icons) {
        options.icons = [];
      }
      for (let i = 0; i < icons.length; i++) {
        icons[i] = this._parseRawData(icons[i]);
      }
      options.icons[options.currentRecordIndex] = icons;
      options.currentRecordIndex++;

      let recordNumbers = options.recordNumbers;
      if (options.currentRecordIndex < recordNumbers.length) {
        let recordNumber = recordNumbers[options.currentRecordIndex];
        this.context.SimRecordHelper.readIMG(recordNumber,
                                             callback,
                                             options.onerror);
      } else {
        if (options.onsuccess) {
          options.onsuccess(options.icons);
        }
      }
    }).bind(this);

    options.currentRecordIndex = 0;
    this.context.SimRecordHelper.readIMG(options.recordNumbers[0],
                                         callback,
                                         options.onerror);
  },

  _parseRawData: function(rawData) {
    let codingScheme = rawData.codingScheme;

    switch (codingScheme) {
      case ICC_IMG_CODING_SCHEME_BASIC:
        return this._decodeBasicImage(rawData.width, rawData.height, rawData.body);

      case ICC_IMG_CODING_SCHEME_COLOR:
      case ICC_IMG_CODING_SCHEME_COLOR_TRANSPARENCY:
        return this._decodeColorImage(codingScheme,
                                      rawData.width, rawData.height,
                                      rawData.bitsPerImgPoint,
                                      rawData.numOfClutEntries,
                                      rawData.clut, rawData.body);
    }

    return null;
  },

  _decodeBasicImage: function(width, height, body) {
    let numOfPixels = width * height;
    let pixelIndex = 0;
    let currentByteIndex = 0;
    let currentByte = 0x00;

    const BLACK = 0x000000FF;
    const WHITE = 0xFFFFFFFF;

    let pixels = [];
    while (pixelIndex < numOfPixels) {
      
      if (pixelIndex % 8 == 0) {
        currentByte = body[currentByteIndex++];
      }
      let bit = (currentByte >> (7 - (pixelIndex % 8))) & 0x01;
      pixels[pixelIndex++] = bit ? WHITE : BLACK;
    }

    return {pixels: pixels,
            codingScheme: GECKO_IMG_CODING_SCHEME_BASIC,
            width: width,
            height: height};
  },

  _decodeColorImage: function(codingScheme, width, height, bitsPerImgPoint,
                              numOfClutEntries, clut, body) {
    let mask = 0xff >> (8 - bitsPerImgPoint);
    let bitsStartOffset = 8 - bitsPerImgPoint;
    let bitIndex = bitsStartOffset;
    let numOfPixels = width * height;
    let pixelIndex = 0;
    let currentByteIndex = 0;
    let currentByte = body[currentByteIndex++];

    let pixels = [];
    while (pixelIndex < numOfPixels) {
      
      if (bitIndex < 0) {
        currentByte = body[currentByteIndex++];
        bitIndex = bitsStartOffset;
      }
      let clutEntry = ((currentByte >> bitIndex) & mask);
      let clutIndex = clutEntry * ICC_CLUT_ENTRY_SIZE;
      let alpha = codingScheme == ICC_IMG_CODING_SCHEME_COLOR_TRANSPARENCY &&
                  clutEntry == numOfClutEntries - 1;
      pixels[pixelIndex++] = alpha ? 0x00
                                   : (clut[clutIndex] << 24 |
                                      clut[clutIndex + 1] << 16 |
                                      clut[clutIndex + 2] << 8 |
                                      0xFF) >>> 0;
      bitIndex -= bitsPerImgPoint;
    }

    return {pixels: pixels,
            codingScheme: ICC_IMG_CODING_SCHEME_TO_GECKO[codingScheme],
            width: width,
            height: height};
  },
};





function Context(aClientId) {
  this.clientId = aClientId;

  this.Buf = new BufObject(this);
  this.RIL = new RilObject(this);
  this.RIL.initRILState();
}
Context.prototype = {
  clientId: null,
  Buf: null,
  RIL: null,

  debug: function(aMessage) {
    GLOBAL.debug("[" + this.clientId + "] " + aMessage);
  }
};

(function() {
  let lazySymbols = [
    "BerTlvHelper", "BitBufferHelper", "CdmaPDUHelper",
    "ComprehensionTlvHelper", "GsmPDUHelper", "ICCContactHelper",
    "ICCFileHelper", "ICCIOHelper", "ICCPDUHelper", "ICCRecordHelper",
    "ICCUtilsHelper", "RuimRecordHelper", "SimRecordHelper",
    "StkCommandParamsFactory", "StkProactiveCmdHelper", "IconLoader",
  ];

  for (let i = 0; i < lazySymbols.length; i++) {
    let symbol = lazySymbols[i];
    Object.defineProperty(Context.prototype, symbol, {
      get: function() {
        let real = new GLOBAL[symbol + "Object"](this);
        Object.defineProperty(this, symbol, {
          value: real,
          enumerable: true
        });
        return real;
      },
      configurable: true,
      enumerable: true
    });
  }
})();

let ContextPool = {
  _contexts: [],

  handleRilMessage: function(aClientId, aUint8Array) {
    let context = this._contexts[aClientId];
    context.Buf.processIncoming(aUint8Array);
  },

  handleChromeMessage: function(aMessage) {
    let clientId = aMessage.rilMessageClientId;
    if (clientId != null) {
      let context = this._contexts[clientId];
      context.RIL.handleChromeMessage(aMessage);
      return;
    }

    if (DEBUG) debug("Received global chrome message " + JSON.stringify(aMessage));
    let method = this[aMessage.rilMessageType];
    if (typeof method != "function") {
      if (DEBUG) {
        debug("Don't know what to do");
      }
      return;
    }
    method.call(this, aMessage);
  },

  setInitialOptions: function(aOptions) {
    DEBUG = DEBUG_WORKER || aOptions.debug;

    let quirks = aOptions.quirks;
    RILQUIRKS_CALLSTATE_EXTRA_UINT32 = quirks.callstateExtraUint32;
    RILQUIRKS_V5_LEGACY = quirks.v5Legacy;
    RILQUIRKS_REQUEST_USE_DIAL_EMERGENCY_CALL = quirks.requestUseDialEmergencyCall;
    RILQUIRKS_SIM_APP_STATE_EXTRA_FIELDS = quirks.simAppStateExtraFields;
    RILQUIRKS_EXTRA_UINT32_2ND_CALL = quirks.extraUint2ndCall;
    RILQUIRKS_HAVE_QUERY_ICC_LOCK_RETRY_COUNT = quirks.haveQueryIccLockRetryCount;
    RILQUIRKS_SEND_STK_PROFILE_DOWNLOAD = quirks.sendStkProfileDownload;
    RILQUIRKS_DATA_REGISTRATION_ON_DEMAND = quirks.dataRegistrationOnDemand;
    RILQUIRKS_SUBSCRIPTION_CONTROL = quirks.subscriptionControl;
    RILQUIRKS_SIGNAL_EXTRA_INT32 = quirks.signalExtraInt;
    RILQUIRKS_SMSC_ADDRESS_FORMAT = quirks.smscAddressFormat;
  },

  setDebugFlag: function(aOptions) {
    DEBUG = DEBUG_WORKER || aOptions.debug;
  },

  registerClient: function(aOptions) {
    let clientId = aOptions.clientId;
    this._contexts[clientId] = new Context(clientId);
  },
};

function onRILMessage(aClientId, aUint8Array) {
  ContextPool.handleRilMessage(aClientId, aUint8Array);
}

onmessage = function onmessage(event) {
  ContextPool.handleChromeMessage(event.data);
};

onerror = function onerror(event) {
  if (DEBUG) debug("onerror" + event.message + "\n");
};
