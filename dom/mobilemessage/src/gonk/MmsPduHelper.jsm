



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let WSP = {};
Cu.import("resource://gre/modules/WspPduHelper.jsm", WSP);

Cu.import("resource://gre/modules/mms_consts.js");

Cu.import("resource://gre/modules/PhoneNumberUtils.jsm");

let DEBUG; 

this.MMS_VERSION = (function() {
  Cu.import("resource://gre/modules/Services.jsm");

  try {
    return Services.prefs.getIntPref("dom.mms.version");
  } catch(ex) {}

  return MMS_VERSION_1_3;
})();

this.translatePduErrorToStatus = function translatePduErrorToStatus(error) {
  if (error == MMS_PDU_ERROR_OK) {
    return MMS_PDU_STATUS_RETRIEVED;
  }

  if ((error >= MMS_PDU_ERROR_TRANSIENT_FAILURE)
      && (error < MMS_PDU_ERROR_PERMANENT_FAILURE)) {
    return MMS_PDU_STATUS_DEFERRED;
  }

  return MMS_PDU_STATUS_UNRECOGNISED;
}

function defineLazyRegExp(obj, name, pattern) {
  obj.__defineGetter__(name, function() {
    delete obj[name];
    return obj[name] = new RegExp(pattern);
  });
}

function RangedValue(name, min, max) {
  this.name = name;
  this.min = min;
  this.max = max;
}
RangedValue.prototype = {
  name: null,
  min: null,
  max: null,

  







  decode: function(data) {
    let value = WSP.Octet.decode(data);
    if ((value >= this.min) && (value <= this.max)) {
      return value;
    }

    throw new WSP.CodeError(this.name + ": invalid value " + value);
  },

  





  encode: function(data, value) {
    if ((value < this.min) || (value > this.max)) {
      throw new WSP.CodeError(this.name + ": invalid value " + value);
    }

    WSP.Octet.encode(data, value);
  },
};








this.BooleanValue = {
  







  decode: function(data) {
    let value = WSP.Octet.decode(data);
    if ((value != 128) && (value != 129)) {
      throw new WSP.CodeError("Boolean-value: invalid value " + value);
    }

    return value == 128;
  },

  





  encode: function(data, value) {
    WSP.Octet.encode(data, value ? 128 : 129);
  },
};








this.Address = {
  





  decode: function(data) {
    let str = EncodedStringValue.decode(data);

    let result;
    if (((result = str.match(this.REGEXP_DECODE_PLMN)) != null)
        || ((result = str.match(this.REGEXP_DECODE_IPV4)) != null)
        || ((result = str.match(this.REGEXP_DECODE_IPV6)) != null)
        || (((result = str.match(this.REGEXP_DECODE_CUSTOM)) != null)
            && (result[2] != "PLMN")
            && (result[2] != "IPv4")
            && (result[2] != "IPv6"))) {
      return {address: result[1], type: result[2]};
    }

    let type;
    if (str.match(this.REGEXP_NUM)) {
      type = "num";
    } else if (str.match(this.REGEXP_ALPHANUM)) {
      type = "alphanum";
    } else if (str.match(this.REGEXP_EMAIL)) {
      type = "email";
    } else {
      throw new WSP.CodeError("Address: invalid address");
    }

    return {address: str, type: type};
  },

  





  encode: function(data, value) {
    if (!value || !value.type || !value.address) {
      throw new WSP.CodeError("Address: invalid value");
    }

    let str;
    switch (value.type) {
      case "email":
        if (value.address.match(this.REGEXP_EMAIL)) {
          str = value.address;
        }
        break;
      case "num":
        if (value.address.match(this.REGEXP_NUM)) {
          str = value.address;
        }
        break;
      case "alphanum":
        if (value.address.match(this.REGEXP_ALPHANUM)) {
          str = value.address;
        }
        break;
      case "IPv4":
        if (value.address.match(this.REGEXP_ENCODE_IPV4)) {
          str = value.address + "/TYPE=IPv4";
        }
        break;
      case "IPv6":
        if (value.address.match(this.REGEXP_ENCODE_IPV6)) {
          str = value.address + "/TYPE=IPv6";
        }
        break;
      case "PLMN":
        if (value.address.match(this.REGEXP_ENCODE_PLMN)) {
          str = value.address + "/TYPE=PLMN";
        }
        break;
      default:
        if (value.type.match(this.REGEXP_ENCODE_CUSTOM_TYPE)
            && value.address.match(this.REGEXP_ENCODE_CUSTOM_ADDR)) {
          str = value.address + "/TYPE=" + value.type;
        }
        break;
    }

    if (!str) {
      throw new WSP.CodeError("Address: invalid value: " + JSON.stringify(value));
    }

    EncodedStringValue.encode(data, str);
  },

  





  resolveType: function(address) {
    if (address.match(this.REGEXP_EMAIL)) {
      return "email";
    }

    if (address.match(this.REGEXP_ENCODE_IPV4)) {
      return "IPv4";
    }

    if (address.match(this.REGEXP_ENCODE_IPV6)) {
      return "IPv6";
    }

    let normalizedAddress = PhoneNumberUtils.normalize(address, false);
    if (PhoneNumberUtils.isPlainPhoneNumber(normalizedAddress)) {
      return "PLMN";
    }

    return "Others";
  },
};

defineLazyRegExp(Address, "REGEXP_DECODE_PLMN",        "^(\\+?[\\d.-]+)\\/TYPE=(PLMN)$");
defineLazyRegExp(Address, "REGEXP_DECODE_IPV4",        "^((?:(?:25[0-5]|(?:2[0-4]|1[0-9]|[1-9]){0,1}[0-9])\\.){3,3}(?:25[0-5]|(?:2[0-4]|1[0-9]|[1-9]){0,1}[0-9]))\\/TYPE=(IPv4)$");
defineLazyRegExp(Address, "REGEXP_DECODE_IPV6",        "^(" +
                                                       "(?:[0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,7}:|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,5}(?::[0-9a-fA-F]{1,4}){1,2}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,4}(?::[0-9a-fA-F]{1,4}){1,3}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,3}(?::[0-9a-fA-F]{1,4}){1,4}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,2}(?::[0-9a-fA-F]{1,4}){1,5}|" +
                                                       "[0-9a-fA-F]{1,4}:(?:(?::[0-9a-fA-F]{1,4}){1,6})|" +
                                                       ":(?:(?::[0-9a-fA-F]{1,4}){1,7}|:)|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,4}:(?:(?:25[0-5]|(?:2[0-4]|1[0-9]|[1-9]){0,1}[0-9])\\.){3,3}(?:25[0-5]|(?:2[0-4]|1[0-9]|[1-9]){0,1}[0-9])" +
                                                       ")\\/TYPE=(IPv6)$");
defineLazyRegExp(Address, "REGEXP_DECODE_CUSTOM",      "^([\\w\\+\\-.%]+)\\/TYPE=(\\w+)$");
defineLazyRegExp(Address, "REGEXP_ENCODE_PLMN",        "^\\+?[\\d.-]+$");
defineLazyRegExp(Address, "REGEXP_ENCODE_IPV4",        "^(?:(?:25[0-5]|(?:2[0-4]|1[0-9]|[1-9]){0,1}[0-9])\\.){3,3}(?:25[0-5]|(?:2[0-4]|1[0-9]|[1-9]){0,1}[0-9])$");
defineLazyRegExp(Address, "REGEXP_ENCODE_IPV6",        "^(?:" +
                                                       "(?:[0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,7}:|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,5}(?::[0-9a-fA-F]{1,4}){1,2}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,4}(?::[0-9a-fA-F]{1,4}){1,3}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,3}(?::[0-9a-fA-F]{1,4}){1,4}|" +
                                                       "(?:[0-9a-fA-F]{1,4}:){1,2}(?::[0-9a-fA-F]{1,4}){1,5}|" +
                                                       "[0-9a-fA-F]{1,4}:(?::[0-9a-fA-F]{1,4}){1,6}|" +
                                                       ":(?:(?::[0-9a-fA-F]{1,4}){1,7}|:)" +
                                                       ")$");
defineLazyRegExp(Address, "REGEXP_ENCODE_CUSTOM_TYPE", "^\\w+$");
defineLazyRegExp(Address, "REGEXP_ENCODE_CUSTOM_ADDR", "^[\\w\\+\\-.%]+$");
defineLazyRegExp(Address, "REGEXP_NUM",                "^[\\+*#]?\\d+$");
defineLazyRegExp(Address, "REGEXP_ALPHANUM",           "^\\w+$");







defineLazyRegExp(Address, "REGEXP_EMAIL",              "(?:" +
                                                       "[a-zA-Z0-9!#$%&'*+/=?^_`{|}~-]+(?:\\.[a-zA-Z0-9!#$%&'*+/=?^_`{|}~-]+)*|" +
                                                       "\"(?:[\x01-\x08\x0b\x0c\x0e-\x1f\x21\x23-\x5b\x5d-\x7f]|\\\\[\x01-\x09\x0b\x0c\x0e-\x7f])*\"" +
                                                       ")@(?:" +
                                                       "[a-zA-Z0-9!#$%&'*+/=?^_`{|}~-]+(?:\\.[a-zA-Z0-9!#$%&'*+/=?^_`{|}~-]+)*|" +
                                                       "\\[(?:" +
                                                       "[\x01-\x08\x0b\x0c\x0e-\x1f\x21-\x5a\x53-\x7f]|\\\\[\x01-\x09\x0b\x0c\x0e-\x7f]" +
                                                       ")*\\]" +
                                                       ")");






this.HeaderField = {
  










  decode: function(data, options) {
    return WSP.decodeAlternatives(data, options,
                                  MmsHeader, WSP.ApplicationHeader);
  },

  







  encode: function(data, value, options) {
    WSP.encodeAlternatives(data, value, options,
                           MmsHeader, WSP.ApplicationHeader);
  },
};







this.MmsHeader = {
  













  decode: function(data, options) {
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

  










  encode: function(data, header) {
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







this.CancelStatusValue = new RangedValue("Cancel-status-value", 128, 129);







this.ContentClassValue = new RangedValue("Content-class-value", 128, 135);













this.ContentLocationValue = {
  








  decode: function(data, options) {
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







this.ElementDescriptorValue = {
  






  decode: function(data) {
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












this.Parameter = {
  








  decodeParameterName: function(data) {
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

  








  decode: function(data) {
    let name = this.decodeParameterName(data);
    let value = WSP.decodeAlternatives(data, null,
                                       WSP.ConstrainedEncoding, WSP.TextString);
    return {
      name: name,
      value: value,
    };
  },

  







  decodeMultiple: function(data, end) {
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

  







  encode: function(data, param, options) {
    if (!param || !param.name) {
      throw new WSP.CodeError("Parameter-name: empty param name");
    }

    let entry = MMS_WELL_KNOWN_PARAMS[param.name.toLowerCase()];
    if (entry) {
      WSP.ShortInteger.encode(data, entry.number);
    } else {
      WSP.TextString.encode(data, param.name);
    }

    WSP.encodeAlternatives(data, param.value, options,
                           WSP.ConstrainedEncoding, WSP.TextString);
  },
};










this.EncodedStringValue = {
  









  decodeCharsetEncodedString: function(data) {
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

  





  decode: function(data) {
    let begin = data.offset;
    try {
      return WSP.TextString.decode(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeCharsetEncodedString(data);
    }
  },

  







  encodeCharsetEncodedString: function(data, str) {
    let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
               .createInstance(Ci.nsIScriptableUnicodeConverter);
    
    
    
    conv.charset = "UTF-8";

    let raw;
    try {
      raw = conv.convertToByteArray(str);
    } catch (e) {
      throw new WSP.CodeError("Charset-encoded-string: " + e.message);
    }

    let length = raw.length + 2; 
    
    if (raw[0] >= 128) {
      ++length;
    }

    WSP.ValueLength.encode(data, length);

    let entry = WSP.WSP_WELL_KNOWN_CHARSETS["utf-8"];
    WSP.IntegerValue.encode(data, entry.number);

    if (raw[0] >= 128) {
      WSP.Octet.encode(data, 127);
    }
    WSP.Octet.encodeMultiple(data, raw);
    WSP.Octet.encode(data, 0);
  },

  





  encode: function(data, str) {
    let begin = data.offset;
    try {
      
      
      
      
      
      
      
      
      
      
      WSP.TextString.encode(data, str, true);
    } catch (e) {
      data.offset = begin;
      this.encodeCharsetEncodedString(data, str);
    }
  },
};








this.ExpiryValue = {
  







  decode: function(data) {
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

  





  encode: function(data, value) {
    let isDate, begin = data.offset;
    if (value instanceof Date) {
      isDate = true;
      WSP.DateValue.encode(data, value);
    } else if (typeof value == "number") {
      isDate = false;
      WSP.DeltaSecondsValue.encode(data, value);
    } else {
      throw new CodeError("Expiry-value: invalid value type");
    }

    
    
    let len = data.offset - begin;
    data.offset = begin;

    WSP.ValueLength.encode(data, len + 1);
    if (isDate) {
      WSP.Octet.encode(data, 128);
      WSP.DateValue.encode(data, value);
    } else {
      WSP.Octet.encode(data, 129);
      WSP.DeltaSecondsValue.encode(data, value);
    }
  },
};








this.FromValue = {
  








  decode: function(data) {
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

  





  encode: function(data, value) {
    if (!value) {
      WSP.ValueLength.encode(data, 1);
      WSP.Octet.encode(data, 129);
      return;
    }

    
    
    let begin = data.offset;
    Address.encode(data, value);
    let len = data.offset - begin;
    data.offset = begin;

    WSP.ValueLength.encode(data, len + 1);
    WSP.Octet.encode(data, 128);
    Address.encode(data, value);
  },
};







this.PreviouslySentByValue = {
  






  decode: function(data) {
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







this.PreviouslySentDateValue = {
  






  decode: function(data) {
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











this.MessageClassValue = {
  WELL_KNOWN_CLASSES: ["personal", "advertisement", "informational", "auto"],

  







  decodeClassIdentifier: function(data) {
    let value = WSP.Octet.decode(data);
    if ((value >= 128) && (value < (128 + this.WELL_KNOWN_CLASSES.length))) {
      return this.WELL_KNOWN_CLASSES[value - 128];
    }

    throw new WSP.CodeError("Class-identifier: invalid id " + value);
  },

  





  decode: function(data) {
    let begin = data.offset;
    try {
      return this.decodeClassIdentifier(data);
    } catch (e) {
      data.offset = begin;
      return WSP.TokenText.decode(data);
    }
  },

  




  encode: function(data, klass) {
    let index = this.WELL_KNOWN_CLASSES.indexOf(klass.toLowerCase());
    if (index >= 0) {
      WSP.Octet.encode(data, index + 128);
    } else {
      WSP.TokenText.encode(data, klass);
    }
  },
};

 




this.MessageTypeValue = new RangedValue("Message-type-value", 128, 151);









this.MmFlagsValue = {
  








  decode: function(data) {
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

  






  encode: function(data, value) {
    if ((value.type < 128) || (value.type > 130)) {
      throw new WSP.CodeError("MM-flags-value: invalid type " + value.type);
    }

    
    
    let begin = data.offset;
    EncodedStringValue.encode(data, value.text);
    let len = data.offset - begin;
    data.offset = begin;

    WSP.ValueLength.encode(data, len + 1);
    WSP.Octet.encode(data, value.type);
    EncodedStringValue.encode(data, value.text);
  },
};











this.MmStateValue = new RangedValue("MM-state-value", 128, 132);









this.PriorityValue = new RangedValue("Priority-value", 128, 130);






this.ReadStatusValue = new RangedValue("Read-status-value", 128, 129);







this.RecommendedRetrievalModeValue = {
  





  decode: function(data) {
    return WSP.Octet.decodeEqualTo(data, 128);
  },
};











this.ReplyChargingValue = new RangedValue("Reply-charging-value", 128, 131);












this.ResponseText = {
  








  decode: function(data, options) {
    let type = WSP.ensureHeader(options, "x-mms-message-type");

    let result = {};
    if ((type == MMS_PDU_TYPE_MBOX_DELETE_CONF)
        || (type == MMS_PDU_TYPE_DELETE_CONF)) {
      let length = WSP.ValueLength.decode(data);
      let end = data.offset + length;

      result.statusCount = WSP.IntegerValue.decode(data);
      result.text = EncodedStringValue.decode(data);

      if (data.offset != end) {
        data.offset = end;
      }
    } else {
      result.text = EncodedStringValue.decode(data);
    }

    return result;
  },
};




















this.RetrieveStatusValue = {
  





  decode: function(data) {
    let value = WSP.Octet.decode(data);
    if (value == MMS_PDU_ERROR_OK) {
      return value;
    }

    if ((value >= MMS_PDU_ERROR_TRANSIENT_FAILURE) && (value < 256)) {
      return value;
    }

    
    
    
    return MMS_PDU_ERROR_PERMANENT_FAILURE;
  },
};






this.SenderVisibilityValue = new RangedValue("Sender-visibility-value", 128, 129);















this.StatusValue = new RangedValue("Status-value", 128, 135);

this.PduHelper = {
  








  parseHeaders: function(data, headers) {
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

  





  parseContent: function(data, msg) {
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

  










  checkMandatoryFields: function(msg) {
    let type = WSP.ensureHeader(msg.headers, "x-mms-message-type");
    let entry = MMS_PDU_TYPES[type];
    if (!entry) {
      throw new WSP.FatalCodeError(
        "checkMandatoryFields: unsupported message type " + type);
    }

    entry.mandatoryFields.forEach(function(name) {
      WSP.ensureHeader(msg.headers, name);
    });

    
    msg.type = type;

    return entry;
  },

  







  parse: function(data, msg) {
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

  







  encodeHeader: function(data, headers, name) {
    let value = headers[name];
    if (Array.isArray(value)) {
      for (let i = 0; i < value.length; i++) {
        HeaderField.encode(data, {name: name, value: value[i]}, headers);
      }
    } else {
      HeaderField.encode(data, {name: name, value: value}, headers);
    }
  },

  





  encodeHeaderIfExists: function(data, headers, name) {
    
    if (headers[name] !== undefined) {
      this.encodeHeader(data, headers, name);
    }
  },

  







  encodeHeaders: function(data, headers) {
    if (!data) {
      data = {array: [], offset: 0};
    }

    
    
    
    
    
    
    this.encodeHeader(data, headers, "x-mms-message-type");
    this.encodeHeaderIfExists(data, headers, "x-mms-transaction-id");
    this.encodeHeaderIfExists(data, headers, "x-mms-mms-version");

    for (let key in headers) {
      if ((key == "x-mms-message-type")
          || (key == "x-mms-transaction-id")
          || (key == "x-mms-mms-version")
          || (key == "content-type")) {
        continue;
      }
      this.encodeHeader(data, headers, key);
    }

    this.encodeHeaderIfExists(data, headers, "content-type");

    return data;
  },

  







  compose: function(multiStream, msg) {
    if (!multiStream) {
      multiStream = Cc["@mozilla.org/io/multiplex-input-stream;1"]
                    .createInstance(Ci.nsIMultiplexInputStream);
    }

    try {
      
      let typeinfo = this.checkMandatoryFields(msg);

      let data = this.encodeHeaders(null, msg.headers);
      debug("Composed PDU Header: " + JSON.stringify(data.array));
      WSP.PduHelper.appendArrayToMultiStream(multiStream, data.array, data.offset);

      if (msg.content) {
        WSP.PduHelper.appendArrayToMultiStream(multiStream, msg.content, msg.content.length);
      } else if (msg.parts) {
        WSP.PduHelper.composeMultiPart(multiStream, msg.parts);
      } else if (typeinfo.hasContent) {
        throw new WSP.CodeError("Missing message content");
      }

      return multiStream;
    } catch (e) {
      debug("Failed to compose MMS message, error message: " + e.message);
      return null;
    }
  },
};

const MMS_PDU_TYPES = (function() {
  let pdus = {};
  function add(number, hasContent, mandatoryFields) {
    pdus[number] = {
      number: number,
      hasContent: hasContent,
      mandatoryFields: mandatoryFields,
    };
  }

  add(MMS_PDU_TYPE_SEND_REQ, true, ["x-mms-message-type",
                                    "x-mms-transaction-id",
                                    "x-mms-mms-version",
                                    "from",
                                    "content-type"]);
  add(MMS_PDU_TYPE_SEND_CONF, false, ["x-mms-message-type",
                                      "x-mms-transaction-id",
                                      "x-mms-mms-version",
                                      "x-mms-response-status"]);
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
  add(MMS_PDU_TYPE_DELIVERY_IND, false, ["x-mms-message-type",
                                         "x-mms-mms-version",
                                         "message-id",
                                         "to",
                                         "date",
                                         "x-mms-status"]);
  add(MMS_PDU_TYPE_ACKNOWLEDGE_IND, false, ["x-mms-message-type",
                                            "x-mms-transaction-id",
                                            "x-mms-mms-version"]);
  add(MMS_PDU_TYPE_READ_REC_IND, false, ["x-mms-message-type",
                                         "message-id",
                                         "x-mms-mms-version",
                                         "to",
                                         "from",
                                         "x-mms-read-status"]);
  add(MMS_PDU_TYPE_READ_ORIG_IND, false, ["x-mms-message-type",
                                          "x-mms-mms-version",
                                          "message-id",
                                          "to",
                                          "from",
                                          "date",
                                          "x-mms-read-status"]);

  return pdus;
})();






const MMS_HEADER_FIELDS = (function() {
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
  add("x-mms-delivery-time",                     0x07, ExpiryValue);
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
  add("x-mms-response-status",                   0x12, RetrieveStatusValue);
  add("x-mms-response-text",                     0x13, ResponseText);
  add("x-mms-sender-visibility",                 0x14, SenderVisibilityValue);
  add("x-mms-status",                            0x15, StatusValue);
  add("subject",                                 0x16, EncodedStringValue);
  add("to",                                      0x17, Address);
  add("x-mms-transaction-id",                    0x18, WSP.TextString);
  add("x-mms-retrieve-status",                   0x19, RetrieveStatusValue);
  add("x-mms-retrieve-text",                     0x1A, EncodedStringValue);
  add("x-mms-read-status",                       0x1B, ReadStatusValue);
  add("x-mms-reply-charging",                    0x1C, ReplyChargingValue);
  add("x-mms-reply-charging-deadline",           0x1D, ExpiryValue);
  add("x-mms-reply-charging-id",                 0x1E, WSP.TextString);
  add("x-mms-reply-charging-size",               0x1F, WSP.LongInteger);
  add("x-mms-previously-sent-by",                0x20, PreviouslySentByValue);
  add("x-mms-previously-sent-date",              0x21, PreviouslySentDateValue);
  add("x-mms-store",                             0x22, BooleanValue);
  add("x-mms-mm-state",                          0x23, MmStateValue);
  add("x-mms-mm-flags",                          0x24, MmFlagsValue);
  add("x-mms-store-status",                      0x25, RetrieveStatusValue);
  add("x-mms-store-status-text",                 0x26, EncodedStringValue);
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
  add("x-mms-cancel-status",                     0x3F, CancelStatusValue);

  return names;
})();


const MMS_WELL_KNOWN_PARAMS = (function() {
  let params = {};

  function add(name, number, coder) {
    let entry = {
      name: name,
      number: number,
      coder: coder,
    };
    params[name] = params[number] = entry;
  }

  
  add("type", 0x02, WSP.TypeValue);

  return params;
})();

let debug;
if (DEBUG) {
  debug = function(s) {
    dump("-$- MmsPduHelper: " + s + "\n");
  };
} else {
  debug = function(s) {};
}

this.EXPORTED_SYMBOLS = ALL_CONST_SYMBOLS.concat([
  
  "MMS_VERSION",

  
  "translatePduErrorToStatus",

  
  "BooleanValue",
  "Address",
  "HeaderField",
  "MmsHeader",
  "CancelStatusValue",
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
  "ReadStatusValue",
  "RecommendedRetrievalModeValue",
  "ReplyChargingValue",
  "ResponseText",
  "RetrieveStatusValue",
  "SenderVisibilityValue",
  "StatusValue",

  
  "PduHelper",
]);

