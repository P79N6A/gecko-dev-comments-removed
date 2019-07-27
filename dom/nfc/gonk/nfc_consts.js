

















this.DEBUG_ALL = false;


this.DEBUG_CONTENT_HELPER = DEBUG_ALL || false;
this.DEBUG_NFC = DEBUG_ALL || false;


this.NFC_GECKO_ERROR_P2P_REG_INVALID = 1;
this.NFC_GECKO_ERROR_SEND_FILE_FAILED = 2;

this.NFC_ERROR_MSG = {};
this.NFC_ERROR_MSG[this.NFC_GECKO_ERROR_P2P_REG_INVALID] = "NfcP2PRegistrationInvalid";
this.NFC_ERROR_MSG[this.NFC_GECKO_ERROR_SEND_FILE_FAILED] = "NfcSendFileFailed";

this.NFC_RF_STATE_IDLE = "idle";
this.NFC_RF_STATE_LISTEN = "listen";
this.NFC_RF_STATE_DISCOVERY = "discovery";

this.TOPIC_MOZSETTINGS_CHANGED      = "mozsettings-changed";
this.TOPIC_XPCOM_SHUTDOWN           = "xpcom-shutdown";

this.SETTING_NFC_DEBUG = "nfc.debugging.enabled";

this.PEER_EVENT_READY = 0x01;
this.PEER_EVENT_LOST  = 0x02;
this.TAG_EVENT_FOUND = 0x03;
this.TAG_EVENT_LOST  = 0x04;
this.PEER_EVENT_FOUND = 0x05;
this.RF_EVENT_STATE_CHANGE = 0x06;


this.EXPORTED_SYMBOLS = Object.keys(this);
