



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
    let msg = {};

    




    if (!contentType || contentType === "application/vnd.wap.slc") {
      let appToken = {
        publicId: PUBLIC_IDENTIFIER_SL,
        tagToken: SL_TAG_FIELDS,
        attrToken: SL_ATTRIBUTE_FIELDS,
        globalTokenOverride: null
      }

      WBXML.PduHelper.parse(data, appToken, msg);

      msg.contentType = "text/vnd.wap.sl";
      return msg;
    }

    


    if (contentType === "text/vnd.wap.sl") {
      let stringData = WSP.Octet.decodeMultiple(data, data.array.length);
      msg.publicId = PUBLIC_IDENTIFIER_SL;
      msg.content = WSP.PduHelper.decodeStringContent(stringData, "UTF-8");
      msg.contentType = "text/vnd.wap.sl";
      return msg;
    }

    return null;
  },

  







  compose: function compose_sl(multiStream, msg) {
    
    return null;
  },
};






const SL_TAG_FIELDS = (function () {
  let names = {};
  function add(name, number) {
    let entry = {
      name: name,
      number: number,
    };
    names[name] = names[number] = entry;
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
    names[name] = names[number] = entry;
  }

  add("action",       "execute-low",    0x05);
  add("action",       "execute-high",   0x06);
  add("action",       "cache",          0x07);
  add("href",         "",               0x08);
  add("href",         "http://",        0x09);
  add("href",         "http://www.",    0x0A);
  add("href",         "https://",       0x0B);
  add("href",         "https://www.",   0x0C);
  add("",             ".com/",          0x85);
  add("",             ".edu/",          0x86);
  add("",             ".net/",          0x87);
  add("",             ".org/",          0x88);

  return names;
})();

this.EXPORTED_SYMBOLS = [
  
  "PduHelper",
];
