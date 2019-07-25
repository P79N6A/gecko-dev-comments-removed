# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http:
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is aboutSupport.xhtml.
#
# The Initial Developer of the Original Code is
# Mozilla Foundation
# Portions created by the Initial Developer are Copyright (C) 2009
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Curtis Bartley <cbartley@mozilla.com>
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

const Cc = Components.classes;
const Ci = Components.interfaces;

let gPrefService = Cc["@mozilla.org/preferences-service;1"]
                     .getService(Ci.nsIPrefService)
                     .QueryInterface(Ci.nsIPrefBranch2);

const ELLIPSIS = gPrefService.getComplexValue("intl.ellipsis",
                                              Ci.nsIPrefLocalizedString).data;





const PREFS_WHITELIST = [
  "accessibility.",
  "browser.fixup.",
  "browser.history_expire_",
  "browser.link.open_newwindow",
  "browser.mousewheel.",
  "browser.places.",
  "browser.startup.homepage",
  "browser.tabs.",
  "browser.zoom.",
  "dom.",
  "extensions.checkCompatibility",
  "extensions.lastAppVersion",
  "font.",
  "general.useragent.",
  "gfx.color_management.mode",
  "javascript.",
  "keyword.",
  "layout.css.dpi",
  "network.",
  "places.",
  "print.",
  "privacy.",
  "security."
];


const PREFS_BLACKLIST = [
  /^network[.]proxy[.]/,
  /[.]print_to_filename$/,
];

window.onload = function () {
  
  let urlFormatter = Cc["@mozilla.org/toolkit/URLFormatterService;1"]
                       .getService(Ci.nsIURLFormatter);
  let supportUrl = urlFormatter.formatURLPref("app.support.baseURL");

  
  document.getElementById("application-box").textContent = Application.name;
  document.getElementById("version-box").textContent = Application.version;
  document.getElementById("supportLink").href = supportUrl;

  
  populatePreferencesSection();
  populateExtensionsSection();
  populateGraphicsSection();
}

function populateExtensionsSection() {
  Application.getExtensions(function (extensions) {
    let all = extensions.all;
    let trExtensions = [];
    for (let i = 0; i < all.length; i++) {
      let extension = all[i];
      let tr = createParentElement("tr", [
        createElement("td", extension.name),
        createElement("td", extension.version),
        createElement("td", extension.enabled),
        createElement("td", extension.id),
      ]);
      trExtensions.push(tr);
    }
    appendChildren(document.getElementById("extensions-tbody"), trExtensions);
  });
}

function populatePreferencesSection() {
  let modifiedPrefs = getModifiedPrefs();

  function comparePrefs(pref1, pref2) {
    if (pref1.name < pref2.name)
      return -1;
    if (pref1.name > pref2.name)
      return 1;
    return 0;
  }

  let sortedPrefs = modifiedPrefs.sort(comparePrefs);

  let trPrefs = [];
  sortedPrefs.forEach(function (pref) {
    let tdName = createElement("td", pref.name, "pref-name");
    let tdValue = createElement("td", formatPrefValue(pref.value), "pref-value");
    let tr = createParentElement("tr", [tdName, tdValue]);
    trPrefs.push(tr);
  });

  appendChildren(document.getElementById("prefs-tbody"), trPrefs);
}

function populateGraphicsSection() {
  function createHeader(name)
  {
    let elem = createElement("th", name);
    elem.className = "column";
    return elem;
  }
  
  try {
    
    let gfxInfo = Cc["@mozilla.org/gfx/info;1"].getService(Ci.nsIGfxInfo);
    let trGraphics = [];
    var SBS = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);
    var bundle = SBS.createBundle("chrome://global/locale/aboutSupport.properties");
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("adapterDescription")),
      createElement("td", gfxInfo.adapterDescription),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("adapterVendorID")),
      
      createElement("td", String('0000'+gfxInfo.adapterVendorID.toString(16)).slice(-4)),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("adapterDeviceID")),
      
      createElement("td", String('0000'+gfxInfo.adapterDeviceID.toString(16)).slice(-4)),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("adapterRAM")),
      createElement("td", gfxInfo.adapterRAM),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("adapterDrivers")),
      createElement("td", gfxInfo.adapterDriver),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("driverVersion")),
      createElement("td", gfxInfo.adapterDriverVersion),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("driverDate")),
      createElement("td", gfxInfo.adapterDriverDate),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("direct2DEnabled")),
      createElement("td", gfxInfo.D2DEnabled),
    ]));
    trGraphics.push(createParentElement("tr", [
      createHeader(bundle.GetStringFromName("directWriteEnabled")),
      createElement("td", gfxInfo.DWriteEnabled),
    ]));

    appendChildren(document.getElementById("graphics-tbody"), trGraphics);

  } catch (e) {
  }

}


function formatPrefValue(prefValue) {
  
  
  
  
  let maxPrefValueLen = 120;
  let text = "" + prefValue;
  if (text.length > maxPrefValueLen)
    text = text.substring(0, maxPrefValueLen) + ELLIPSIS;
  return text;
}

function getModifiedPrefs() {
  
  
  
  
  let prefNames = getWhitelistedPrefNames();
  let prefs = [Application.prefs.get(prefName)
                      for each (prefName in prefNames)
                          if (gPrefService.prefHasUserValue(prefName)
                            && !isBlacklisted(prefName))];
  return prefs;
}

function getWhitelistedPrefNames() {
  let results = [];
  PREFS_WHITELIST.forEach(function (prefStem) {
    let prefNames = gPrefService.getChildList(prefStem);
    results = results.concat(prefNames);
  });
  return results;
}

function isBlacklisted(prefName) {
  return PREFS_BLACKLIST.some(function (re) re.test(prefName));
}

function createParentElement(tagName, childElems) {
  let elem = document.createElement(tagName);
  appendChildren(elem, childElems);
  return elem;
}

function createElement(tagName, textContent, opt_class) {
  let elem = document.createElement(tagName);
  elem.textContent = textContent;
  elem.className = opt_class || "";
  return elem;
}

function appendChildren(parentElem, childNodes) {
  for (let i = 0; i < childNodes.length; i++)
    parentElem.appendChild(childNodes[i]);
}

function copyContentsToClipboard() {
  
  let contentsDiv = document.getElementById("contents");
  let dataHtml = contentsDiv.innerHTML;
  let dataText = createTextForElement(contentsDiv);

  
  let supportsStringClass = Cc["@mozilla.org/supports-string;1"];
  let ssHtml = supportsStringClass.createInstance(Ci.nsISupportsString);
  let ssText = supportsStringClass.createInstance(Ci.nsISupportsString);

  let transferable = Cc["@mozilla.org/widget/transferable;1"]
                       .createInstance(Ci.nsITransferable);

  
  transferable.addDataFlavor("text/html");
  ssHtml.data = dataHtml;
  transferable.setTransferData("text/html", ssHtml, dataHtml.length * 2);

  
  transferable.addDataFlavor("text/unicode");
  ssText.data = dataText;
  transferable.setTransferData("text/unicode", ssText, dataText.length * 2);

  
  let clipboard = Cc["@mozilla.org/widget/clipboard;1"]
                    .getService(Ci.nsIClipboard);
  clipboard.setData(transferable, null, clipboard.kGlobalClipboard);
}



function createTextForElement(elem) {
  
  let textFragmentAccumulator = [];
  generateTextForElement(elem, "", textFragmentAccumulator);
  let text = textFragmentAccumulator.join("");

  
  
  text = text.replace(/[ \t]+\n/g, "\n");
  text = text.replace(/\n\n\n+/g, "\n\n");

  
#ifdef XP_WIN
  text = text.replace(/\n/g, "\r\n");
#endif

  return text;
}

function generateTextForElement(elem, indent, textFragmentAccumulator) {
  
  if (elem.tagName != "td")
    textFragmentAccumulator.push("\n");

  
  let node = elem.firstChild;
  while (node) {

    if (node.nodeType == Node.TEXT_NODE) {
      
      generateTextForTextNode(node, indent, textFragmentAccumulator);
    }
    else if (node.nodeType == Node.ELEMENT_NODE) {
      
      generateTextForElement(node, indent + "  ", textFragmentAccumulator);
    }

    
    node = node.nextSibling;
  }
}

function generateTextForTextNode(node, indent, textFragmentAccumulator) {
  
  
  let prevNode = node.previousSibling;
  if (!prevNode || prevNode.nodeType == Node.TEXT_NODE)
    textFragmentAccumulator.push("\n" + indent);

  
  
  let text = node.textContent.trim().replace("\n", "\n" + indent, "g");
  textFragmentAccumulator.push(text);
}

function openProfileDirectory() {
  
  let propertiesService = Cc["@mozilla.org/file/directory_service;1"]
                            .getService(Ci.nsIProperties);
  let currProfD = propertiesService.get("ProfD", Ci.nsIFile);
  let profileDir = currProfD.path;

  
  let nsLocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                           "nsILocalFile", "initWithPath");
  new nsLocalFile(profileDir).reveal();
}
