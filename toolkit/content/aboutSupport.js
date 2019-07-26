



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Troubleshoot.jsm");

window.addEventListener("load", function onload(event) {
  window.removeEventListener("load", onload, false);
  Troubleshoot.snapshot(function (snapshot) {
    for (let prop in snapshotFormatters)
      snapshotFormatters[prop](snapshot[prop]);
  });
  populateResetBox();
}, false);




let snapshotFormatters = {

  application: function application(data) {
    $("application-box").textContent = data.name;
    $("useragent-box").textContent = data.userAgent;
    $("supportLink").href = data.supportURL;
    let version = data.version;
    if (data.vendor)
      version += " (" + data.vendor + ")";
    $("version-box").textContent = version;
  },

  extensions: function extensions(data) {
    $.append($("extensions-tbody"), data.map(function (extension) {
      return $.new("tr", [
        $.new("td", extension.name),
        $.new("td", extension.version),
        $.new("td", extension.isActive),
        $.new("td", extension.id),
      ]);
    }));
  },

  modifiedPreferences: function modifiedPreferences(data) {
    $.append($("prefs-tbody"), sortedArrayFromObject(data).map(
      function ([name, value]) {
        return $.new("tr", [
          $.new("td", name, "pref-name"),
          
          
          
          $.new("td", String(value).substr(0, 120), "pref-value"),
        ]);
      }
    ));
  },

  graphics: function graphics(data) {
    
    if ("info" in data) {
      let trs = sortedArrayFromObject(data.info).map(function ([prop, val]) {
        return $.new("tr", [
          $.new("th", prop, "column"),
          $.new("td", String(val)),
        ]);
      });
      $.append($("graphics-info-properties"), trs);
      delete data.info;
    }

    
    if ("failures" in data) {
      $.append($("graphics-failures-tbody"), data.failures.map(function (val) {
        return $.new("tr", [$.new("td", val)]);
      }));
      delete data.failures;
    }

    

    function localizedMsg(msgArray) {
      let nameOrMsg = msgArray.shift();
      try {
        return strings.formatStringFromName(nameOrMsg, msgArray,
                                            msgArray.length);
      }
      catch (err) {}
      return nameOrMsg;
    }

    let out = Object.create(data);
    let strings = stringBundle();

    out.acceleratedWindows =
      data.numAcceleratedWindows + "/" + data.numTotalWindows;
    if (data.windowLayerManagerType)
      out.acceleratedWindows += " " + data.windowLayerManagerType;
    if (data.numAcceleratedWindowsMessage)
      out.acceleratedWindows +=
        " " + localizedMsg(data.numAcceleratedWindowsMessage);
    delete data.numAcceleratedWindows;
    delete data.numTotalWindows;
    delete data.windowLayerManagerType;
    delete data.numAcceleratedWindowsMessage;

    if ("direct2DEnabledMessage" in data) {
      out.direct2DEnabled = localizedMsg(data.direct2DEnabledMessage);
      delete data.direct2DEnabledMessage;
      delete data.direct2DEnabled;
    }

    if ("directWriteEnabled" in data) {
      out.directWriteEnabled = data.directWriteEnabled;
      if ("directWriteVersion" in data)
        out.directWriteEnabled += " (" + data.directWriteVersion + ")";
      delete data.directWriteEnabled;
      delete data.directWriteVersion;
    }

    if ("webglRendererMessage" in data) {
      out.webglRenderer = localizedMsg(data.webglRendererMessage);
      delete data.webglRendererMessage;
      delete data.webglRenderer;
    }

    let localizedOut = {};
    for (let prop in out) {
      let val = out[prop];
      if (typeof(val) == "string" && !val)
        
        continue;
      try {
        var localizedName = strings.GetStringFromName(prop);
      }
      catch (err) {
        
        
        localizedName = prop;
      }
      localizedOut[localizedName] = val;
    }
    let trs = sortedArrayFromObject(localizedOut).map(function ([prop, val]) {
      return $.new("tr", [
        $.new("th", prop, "column"),
        $.new("td", val),
      ]);
    });
    $.append($("graphics-tbody"), trs);
  },

  javaScript: function javaScript(data) {
    $("javascript-incremental-gc").textContent = data.incrementalGCEnabled;
  },

  accessibility: function accessibility(data) {
    $("a11y-activated").textContent = data.isActive;
    $("a11y-force-disabled").textContent = data.forceDisabled || 0;
  },

  libraryVersions: function libraryVersions(data) {
    let strings = stringBundle();
    let trs = [
      $.new("tr", [
        $.new("th", ""),
        $.new("th", strings.GetStringFromName("minLibVersions")),
        $.new("th", strings.GetStringFromName("loadedLibVersions")),
      ])
    ];
    sortedArrayFromObject(data).forEach(
      function ([name, val]) {
        trs.push($.new("tr", [
          $.new("td", name),
          $.new("td", val.minVersion),
          $.new("td", val.version),
        ]));
      }
    );
    $.append($("libversions-tbody"), trs);
  },

  userJS: function userJS(data) {
    if (!data.exists)
      return;
    let userJSFile = Services.dirsvc.get("PrefD", Ci.nsIFile);
    userJSFile.append("user.js");
    $("prefs-user-js-link").href = Services.io.newFileURI(userJSFile).spec;
    $("prefs-user-js-section").style.display = "";
    
    $("prefs-user-js-section").className = "";
  },
};

let $ = document.getElementById.bind(document);

$.new = function $_new(tag, textContentOrChildren, className) {
  let elt = document.createElement(tag);
  if (className)
    elt.className = className;
  if (Array.isArray(textContentOrChildren))
    this.append(elt, textContentOrChildren);
  else
    elt.textContent = String(textContentOrChildren);
  return elt;
};

$.append = function $_append(parent, children) {
  children.forEach(function (c) parent.appendChild(c));
};

function stringBundle() {
  return Services.strings.createBundle(
           "chrome://global/locale/aboutSupport.properties");
}

function sortedArrayFromObject(obj) {
  let tuples = [];
  for (let prop in obj)
    tuples.push([prop, obj[prop]]);
  tuples.sort(function ([prop1, v1], [prop2, v2]) prop1.localeCompare(prop2));
  return tuples;
}

function copyRawDataToClipboard(button) {
  if (button)
    button.disabled = true;
  try {
    Troubleshoot.snapshot(function (snapshot) {
      if (button)
        button.disabled = false;
      let str = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      str.data = JSON.stringify(snapshot, undefined, 2);
      let transferable = Cc["@mozilla.org/widget/transferable;1"].
                         createInstance(Ci.nsITransferable);
      transferable.init(getLoadContext());
      transferable.addDataFlavor("text/unicode");
      transferable.setTransferData("text/unicode", str, str.data.length * 2);
      Cc["@mozilla.org/widget/clipboard;1"].
        getService(Ci.nsIClipboard).
        setData(transferable, null, Ci.nsIClipboard.kGlobalClipboard);
#ifdef ANDROID
      
      let message = {
        type: "Toast:Show",
        message: stringBundle().GetStringFromName("rawDataCopied"),
        duration: "short"
      };
      Cc["@mozilla.org/android/bridge;1"].
        getService(Ci.nsIAndroidBridge).
        handleGeckoMessage(JSON.stringify(message));
#endif
    });
  }
  catch (err) {
    if (button)
      button.disabled = false;
    throw err;
  }
}

function getLoadContext() {
  return window.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIWebNavigation)
               .QueryInterface(Ci.nsILoadContext);
}

function copyContentsToClipboard() {
  
  let contentsDiv = $("contents");
  let dataHtml = contentsDiv.innerHTML;
  let dataText = createTextForElement(contentsDiv);

  
  let supportsStringClass = Cc["@mozilla.org/supports-string;1"];
  let ssHtml = supportsStringClass.createInstance(Ci.nsISupportsString);
  let ssText = supportsStringClass.createInstance(Ci.nsISupportsString);

  let transferable = Cc["@mozilla.org/widget/transferable;1"]
                       .createInstance(Ci.nsITransferable);
  transferable.init(getLoadContext());

  
  transferable.addDataFlavor("text/html");
  ssHtml.data = dataHtml;
  transferable.setTransferData("text/html", ssHtml, dataHtml.length * 2);

  
  transferable.addDataFlavor("text/unicode");
  ssText.data = dataText;
  transferable.setTransferData("text/unicode", ssText, dataText.length * 2);

  
  let clipboard = Cc["@mozilla.org/widget/clipboard;1"]
                    .getService(Ci.nsIClipboard);
  clipboard.setData(transferable, null, clipboard.kGlobalClipboard);

#ifdef ANDROID
  
  let message = {
    type: "Toast:Show",
    message: stringBundle().GetStringFromName("textCopied"),
    duration: "short"
  };
  Cc["@mozilla.org/android/bridge;1"].
    getService(Ci.nsIAndroidBridge).
    handleGeckoMessage(JSON.stringify(message));
#endif
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
  if (elem.classList.contains("no-copy"))
    return;

  
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
  
  let currProfD = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let profileDir = currProfD.path;

  
  let nsLocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                           "nsILocalFile", "initWithPath");
  new nsLocalFile(profileDir).reveal();
}




function populateResetBox() {
  if (resetSupported())
    $("reset-box").style.visibility = "visible";
}




function resetProfileAndRestart() {
  let branding = Services.strings.createBundle("chrome://branding/locale/brand.properties");
  let brandShortName = branding.GetStringFromName("brandShortName");

  
  let retVals = {
    reset: false,
  };
  window.openDialog("chrome://global/content/resetProfile.xul", null,
                    "chrome,modal,centerscreen,titlebar,dialog=yes", retVals);
  if (!retVals.reset)
    return;

  
  let env = Cc["@mozilla.org/process/environment;1"]
              .getService(Ci.nsIEnvironment);
  env.set("MOZ_RESET_PROFILE_RESTART", "1");

  let appStartup = Cc["@mozilla.org/toolkit/app-startup;1"].getService(Ci.nsIAppStartup);
  appStartup.quit(Ci.nsIAppStartup.eForceQuit | Ci.nsIAppStartup.eRestart);
}
