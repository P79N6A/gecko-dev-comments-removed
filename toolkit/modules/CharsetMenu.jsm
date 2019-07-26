



this.EXPORTED_SYMBOLS = [ "CharsetMenu" ];

const { classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyGetter(this, "gBundle", function() {
  const kUrl = "chrome://global/locale/charsetMenu.properties";
  return Services.strings.createBundle(kUrl);
});

const kAutoDetectors = [
  ["off", "off"],
  ["ja", "ja_parallel_state_machine"],
  ["ru", "ruprob"],
  ["uk", "ukprob"]
];









const kEncodings = new Set([
  
  "UTF-8",
  "windows-1252",
  
  "windows-1256",
  "ISO-8859-6",
  
  "windows-1257",
  "ISO-8859-4",
  
  
  "windows-1250",
  "ISO-8859-2",
  
  "gbk",
  
  "Big5",
  
  "windows-1251",
  "ISO-8859-5",
  "KOI8-R",
  "KOI8-U",
  "IBM866", 
  
  
  "windows-1253",
  "ISO-8859-7",
  
  "windows-1255",
  "ISO-8859-8",
  
  "Shift_JIS",
  "EUC-JP",
  "ISO-2022-JP",
  
  "EUC-KR",
  
  "windows-874",
  
  "windows-1254",
  
  "windows-1258",
  
  
  
  
  
  
  
  
]);


const kPinned = [
  "UTF-8",
  "windows-1252"
];

kPinned.forEach(x => kEncodings.delete(x));


let gDetectorInfoCache, gCharsetInfoCache, gPinnedInfoCache;

let CharsetMenu = {
  build: function(parent, idPrefix="", showAccessKeys=true) {
    function createDOMNode(doc, nodeInfo) {
      let node = doc.createElement("menuitem");
      node.setAttribute("type", "radio");
      node.setAttribute("name", nodeInfo.name);
      node.setAttribute("label", nodeInfo.label);
      if (showAccessKeys && nodeInfo.accesskey) {
        node.setAttribute("accesskey", nodeInfo.accesskey);
      }
      if (idPrefix) {
        node.id = idPrefix + nodeInfo.id;
      } else {
        node.id = nodeInfo.id;
      }
      return node;
    }

    if (parent.childElementCount > 0) {
      
      return;
    }
    let doc = parent.ownerDocument;

    let menuNode = doc.createElement("menu");
    menuNode.setAttribute("label", gBundle.GetStringFromName("charsetMenuAutodet"));
    if (showAccessKeys) {
      menuNode.setAttribute("accesskey", gBundle.GetStringFromName("charsetMenuAutodet.key"));
    }
    parent.appendChild(menuNode);

    let menuPopupNode = doc.createElement("menupopup");
    menuNode.appendChild(menuPopupNode);

    this._ensureDataReady();
    gDetectorInfoCache.forEach(detectorInfo => menuPopupNode.appendChild(createDOMNode(doc, detectorInfo)));
    parent.appendChild(doc.createElement("menuseparator"));
    gPinnedInfoCache.forEach(charsetInfo => parent.appendChild(createDOMNode(doc, charsetInfo)));
    parent.appendChild(doc.createElement("menuseparator"));
    gCharsetInfoCache.forEach(charsetInfo => parent.appendChild(createDOMNode(doc, charsetInfo)));
  },

  getData: function() {
    this._ensureDataReady();
    return {
      detectors: gDetectorInfoCache,
      pinnedCharsets: gPinnedInfoCache,
      otherCharsets: gCharsetInfoCache
    };
  },

  _ensureDataReady: function() {
    if (!gDetectorInfoCache) {
      gDetectorInfoCache = this.getDetectorInfo();
      gPinnedInfoCache = this.getCharsetInfo(kPinned, false);
      gCharsetInfoCache = this.getCharsetInfo([...kEncodings]);
    }
  },

  getDetectorInfo: function() {
    return kAutoDetectors.map(([detectorName, nodeId]) => ({
      id: "chardet." + nodeId,
      label: this._getDetectorLabel(detectorName),
      accesskey: this._getDetectorAccesskey(detectorName),
      name: "detectorGroup",
    }));
  },

  getCharsetInfo: function(charsets, sort=true) {
    let list = charsets.map(charset => ({
      id: "charset." + charset,
      label: this._getCharsetLabel(charset),
      accesskey: this._getCharsetAccessKey(charset),
      name: "charsetGroup",
    }));

    if (!sort) {
      return list;
    }

    list.sort(function (a, b) {
      let titleA = a.label;
      let titleB = b.label;
      
      
      let index;
      if ((index = titleA.indexOf("(")) > -1) {
        titleA = titleA.substring(0, index);
      }
      if ((index = titleB.indexOf("(")) > -1) {
        titleA = titleB.substring(0, index);
      }
      let comp = titleA.localeCompare(titleB);
      if (comp) {
        return comp;
      }
      
      
      
      if (a.id < b.id) {
        return 1;
      }
      if (b.id < a.id) {
        return -1;
      }
      return 0;
    });
    return list;
  },

  _getDetectorLabel: function(detector) {
    try {
      return gBundle.GetStringFromName("charsetMenuAutodet." + detector);
    } catch (ex) {}
    return detector;
  },
  _getDetectorAccesskey: function(detector) {
    try {
      return gBundle.GetStringFromName("charsetMenuAutodet." + detector + ".key");
    } catch (ex) {}
    return "";
  },

  _getCharsetLabel: function(charset) {
    if (charset == "gbk") {
      
      charset = "gbk.bis";
    }
    try {
      return gBundle.GetStringFromName(charset);
    } catch (ex) {}
    return charset;
  },
  _getCharsetAccessKey: function(charset) {
    if (charset == "gbk") {
      
      charset = "gbk.bis";
    }
    try {
      return gBundle.GetStringFromName(charset + ".key");
    } catch (ex) {}
    return "";
  },
};

Object.freeze(CharsetMenu);

