



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let WSP = {};
Cu.import("resource://gre/modules/WspPduHelper.jsm", WSP);

Cu.import("resource://gre/modules/mms_consts.js");

let DEBUG; 

function translatePduErrorToStatus(error) {
  switch (error) {
    case MMS_PDU_ERROR_OK:
      return MMS_PDU_STATUS_RETRIEVED;
    case MMS_PDU_ERROR_TRANSIENT_FAILURE:
    case MMS_PDU_ERROR_TRANSIENT_MESSAGE_NOT_FOUND:
    case MMS_PDU_ERROR_TRANSIENT_NETWORK_PROBLEM:
      return MMS_PDU_STATUS_DEFERRED;
    default:
      return MMS_PDU_STATUS_UNRECOGNISED;
  }
}








let BooleanValue = {
  







  decode: function decode(data) {
    let value = WSP.Octet.decode(data);
    if ((value != 128) && (value != 129)) {
      throw new WSP.CodeError("Boolean-value: invalid value " + value);
    }

    return value == 128;
  },

  





  encode: function encode(data, value) {
    WSP.Octet.encode(data, value ? 128 : 129);
  },
};








let Address = {
  





  decode: function decode(data) {
    let str = EncodedStringValue.decode(data);

    let result;
    if (((result = str.match(/^(\+?[\d.-]+)\/TYPE=(PLMN)$/)) != null)
        || ((result = str.match(/^(\d{1,3}(?:\.\d{1,3}){3})\/TYPE=(IPv4)$/)) != null)
        || ((result = str.match(/^([\da-fA-F]{4}(?::[\da-fA-F]{4}){7})\/TYPE=(IPv6)$/)) != null)
        || ((result = str.match(/^([\w\+\-.%]+)\/TYPE=(\w+)$/)) != null)) {
      return {address: result[1], type: result[2]};
    }

    let type;
    if (str.match(/^[\+*#]\d+$/)) {
      type = "num";
    } else if (str.match(/^\w+$/)) {
      type = "alphanum";
    } else {
      type = "unknown";
    }

    return {address: str, type: type};
  },
};






let HeaderField = {
  










  decode: function decode(data, options) {
    return WSP.decodeAlternatives(data, options,
                                  MmsHeader, WSP.ApplicationHeader);
  },

  







  encode: function encode(data, value, options) {
    WSP.encodeAlternatives(data, value, options,
                           MmsHeader, WSP.ApplicationHeader);
  },
};







let MmsHeader = {
  













  decode: function decode(data, options) {
    let index = WSP.ShortInteger.decode(data);

    let entry = MMS_HEADER_FIELDS[index];
    if (!entry) {
      throw new WSP.NotWellKnownEncodingError(
        "MMS-header: not well known header " + index);
    }

    let cur = data.offset, value;
    try {
      value = entry.coder.decode(data, options);
    } catch (e) {
      data.offset = cur;

      value = WSP.skipValue(data);
      debug("Skip malformed well known header: "
            + JSON.stringify({name: entry.name, value: value}));

      return null;
    }

    return {
      name: entry.name,
      value: value,
    };
  },

  










  encode: function encode(data, header) {
    if (!header.name) {
      throw new WSP.CodeError("MMS-header: empty header name");
    }

    let entry = MMS_HEADER_FIELDS[header.name.toLowerCase()];
    if (!entry) {
      throw new WSP.NotWellKnownEncodingError(
        "MMS-header: not well known header " + header.name);
    }

    WSP.ShortInteger.encode(data, entry.number);
    entry.coder.encode(data, header.value);
  },
};







let ContentClassValue = {
  







  decode: function decode(data) {
    let value = WSP.Octet.decode(data);
    if ((value >= 128) && (value <= 135)) {
      return value;
    }

    throw new WSP.CodeError("Content-class-value: invalid class " + value);
  },
};













let ContentLocationValue = {
  








  decode: function decode(data, options) {
    let type = WSP.ensureHeader(options, "x-mms-message-type");

    let result = {};
    if ((type == MMS_PDU_TYPE_MBOX_DELETE_CONF)
        || (type == MMS_PDU_TYPE_DELETE_CONF)) {
      let length = WSP.ValueLength.decode(data);
      let end = data.offset + length;

      result.statusCount = WSP.IntegerValue.decode(data);
      result.uri = WSP.UriValue.decode(data);

      if (data.offset != end) {
        data.offset = end;
      }
    } else {
      result.uri = WSP.UriValue.decode(data);
    }

    return result;
  },
};







let ElementDescriptorValue = {
  






  decode: function decode(data) {
    let length = WSP.ValueLength.decode(data);
    let end = data.offset + length;

    let result = {};
    result.contentReference = WSP.TextString.decode(data);
    if (data.offset < end) {
      result.params = Parameter.decodeMultiple(data, end);
    }

    if (data.offset != end) {
      
      data.offset = end;
    }

    return result;
  },
};












let Parameter = {
  








  decodeParameterName: function decodeParameterName(data) {
    let begin = data.offset;
    let number;
    try {
      number = WSP.ShortInteger.decode(data);
    } catch (e) {
      data.offset = begin;
      return WSP.TextString.decode(data).toLowerCase();
    }

    let entry = MMS_WELL_KNOWN_PARAMS[number];
    if (!entry) {
      throw new WSP.NotWellKnownEncodingError(
        "Parameter-name: not well known parameter " + number);
    }

    return entry.name;
  },

  








  decode: function decode(data) {
    let name = this.decodeParameterName(data);
    let value = WSP.decodeAlternatives(data, null,
                                       WSP.ConstrainedEncoding, WSP.TextString);
    return {
      name: name,
      value: value,
    };
  },

  







  decodeMultiple: function decodeMultiple(data, end) {
    let params, param;

    while (data.offset < end) {
      try {
        param = this.decode(data);
      } catch (e) {
        break;
      }
      if (param) {
        if (!params) {
          params = {};
        }
        params[param.name] = param.value;
      }
    }

    return params;
  },
};










let EncodedStringValue = {
  









  decodeCharsetEncodedString: function decodeCharsetEncodedString(data) {
    let length = WSP.ValueLength.decode(data);
    let end = data.offset + length;

    let charset = WSP.IntegerValue.decode(data);
    let entry = WSP.WSP_WELL_KNOWN_CHARSETS[charset];
    if (!entry) {
      throw new WSP.NotWellKnownEncodingError(
        "Charset-encoded-string: not well known charset " + charset);
    }

    let str;
    if (entry.converter) {
      
      let begin = data.offset;
      if (WSP.Octet.decode(data) != 127) {
        data.offset = begin;
      }

      let raw = WSP.Octet.decodeMultiple(data, end - 1);
      
      WSP.Octet.decodeEqualTo(data, 0);

      if (!raw) {
        str = "";
      } else {
        let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                   .createInstance(Ci.nsIScriptableUnicodeConverter);
        conv.charset = entry.converter;
        try {
          str = conv.convertFromByteArray(raw, raw.length);
        } catch (e) {
          throw new WSP.CodeError("Charset-encoded-string: " + e.message);
        }
      }
    } else {
      str = WSP.TextString.decode(data);
    }

    if (data.offset != end) {
      data.offset = end;
    }

    return str;
  },

  





  decode: function decode(data) {
    let begin = data.offset;
    try {
      return WSP.TextString.decode(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeCharsetEncodedString(data);
    }
  },
};








let ExpiryValue = {
  







  decode: function decode(data) {
    let length = WSP.ValueLength.decode(data);
    let end = data.offset + length;

    let token = WSP.Octet.decode(data);
    if ((token != 128) && (token != 129)) {
      throw new WSP.CodeError("Expiry-value: invalid token " + token);
    }

    let result;
    if (token == 128) {
      result = WSP.DateValue.decode(data);
    } else {
      result = WSP.DeltaSecondsValue.decode(data);
    }

    if (data.offset != end) {
      data.offset = end;
    }

    return result;
  },
};








let FromValue = {
  








  decode: function decode(data) {
    let length = WSP.ValueLength.decode(data);
    let end = data.offset + length;

    let token = WSP.Octet.decode(data);
    if ((token != 128) && (token != 129)) {
      throw new WSP.CodeError("From-value: invalid token " + token);
    }

    let result = null;
    if (token == 128) {
      result = Address.decode(data);
    }

    if (data.offset != end) {
      data.offset = end;
    }

    return result;
  },
};







let PreviouslySentByValue = {
  






  decode: function decode(data) {
    let length = WSP.ValueLength.decode(data);
    let end = data.offset + length;

    let result = {};
    result.forwardedCount = WSP.IntegerValue.decode(data);
    result.originator = Address.decode(data);

    if (data.offset != end) {
      data.offset = end;
    }

    return result;
  },
};







let PreviouslySentDateValue = {
  






  decode: function decode(data) {
    let length = WSP.ValueLength.decode(data);
    let end = data.offset + length;

    let result = {};
    result.forwardedCount = WSP.IntegerValue.decode(data);
    result.timestamp = WSP.DateValue.decode(data);

    if (data.offset != end) {
      data.offset = end;
    }

    return result;
  },
};











let MessageClassValue = {
  







  decodeClassIdentifier: function decodeClassIdentifier(data) {
    let value = WSP.Octet.decode(data);
    switch (value) {
      case 128: return "personal";
      case 129: return "advertisement";
      case 130: return "informational";
      case 131: return "auto";
    }

    throw new WSP.CodeError("Class-identifier: invalid id " + value);
  },

  





  decode: function decode(data) {
    let begin = data.offset;
    try {
      return this.decodeClassIdentifier(data);
    } catch (e) {
      data.offset = begin;
      return WSP.TokenText.decode(data);
    }
  },
};

 




let MessageTypeValue = {
  







  decode: function decode(data) {
    let type = WSP.Octet.decode(data);
    if ((type >= 128) && (type <= 151)) {
      return type;
    }

    throw new WSP.CodeError("Message-type-value: invalid type " + type);
  },

  







  encode: function encode(data, type) {
    if ((type < 128) || (type > 151)) {
      throw new WSP.CodeError("Message-type-value: invalid type " + type);
    }

    WSP.Octet.encode(data, type);
  },
};









let MmFlagsValue = {
  








  decode: function decode(data) {
    let length = WSP.ValueLength.decode(data);
    let end = data.offset + length;

    let result = {};
    result.type = WSP.Octet.decode(data);
    if ((result.type < 128) || (result.type > 130)) {
      throw new WSP.CodeError("MM-flags-value: invalid type " + result.type);
    }
    result.text = EncodedStringValue.decode(data);

    if (data.offset != end) {
      data.offset = end;
    }

    return result;
  },
};











let MmStateValue = {
  







  decode: function decode(data) {
    let state = WSP.Octet.decode(data);
    if ((state >= 128) && (state <= 132)) {
      return state;
    }

    throw new WSP.CodeError("MM-state-value: invalid state " + state);
  },
};









let PriorityValue = {
  







  decode: function decode(data) {
    let priority = WSP.Octet.decode(data);
    if ((priority >= 128) && (priority <= 130)) {
      return priority;
    }

    throw new WSP.CodeError("Priority-value: invalid priority " + priority);
  },
};







let RecommendedRetrievalModeValue = {
  





  decode: function decode(data) {
    return WSP.Octet.decodeEqualTo(data, 128);
  },
};











let ReplyChargingValue = {
  







  decode: function decode(data) {
    let value = WSP.Octet.decode(data);
    if ((value >= 128) && (value <= 131)) {
      return value;
    }

    throw new WSP.CodeError("Reply-charging-value: invalid value " + value);
  },
};




















let RetrieveStatusValue = {
  





  decode: function decode(data) {
    let value = WSP.Octet.decode(data);
    if ((value == 128)
        || ((value >= 192) && (value <= 194))
        || ((value >= 224) && (value <= 227))) {
      return value;
    }

    if ((value >= 195) && (value <= 223)) {
      
      
      
      
      return MMS_PDU_ERROR_TRANSIENT_FAILURE;
    }

    
    
    
    

    
    
    
    return MMS_PDU_ERROR_PERMANENT_FAILURE;
  },
};















let StatusValue = {
  







  decode: function decode(data) {
    let status = WSP.Octet.decode(data);
    if ((status >= 128) && (status <= 135)) {
      return status;
    }

    throw new WSP.CodeError("Status-value: invalid status " + status);
  },

  







  encode: function encode(data, value) {
    if ((value < 128) || (value > 135)) {
      throw new WSP.CodeError("Status-value: invalid status " + value);
    }

    WSP.Octet.encode(data, value);
  },
};

let PduHelper = {
  








  parseHeaders: function parseHeaders(data, headers) {
    if (!headers) {
      headers = {};
    }

    let header;
    while (data.offset < data.array.length) {
      
      
      
      header = HeaderField.decode(data, headers);

      if (header) {
        let orig = headers[header.name];
        if (Array.isArray(orig)) {
          headers[header.name].push(header.value);
        } else if (orig) {
          headers[header.name] = [orig, header.value];
        } else {
          headers[header.name] = header.value;
        }
        if (header.name == "content-type") {
          
          
          
          break;
        }
      }
    }

    return headers;
  },

  





  parseContent: function parseContent(data, msg) {
    let contentType = msg.headers["content-type"].media;
    if ((contentType == "application/vnd.wap.multipart.related")
        || (contentType == "application/vnd.wap.multipart.mixed")) {
      msg.parts = WSP.PduHelper.parseMultiPart(data);
      return;
    }

    if (data.offset >= data.array.length) {
      return;
    }

    msg.content = WSP.Octet.decodeMultiple(data, data.array.length);
    if (false) {
      for (let begin = 0; begin < msg.content.length; begin += 20) {
        debug("content: " + JSON.stringify(msg.content.subarray(begin, begin + 20)));
      }
    }
  },

  










  checkMandatoryFields: function checkMandatoryFields(msg) {
    let type = WSP.ensureHeader(msg.headers, "x-mms-message-type");
    let entry = MMS_PDU_TYPES[type];
    if (!entry) {
      throw new WSP.FatalCodeError(
        "checkMandatoryFields: unsupported message type " + type);
    }

    entry.mandatoryFields.forEach(function (name) {
      WSP.ensureHeader(msg.headers, name);
    });

    
    msg.type = type;

    return entry;
  },

  







  parse: function parse(data, msg) {
    if (!msg) {
      msg = {};
    }

    try {
      msg.headers = this.parseHeaders(data, msg.headers);

      
      let typeinfo = this.checkMandatoryFields(msg);
      if (typeinfo.hasContent) {
        this.parseContent(data, msg);
      }
    } catch (e) {
      debug("Failed to parse MMS message, error message: " + e.message);
      return null;
    }

    return msg;
  },

  


  convertArrayToInputStream: function convertDataToInputStream(array) {
    let storageStream = Cc["@mozilla.org/storagestream;1"]
                        .createInstance(Ci.nsIStorageStream);
    storageStream.init(4096, array.length, null);

    let boStream = Cc["@mozilla.org/binaryoutputstream;1"]
                   .createInstance(Ci.nsIBinaryOutputStream);
    boStream.setOutputStream(storageStream.getOutputStream(0));
    boStream.writeByteArray(array, array.length)
    boStream.close();

    return storageStream.newInputStream(0);
  },

  







  encodeHeaders: function encodeHeaders(data, headers) {
    if (!data) {
      data = {array: [], offset: 0};
    }

    function encodeHeader(name) {
      HeaderField.encode(data, {name: name, value: headers[name]});
    }

    function encodeHeaderIfExists(name) {
      
      if (headers[name] !== undefined) {
        encodeHeader(name);
      }
    }

    
    
    
    
    
    
    encodeHeader("x-mms-message-type");
    encodeHeaderIfExists("x-mms-transaction-id");
    encodeHeaderIfExists("x-mms-mms-version");

    for (let key in headers) {
      if ((key == "x-mms-message-type")
          || (key == "x-mms-transaction-id")
          || (key == "x-mms-mms-version")
          || (key == "content-type")) {
        continue;
      }
      encodeHeader(key);
    }

    encodeHeaderIfExists("content-type");

    
    while (data.array.length > data.offset) {
      data.array.pop();
    }

    return data;
  },

  







  compose: function compose(multiStream, msg) {
    if (!multiStream) {
      multiStream = Cc["@mozilla.org/io/multiplex-input-stream;1"]
                    .createInstance(Ci.nsIMultiplexInputStream);
    }

    try {
      
      let typeinfo = this.checkMandatoryFields(msg);

      let data = this.encodeHeaders(null, msg.headers);
      debug("Composed PDU Header: " + JSON.stringify(data.array));
      let headerStream = this.convertArrayToInputStream(data.array);
      multiStream.appendStream(headerStream);

      return multiStream;
    } catch (e) {
      debug("Failed to compose MMS message, error message: " + e.message);
      return null;
    }
  },
};

const MMS_PDU_TYPES = (function () {
  let pdus = {};
  function add(number, hasContent, mandatoryFields) {
    pdus[number] = {
      number: number,
      hasContent: hasContent,
      mandatoryFields: mandatoryFields,
    };
  }

  add(MMS_PDU_TYPE_NOTIFICATION_IND, false, ["x-mms-message-type",
                                             "x-mms-transaction-id",
                                             "x-mms-mms-version",
                                             "x-mms-message-class",
                                             "x-mms-message-size",
                                             "x-mms-expiry",
                                             "x-mms-content-location"]);
  add(MMS_PDU_TYPE_RETRIEVE_CONF, true, ["x-mms-message-type",
                                         "x-mms-mms-version",
                                         "date",
                                         "content-type"]);
  add(MMS_PDU_TYPE_NOTIFYRESP_IND, false, ["x-mms-message-type",
                                           "x-mms-transaction-id",
                                           "x-mms-mms-version",
                                           "x-mms-status"]);

  return pdus;
})();






const MMS_HEADER_FIELDS = (function () {
  let names = {};
  function add(name, number, coder) {
    let entry = {
      name: name,
      number: number,
      coder: coder,
    };
    names[name] = names[number] = entry;
  }

  add("bcc",                                     0x01, Address);
  add("cc",                                      0x02, Address);
  add("x-mms-content-location",                  0x03, ContentLocationValue);
  add("content-type",                            0x04, WSP.ContentTypeValue);
  add("date",                                    0x05, WSP.DateValue);
  add("x-mms-delivery-report",                   0x06, BooleanValue);
  
  add("x-mms-expiry",                            0x08, ExpiryValue);
  add("from",                                    0x09, FromValue);
  add("x-mms-message-class",                     0x0A, MessageClassValue);
  add("message-id",                              0x0B, WSP.TextString);
  add("x-mms-message-type",                      0x0C, MessageTypeValue);
  add("x-mms-mms-version",                       0x0D, WSP.ShortInteger);
  add("x-mms-message-size",                      0x0E, WSP.LongInteger);
  add("x-mms-priority",                          0x0F, PriorityValue);
  add("x-mms-read-report",                       0x10, BooleanValue);
  add("x-mms-report-allowed",                    0x11, BooleanValue);
  
  
  
  add("x-mms-status",                            0x15, StatusValue);
  add("subject",                                 0x16, EncodedStringValue);
  add("to",                                      0x17, Address);
  add("x-mms-transaction-id",                    0x18, WSP.TextString);
  add("x-mms-retrieve-status",                   0x19, RetrieveStatusValue);
  add("x-mms-retrieve-text",                     0x1A, EncodedStringValue);
  
  add("x-mms-reply-charging",                    0x1C, ReplyChargingValue);
  add("x-mms-reply-charging-deadline",           0x1D, ExpiryValue);
  add("x-mms-reply-charging-id",                 0x1E, WSP.TextString);
  add("x-mms-reply-charging-size",               0x1F, WSP.LongInteger);
  add("x-mms-previously-sent-by",                0x20, PreviouslySentByValue);
  add("x-mms-previously-sent-date",              0x21, PreviouslySentDateValue);
  add("x-mms-store",                             0x22, BooleanValue);
  add("x-mms-mm-state",                          0x23, MmStateValue);
  add("x-mms-mm-flags",                          0x24, MmFlagsValue);
  
  
  add("x-mms-stored",                            0x27, BooleanValue);
  
  add("x-mms-totals",                            0x29, BooleanValue);
  
  add("x-mms-quotas",                            0x2B, BooleanValue);
  
  add("x-mms-message-count",                     0x2D, WSP.IntegerValue);
  
  add("x-mms-start",                             0x2F, WSP.IntegerValue);
  
  add("x-mms-distribution-indicator",            0x31, BooleanValue);
  add("x-mms-element-descriptor",                0x32, ElementDescriptorValue);
  add("x-mms-limit",                             0x33, WSP.IntegerValue);
  add("x-mms-recommended-retrieval-mode",        0x34, RecommendedRetrievalModeValue);
  add("x-mms-recommended-retrieval-mode-text",   0x35, EncodedStringValue);
  
  add("x-mms-applic-id",                         0x37, WSP.TextString);
  add("x-mms-reply-applic-id",                   0x38, WSP.TextString);
  add("x-mms-aux-applic-id",                     0x39, WSP.TextString);
  add("x-mms-content-class",                     0x3A, ContentClassValue);
  add("x-mms-drm-content",                       0x3B, BooleanValue);
  add("x-mms-adaptation-allowed",                0x3C, BooleanValue);
  add("x-mms-replace-id",                        0x3D, WSP.TextString);
  add("x-mms-cancel-id",                         0x3E, WSP.TextString);
  add("x-mms-cancel-status",                     0x3F, BooleanValue);

  return names;
})();


const MMS_WELL_KNOWN_PARAMS = (function () {
  let params = {};

  function add(name, number, coder) {
    let entry = {
      name: name,
      number: number,
      coder: coder,
    };
    params[name] = params[number] = entry;
  }

  add("type", 0x02, WSP.ConstrainedEncoding);

  return params;
})();

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-$- MmsPduHelper: " + s + "\n");
  };
} else {
  debug = function (s) {};
}

const EXPORTED_SYMBOLS = ALL_CONST_SYMBOLS.concat([
  
  "translatePduErrorToStatus",

  
  "BooleanValue",
  "Address",
  "HeaderField",
  "MmsHeader",
  "ContentClassValue",
  "ContentLocationValue",
  "ElementDescriptorValue",
  "Parameter",
  "EncodedStringValue",
  "ExpiryValue",
  "FromValue",
  "PreviouslySentByValue",
  "PreviouslySentDateValue",
  "MessageClassValue",
  "MessageTypeValue",
  "MmFlagsValue",
  "MmStateValue",
  "PriorityValue",
  "RecommendedRetrievalModeValue",
  "ReplyChargingValue",
  "RetrieveStatusValue",
  "StatusValue",

  
  "PduHelper",
]);

