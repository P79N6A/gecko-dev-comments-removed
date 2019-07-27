

















this.DEBUG_ALL = false;


this.DEBUG_CONNECTOR = DEBUG_ALL || false;
this.DEBUG_SE = DEBUG_ALL || false ;






this.MAX_CHANNELS_ALLOWED_PER_SESSION = 4;

this.BASIC_CHANNEL = 0;


this.MAX_APDU_LEN = 255; 


this.APDU_HEADER_LEN = 4;

this.LOGICAL_CHANNEL_NUMBER_LIMIT = 4;
this.SUPPLEMENTARY_LOGICAL_CHANNEL_NUMBER_LIMIT = 20;

this.MIN_AID_LEN = 5;
this.MAX_AID_LEN = 16;

this.CLA_GET_RESPONSE = 0x00;

this.INS_SELECT = 0xA4;
this.INS_MANAGE_CHANNEL = 0x70;
this.INS_GET_RESPONSE = 0xC0;


this.ERROR_NONE               = "";
this.ERROR_SECURITY           = "SESecurityError";
this.ERROR_IO                 = "SEIoError";
this.ERROR_BADSTATE           = "SEBadStateError";
this.ERROR_INVALIDCHANNEL     = "SEInvalidChannelError";
this.ERROR_INVALIDAPPLICATION = "SEInvalidApplicationError";
this.ERROR_GENERIC            = "SEGenericError";
this.ERROR_NOTPRESENT         = "SENotPresentError";

this.TYPE_UICC = "uicc";
this.TYPE_ESE = "eSE";


this.EXPORTED_SYMBOLS = Object.keys(this);
