



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/Troubleshoot.jsm");
Components.utils.import("resource://gre/modules/PluralForm.jsm");
Components.utils.import("resource://gre/modules/ResetProfile.jsm");

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

#ifdef MOZ_CRASHREPORTER
  crashes: function crashes(data) {
    let strings = stringBundle();
    let daysRange = Troubleshoot.kMaxCrashAge / (24 * 60 * 60 * 1000);
    $("crashes-title").textContent =
      PluralForm.get(daysRange, strings.GetStringFromName("crashesTitle"))
                .replace("#1", daysRange);
    let reportURL;
    try {
      reportURL = Services.prefs.getCharPref("breakpad.reportURL");
      
      if (!/^https?:/i.test(reportURL))
        reportURL = null;
    }
    catch (e) { }
    if (!reportURL) {
      $("crashes-noConfig").style.display = "block";
      $("crashes-noConfig").classList.remove("no-copy");
      return;
    }
    else {
      $("crashes-allReports").style.display = "block";
      $("crashes-allReports").classList.remove("no-copy");
    }

    if (data.pending > 0) {
      $("crashes-allReportsWithPending").textContent =
        PluralForm.get(data.pending, strings.GetStringFromName("pendingReports"))
                  .replace("#1", data.pending);
    }

    let dateNow = new Date();
    $.append($("crashes-tbody"), data.submitted.map(function (crash) {
      let date = new Date(crash.date);
      let timePassed = dateNow - date;
      let formattedDate;
      if (timePassed >= 24 * 60 * 60 * 1000)
      {
        let daysPassed = Math.round(timePassed / (24 * 60 * 60 * 1000));
        let daysPassedString = strings.GetStringFromName("crashesTimeDays");
        formattedDate = PluralForm.get(daysPassed, daysPassedString)
                                  .replace("#1", daysPassed);
      }
      else if (timePassed >= 60 * 60 * 1000)
      {
        let hoursPassed = Math.round(timePassed / (60 * 60 * 1000));
        let hoursPassedString = strings.GetStringFromName("crashesTimeHours");
        formattedDate = PluralForm.get(hoursPassed, hoursPassedString)
                                  .replace("#1", hoursPassed);
      }
      else
      {
        let minutesPassed = Math.max(Math.round(timePassed / (60 * 1000)), 1);
        let minutesPassedString = strings.GetStringFromName("crashesTimeMinutes");
        formattedDate = PluralForm.get(minutesPassed, minutesPassedString)
                                  .replace("#1", minutesPassed);
      }
      return $.new("tr", [
        $.new("td", [
          $.new("a", crash.id, null, {href : reportURL + crash.id})
        ]),
        $.new("td", formattedDate)
      ]);
    }));
  },
#endif

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
      if (msgArray.length) {
        
        
        try {
          return strings.formatStringFromName(nameOrMsg, msgArray,
                                              msgArray.length);
        }
        catch (err) {
          
          
          
          
          return nameOrMsg;
        }
      }
      try {
        return strings.GetStringFromName(nameOrMsg);
      }
      catch (err) {
        
      }
      return nameOrMsg;
    }

    let out = Object.create(data);
    let strings = stringBundle();

    out.acceleratedWindows =
      data.numAcceleratedWindows + "/" + data.numTotalWindows;
    if (data.windowLayerManagerType)
      out.acceleratedWindows += " " + data.windowLayerManagerType;
    if (data.windowLayerManagerRemote)
      out.acceleratedWindows += " (OMTC)";
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

$.new = function $_new(tag, textContentOrChildren, className, attributes) {
  let elt = document.createElement(tag);
  if (className)
    elt.className = className;
  if (attributes) {
    for (let attrName in attributes)
      elt.setAttribute(attrName, attributes[attrName]);
  }
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
  let serializer = new Serializer();
  let text = serializer.serialize(elem);

  
#ifdef XP_WIN
  text = text.replace(/\n/g, "\r\n");
#endif

  return text;
}

function Serializer() {
}

Serializer.prototype = {

  serialize: function (rootElem) {
    this._lines = [];
    this._startNewLine();
    this._serializeElement(rootElem);
    this._startNewLine();
    return this._lines.join("\n").trim() + "\n";
  },

  
  
  
  get _currentLine() {
    return this._lines.length ? this._lines[this._lines.length - 1] : null;
  },

  set _currentLine(val) {
    return this._lines[this._lines.length - 1] = val;
  },

  _serializeElement: function (elem) {
    if (this._ignoreElement(elem))
      return;

    
    if (elem.localName == "table") {
      this._serializeTable(elem);
      return;
    }

    

    let hasText = false;
    for (let child of elem.childNodes) {
      if (child.nodeType == Node.TEXT_NODE) {
        let text = this._nodeText(child);
        this._appendText(text);
        hasText = hasText || !!text.trim();
      }
      else if (child.nodeType == Node.ELEMENT_NODE)
        this._serializeElement(child);
    }

    
    if (/^h[0-9]+$/.test(elem.localName)) {
      let headerText = (this._currentLine || "").trim();
      if (headerText) {
        this._startNewLine();
        this._appendText("-".repeat(headerText.length));
      }
    }

    
    if (hasText) {
      let display = window.getComputedStyle(elem).getPropertyValue("display");
      if (display == "block") {
        this._startNewLine();
        this._startNewLine();
      }
    }
  },

  _startNewLine: function (lines) {
    let currLine = this._currentLine;
    if (currLine) {
      
      this._currentLine = currLine.trim();
      if (!this._currentLine)
        
        this._lines.pop();
    }
    this._lines.push("");
  },

  _appendText: function (text, lines) {
    this._currentLine += text;
  },

  _serializeTable: function (table) {
    
    
    let colHeadings = {};
    let tableHeadingElem = table.querySelector("thead");
    if (!tableHeadingElem)
      tableHeadingElem = table.querySelector("tr");
    if (tableHeadingElem) {
      let tableHeadingCols = tableHeadingElem.querySelectorAll("th,td");
      
      
      for (let i = tableHeadingCols.length - 1; i >= 0; i--) {
        if (tableHeadingCols[i].localName != "th")
          break;
        colHeadings[i] = this._nodeText(tableHeadingCols[i]).trim();
      }
    }
    let hasColHeadings = Object.keys(colHeadings).length > 0;
    if (!hasColHeadings)
      tableHeadingElem = null;

    let trs = table.querySelectorAll("table > tr, tbody > tr");
    let startRow =
      tableHeadingElem && tableHeadingElem.localName == "tr" ? 1 : 0;

    if (startRow >= trs.length)
      
      return;

    if (hasColHeadings && !this._ignoreElement(tableHeadingElem)) {
      
      
      
      for (let i = startRow; i < trs.length; i++) {
        if (this._ignoreElement(trs[i]))
          continue;
        let children = trs[i].querySelectorAll("td");
        for (let j = 0; j < children.length; j++) {
          let text = "";
          if (colHeadings[j])
            text += colHeadings[j] + ": ";
          text += this._nodeText(children[j]).trim();
          this._appendText(text);
          this._startNewLine();
        }
        this._startNewLine();
      }
      return;
    }

    
    
    
    for (let i = startRow; i < trs.length; i++) {
      if (this._ignoreElement(trs[i]))
        continue;
      let children = trs[i].querySelectorAll("th,td");
      let rowHeading = this._nodeText(children[0]).trim();
      this._appendText(rowHeading + ": " + this._nodeText(children[1]).trim());
      this._startNewLine();
    }
    this._startNewLine();
  },

  _ignoreElement: function (elem) {
    return elem.classList.contains("no-copy");
  },

  _nodeText: function (node) {
    return node.textContent.replace(/\s+/g, " ");
  },
};

function openProfileDirectory() {
  
  let currProfD = Services.dirsvc.get("ProfD", Ci.nsIFile);
  let profileDir = currProfD.path;

  
  let nsLocalFile = Components.Constructor("@mozilla.org/file/local;1",
                                           "nsILocalFile", "initWithPath");
  new nsLocalFile(profileDir).reveal();
}

function showUpdateHistory() {
  var prompter = Cc["@mozilla.org/updates/update-prompt;1"]
                   .createInstance(Ci.nsIUpdatePrompt);
  prompter.showUpdateHistory(window);
}




function populateResetBox() {
  if (ResetProfile.resetSupported())
    $("reset-box").style.visibility = "visible";
}
