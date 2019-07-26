



this.EXPORTED_SYMBOLS = [ "CharsetMenu" ];

const { classes: Cc, interfaces: Ci, utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
XPCOMUtils.defineLazyGetter(this, "gBundle", function() {
  const kUrl = "chrome://global/locale/charsetMenu.properties";
  return Services.strings.createBundle(kUrl);
});

const kAutoDetectors = [
  ["off", ""],
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

function CharsetComparator(a, b) {
  
  
  let titleA = a.label.replace(/\(.*/, "") + b.value;
  let titleB = b.label.replace(/\(.*/, "") + a.value;
  
  
  return titleA.localeCompare(titleB) || b.value.localeCompare(a.value);
}

function SetDetector(event) {
  let str = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
  str.data = event.target.getAttribute("detector");
  Services.prefs.setComplexValue("intl.charset.detector", Ci.nsISupportsString, str);
}

function UpdateDetectorMenu(event) {
  event.stopPropagation();
  let detector = Services.prefs.getComplexValue("intl.charset.detector", Ci.nsIPrefLocalizedString);
  let menuitem = this.getElementsByAttribute("detector", detector).item(0);
  if (menuitem) {
    menuitem.setAttribute("checked", "true");
  }
}

let gDetectorInfoCache, gCharsetInfoCache, gPinnedInfoCache;

let CharsetMenu = {
  build: function(parent, showAccessKeys=true, showDetector=true) {
    function createDOMNode(doc, nodeInfo) {
      let node = doc.createElement("menuitem");
      node.setAttribute("type", "radio");
      node.setAttribute("name", nodeInfo.name + "Group");
      node.setAttribute(nodeInfo.name, nodeInfo.value);
      node.setAttribute("label", nodeInfo.label);
      if (showAccessKeys && nodeInfo.accesskey) {
        node.setAttribute("accesskey", nodeInfo.accesskey);
      }
      return node;
    }

    if (parent.hasChildNodes()) {
      
      return;
    }
    this._ensureDataReady();
    let doc = parent.ownerDocument;

    if (showDetector) {
      let menuNode = doc.createElement("menu");
      menuNode.setAttribute("label", gBundle.GetStringFromName("charsetMenuAutodet"));
      if (showAccessKeys) {
        menuNode.setAttribute("accesskey", gBundle.GetStringFromName("charsetMenuAutodet.key"));
      }
      parent.appendChild(menuNode);

      let menuPopupNode = doc.createElement("menupopup");
      menuNode.appendChild(menuPopupNode);
      menuPopupNode.addEventListener("command", SetDetector);
      menuPopupNode.addEventListener("popupshown", UpdateDetectorMenu);

      gDetectorInfoCache.forEach(detectorInfo => menuPopupNode.appendChild(createDOMNode(doc, detectorInfo)));
      parent.appendChild(doc.createElement("menuseparator"));
    }

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
      gCharsetInfoCache = this.getCharsetInfo(kEncodings);
    }
  },

  getDetectorInfo: function() {
    return kAutoDetectors.map(([detectorName, nodeId]) => ({
      label: this._getDetectorLabel(detectorName),
      accesskey: this._getDetectorAccesskey(detectorName),
      name: "detector",
      value: nodeId
    }));
  },

  getCharsetInfo: function(charsets, sort=true) {
    let list = [{
      label: this._getCharsetLabel(charset),
      accesskey: this._getCharsetAccessKey(charset),
      name: "charset",
      value: charset
    } for (charset of charsets)];

    if (sort) {
      list.sort(CharsetComparator);
    }
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

  



  foldCharset: function(charset) {
    switch (charset) {
      case "ISO-8859-8-I":
        return "windows-1255";

      case "gb18030":
        return "gbk";

      default:
        return charset;
    }
  },

  update: function(parent, charset) {
    let menuitem = parent.getElementsByAttribute("charset", this.foldCharset(charset)).item(0);
    if (menuitem) {
      menuitem.setAttribute("checked", "true");
    }
  },
};

Object.freeze(CharsetMenu);

