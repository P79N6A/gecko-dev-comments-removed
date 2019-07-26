



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let WSP = {};
Cu.import("resource://gre/modules/WspPduHelper.jsm", WSP);
let WBXML = {};
Cu.import("resource://gre/modules/WbxmlPduHelper.jsm", WBXML);


let DEBUG = WBXML.DEBUG_ALL | false;






const PUBLIC_IDENTIFIER_SL    = "-//WAPFORUM//DTD SL 1.0//EN";

this.PduHelper = {

  












  parse: function parse_sl(data, contentType) {
    
    let msg = {
      contentType: contentType
    };

    




    if (contentType === "application/vnd.wap.slc") {
      let appToken = {
        publicId: PUBLIC_IDENTIFIER_SL,
        tagTokenList: SL_TAG_FIELDS,
        attrTokenList: SL_ATTRIBUTE_FIELDS,
        valueTokenList: SL_VALUE_FIELDS,
        globalTokenOverride: null
      }

      try {
        let parseResult = WBXML.PduHelper.parse(data, appToken);
        msg.content = parseResult.content;
        msg.contentType = "text/vnd.wap.sl";
      } catch (e) {
        
        msg.content = data.array;
      }

      return msg;
    }

    


    try {
      let stringData = WSP.Octet.decodeMultiple(data, data.array.length);
      msg.content = WSP.PduHelper.decodeStringContent(stringData, "UTF-8");
    } catch (e) {
      
      msg.content = data.array;
    }
    return msg;

  }
};






const SL_TAG_FIELDS = (function () {
  let names = {};
  function add(name, number) {
    let entry = {
      name: name,
      number: number,
    };
    names[number] = entry;
  }

  add("sl",           0x05);

  return names;
})();






const SL_ATTRIBUTE_FIELDS = (function () {
  let names = {};
  function add(name, value, number) {
    let entry = {
      name: name,
      value: value,
      number: number,
    };
    names[number] = entry;
  }

  add("action",       "execute-low",    0x05);
  add("action",       "execute-high",   0x06);
  add("action",       "cache",          0x07);
  add("href",         "",               0x08);
  add("href",         "http://",        0x09);
  add("href",         "http://www.",    0x0A);
  add("href",         "https://",       0x0B);
  add("href",         "https://www.",   0x0C);

  return names;
})();

const SL_VALUE_FIELDS = (function () {
  let names = {};
  function add(value, number) {
    let entry = {
      value: value,
      number: number,
    };
    names[number] = entry;
  }

  add(".com/",          0x85);
  add(".edu/",          0x86);
  add(".net/",          0x87);
  add(".org/",          0x88);

  return names;
})();

this.EXPORTED_SYMBOLS = [
  
  "PduHelper",
];
