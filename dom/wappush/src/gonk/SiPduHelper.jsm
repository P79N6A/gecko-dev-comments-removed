



"use strict";

const {classes: Cc, interfaces: Ci, utils: Cu, results: Cr} = Components;

let WSP = {};
Cu.import("resource://gre/modules/WspPduHelper.jsm", WSP);
let WBXML = {};
Cu.import("resource://gre/modules/WbxmlPduHelper.jsm", WBXML);


let DEBUG = WBXML.DEBUG_ALL | false;






const PUBLIC_IDENTIFIER_SI    = "-//WAPFORUM//DTD SI 1.0//EN";












this.OpaqueAsDate = {
  decode: function decode_opaque_as_date(data) {
    let size = WSP.UintVar.decode(data);
    let dateBuf = [0, 0, 0, 0, 0, 0, 0];

    
    if (size > dateBuf.length)
      size = dateBuf.length

    
    for (let i = 0; i < size; i++) {
      dateBuf[i] = WSP.Octet.decode(data);
    }

    
    let year = ((dateBuf[0] >> 4) & 0x0F) * 1000 + (dateBuf[0] & 0x0F) * 100 +
               ((dateBuf[1] >> 4) & 0x0F) * 10 + (dateBuf[1] & 0x0F);
    let month = ((dateBuf[2] >> 4) & 0x0F) * 10 + (dateBuf[2] & 0x0F);
    let date = ((dateBuf[3] >> 4) & 0x0F) * 10 + (dateBuf[3] & 0x0F);
    let hour = ((dateBuf[4] >> 4) & 0x0F) * 10 + (dateBuf[4] & 0x0F);
    let minute = ((dateBuf[5] >> 4) & 0x0F) * 10 + (dateBuf[5] & 0x0F);
    let second = ((dateBuf[6] >> 4) & 0x0F) * 10 + (dateBuf[6] & 0x0F);
    let dateValue = new Date(Date.UTC(year, month - 1, date, hour, minute, second));

    return dateValue.toISOString().replace(".000", "");
  },
};

this.PduHelper = {

  











  parse: function parse_si(data, contentType, msg) {
    if (!msg) {
      msg = {};
    }

    




    if (!contentType || contentType === "application/vnd.wap.sic") {
      let globalTokenOverride = {};
      globalTokenOverride[0xC3] = {
        number: 0xC3,
        coder: OpaqueAsDate
      };

      let appToken = {
        publicId: PUBLIC_IDENTIFIER_SI,
        tagToken: SI_TAG_FIELDS,
        attrToken: SI_ATTRIBUTE_FIELDS,
        globalTokenOverride: globalTokenOverride
      }

      WBXML.PduHelper.parse(data, appToken, msg);

      return msg;
    }

    


    if (contentType === "text/vnd.wap.si") {
      let stringData = WSP.Octet.decodeMultiple(data, data.array.length);
      msg.publicId = PUBLIC_IDENTIFIER_SI;
      msg.content = WSP.PduHelper.decodeStringContent(stringData, "UTF-8");
      return msg;
    }

    return null;
  },

  







  compose: function compose_si(multiStream, msg) {
    
    return null;
  },
};






const SI_TAG_FIELDS = (function () {
  let names = {};
  function add(name, number) {
    let entry = {
      name: name,
      number: number,
    };
    names[name] = names[number] = entry;
  }

  add("si",           0x05);
  add("indication",   0x06);
  add("info",         0x07);
  add("item",         0x08);

  return names;
})();






const SI_ATTRIBUTE_FIELDS = (function () {
  let names = {};
  function add(name, value, number) {
    let entry = {
      name: name,
      value: value,
      number: number,
    };
    names[name] = names[number] = entry;
  }

  add("action",       "signal-none",    0x05);
  add("action",       "signal-low",     0x06);
  add("action",       "signal-medium",  0x07);
  add("action",       "signal-high",    0x08);
  add("action",       "delete",         0x09);
  add("created",      "",               0x0A);
  add("href",         "",               0x0B);
  add("href",         "http://",        0x0C);
  add("href",         "http://www.",    0x0D);
  add("href",         "https://",       0x0E);
  add("href",         "https://www.",   0x0F);
  add("si-expires",   "",               0x10);
  add("si-id",        "",               0x11);
  add("class",        "",               0x12);
  add("",             ".com/",          0x85);
  add("",             ".edu/",          0x86);
  add("",             ".net/",          0x87);
  add("",             ".org/",          0x88);

  return names;
})();

this.EXPORTED_SYMBOLS = [
  
  "PduHelper",
];
