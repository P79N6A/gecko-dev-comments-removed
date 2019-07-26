



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

Cu.import("resource://gre/modules/wap_consts.js");

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




function CodeError(message) {
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











function FatalCodeError(message) {
  this.name = "FatalCodeError";
  this.message = message || "Decoding fails";
}
FatalCodeError.prototype = new Error();
FatalCodeError.prototype.constructor = FatalCodeError;














function NotWellKnownEncodingError(message) {
  this.name = "NotWellKnownEncodingError";
  this.message = message || "Not well known encoding";
}
NotWellKnownEncodingError.prototype = new FatalCodeError();
NotWellKnownEncodingError.prototype.constructor = NotWellKnownEncodingError;














function ensureHeader(headers, name) {
  let value = headers[name];
  
  if (value === undefined) {
    throw new FatalCodeError("ensureHeader: header " + name + " not defined");
  }
  return value;
}


























function skipValue(data) {
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











function decodeAlternatives(data, options) {
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











function encodeAlternatives(data, value, options) {
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

let Octet = {
  





  decode: function decode(data) {
    if (data.offset >= data.array.length) {
      throw new RangeError();
    }

    return data.array[data.offset++];
  },

  










  decodeMultiple: function decodeMultiple(data, end) {
    if ((end < data.offset) || (end > data.array.length)) {
      throw new RangeError();
    }
    if (end == data.offset) {
      return null;
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

  











  decodeEqualTo: function decodeEqualTo(data, expected) {
    if (this.decode(data) != expected) {
      throw new CodeError("Octet - decodeEqualTo: doesn't match " + expected);
    }

    return expected;
  },

  





  encode: function encode(data, octet) {
    if (data.offset >= data.array.length) {
      data.array.push(octet);
      data.offset++;
    } else {
      data.array[data.offset++] = octet;
    }
  },

  





  encodeMultiple: function encodeMultiple(data, array) {
    for (let i = 0; i < array.length; i++) {
      this.encode(data, array[i]);
    }
  },
};













let Text = {
  








  decode: function decode(data) {
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

  







  encode: function encode(data, text) {
    if (!text) {
      throw new CodeError("Text: empty string");
    }

    let code = text.charCodeAt(0);
    if ((code < CTLS) || (code == DEL) || (code > 255)) {
      throw new CodeError("Text: invalid char code " + code);
    }
    Octet.encode(data, code);
  },
};

let NullTerminatedTexts = {
  







  decode: function decode(data) {
    let str = "";
    try {
      
      while (true) {
        str += Text.decode(data);
      }
    } catch (e if e instanceof NullCharError) {
      return str;
    }
  },

  





  encode: function encode(data, str) {
    if (str) {
      for (let i = 0; i < str.length; i++) {
        Text.encode(data, str.charAt(i));
      }
    }
    Octet.encode(data, 0);
  },
};








let Token = {
  








  decode: function decode(data) {
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

  







  encode: function encode(data, token) {
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













let URIC = {
  








  decode: function decode(data) {
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











let TextString = {
  





  decode: function decode(data) {
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

  





  encode: function encode(data, str) {
    if (!str) {
      Octet.encode(data, 0);
      return;
    }

    let firstCharCode = str.charCodeAt(0);
    if (firstCharCode >= 128) {
      Octet.encode(data, 127);
    }

    NullTerminatedTexts.encode(data, str);
  },
};






let TokenText = {
  





  decode: function decode(data) {
    let str = "";
    try {
      
      while (true) {
        str += Token.decode(data);
      }
    } catch (e if e instanceof NullCharError) {
      return str;
    }
  },

  





  encode: function encode(data, str) {
    if (str) {
      for (let i = 0; i < str.length; i++) {
        Token.encode(data, str.charAt(i));
      }
    }
    Octet.encode(data, 0);
  },
};









let QuotedString = {
  







  decode: function decode(data) {
    let value = Octet.decode(data);
    if (value != 34) {
      throw new CodeError("Quoted-string: not quote " + value);
    }

    return NullTerminatedTexts.decode(data);
  },

  





  encode: function encode(data, str) {
    Octet.encode(data, 34);
    NullTerminatedTexts.encode(data, str);
  },
};










let ShortInteger = {
  







  decode: function decode(data) {
    let value = Octet.decode(data);
    if (!(value & 0x80)) {
      throw new CodeError("Short-integer: invalid value " + value);
    }

    return (value & 0x7F);
  },

  







  encode: function encode(data, value) {
    if (value >= 0x80) {
      throw new CodeError("Short-integer: invalid value " + value);
    }

    Octet.encode(data, value | 0x80);
  },
};












let LongInteger = {
  







  decodeMultiOctetInteger: function decodeMultiOctetInteger(data, length) {
    if (length < 7) {
      
      
      
      
      let value = 0;
      while (length--) {
        value = value * 256 + Octet.decode(data);
      }
      return value;
    }

    return Octet.decodeMultiple(data, data.offset + length);
  },

  







  decode: function decode(data) {
    let length = Octet.decode(data);
    if ((length < 1) || (length > 30)) {
      throw new CodeError("Long-integer: invalid length " + length);
    }

    return this.decodeMultiOctetInteger(data, length);
  },

  






  encode: function encode(data, numOrArray) {
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




let UintVar = {
  





  decode: function decode(data) {
    let value = Octet.decode(data);
    let result = value & 0x7F;
    while (value & 0x80) {
      value = Octet.decode(data);
      result = result * 128 + (value & 0x7F);
    }

    return result;
  },

  





  encode: function encode(data, value) {
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











let ConstrainedEncoding = {
  





  decode: function decode(data) {
    return decodeAlternatives(data, null, NullTerminatedTexts, ShortInteger);
  },

  





  encode: function encode(data, value) {
    if (typeof value == "number") {
      ShortInteger.encode(data, value);
    } else {
      NullTerminatedTexts.encode(data, value);
    }
  },
};









let ValueLength = {
  







  decode: function decode(data) {
    let value = Octet.decode(data);
    if (value <= 30) {
      return value;
    }

    if (value == 31) {
      return UintVar.decode(data);
    }

    throw new CodeError("Value-length: invalid value " + value);
  },

  




  encode: function encode(data, value) {
    if (value <= 30) {
      Octet.encode(data, value);
    } else {
      Octet.encode(data, 31);
      UintVar.encode(data, value);
    }
  },
};






let NoValue = {
  





  decode: function decode(data) {
    Octet.decodeEqualTo(data, 0);
    return null;
  },

  





  encode: function encode(data, value) {
    if (value != null) {
      throw new CodeError("No-value: invalid value " + value);
    }
    Octet.encode(data, 0);
  },
};






let TextValue = {
  





  decode: function decode(data) {
    return decodeAlternatives(data, null, NoValue, TokenText, QuotedString);
  },

  





  encode: function encode(data, text) {
    encodeAlternatives(data, text, null, NoValue, TokenText, QuotedString);
  },
};






let IntegerValue = {
  





  decode: function decode(data) {
    return decodeAlternatives(data, null, ShortInteger, LongInteger);
  },

  





  encode: function encode(data, value) {
    if (typeof value === "number") {
      encodeAlternatives(data, value, null, ShortInteger, LongInteger);
    } else if (Array.isArray(value) || (value instanceof Uint8Array)) {
      LongInteger.encode(data, value);
    } else {
      throw new CodeError("Integer-Value: invalid value type");
    }
  },
};









let DateValue = {
  





  decode: function decode(data) {
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

  





  encode: function encode(data, date) {
    let seconds = date.getTime() / 1000;
    if (seconds < 0) {
      throw new CodeError("Date-value: negative seconds " + seconds);
    }

    LongInteger.encode(data, seconds);
  },
};






let DeltaSecondsValue = IntegerValue;









let QValue = {
  







  decode: function decode(data) {
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

  





  encode: function encode(data, value) {
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












let VersionValue = {
  





  decode: function decode(data) {
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

  





  encode: function encode(data, version) {
    if ((version < 0x10) || (version >= 0x80)) {
      throw new CodeError("Version-value: invalid version " + version);
    }

    ShortInteger.encode(data, version);
  },
};










let UriValue = {
  





  decode: function decode(data) {
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








let TypeValue = {
  





  decode: function decode(data) {
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

  





  encode: function encode(data, type) {
    let entry = WSP_WELL_KNOWN_CONTENT_TYPES[type.toLowerCase()];
    if (entry) {
      ShortInteger.encode(data, entry.number);
    } else {
      NullTerminatedTexts.encode(data, type);
    }
  },
};























let Parameter = {
  











  decodeTypedParameter: function decodeTypedParameter(data) {
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

  







  decodeUntypedParameter: function decodeUntypedParameter(data) {
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

  







  decode: function decode(data) {
    let begin = data.offset;
    try {
      return this.decodeTypedParameter(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeUntypedParameter(data);
    }
  },

  







  decodeMultiple: function decodeMultiple(data, end) {
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

  





  encodeTypedParameter: function decodeTypedParameter(data, param) {
    let entry = WSP_WELL_KNOWN_PARAMS[param.name.toLowerCase()];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Typed-parameter: not well known parameter " + param.name);
    }

    IntegerValue.encode(data, entry.number);
    encodeAlternatives(data, param.value, null,
                       entry.coder, TextValue, TextString);
  },

  





  encodeUntypedParameter: function encodeUntypedParameter(data, param) {
    TokenText.encode(data, param.name);
    encodeAlternatives(data, param.value, null, IntegerValue, TextValue);
  },

  





  encodeMultiple: function encodeMultiple(data, params) {
    for (let name in params) {
      this.encode(data, {name: name, value: params[name]});
    }
  },

  





  encode: function encode(data, param) {
    let begin = data.offset;
    try {
      this.encodeTypedParameter(data, param);
    } catch (e) {
      data.offset = begin;
      this.encodeUntypedParameter(data, param);
    }
  },
};







let Header = {
  








  decodeMessageHeader: function decodeMessageHeader(data) {
    return decodeAlternatives(data, null, WellKnownHeader, ApplicationHeader);
  },

  








  decode: function decode(data) {
    
    return this.decodeMessageHeader(data);
  },

  encodeMessageHeader: function encodeMessageHeader(data, header) {
    encodeAlternatives(data, header, null, WellKnownHeader, ApplicationHeader);
  },

  






  encode: function encode(data, header) {
    
    this.encodeMessageHeader(data, header);
  },
};







let WellKnownHeader = {
  











  decode: function decode(data) {
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

  






  encode: function encode(data, header) {
    let entry = WSP_HEADER_FIELDS[header.name.toLowerCase()];
    if (!entry) {
      throw new NotWellKnownEncodingError(
        "Well-known-header: not well known header " + header.name);
    }

    ShortInteger.encode(data, entry.number);
    encodeAlternatives(data, header.value, null, entry.coder, TextValue);
  },
};







let ApplicationHeader = {
  








  decode: function decode(data) {
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

  








  encode: function encode(data, header) {
    if (!header.name) {
      throw new CodeError("Application-header: empty header name");
    }

    TokenText.encode(data, header.name);
    TextString.encode(data, header.value);
  },
};







let FieldName = {
  








  decode: function decode(data) {
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

  





  encode: function encode(data, name) {
    let entry = WSP_HEADER_FIELDS[name.toLowerCase()];
    if (entry) {
      ShortInteger.encode(data, entry.number);
    } else {
      TokenText.encode(data, name);
    }
  },
};









let AcceptCharsetValue = {
  





  decodeAnyCharset: function decodeAnyCharset(data) {
    Octet.decodeEqualTo(data, 128);
    return {charset: "*"};
  },

  









  decodeConstrainedCharset: function decodeConstrainedCharset(data) {
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

  






  decodeAcceptCharsetGeneralForm: function decodeAcceptCharsetGeneralForm(data) {
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

  






  decode: function decode(data) {
    let begin = data.offset;
    try {
      return this.decodeConstrainedCharset(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeAcceptCharsetGeneralForm(data);
    }
  },

  





  encodeAnyCharset: function encodeAnyCharset(data, value) {
    if (!value || !value.charset || (value.charset === "*")) {
      Octet.encode(data, 128);
      return;
    }

    throw new CodeError("Any-charset: invalid value " + value);
  },
};






let WellKnownCharset = {
  









  decode: function decode(data) {
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

  




  encode: function encode(data, value) {
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
















let ContentTypeValue = {
  










  decodeConstrainedMedia: function decodeConstrainedMedia(data) {
    return {
      media: TypeValue.decode(data),
      params: null,
    };
  },

  









  decodeMedia: function decodeMedia(data) {
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

  










  decodeMediaType: function decodeMediaType(data, end) {
    let media = this.decodeMedia(data);
    let params = Parameter.decodeMultiple(data, end);

    return {
      media: media,
      params: params,
    };
  },

  








  decodeContentGeneralForm: function decodeContentGeneralForm(data) {
    let length = ValueLength.decode(data);
    let end = data.offset + length;

    let value = this.decodeMediaType(data, end);

    if (data.offset != end) {
      data.offset = end;
    }

    return value;
  },

  








  decode: function decode(data) {
    let begin = data.offset;

    try {
      return this.decodeConstrainedMedia(data);
    } catch (e) {
      data.offset = begin;
      return this.decodeContentGeneralForm(data);
    }
  },

  





  encodeConstrainedMedia: function encodeConstrainedMedia(data, value) {
    if (value.params) {
      throw new CodeError("Constrained-media: should use general form instread");
    }

    TypeValue.encode(data, value.media);
  },

  





  encodeMediaType: function encodeMediaType(data, value) {
    let entry = WSP_WELL_KNOWN_CONTENT_TYPES[value.media.toLowerCase()];
    if (entry) {
      IntegerValue.encode(data, entry.number);
    } else {
      NullTerminatedTexts.encode(data, value.media);
    }

    Parameter.encodeMultiple(data, value.params);
  },

  





  encodeContentGeneralForm: function encodeContentGeneralForm(data, value) {
    let begin = data.offset;
    this.encodeMediaType(data, value);

    
    
    let len = data.offset - begin;
    data.offset = begin;

    ValueLength.encode(data, len);
    this.encodeMediaType(data, value);
  },

  





  encode: function encode(data, value) {
    let begin = data.offset;

    try {
      this.encodeConstrainedMedia(data, value);
    } catch (e) {
      data.offset = begin;
      this.encodeContentGeneralForm(data, value);
    }
  },
};







let ApplicationIdValue = {
  









  decode: function decode(data) {
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

let PduHelper = {
  












  parseHeaders: function parseHeaders(data, end, headers) {
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

  







  parsePushHeaders: function parsePushHeaders(data, msg) {
    if (!msg.headers) {
      msg.headers = {};
    }

    let headersLen = UintVar.decode(data);
    let headersEnd = data.offset + headersLen;

    let contentType = ContentTypeValue.decode(data);
    msg.headers["content-type"] = contentType;

    msg.headers = this.parseHeaders(data, headersEnd, msg.headers);
  },

  








  parseMultiPart: function parseMultiPart(data) {
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

        let content = Octet.decodeMultiple(data, contentEnd);

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

  









  parse: function parse(data, isSessionless, msg) {
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

  







  appendArrayToMultiStream: function appendArrayToMultiStream(multiStream, array, length) {
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

  







  composeMultiPart: function composeMultiPart(multiStream, parts) {
    
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
      UintVar.encode(data, headersLen);
      UintVar.encode(data, part.content.length);

      
      let slice1 = data.array.slice(headersLen);
      let slice2 = data.array.slice(0, headersLen);
      data.array = slice1.concat(slice2);
      debug("Encoded per-part header: " + JSON.stringify(data.array));

      
      this.appendArrayToMultiStream(multiStream, data.array, data.offset);
      
      this.appendArrayToMultiStream(multiStream, part.content, part.content.length);
    }
  },
};






const WSP_HEADER_FIELDS = (function () {
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



const WSP_WELL_KNOWN_CONTENT_TYPES = (function () {
  let types = {};

  function add(type, number) {
    let entry = {
      type: type,
      number: number,
    };
    types[type] = types[number] = entry;
  }

  
  
  add("application/vnd.wap.multipart.mixed", 0x23);

  
  add("application/vnd.wap.multipart.related", 0x33);

  
  add("application/vnd.wap.mms-message", 0x3E);

  return types;
})();





const WSP_WELL_KNOWN_PARAMS = (function () {
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




const WSP_WELL_KNOWN_CHARSETS = (function () {
  let charsets = {};

  function add(name, number, converter) {
    let entry = {
      name: name,
      number: number,
      converter: converter,
    };

    charsets[name] = charsets[number] = entry;
  }

  add("ansi_x3.4-1968",     3, null);
  add("iso_8859-1:1987",    4, "ISO-8859-1");
  add("utf-8",            106, "UTF-8");
  add("windows-1252",    2252, "windows-1252");

  return charsets;
})();



const OMNA_PUSH_APPLICATION_IDS = (function () {
  let ids = {};

  function add(urn, number) {
    let entry = {
      urn: urn,
      number: number,
    };

    ids[urn] = ids[number] = entry;
  }

  add("x-wap-application:mms.ua", 0x04);

  return ids;
})();

let debug;
if (DEBUG) {
  debug = function (s) {
    dump("-@- WspPduHelper: " + s + "\n");
  };
} else {
  debug = function (s) {};
}

const EXPORTED_SYMBOLS = ALL_CONST_SYMBOLS.concat([
  
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

