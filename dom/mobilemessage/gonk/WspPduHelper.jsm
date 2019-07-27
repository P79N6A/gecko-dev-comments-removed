



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/wap_consts.js", this);

let DEBUG; 


const NUL = 0;
const CR = 13;
const LF = 10;
const SP = 32;
const HT = 9;
const DQUOTE = 34;
const DEL = 127;


const CTLS = 32;
const ASCIIS = 128;




this.CodeError = function CodeError(message) {
  this.name = "CodeError";
  this.message = message || "Invalid format";
}
CodeError.prototype = new Error();
CodeError.prototype.constructor = CodeError;







function NullCharError(message) {
  this.name = "NullCharError";
  this.message = message || "Null character found";
}
NullCharError.prototype = new CodeError();
NullCharError.prototype.constructor = NullCharError;











this.FatalCodeError = function FatalCodeError(message) {
  this.name = "FatalCodeError";
  this.message = message || "Decoding fails";
}
FatalCodeError.prototype = new Error();
FatalCodeError.prototype.constructor = FatalCodeError;














this.NotWellKnownEncodingError = function NotWellKnownEncodingError(message) {
  this.name = "NotWellKnownEncodingError";
  this.message = message || "Not well known encoding";
}
NotWellKnownEncodingError.prototype = new FatalCodeError();
NotWellKnownEncodingError.prototype.constructor = NotWellKnownEncodingError;














this.ensureHeader = function ensureHeader(headers, name) {
  let value = headers[name];
  
  if (value === undefined) {
    throw new FatalCodeError("ensureHeader: header " + name + " not defined");
  }
  return value;
}


























this.skipValue = function skipValue(data) {
  let begin = data.offset;
  let value = Octet.decode(data);
  if (value <= 31) {
    if (value == 31) {
      value = UintVar.decode(data);
    }

    if (value) {
      
      
      value = Octet.decodeMultiple(data, data.offset + value);
    } else {
      value = null;
    }
  } else if (value <= 127) {
    data.offset = begin;
    value = NullTerminatedTexts.decode(data);
  } else {
    value &= 0x7F;
  }

  return value;
}











this.decodeAlternatives = function decodeAlternatives(data, options) {
  let begin = data.offset;
  for (let i = 2; i < arguments.length; i++) {
    try {
      return arguments[i].decode(data, options);
    } catch (e) {
      
      if (i == (arguments.length - 1)) {
        throw e;
      }

      data.offset = begin;
    }
  }
}











this.encodeAlternatives = function encodeAlternatives(data, value, options) {
  let begin = data.offset;
  for (let i = 3; i < arguments.length; i++) {
    try {
      arguments[i].encode(data, value, options);
      return;
    } catch (e) {
      
      if (i == (arguments.length - 1)) {
        throw e;
      }

      data.offset = begin;
    }
  }
}

this.Octet = {
  





  decode: function(data) {
    if (data.offset >= data.array.length) {
      throw new RangeError();
    }

    return data.array[data.offset++];
  },

  










  decodeMultiple: function(data, end) {
    if ((end < data.offset) || (end > data.array.length)) {
      throw new RangeError();
    }
    if (end == data.offset) {
      return [];
    }

    let result;
    if (data.array.subarray) {
      result = data.array.subarray(data.offset, end);
    } else if (data.array.slice) {
      result = data.array.slice(data.offset, end);
    } else {
      throw new TypeError();
    }

    data.offset = end;
    return result;
  },

  











  decodeEqualTo: function(data, expected) {
    if (this.decode(data) != expected) {
      throw new CodeError("Octet - decodeEqualTo: doesn't match " + expected);
    }

    return expected;
  },

  





  encode: function(data, octet) {
    if (data.offset >= data.array.length) {
      data.array.push(octet);
      data.offset++;
    } else {
      data.array[data.offset++] = octet;
    }
  },

  





  encodeMultiple: function(data, array) {
    for (let i = 0; i < array.length; i++) {
      this.encode(data, array[i]);
    }
  },
};













this.Text = {
  








  decode: function(data) {
    let code = Octet.decode(data);
    if ((code >= CTLS) && (code != DEL)) {
      return String.fromCharCode(code);
    }

    if (code == NUL) {
      throw new NullCharError();
    }

    if (code != CR) {
      throw new CodeError("Text: invalid char code " + code);
    }

    
    
    
    

    let extra;

    
    try {
      extra = Octet.decode(data);
      if (extra != LF) {
        throw new CodeError("Text: doesn't match LWS sequence");
      }

      extra = Octet.decode(data);
      if ((extra != SP) && (extra != HT)) {
        throw new CodeError("Text: doesn't match LWS sequence");
      }
    } catch (e if e instanceof CodeError) {
      throw e;
    } catch (e) {
      throw new CodeError("Text: doesn't match LWS sequence");
    }

    
    let begin;

    
    try {
      do {
        begin = data.offset;
        extra = Octet.decode(data);
      } while ((extra == SP) || (extra == HT));
    } catch (e) {}

    data.offset = begin;
    return " ";
  },

  









  encode: function(data, text, asciiOnly) {
    if (!text) {
      throw new CodeError("Text: empty string");
    }

    let code = text.charCodeAt(0);
    if ((code < CTLS) || (code == DEL) || (code > 255) ||
        (code >= 128 && asciiOnly)) {
      throw new CodeError("Text: invalid char code " + code);
    }
    Octet.encode(data, code);
  },
};

this.NullTerminatedTexts = {
  







  decode: function(data) {
    let str = "";
    try {
      
      while (true) {
        str += Text.decode(data);
      }
    } catch (e if e instanceof NullCharError) {
      return str;
    }
  },

  







  encode: function(data, str, asciiOnly) {
    if (str) {
      for (let i = 0; i < str.length; i++) {
        Text.encode(data, str.charAt(i), asciiOnly);
      }
    }
    Octet.encode(data, 0);
  },
};








this.Token = {
  








  decode: function(data) {
    let code = Octet.decode(data);
    if ((code < ASCIIS) && (code >= CTLS)) {
      if ((code == HT) || (code == SP)
          || (code == 34) || (code == 40) || (code == 41) 
          || (code == 44) || (code == 47)                 
          || ((code >= 58) && (code <= 64))               
          || ((code >= 91) && (code <= 93))               
          || (code == 123) || (code == 125)) {            
        throw new CodeError("Token: invalid char code " + code);
      }

      return String.fromCharCode(code);
    }

    if (code == NUL) {
      throw new NullCharError();
    }

    throw new CodeError("Token: invalid char code " + code);
  },

  







  encode: function(data, token) {
    if (!token) {
      throw new CodeError("Token: empty string");
    }

    let code = token.charCodeAt(0);
    if ((code < ASCIIS) && (code >= CTLS)) {
      if ((code == HT) || (code == SP)
          || (code == 34) || (code == 40) || (code == 41) 
          || (code == 44) || (code == 47)                 
          || ((code >= 58) && (code <= 64))               
          || ((code >= 91) && (code <= 93))               
          || (code == 123) || (code == 125)) {            
        
      } else {
        Octet.encode(data, token.charCodeAt(0));
        return;
      }
    }

    throw new CodeError("Token: invalid char code " + code);
  },
};













this.URIC = {
  








  decode: function(data) {
    let code = Octet.decode(data);
    if (code == NUL) {
      throw new NullCharError();
    }

    if ((code <= CTLS) || (code >= ASCIIS) || (code == 34) || (code == 60)
        || (code == 62) || ((code >= 91) && (code <= 94)) || (code == 96)
        || ((code >= 123) && (code <= 125)) || (code == 127)) {
      throw new CodeError("URIC: invalid char code " + code);
    }

    return String.fromCharCode(code);
  },
};











this.TextString = {
  





  decode: function(data) {
    let begin = data.offset;
    let firstCode = Octet.decode(data);
    if (firstCode == 127) {
      
      begin = data.offset;
      try {
        if (Octet.decode(data) < 128) {
          throw new CodeError("Text-string: illegal quote found.");
        }
      } catch (e if e instanceof CodeError) {
        throw e;
      } catch (e) {
        throw new CodeError("Text-string: unexpected error.");
      }
    } else if (firstCode >= 128) {
      throw new CodeError("Text-string: invalid char code " + firstCode);
    }

    data.offset = begin;
    return NullTerminatedTexts.decode(data);
  },

  







  encode: function(data, str, asciiOnly) {
    if (!str) {
      Octet.encode(data, 0);
      return;
    }

    let firstCharCode = str.charCodeAt(0);
    if (firstCharCode >= 128) {
      if (asciiOnly) {
        throw new CodeError("Text: invalid char code " + code);
      }

      Octet.encode(data, 127);
    }

    NullTerminatedTexts.encode(data, str, asciiOnly);
  },
};






this.TokenText = {
  





  decode: function(data) {
    let str = "";
    try {
      
      while (true) {
        str += Token.decode(data);
      }
    } catch (e if e instanceof NullCharError) {
      return str;
    }
  },

  





  encode: function(data, str) {
    if (str) {
      for (let i = 0; i < str.length; i++) {
        Token.encode(data, str.charAt(i));
      }
    }
    Octet.encode(data, 0);
  },
};









this.QuotedString = {
  







  decode: function(data) {
    let value = Octet.decode(data);
    if (value != 34) {
      throw new CodeError("Quoted-string: not quote " + value);
    }

    return NullTerminatedTexts.decode(data);
  },

  





  encode: function(data, str) {
    Octet.encode(data, 34);
    NullTerminatedTexts.encode(data, str);
  },
};










this.ShortInteger = {
  







  decode: function(data) {
    let value = Octet.decode(data);
    if (!(value & 0x80)) {
      throw new CodeError("Short-integer: invalid value " + value);
    }

    return (value & 0x7F);
  },

  







  encode: function(data, value) {
    if (value >= 0x80) {
      throw new CodeError("Short-integer: invalid value " + value);
    }

    Octet.encode(data, value | 0x80);
  },
};












this.LongInteger = {
  







  decodeMultiOctetInteger: function(data, length) {
    if (length < 7) {
      
      
      
      
      let value = 0;
      while (length--) {
        value = value * 256 + Octet.decode(data);
      }
      return value;
    }

    return Octet.decodeMultiple(data, data.offset + length);
  },

  







  decode: function(data) {
    let length = Octet.decode(data);
    if ((length < 1) || (length > 30)) {
      throw new CodeError("Long-integer: invalid length " + length);
    }

    return this.decodeMultiOctetInteger(data, length);
  },

  






  encode: function(data, numOrArray) {
    if (typeof numOrArray === "number") {
      let num = numOrArray;
      if (num >= 0x1000000000000) {
        throw new CodeError("Long-integer: number too large " + num);
      }

      let stack = [];
      do {
        stack.push(Math.floor(num % 256));
        num = Math.floor(num / 256);
      } while (num);

      Octet.encode(data, stack.length);
      while (stack.length) {
        Octet.encode(data, stack.pop());
      }
      return;
    }

    let array = numOrArray;
    if ((array.length < 1) || (array.length > 30)) {
      throw new CodeError("Long-integer: invalid length " + array.length);
    }

    Octet.encode(data, array.length);
    Octet.encodeMultiple(data, array);
  },
};




this.UintVar = {
  





  decode: function(data) {
    let value = Octet.decode(data);
    let result = value & 0x7F;
    while (value & 0x80) {
      value = Octet.decode(data);
      result = result * 128 + (value & 0x7F);
    }

    return result;
  },

  





  encode: function(data, value) {
    if (value < 0) {
      throw new CodeError("UintVar: invalid value " + value);
    }

    let stack = [];
    while (value >= 128) {
      stack.push(Math.floor(value % 128));
      value = Math.floor(value / 128);
    }

    while (stack.length) {
      Octet.encode(data, value | 0x80);
      value = stack.pop();
    }
    Octet.encode(data, value);
  },
};













this.ConstrainedEncoding = {
  





  decode: function(data) {
    return decodeAlternatives(data, null, TextString, ShortInteger);
  },

  





  encode: function(data, value) {
    if (typeof value == "number") {
      ShortInteger.encode(data, value);
    } else {
      TextString.encode(data, value);
    }
  },
};









this.ValueLength = {
  







  decode: function(data) {
    let value = Octet.decode(data);
    if (value <= 30) {
      return value;
    }

    if (value == 31) {
      return UintVar.decode(data);
    }

    throw new CodeError("Value-length: invalid value " + value);
  },

  




  encode: function(data, value) {
    if (value <= 30) {
      Octet.encode(data, value);
    } else {
      Octet.encode(data, 31);
      UintVar.encode(data, value);
    }
  },
};






this.NoValue = {
  





  decode: function(data) {
    Octet.decodeEqualTo(data, 0);
    return null;
  },

  





  encode: function(data, value) {
    if (value != null) {
      throw new CodeError("No-value: invalid value " + value);
    }
    Octet.encode(data, 0);
  },
};






this.TextValue = {
  





  decode: function(data) {
    return decodeAlternatives(data, null, NoValue, TokenText, QuotedString);
  },

  





  encode: function(data, text) {
    encodeAlternatives(data, text, null, NoValue, TokenText, QuotedString);
  },
};






this.IntegerValue = {
  





  decode: function(data) {
    return decodeAlternatives(data, null, ShortInteger, LongInteger);
  },

  





  encode: function(data, value) {
    if (typeof value === "number") {
      encodeAlternatives(data, value, null, ShortInteger, LongInteger);
    } else if (Array.isArray(value) || (value instanceof Uint8Array)) {
      LongInteger.encode(data, value);
    } else {
      throw new CodeError("Integer-Value: invalid value type");
    }
  },
};









this.DateValue = {
  





  decode: function(data) {
    let numOrArray = LongInteger.decode(data);
    let seconds;
    if (typeof numOrArray == "number") {
      seconds = numOrArray;
    } else {
      seconds = 0;
      for (let i = 0; i < numOrArray.length; i++) {
        seconds = seconds * 256 + numOrArray[i];
      }
    }

    return new Date(seconds * 1000);
  },

  





  encode: function(data, date) {
    let seconds = date.getTime() / 1000;
    if (seconds < 0) {
      throw new CodeError("Date-value: negative seconds " + seconds);
    }

    LongInteger.encode(data, seconds);
  },
};






this.DeltaSecondsValue = IntegerValue;









this.QValue = {
  







  decode: function(data) {
    let value = UintVar.decode(data);
    if (value > 0) {
      if (value <= 100) {
        return (value - 1) / 100.0;
      }
      if (value <= 1099) {
        return (value - 100) / 1000.0;
      }
    }

    throw new CodeError("Q-value: invalid value " + value);
  },

  





  encode: function(data, value) {
    if ((value < 0) || (value >= 1)) {
      throw new CodeError("Q-value: invalid value " + value);
    }

    value *= 1000;
    if ((value % 10) == 0) {
      
      UintVar.encode(data, Math.floor(value / 10 + 1));
    } else {
      
      UintVar.encode(data, Math.floor(value + 100));
    }
  },
};












this.VersionValue = {
  





  decode: function(data) {
    let begin = data.offset;
    let value;
    try {
      value = ShortInteger.decode(data);
      if ((value >= 0x10) && (value < 0x80)) {
        return value;
      }

      throw new CodeError("Version-value: invalid value " + value);
    } catch (e) {}

    data.offset = begin;

    let str = TextString.decode(data);
    if (!str.match(/^[1-7](\.1?\d)?$/)) {
      throw new CodeError("Version-value: invalid value " + str);
    }

    let major = str.charCodeAt(0) - 0x30;
    let minor = 0x0F;
    if (str.length > 1) {
      minor = str.charCodeAt(2) - 0x30;
      if (str.length > 3) {
        minor = 10 + (str.charCodeAt(3) - 0x30);
        if (minor > 14) {
          throw new CodeError("Version-value: invalid minor " + minor);
        }
      }
    }

    return major << 4 | minor;
  },

  





  encode: function(data, version) {
    if ((version < 0x10) || (version >= 0x80)) {
      throw new CodeError("Version-value: invalid version " + version);
    }

    ShortInteger.encode(data, version);
  },
};










this.UriValue = {
  





  decode: function(data) {
    let str = "";
    try {
      
      while (true) {
        str += URIC.decode(data);
      }
    } catch (e if e instanceof NullCharError) {
      return str;
    }
  },
};








this.TypeValue = {
  





  decode: function(data) {
    let numOrStr = ConstrainedEncoding.decode(data);
    if (typeof numOrStr == "string") {
      return numOrStr.toLowerCase();
    }

    let number = numOrStr;
    let entry = WSP_WELL_KNOWN_CONTENT_TYPES[number];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Constrained-media: not well known media " + number);
    }

    return entry.type;
  },

  





  encode: function(data, type) {
    let entry = WSP_WELL_KNOWN_CONTENT_TYPES[type.toLowerCase()];
    if (entry) {
      ConstrainedEncoding.encode(data, entry.number);
    } else {
      ConstrainedEncoding.encode(data, type);
    }
  },
};























this.Parameter = {
  











  decodeTypedParameter: function(data) {
    let numOrArray = IntegerValue.decode(data);
    
    if (typeof numOrArray != "number") {
      throw new CodeError("Typed-parameter: invalid integer type");
    }

    let number = numOrArray;
    let param = WSP_WELL_KNOWN_PARAMS[number];
    if (!param) {
      throw new NotWellKnownEncodingError(
        "Typed-parameter: not well known parameter " + number);
    }

    let begin = data.offset, value;
    try {
      
      
      
      
      
      value = decodeAlternatives(data, null,
                                 param.coder, TextValue, TextString);
    } catch (e) {
      data.offset = begin;

      
      value = skipValue(data);
      debug("Skip malformed typed parameter: "
            + JSON.stringify({name: param.name, value: value}));

      return null;
    }

    return {
      name: param.name,
      value: value,
    };
  },

  







  decodeUntypedParameter: function(data) {
    let name = TokenText.decode(data);

    let begin = data.offset, value;
    try {
      value = decodeAlternatives(data, null, IntegerValue, TextValue);
    } catch (e) {
      data.offset = begin;

      
      value = skipValue(data);
      debug("Skip malformed untyped parameter: "
            + JSON.stringify({name: name, value: value}));

      return null;
    }

    return {
      name: name.toLowerCase(),
      value: value,
    };
  },

  







  decode: function(data) {
    let begin = data.offset;
    try {
      return this.decodeTypedParameter(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeUntypedParameter(data);
    }
  },

  







  decodeMultiple: function(data, end) {
    let params = null, param;

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

  





  encodeTypedParameter: function(data, param) {
    let entry = WSP_WELL_KNOWN_PARAMS[param.name.toLowerCase()];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Typed-parameter: not well known parameter " + param.name);
    }

    IntegerValue.encode(data, entry.number);
    encodeAlternatives(data, param.value, null,
                       entry.coder, TextValue, TextString);
  },

  





  encodeUntypedParameter: function(data, param) {
    TokenText.encode(data, param.name);
    encodeAlternatives(data, param.value, null, IntegerValue, TextValue);
  },

  





  encodeMultiple: function(data, params) {
    for (let name in params) {
      this.encode(data, {name: name, value: params[name]});
    }
  },

  





  encode: function(data, param) {
    let begin = data.offset;
    try {
      this.encodeTypedParameter(data, param);
    } catch (e) {
      data.offset = begin;
      this.encodeUntypedParameter(data, param);
    }
  },
};







this.Header = {
  








  decodeMessageHeader: function(data) {
    return decodeAlternatives(data, null, WellKnownHeader, ApplicationHeader);
  },

  








  decode: function(data) {
    
    return this.decodeMessageHeader(data);
  },

  encodeMessageHeader: function(data, header) {
    encodeAlternatives(data, header, null, WellKnownHeader, ApplicationHeader);
  },

  






  encode: function(data, header) {
    
    this.encodeMessageHeader(data, header);
  },
};







this.WellKnownHeader = {
  











  decode: function(data) {
    let index = ShortInteger.decode(data);

    let entry = WSP_HEADER_FIELDS[index];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Well-known-header: not well known header " + index);
    }

    let begin = data.offset, value;
    try {
      value = decodeAlternatives(data, null, entry.coder, TextValue);
    } catch (e) {
      data.offset = begin;

      value = skipValue(data);
      debug("Skip malformed well known header(" + index + "): "
            + JSON.stringify({name: entry.name, value: value}));

      return null;
    }

    return {
      name: entry.name,
      value: value,
    };
  },

  






  encode: function(data, header) {
    let entry = WSP_HEADER_FIELDS[header.name.toLowerCase()];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Well-known-header: not well known header " + header.name);
    }

    ShortInteger.encode(data, entry.number);
    encodeAlternatives(data, header.value, null, entry.coder, TextValue);
  },
};







this.ApplicationHeader = {
  








  decode: function(data) {
    let name = TokenText.decode(data);

    let begin = data.offset, value;
    try {
      value = TextString.decode(data);
    } catch (e) {
      data.offset = begin;

      value = skipValue(data);
      debug("Skip malformed application header: "
            + JSON.stringify({name: name, value: value}));

      return null;
    }

    return {
      name: name.toLowerCase(),
      value: value,
    };
  },

  








  encode: function(data, header) {
    if (!header.name) {
      throw new CodeError("Application-header: empty header name");
    }

    TokenText.encode(data, header.name);
    TextString.encode(data, header.value);
  },
};







this.FieldName = {
  








  decode: function(data) {
    let begin = data.offset;
    try {
      return TokenText.decode(data).toLowerCase();
    } catch (e) {}

    data.offset = begin;

    let number = ShortInteger.decode(data);
    let entry = WSP_HEADER_FIELDS[number];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Field-name: not well known encoding " + number);
    }

    return entry.name;
  },

  





  encode: function(data, name) {
    let entry = WSP_HEADER_FIELDS[name.toLowerCase()];
    if (entry) {
      ShortInteger.encode(data, entry.number);
    } else {
      TokenText.encode(data, name);
    }
  },
};









this.AcceptCharsetValue = {
  





  decodeAnyCharset: function(data) {
    Octet.decodeEqualTo(data, 128);
    return {charset: "*"};
  },

  









  decodeConstrainedCharset: function(data) {
    let begin = data.offset;
    try {
      return this.decodeAnyCharset(data);
    } catch (e) {}

    data.offset = begin;

    let numOrStr = ConstrainedEncoding.decode(data);
    if (typeof numOrStr == "string") {
      return {charset: numOrStr};
    }

    let charset = numOrStr;
    let entry = WSP_WELL_KNOWN_CHARSETS[charset];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Constrained-charset: not well known charset: " + charset);
    }

    return {charset: entry.name};
  },

  






  decodeAcceptCharsetGeneralForm: function(data) {
    let length = ValueLength.decode(data);

    let begin = data.offset;
    let end = begin + length;

    let result;
    try {
      result = WellKnownCharset.decode(data);
    } catch (e) {
      data.offset = begin;

      result = {charset: TokenText.decode(data)};
      if (data.offset < end) {
        result.q = QValue.decode(data);
      }
    }

    if (data.offset != end) {
      data.offset = end;
    }

    return result;
  },

  






  decode: function(data) {
    let begin = data.offset;
    try {
      return this.decodeConstrainedCharset(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeAcceptCharsetGeneralForm(data);
    }
  },

  





  encodeAnyCharset: function(data, value) {
    if (!value || !value.charset || (value.charset === "*")) {
      Octet.encode(data, 128);
      return;
    }

    throw new CodeError("Any-charset: invalid value " + value);
  },
};






this.WellKnownCharset = {
  









  decode: function(data) {
    let begin = data.offset;

    try {
      return AcceptCharsetValue.decodeAnyCharset(data);
    } catch (e) {}

    data.offset = begin;

    
    let numOrArray = IntegerValue.decode(data);
    if (typeof numOrArray != "number") {
      throw new CodeError("Well-known-charset: invalid integer type");
    }

    let charset = numOrArray;
    let entry = WSP_WELL_KNOWN_CHARSETS[charset];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Well-known-charset: not well known charset " + charset);
    }

    return {charset: entry.name};
  },

  




  encode: function(data, value) {
    let begin = data.offset;
    try {
      AcceptCharsetValue.encodeAnyCharset(data, value);
      return;
    } catch (e) {}

    data.offset = begin;
    let entry = WSP_WELL_KNOWN_CHARSETS[value.charset.toLowerCase()];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Well-known-charset: not well known charset " + value.charset);
    }

    IntegerValue.encode(data, entry.number);
  },
};
















this.ContentTypeValue = {
  










  decodeConstrainedMedia: function(data) {
    return {
      media: TypeValue.decode(data),
      params: null,
    };
  },

  









  decodeMedia: function(data) {
    let begin = data.offset, number;
    try {
      number = IntegerValue.decode(data);
    } catch (e) {
      data.offset = begin;
      return NullTerminatedTexts.decode(data).toLowerCase();
    }

    
    if (typeof number != "number") {
      throw new CodeError("Media: invalid integer type");
    }

    let entry = WSP_WELL_KNOWN_CONTENT_TYPES[number];
    if (!entry) {
      throw new NotWellKnownEncodingError("Media: not well known media " + number);
    }

    return entry.type;
  },

  










  decodeMediaType: function(data, end) {
    let media = this.decodeMedia(data);
    let params = Parameter.decodeMultiple(data, end);

    return {
      media: media,
      params: params,
    };
  },

  








  decodeContentGeneralForm: function(data) {
    let length = ValueLength.decode(data);
    let end = data.offset + length;

    let value = this.decodeMediaType(data, end);

    if (data.offset != end) {
      data.offset = end;
    }

    return value;
  },

  








  decode: function(data) {
    let begin = data.offset;

    try {
      return this.decodeConstrainedMedia(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeContentGeneralForm(data);
    }
  },

  





  encodeConstrainedMedia: function(data, value) {
    if (value.params) {
      throw new CodeError("Constrained-media: should use general form instead");
    }

    TypeValue.encode(data, value.media);
  },

  





  encodeMediaType: function(data, value) {
    let entry = WSP_WELL_KNOWN_CONTENT_TYPES[value.media.toLowerCase()];
    if (entry) {
      IntegerValue.encode(data, entry.number);
    } else {
      NullTerminatedTexts.encode(data, value.media);
    }

    Parameter.encodeMultiple(data, value.params);
  },

  





  encodeContentGeneralForm: function(data, value) {
    let begin = data.offset;
    this.encodeMediaType(data, value);

    
    
    let len = data.offset - begin;
    data.offset = begin;

    ValueLength.encode(data, len);
    this.encodeMediaType(data, value);
  },

  





  encode: function(data, value) {
    let begin = data.offset;

    try {
      this.encodeConstrainedMedia(data, value);
    } catch (e) {
      data.offset = begin;
      this.encodeContentGeneralForm(data, value);
    }
  },
};







this.ApplicationIdValue = {
  









  decode: function(data) {
    let begin = data.offset;
    try {
      return UriValue.decode(data);
    } catch (e) {}

    data.offset = begin;

    
    let numOrArray = IntegerValue.decode(data);
    if (typeof numOrArray != "number") {
      throw new CodeError("Application-id-value: invalid integer type");
    }

    let id = numOrArray;
    let entry = OMNA_PUSH_APPLICATION_IDS[id];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Application-id-value: not well known id: " + id);
    }

    return entry.urn;
  },
};

this.PduHelper = {
  







  decodeStringContent: function(data, charset) {
      let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                 .createInstance(Ci.nsIScriptableUnicodeConverter);

      let entry;
      if (charset) {
        entry = WSP_WELL_KNOWN_CHARSETS[charset];
      }
      
      
      conv.charset = (entry && entry.converter) || "UTF-8";
      try {
        return conv.convertFromByteArray(data, data.length);
      } catch (e) {
      }
      return null;
  },

  







  encodeStringContent: function(strContent, charset) {
    let conv = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
               .createInstance(Ci.nsIScriptableUnicodeConverter);

    let entry;
    if (charset) {
      entry = WSP_WELL_KNOWN_CHARSETS[charset];
    }
    
    
    conv.charset = (entry && entry.converter) || "UTF-8";
    try {
      return conv.convertToByteArray(strContent);
    } catch (e) {
    }
    return null;
  },

  












  parseHeaders: function(data, end, headers) {
    if (!headers) {
      headers = {};
    }

    let header;
    while (data.offset < end) {
      try {
        header = Header.decode(data);
      } catch (e) {
        break;
      }
      if (header) {
        headers[header.name] = header.value;
      }
    }

    if (data.offset != end) {
      debug("Parser expects ending in " + end + ", but in " + data.offset);
      
      data.offset = end;
    }

    return headers;
  },

  







  parsePushHeaders: function(data, msg) {
    if (!msg.headers) {
      msg.headers = {};
    }

    let headersLen = UintVar.decode(data);
    let headersEnd = data.offset + headersLen;

    let contentType = ContentTypeValue.decode(data);
    msg.headers["content-type"] = contentType;

    msg.headers = this.parseHeaders(data, headersEnd, msg.headers);
  },

  








  parseMultiPart: function(data) {
    let nEntries = UintVar.decode(data);
    if (!nEntries) {
      return null;
    }

    let parts = new Array(nEntries);
    for (let i = 0; i < nEntries; i++) {
      
      let headersLen = UintVar.decode(data);
      
      let contentLen = UintVar.decode(data);

      let headersEnd = data.offset + headersLen;
      let contentEnd = headersEnd + contentLen;

      try {
        let headers = {};

        let contentType = ContentTypeValue.decode(data);
        headers["content-type"] = contentType;
        headers["content-length"] = contentLen;

        headers = this.parseHeaders(data, headersEnd, headers);

        let octetArray = Octet.decodeMultiple(data, contentEnd);
        let content = null;
        let charset = headers["content-type"].params &&
                      headers["content-type"].params.charset
                    ? headers["content-type"].params.charset.charset
                    : null;

        let mimeType = headers["content-type"].media;

        if (mimeType) {
          if (mimeType == "application/smil") {
            
            
            content = this.decodeStringContent(octetArray, charset);
          } else if (mimeType.indexOf("text/") == 0 && charset != "utf-8") {
            
            
            let tmpStr = this.decodeStringContent(octetArray, charset);
            let encoder = new TextEncoder("UTF-8");
            content = new Blob([encoder.encode(tmpStr)], {type : mimeType});

            
            if (!headers["content-type"].params) {
              headers["content-type"].params = {};
            }
            if (!headers["content-type"].params.charset) {
              headers["content-type"].params.charset = {};
            }
            headers["content-type"].params.charset.charset = "utf-8";
          }
        }

        if (!content) {
          content = new Blob([octetArray], {type : mimeType});
        }

        parts[i] = {
          index: i,
          headers: headers,
          content: content,
        };
      } catch (e) {
        debug("Failed to parse multipart entry, message: " + e.message);
        
        parts[i] = null;
      }

      if (data.offset != contentEnd) {
        
        data.offset = contentEnd;
      }
    }

    return parts;
  },

  









  parse: function(data, isSessionless, msg) {
    if (!msg) {
      msg = {
        type: null,
      };
    }

    try {
      if (isSessionless) {
        
        
        msg.transactionId = Octet.decode(data);
      }

      msg.type = Octet.decode(data);
      switch (msg.type) {
        case WSP_PDU_TYPE_PUSH:
          this.parsePushHeaders(data, msg);
          break;
      }
    } catch (e) {
      debug("Parse error. Message: " + e.message);
      msg = null;
    }

    return msg;
  },

  







  appendArrayToMultiStream: function(multiStream, array, length) {
    let storageStream = Cc["@mozilla.org/storagestream;1"]
                        .createInstance(Ci.nsIStorageStream);
    storageStream.init(4096, length, null);

    let boStream = Cc["@mozilla.org/binaryoutputstream;1"]
                   .createInstance(Ci.nsIBinaryOutputStream);
    boStream.setOutputStream(storageStream.getOutputStream(0));
    boStream.writeByteArray(array, length);
    boStream.close();

    multiStream.appendStream(storageStream.newInputStream(0));
  },

  







  composeMultiPart: function(multiStream, parts) {
    
    {
      let data = {array: [], offset: 0};
      UintVar.encode(data, parts.length);
      debug("Encoded multipart header: " + JSON.stringify(data.array));
      this.appendArrayToMultiStream(multiStream, data.array, data.offset);
    }

    
    for (let i = 0; i < parts.length; i++) {
      let part = parts[i];
      let data = {array: [], offset: 0};

      
      let contentType = part.headers["content-type"];
      ContentTypeValue.encode(data, contentType);

      
      if (Object.keys(part).length > 1) {
        
        delete part.headers["content-type"];

        for (let name in part.headers) {
          Header.encode(data, {name: name, value: part.headers[name]});
        }

        
        part.headers["content-type"] = contentType;
      }

      
      let headersLen = data.offset;
      let content = part.content;
      UintVar.encode(data, headersLen);
      if (typeof content === "string") {
        let charset;
        if (contentType && contentType.params && contentType.params.charset &&
          contentType.params.charset.charset) {
          charset = contentType.params.charset.charset;
        }
        content = this.encodeStringContent(content, charset);
        UintVar.encode(data, content.length);
      } else if (part.content instanceof Uint8Array) {
        UintVar.encode(data, content.length);
      } else {
        throw new TypeError();
      }

      
      let slice1 = data.array.slice(headersLen);
      let slice2 = data.array.slice(0, headersLen);
      data.array = slice1.concat(slice2);
      debug("Encoded per-part header: " + JSON.stringify(data.array));

      
      this.appendArrayToMultiStream(multiStream, data.array, data.offset);
      
      this.appendArrayToMultiStream(multiStream, content, content.length);
    }
  },
};






this.WSP_HEADER_FIELDS = (function() {
  let names = {};
  function add(name, number, coder) {
    let entry = {
      name: name,
      number: number,
      coder: coder,
    };
    names[name] = names[number] = entry;
  }

  
  
  
  
  
  
  add("age",                    0x05, DeltaSecondsValue);
  
  
  
  
  
  
  
  add("content-length",         0x0D, IntegerValue);
  add("content-location",       0x0E, UriValue);
  
  
  add("content-type",           0x11, ContentTypeValue);
  add("date",                   0x12, DateValue);
  add("etag",                   0x13, TextString);
  add("expires",                0x14, DateValue);
  add("from",                   0x15, TextString);
  add("host",                   0x16, TextString);
  add("if-modified-since",      0x17, DateValue);
  add("if-match",               0x18, TextString);
  add("if-none-match",          0x19, TextString);
  
  add("if-unmodified-since",    0x1B, DateValue);
  add("location",               0x1C, UriValue);
  add("last-modified",          0x1D, DateValue);
  add("max-forwards",           0x1E, IntegerValue);
  
  
  
  
  
  add("referer",                0x24, UriValue);
  
  add("server",                 0x26, TextString);
  
  add("upgrade",                0x28, TextString);
  add("user-agent",             0x29, TextString);
  
  add("via",                    0x2B, TextString);
  
  
  

  
  add("x-wap-application-id",   0x2F, ApplicationIdValue);
  add("x-wap-content-uri",      0x30, UriValue);
  add("x-wap-initiator-uri",    0x31, UriValue);
  
  add("bearer-indication",      0x33, IntegerValue);
  add("push-flag",              0x34, ShortInteger);
  add("profile",                0x35, UriValue);
  
  

  
  
  
  
  add("accept-charset",         0x3B, AcceptCharsetValue);
  
  
  
  add("x-wap-tod",              0x3F, DateValue);
  add("content-id",             0x40, QuotedString);
  
  
  

  
  
  
  
  

  return names;
})();



this.WSP_WELL_KNOWN_CONTENT_TYPES = (function() {
  let types = {};

  function add(type, number) {
    let entry = {
      type: type,
      number: number,
    };
    
    
    types[type.toLowerCase()] = types[number] = entry;
  }

  
  
  add("*/*", 0x00);
  add("text/*", 0x01);
  add("text/html", 0x02);
  add("text/plain", 0x03);
  add("text/x-hdml", 0x04);
  add("text/x-ttml", 0x05);
  add("text/x-vCalendar", 0x06);
  add("text/x-vCard", 0x07);
  add("text/vnd.wap.wml", 0x08);
  add("text/vnd.wap.wmlscript", 0x09);
  add("text/vnd.wap.wta-event", 0x0A);
  add("multipart/*", 0x0B);
  add("multipart/mixed", 0x0C);
  add("multipart/form-data", 0x0D);
  add("multipart/byterantes", 0x0E);
  add("multipart/alternative", 0x0F);
  add("application/*", 0x10);
  add("application/java-vm", 0x11);
  add("application/x-www-form-urlencoded", 0x12);
  add("application/x-hdmlc", 0x13);
  add("application/vnd.wap.wmlc", 0x14);
  add("application/vnd.wap.wmlscriptc", 0x15);
  add("application/vnd.wap.wta-eventc", 0x16);
  add("application/vnd.wap.uaprof", 0x17);
  add("application/vnd.wap.wtls-ca-certificate", 0x18);
  add("application/vnd.wap.wtls-user-certificate", 0x19);
  add("application/x-x509-ca-cert", 0x1A);
  add("application/x-x509-user-cert", 0x1B);
  add("image/*", 0x1C);
  add("image/gif", 0x1D);
  add("image/jpeg", 0x1E);
  add("image/tiff", 0x1F);
  add("image/png", 0x20);
  add("image/vnd.wap.wbmp", 0x21);
  add("application/vnd.wap.multipart.*", 0x22);
  add("application/vnd.wap.multipart.mixed", 0x23);
  add("application/vnd.wap.multipart.form-data", 0x24);
  add("application/vnd.wap.multipart.byteranges", 0x25);
  add("application/vnd.wap.multipart.alternative", 0x26);
  add("application/xml", 0x27);
  add("text/xml", 0x28);
  add("application/vnd.wap.wbxml", 0x29);
  add("application/x-x968-cross-cert", 0x2A);
  add("application/x-x968-ca-cert", 0x2B);
  add("application/x-x968-user-cert", 0x2C);
  add("text/vnd.wap.si", 0x2D);

  
  add("application/vnd.wap.sic", 0x2E);
  add("text/vnd.wap.sl", 0x2F);
  add("application/vnd.wap.slc", 0x30);
  add("text/vnd.wap.co", 0x31);
  add("application/vnd.wap.coc", 0x32);
  add("application/vnd.wap.multipart.related", 0x33);
  add("application/vnd.wap.sia", 0x34);

  
  add("text/vnd.wap.connectivity-xml", 0x35);
  add("application/vnd.wap.connectivity-wbxml", 0x36);

  
  add("application/pkcs7-mime", 0x37);
  add("application/vnd.wap.hashed-certificate", 0x38);
  add("application/vnd.wap.signed-certificate", 0x39);
  add("application/vnd.wap.cert-response", 0x3A);
  add("application/xhtml+xml", 0x3B);
  add("application/wml+xml", 0x3C);
  add("text/css", 0x3D);
  add("application/vnd.wap.mms-message", 0x3E);
  add("application/vnd.wap.rollover-certificate", 0x3F);

  
  add("application/vnd.wap.locc+wbxml", 0x40);
  add("application/vnd.wap.loc+xml", 0x41);
  add("application/vnd.syncml.dm+wbxml", 0x42);
  add("application/vnd.syncml.dm+xml", 0x43);
  add("application/vnd.syncml.notification", 0x44);
  add("application/vnd.wap.xhtml+xml", 0x45);
  add("application/vnd.wv.csp.cir", 0x46);
  add("application/vnd.oma.dd+xml", 0x47);
  add("application/vnd.oma.drm.message", 0x48);
  add("application/vnd.oma.drm.content", 0x49);
  add("application/vnd.oma.drm.rights+xml", 0x4A);
  add("application/vnd.oma.drm.rights+wbxml", 0x4B);
  add("application/vnd.wv.csp+xml", 0x4C);
  add("application/vnd.wv.csp+wbxml", 0x4D);
  add("application/vnd.syncml.ds.notification", 0x4E);

  
  add("audio/*", 0x4F);
  add("video/*", 0x50);

  
  add("application/vnd.oma.dd2+xml", 0x51);
  add("application/mikey", 0x52);
  add("application/vnd.oma.dcd", 0x53);
  add("application/vnd.oma.dcdc", 0x54);
  add("text/x-vMessage", 0x55);
  add("application/vnd.omads-email+wbxml", 0x56);
  add("text/x-vBookmark", 0x57);
  add("application/vnd.syncml.dm.notification", 0x58);
  add("application/octet-stream", 0x5A);

  return types;
})();





this.WSP_WELL_KNOWN_PARAMS = (function() {
  let params = {};

  function add(name, number, coder) {
    let entry = {
      name: name,
      number: number,
      coder: coder,
    };
    params[name] = params[number] = entry;
  }

  
  add("q",                 0x00, QValue);
  add("charset",           0x01, WellKnownCharset);
  add("level",             0x02, VersionValue);
  add("type",              0x03, IntegerValue);
  add("name",              0x05, TextValue); 
  
  add("differences",       0x07, FieldName);
  add("padding",           0x08, ShortInteger);

  
  add("type",              0x09, TypeValue);
  add("start",             0x0A, TextValue); 
  

  
  
  
  add("max-age",           0x0E, DeltaSecondsValue);
  
  add("secure",            0x10, NoValue);

  
  add("sec",               0x11, ShortInteger);
  add("mac",               0x12, TextValue);
  add("creation-date",     0x13, DateValue);
  add("modification-date", 0x14, DateValue);
  add("read-date",         0x15, DateValue);
  add("size",              0x16, IntegerValue);
  
  add("filename",          0x18, TextValue);
  
  add("start-info",        0x1A, TextValue);
  add("comment",           0x1B, TextValue);
  add("domain",            0x1C, TextValue);
  add("path",              0x1D, TextValue);

  return params;
})();




this.WSP_WELL_KNOWN_CHARSETS = (function() {
  let charsets = {};

  function add(name, number, converter) {
    let entry = {
      name: name,
      number: number,
      converter: converter,
    };

    charsets[name] = charsets[number] = entry;
  }

  add("us-ascii",           3, null);
  add("iso-8859-1",         4, "ISO-8859-1");
  add("iso-8859-2",         5, "ISO-8859-2");
  add("iso-8859-3",         6, "ISO-8859-3");
  add("iso-8859-4",         7, "ISO-8859-4");
  add("iso-8859-5",         8, "ISO-8859-5");
  add("iso-8859-6",         9, "ISO-8859-6");
  add("iso-8859-7",        10, "ISO-8859-7");
  add("iso-8859-8",        11, "ISO-8859-8");
  add("iso-8859-9",        12, "ISO-8859-9");
  add("iso-8859-10",       13, "ISO-8859-10");
  add("shift_jis",         17, "Shift_JIS");
  add("euc-jp",            18, "EUC-JP");
  add("iso-2022-kr",       37, "ISO-2022-KR");
  add("euc-kr",            38, "EUC-KR");
  add("iso-2022-jp",       39, "ISO-2022-JP");
  add("iso-2022-jp-2",     40, "iso-2022-jp-2");
  add("iso-8859-6-e",      81, "ISO-8859-6-E");
  add("iso-8859-6-i",      82, "ISO-8859-6-I");
  add("iso-8859-8-e",      84, "ISO-8859-8-E");
  add("iso-8859-8-i",      85, "ISO-8859-8-I");
  add("utf-8",            106, "UTF-8");
  add("iso-10646-ucs-2", 1000, "iso-10646-ucs-2");
  add("utf-16",          1015, "UTF-16");
  add("gb2312",          2025, "GB2312");
  add("big5",            2026, "Big5");
  add("koi8-r",          2084, "KOI8-R");
  add("windows-1252",    2252, "windows-1252");

  return charsets;
})();



this.OMNA_PUSH_APPLICATION_IDS = (function() {
  let ids = {};

  function add(urn, number) {
    let entry = {
      urn: urn,
      number: number,
    };

    ids[urn] = ids[number] = entry;
  }

  add("x-wap-application:wml.ua", 0x02);
  add("x-wap-application:mms.ua", 0x04);

  return ids;
})();

let debug;
if (DEBUG) {
  debug = function(s) {
    dump("-@- WspPduHelper: " + s + "\n");
  };
} else {
  debug = function(s) {};
}

this.EXPORTED_SYMBOLS = ALL_CONST_SYMBOLS.concat([
  
  "WSP_HEADER_FIELDS",
  "WSP_WELL_KNOWN_CONTENT_TYPES",
  "WSP_WELL_KNOWN_PARAMS",
  "WSP_WELL_KNOWN_CHARSETS",
  "OMNA_PUSH_APPLICATION_IDS",

  
  "CodeError",
  "FatalCodeError",
  "NotWellKnownEncodingError",

  
  "ensureHeader",
  "skipValue",
  "decodeAlternatives",
  "encodeAlternatives",

  
  "Octet",
  "Text",
  "NullTerminatedTexts",
  "Token",
  "URIC",
  "TextString",
  "TokenText",
  "QuotedString",
  "ShortInteger",
  "LongInteger",
  "UintVar",
  "ConstrainedEncoding",
  "ValueLength",
  "NoValue",
  "TextValue",
  "IntegerValue",
  "DateValue",
  "DeltaSecondsValue",
  "QValue",
  "VersionValue",
  "UriValue",
  "TypeValue",
  "Parameter",
  "Header",
  "WellKnownHeader",
  "ApplicationHeader",
  "FieldName",
  "AcceptCharsetValue",
  "WellKnownCharset",
  "ContentTypeValue",
  "ApplicationIdValue",

  
  "PduHelper",
]);
