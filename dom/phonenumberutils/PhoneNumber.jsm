





"use strict";

this.EXPORTED_SYMBOLS = ["PhoneNumber"];

Components.utils.import("resource://gre/modules/PhoneNumberMetaData.jsm");

this.PhoneNumber = (function (dataBase) {
  
  'use strict';

  
  const MIN_LENGTH_FOR_NSN = 2;

  const STAR_SIGN = "*";
  const UNICODE_DIGITS = /[\uFF10-\uFF19\u0660-\u0669\u06F0-\u06F9]/g;
  const NON_ALPHA_CHARS = /[^a-zA-Z]/g;
  const NON_DIALABLE_CHARS = /[^,#+\*\d]/g;
  const PLUS_CHARS = "+\uFF0B";
  const BACKSLASH = /\\/g;
  const SPLIT_FIRST_GROUP = /^(\d+)(.*)$/;

  







  const VALID_PUNCTUATION = "-x\u2010-\u2015\u2212\u30FC\uFF0D-\uFF0F \u00A0"
                          + "\u200B\u2060\u3000()\uFF08\uFF09\uFF3B\uFF3D."
                          + "\\[\\]/~\u2053\u223C\uFF5E";
  const VALID_DIGITS = "0-9\uFF10-\uFF19\u0660-\u0669\u06F0-\u06F9";
  const VALID_ALPHA = "a-zA-Z";

  





















  const MIN_LENGTH_PHONE_NUMBER
    = "[" + VALID_DIGITS + "]{" + MIN_LENGTH_FOR_NSN + "}";
  const VALID_PHONE_NUMBER
    = "[" + PLUS_CHARS + "]*"
    + "(?:[" + VALID_PUNCTUATION + STAR_SIGN + "]*" + "[" + VALID_DIGITS + "]){3,}"
    + "[" + VALID_PUNCTUATION + STAR_SIGN + VALID_ALPHA + VALID_DIGITS + "]*";

  



  const CAPTURING_EXTN_DIGITS = "([" + VALID_DIGITS + "]{1,7})";

  













  const EXTN_PATTERNS_FOR_PARSING
    = ";ext=" + CAPTURING_EXTN_DIGITS + "|" + "[ \u00A0\\t,]*"
    + "(?:e?xt(?:ensi(?:o\u0301?|\u00F3))?n?|\uFF45?\uFF58\uFF54\uFF4E?|"
    + "[,x\uFF58#\uFF03~\uFF5E]|int|anexo|\uFF49\uFF4E\uFF54)"
    + "[:\\.\uFF0E]?[ \u00A0\\t,-]*" + CAPTURING_EXTN_DIGITS + "#?|"
    + "[- ]+([" + VALID_DIGITS + "]{1,5})#";

  const VALID_ALPHA_PATTERN = new RegExp("[" + VALID_ALPHA + "]", "g");
  const LEADING_PLUS_CHARS_PATTERN = new RegExp("^[" + PLUS_CHARS + "]+", "g");

  
  const VENEZUELA_SHORT_NUMBER = "\\*[" + VALID_DIGITS + "]";

  
  
  
  const VALID_PHONE_NUMBER_PATTERN =
    new RegExp("^" + MIN_LENGTH_PHONE_NUMBER + "$|"
               + "^" + VENEZUELA_SHORT_NUMBER + "$|"
               + "^" + VALID_PHONE_NUMBER + "(?:" + EXTN_PATTERNS_FOR_PARSING + ")?$", "i");

  
  
  
  const META_DATA_ENCODING = ["region",
                              "^internationalPrefix",
                              "nationalPrefix",
                              "^nationalPrefixForParsing",
                              "nationalPrefixTransformRule",
                              "nationalPrefixFormattingRule",
                              "^possiblePattern$",
                              "^nationalPattern$",
                              "formats"];

  const FORMAT_ENCODING = ["^pattern$",
                           "nationalFormat",
                           "^leadingDigits",
                           "nationalPrefixFormattingRule",
                           "internationalFormat"];

  var regionCache = Object.create(null);

  
  
  function ParseArray(array, encoding, obj) {
    for (var n = 0; n < encoding.length; ++n) {
      var value = array[n];
      if (!value)
        continue;
      var field = encoding[n];
      var fieldAlpha = field.replace(NON_ALPHA_CHARS, "");
      if (field != fieldAlpha)
        value = new RegExp(field.replace(fieldAlpha, value));
      obj[fieldAlpha] = value;
    }
    return obj;
  }

  
  
  function ParseMetaData(countryCode, md) {
    var array = eval(md.replace(BACKSLASH, "\\\\"));
    md = ParseArray(array,
                    META_DATA_ENCODING,
                    { countryCode: countryCode });
    regionCache[md.region] = md;
    return md;
  }

  
  
  function ParseFormat(md) {
    var formats = md.formats;
    
    if (!(Array.isArray(formats[0])))
      return;
    for (var n = 0; n < formats.length; ++n) {
      formats[n] = ParseArray(formats[n],
                              FORMAT_ENCODING,
                              {});
    }
  }

  
  
  
  
  function FindMetaDataForRegion(region) {
    
    
    var md = regionCache[region];
    if (md)
      return md;
    for (var countryCode in dataBase) {
      var entry = dataBase[countryCode];
      
      
      
      
      
      
      
      if (Array.isArray(entry)) {
        for (var n = 0; n < entry.length; n++) {
          if (typeof entry[n] == "string" && entry[n].substr(2,2) == region) {
            if (n > 0) {
              
              
              
              if (typeof entry[0] == "string")
                entry[0] = ParseMetaData(countryCode, entry[0]);
              let formats = entry[0].formats;
              let current = ParseMetaData(countryCode, entry[n]);
              current.formats = formats;
              return entry[n] = current;
            }

            entry[n] = ParseMetaData(countryCode, entry[n]);
            return entry[n];
          }
        }
        continue;
      }
      if (typeof entry == "string" && entry.substr(2,2) == region)
        return dataBase[countryCode] = ParseMetaData(countryCode, entry);
    }
  }

  
  
  function FormatNumber(regionMetaData, number, intl) {
    
    
    ParseFormat(regionMetaData);
    var formats = regionMetaData.formats;
    for (var n = 0; n < formats.length; ++n) {
      var format = formats[n];
      
      
      if (format.leadingDigits && !format.leadingDigits.test(number))
        continue;
      if (!format.pattern.test(number))
        continue;
      if (intl) {
        
        
        var internationalFormat = format.internationalFormat;
        if (!internationalFormat)
          internationalFormat = format.nationalFormat;
        
        
        
        if (internationalFormat == "NA")
          return null;
        
        number = "+" + regionMetaData.countryCode + " " +
                 number.replace(format.pattern, internationalFormat);
      } else {
        number = number.replace(format.pattern, format.nationalFormat);
        
        
        var nationalPrefixFormattingRule = regionMetaData.nationalPrefixFormattingRule;
        if (format.nationalPrefixFormattingRule)
          nationalPrefixFormattingRule = format.nationalPrefixFormattingRule;
        if (nationalPrefixFormattingRule) {
          
          
          
          var match = number.match(SPLIT_FIRST_GROUP);
          var firstGroup = match[1];
          var rest = match[2];
          var prefix = nationalPrefixFormattingRule;
          prefix = prefix.replace("$NP", regionMetaData.nationalPrefix);
          prefix = prefix.replace("$FG", firstGroup);
          number = prefix + rest;
        }
      }
      return (number == "NA") ? null : number;
    }
    return null;
  }

  function NationalNumber(regionMetaData, number) {
    this.region = regionMetaData.region;
    this.regionMetaData = regionMetaData;
    this.nationalNumber = number;
  }

  
  
  
  
  NationalNumber.prototype = {
    
    get internationalFormat() {
      var value = FormatNumber(this.regionMetaData, this.nationalNumber, true);
      Object.defineProperty(this, "internationalFormat", { value: value, enumerable: true });
      return value;
    },
    
    get nationalFormat() {
      var value = FormatNumber(this.regionMetaData, this.nationalNumber, false);
      Object.defineProperty(this, "nationalFormat", { value: value, enumerable: true });
      return value;
    },
    
    get internationalNumber() {
      var value = this.internationalFormat ? this.internationalFormat.replace(NON_DIALABLE_CHARS, "")
                                           : null;
      Object.defineProperty(this, "internationalNumber", { value: value, enumerable: true });
      return value;
    }
  };

  
  
  function NormalizeNumber(number) {
    number = number.replace(UNICODE_DIGITS,
                            function (ch) {
                              return String.fromCharCode(48 + (ch.charCodeAt(0) & 0xf));
                            });
    number = number.replace(VALID_ALPHA_PATTERN,
                            function (ch) {
                              return (ch.toLowerCase().charCodeAt(0) - 97)/3+2 | 0;
                            });
    number = number.replace(LEADING_PLUS_CHARS_PATTERN, "+");
    number = number.replace(NON_DIALABLE_CHARS, "");
    return number;
  }

  
  function IsValidNumber(number, md) {
    return md.possiblePattern.test(number);
  }

  
  function IsNationalNumber(number, md) {
    return IsValidNumber(number, md) && md.nationalPattern.test(number);
  }

  
  
  function ParseCountryCode(number) {
    for (var n = 1; n <= 3; ++n) {
      var cc = number.substr(0,n);
      if (dataBase[cc])
        return cc;
    }
    return null;
  }

  
  
  function ParseInternationalNumber(number) {
    var ret;

    
    var countryCode = ParseCountryCode(number);
    if (!countryCode)
      return null;
    number = number.substr(countryCode.length);

    
    
    var entry = dataBase[countryCode];
    if (Array.isArray(entry)) {
      for (var n = 0; n < entry.length; ++n) {
        if (typeof entry[n] == "string")
          entry[n] = ParseMetaData(countryCode, entry[n]);
        ret = ParseNationalNumber(number, entry[n])
        if (ret)
          return ret;
      }
      return null;
    }
    if (typeof entry == "string")
      entry = dataBase[countryCode] = ParseMetaData(countryCode, entry);
    return ParseNationalNumber(number, entry);
  }

  
  
  
  function ParseNationalNumber(number, md) {
    if (!md.possiblePattern.test(number) ||
        !md.nationalPattern.test(number)) {
      return null;
    }
    
    return new NationalNumber(md, number);
  }

  
  
  function ParseNumber(number, defaultRegion) {
    var ret;

    
    number = NormalizeNumber(number);

    
    if (!defaultRegion && number[0] !== '+')
      return null;

    
    if (number[0] === '+')
      return ParseInternationalNumber(number.replace(LEADING_PLUS_CHARS_PATTERN, ""));

    
    var md = FindMetaDataForRegion(defaultRegion.toUpperCase());

    
    
    
    if (md.internationalPrefix.test(number)) {
      var possibleNumber = number.replace(md.internationalPrefix, "");
      ret = ParseInternationalNumber(possibleNumber)
      if (ret)
        return ret;
    }

    
    
    
    if (md.nationalPrefixForParsing) {
      
      var withoutPrefix = number.replace(md.nationalPrefixForParsing,
                                         md.nationalPrefixTransformRule);
      ret = ParseNationalNumber(withoutPrefix, md)
      if (ret)
        return ret;
    } else {
      
      
      var nationalPrefix = md.nationalPrefix;
      if (nationalPrefix && number.indexOf(nationalPrefix) == 0 &&
          (ret = ParseNationalNumber(number.substr(nationalPrefix.length), md))) {
        return ret;
      }
    }
    ret = ParseNationalNumber(number, md)
    if (ret)
      return ret;

    
    
    if (md.possiblePattern.test(number))
      return new NationalNumber(md, number);

    
    
    ret = ParseInternationalNumber(number)
    if (ret)
      return ret;

    
    return null;
  }

  function IsViablePhoneNumber(number) {
    if (number == null || number.length < MIN_LENGTH_FOR_NSN) {
      return false;
    }

    let matchedGroups = number.match(VALID_PHONE_NUMBER_PATTERN);
    if (matchedGroups && matchedGroups[0].length == number.length) {
      return true;
    }

    return false;
  }

  return {
    IsViable: IsViablePhoneNumber,
    Parse: ParseNumber,
    Normalize: NormalizeNumber
  };
})(PHONE_NUMBER_META_DATA);
