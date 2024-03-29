



'use strict';

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/TelemetryTimestamps.jsm");
Cu.import("resource://gre/modules/TelemetryController.jsm");
Cu.import("resource://gre/modules/TelemetrySession.jsm");
Cu.import("resource://gre/modules/TelemetryArchive.jsm");
Cu.import("resource://gre/modules/TelemetryUtils.jsm");
Cu.import("resource://gre/modules/TelemetryLog.jsm");
Cu.import("resource://gre/modules/Preferences.jsm");

const Telemetry = Services.telemetry;
const bundle = Services.strings.createBundle(
  "chrome://global/locale/aboutTelemetry.properties");
const brandBundle = Services.strings.createBundle(
  "chrome://branding/locale/brand.properties");


const MAX_BAR_HEIGHT = 18;
const MAX_BAR_CHARS = 25;
const PREF_TELEMETRY_SERVER_OWNER = "toolkit.telemetry.server_owner";
const PREF_TELEMETRY_ENABLED = "toolkit.telemetry.enabled";
const PREF_DEBUG_SLOW_SQL = "toolkit.telemetry.debugSlowSql";
const PREF_SYMBOL_SERVER_URI = "profiler.symbolicationUrl";
const DEFAULT_SYMBOL_SERVER_URI = "http://symbolapi.mozilla.org";
const PREF_FHR_UPLOAD_ENABLED = "datareporting.healthreport.uploadEnabled";


const FILTER_IDLE_TIMEOUT = 500;

const isWindows = (Services.appinfo.OS == "WINNT");
const EOL = isWindows ? "\r\n" : "\n";


let gPingData = null;


let documentRTLMode = "";





function isRTL() {
  if (!documentRTLMode)
    documentRTLMode = window.getComputedStyle(document.body).direction;
  return (documentRTLMode == "rtl");
}

function isArray(arg) {
  return Object.prototype.toString.call(arg) === '[object Array]';
}

function isFlatArray(obj) {
  if (!isArray(obj)) {
    return false;
  }
  return !obj.some(e => typeof(e) == "object");
}




function flattenObject(obj, map, path, array) {
  for (let k of Object.keys(obj)) {
    let newPath = [...path, array ? "[" + k + "]" : k];
    let v = obj[k];
    if (!v || (typeof(v) != "object")) {
      map.set(newPath.join("."), v);
    } else if (isFlatArray(v)) {
      map.set(newPath.join("."), "[" + v.join(", ") + "]");
    } else {
      flattenObject(v, map, newPath, isArray(v));
    }
  }
}







function explodeObject(obj) {
  let map = new Map();
  flattenObject(obj, map, []);
  return map;
}

function filterObject(obj, filterOut) {
  let ret = {};
  for (let k of Object.keys(obj)) {
    if (filterOut.indexOf(k) == -1) {
      ret[k] = obj[k];
    }
  }
  return ret;
}
















function sectionalizeObject(obj) {
  let map = new Map();
  for (let k of Object.keys(obj)) {
    map.set(k, explodeObject(obj[k]));
  }
  return map;
}




function getMainWindow() {
  return window.QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIWebNavigation)
               .QueryInterface(Ci.nsIDocShellTreeItem)
               .rootTreeItem
               .QueryInterface(Ci.nsIInterfaceRequestor)
               .getInterface(Ci.nsIDOMWindow);
}










function getMainWindowWithPreferencesPane() {
  let mainWindow = getMainWindow();
  if (mainWindow && "openAdvancedPreferences" in mainWindow) {
    return mainWindow;
  } else {
    return null;
  }
}




function removeAllChildNodes(node) {
  while (node.hasChildNodes()) {
    node.removeChild(node.lastChild);
  }
}




function padToTwoDigits(n) {
  return (n > 9) ? n: "0" + n;
}




function yesterday(date) {
  let d = new Date(date);
  d.setDate(d.getDate() - 1);
  return d;
}




function shortDateString(date) {
  return date.getFullYear()
         + "/" + padToTwoDigits(date.getMonth() + 1)
         + "/" + padToTwoDigits(date.getDate());
}




function shortTimeString(date) {
  return padToTwoDigits(date.getHours())
         + ":" + padToTwoDigits(date.getMinutes())
         + ":" + padToTwoDigits(date.getSeconds());
}

let Settings = {
  SETTINGS: [
    
    {
      pref: PREF_FHR_UPLOAD_ENABLED,
      defaultPrefValue: false,
      descriptionEnabledId: "description-upload-enabled",
      descriptionDisabledId: "description-upload-disabled",
    },
    
    {
      pref: PREF_TELEMETRY_ENABLED,
      defaultPrefValue: false,
      descriptionEnabledId: "description-extended-recording-enabled",
      descriptionDisabledId: "description-extended-recording-disabled",
    },
  ],

  attachObservers: function() {
    for (let s of this.SETTINGS) {
      let setting = s;
      Preferences.observe(setting.pref, this.render, this);
    }

    let elements = document.getElementsByClassName("change-data-choices-link");
    for (let el of elements) {
      el.addEventListener("click", function() {
        let mainWindow = getMainWindowWithPreferencesPane();
        mainWindow.openAdvancedPreferences("dataChoicesTab");
      }, false);
    }
  },

  detachObservers: function() {
    for (let setting of this.SETTINGS) {
      Preferences.ignore(setting.pref, this.render, this);
    }
  },

  


  render: function() {
    for (let setting of this.SETTINGS) {
      let enabledElement = document.getElementById(setting.descriptionEnabledId);
      let disabledElement = document.getElementById(setting.descriptionDisabledId);

      if (Preferences.get(setting.pref, setting.defaultPrefValue)) {
        enabledElement.classList.remove("hidden");
        disabledElement.classList.add("hidden");
      } else {
        enabledElement.classList.add("hidden");
        disabledElement.classList.remove("hidden");
      }
    }
  }
};

let PingPicker = {
  viewCurrentPingData: true,
  _archivedPings: null,

  attachObservers: function() {
    let elements = document.getElementsByName("choose-ping-source");
    for (let el of elements) {
      el.addEventListener("change", () => this.onPingSourceChanged(), false);
    }

    document.getElementById("show-subsession-data").addEventListener("change", () => {
      this._updateCurrentPingData();
    });

    document.getElementById("choose-ping-week").addEventListener("change", () => {
      this._renderPingList();
      this._updateArchivedPingData();
    }, false);
    document.getElementById("choose-ping-id").addEventListener("change", () => {
      this._updateArchivedPingData()
    }, false);

    document.getElementById("newer-ping")
            .addEventListener("click", () => this._movePingIndex(-1), false);
    document.getElementById("older-ping")
            .addEventListener("click", () => this._movePingIndex(1), false);
  },

  onPingSourceChanged: function() {
    this.update();
  },

  update: function() {
    let el = document.getElementById("ping-source-current");
    this.viewCurrentPingData = el.checked;

    if (this.viewCurrentPingData) {
      document.getElementById("current-ping-picker").classList.remove("hidden");
      document.getElementById("archived-ping-picker").classList.add("hidden");
      this._updateCurrentPingData();
    } else {
      document.getElementById("current-ping-picker").classList.add("hidden");
      this._updateArchivedPingList().then(() =>
        document.getElementById("archived-ping-picker").classList.remove("hidden"));
    }
  },

  _updateCurrentPingData: function() {
    const subsession = document.getElementById("show-subsession-data").checked;
    const ping = TelemetryController.getCurrentPingData(subsession);
    displayPingData(ping);
  },

  _updateArchivedPingData: function() {
    let id = this._getSelectedPingId();
    TelemetryArchive.promiseArchivedPingById(id)
                    .then((ping) => displayPingData(ping));
  },

  _updateArchivedPingList: function() {
    return TelemetryArchive.promiseArchivedPingList().then((pingList) => {
      
      
      pingList.reverse();

      
      
      pingList = pingList.filter(
        (p) => ["main", "saved-session"].indexOf(p.type) != -1);
      this._archivedPings = pingList;

      
      let weekStart = (date) => {
        let weekDay = (date.getDay() + 6) % 7;
        let monday = new Date(date);
        monday.setDate(date.getDate() - weekDay);
        return TelemetryUtils.truncateToDays(monday);
      };

      let weekStartDates = new Set();
      for (let p of pingList) {
        weekStartDates.add(weekStart(new Date(p.timestampCreated)).getTime());
      }

      
      let plusOneWeek = (date) => {
        let d = date;
        d.setDate(d.getDate() + 7);
        return d;
      };

      this._weeks = [for (startTime of weekStartDates.values()) {
        startDate: new Date(startTime),
        endDate: plusOneWeek(new Date(startTime)),
      }];

      
      this._renderWeeks();
      this._renderPingList();

      
      this._updateArchivedPingData();
    });
  },

  _renderWeeks: function() {
    let weekSelector = document.getElementById("choose-ping-week");
    removeAllChildNodes(weekSelector);

    let index = 0;
    for (let week of this._weeks) {
      let text = shortDateString(week.startDate)
                 + " - " + shortDateString(yesterday(week.endDate));

      let option = document.createElement("option");
      let content = document.createTextNode(text);
      option.appendChild(content);
      weekSelector.appendChild(option);
    }
  },

  _getSelectedWeek: function() {
    let weekSelector = document.getElementById("choose-ping-week");
    return this._weeks[weekSelector.selectedIndex];
  },

  _renderPingList: function(id = null) {
    let pingSelector = document.getElementById("choose-ping-id");
    removeAllChildNodes(pingSelector);

    let weekRange = this._getSelectedWeek();
    let pings = this._archivedPings.filter(
      (p) => p.timestampCreated >= weekRange.startDate.getTime() &&
             p.timestampCreated < weekRange.endDate.getTime());

    for (let p of pings) {
      let date = new Date(p.timestampCreated);
      let text = shortDateString(date)
                 + " " + shortTimeString(date)
                 + " - " + p.type;

      let option = document.createElement("option");
      let content = document.createTextNode(text);
      option.appendChild(content);
      option.setAttribute("value", p.id);
      if (id && p.id == id) {
        option.selected = true;
      }
      pingSelector.appendChild(option);
    }
  },

  _getSelectedPingId: function() {
    let pingSelector = document.getElementById("choose-ping-id");
    let selected = pingSelector.selectedOptions.item(0);
    return selected.getAttribute("value");
  },

  _movePingIndex: function(offset) {
    const id = this._getSelectedPingId();
    const index = this._archivedPings.findIndex((p) => p.id == id);
    const newIndex = Math.min(Math.max(index + offset, 0), this._archivedPings.length - 1);
    const ping = this._archivedPings[newIndex];

    const weekIndex = this._weeks.findIndex(
      (week) => ping.timestampCreated >= week.startDate.getTime() &&
                ping.timestampCreated < week.endDate.getTime());
    const options = document.getElementById("choose-ping-week").options;
    options.item(weekIndex).selected = true;

    this._renderPingList(ping.id);
    this._updateArchivedPingData();
  },
};

let GeneralData = {
  


  render: function(aPing) {
    setHasData("general-data-section", true);
    let table = document.createElement("table");

    let caption = document.createElement("caption");
    let captionString = bundle.GetStringFromName("generalDataTitle");
    caption.appendChild(document.createTextNode(captionString + "\n"));
    table.appendChild(caption);

    let headings = document.createElement("tr");
    this.appendColumn(headings, "th", bundle.GetStringFromName("generalDataHeadingName") + "\t");
    this.appendColumn(headings, "th", bundle.GetStringFromName("generalDataHeadingValue") + "\t");
    table.appendChild(headings);

    
    let ignoreSections = ["payload", "environment"];
    let data = explodeObject(filterObject(aPing, ignoreSections));

    for (let [path, value] of data) {
        let row = document.createElement("tr");
        this.appendColumn(row, "td", path + "\t");
        this.appendColumn(row, "td", value + "\t");
        table.appendChild(row);
    }

    let dataDiv = document.getElementById("general-data");
    removeAllChildNodes(dataDiv);
    dataDiv.appendChild(table);
  },

  






  appendColumn: function(aRowElement, aColType, aColText) {
    let colElement = document.createElement(aColType);
    let colTextElement = document.createTextNode(aColText);
    colElement.appendChild(colTextElement);
    aRowElement.appendChild(colElement);
  },
};

let EnvironmentData = {
  


  render: function(ping) {
    let dataDiv = document.getElementById("environment-data");
    removeAllChildNodes(dataDiv);
    const hasData = !!ping.environment;
    setHasData("environment-data-section", hasData);
    if (!hasData) {
      return;
    }

    let data = sectionalizeObject(ping.environment);

    for (let [section, sectionData] of data) {
      let table = document.createElement("table");
      let caption = document.createElement("caption");
      caption.appendChild(document.createTextNode(section + "\n"));
      table.appendChild(caption);

      let headings = document.createElement("tr");
      this.appendColumn(headings, "th", bundle.GetStringFromName("environmentDataHeadingName") + "\t");
      this.appendColumn(headings, "th", bundle.GetStringFromName("environmentDataHeadingValue") + "\t");
      table.appendChild(headings);

      for (let [path, value] of sectionData) {
          let row = document.createElement("tr");
          this.appendColumn(row, "td", path + "\t");
          this.appendColumn(row, "td", value + "\t");
          table.appendChild(row);
      }

      dataDiv.appendChild(table);
    }
  },

  






  appendColumn: function(aRowElement, aColType, aColText) {
    let colElement = document.createElement(aColType);
    let colTextElement = document.createTextNode(aColText);
    colElement.appendChild(colTextElement);
    aRowElement.appendChild(colElement);
  },
};

let TelLog = {
  


  render: function(aPing) {
    let entries = aPing.payload.log;
    const hasData = entries && entries.length > 0;
    setHasData("telemetry-log-section", hasData);
    if (!hasData) {
      return;
    }

    let table = document.createElement("table");

    let caption = document.createElement("caption");
    let captionString = bundle.GetStringFromName("telemetryLogTitle");
    caption.appendChild(document.createTextNode(captionString + "\n"));
    table.appendChild(caption);

    let headings = document.createElement("tr");
    this.appendColumn(headings, "th", bundle.GetStringFromName("telemetryLogHeadingId") + "\t");
    this.appendColumn(headings, "th", bundle.GetStringFromName("telemetryLogHeadingTimestamp") + "\t");
    this.appendColumn(headings, "th", bundle.GetStringFromName("telemetryLogHeadingData") + "\t");
    table.appendChild(headings);

    for (let entry of entries) {
        let row = document.createElement("tr");
        for (let elem of entry) {
            this.appendColumn(row, "td", elem + "\t");
        }
        table.appendChild(row);
    }

    let dataDiv = document.getElementById("telemetry-log");
    removeAllChildNodes(dataDiv);
    dataDiv.appendChild(table);
  },

  






  appendColumn: function(aRowElement, aColType, aColText) {
    let colElement = document.createElement(aColType);
    let colTextElement = document.createTextNode(aColText);
    colElement.appendChild(colTextElement);
    aRowElement.appendChild(colElement);
  },
};

let SlowSQL = {

  slowSqlHits: bundle.GetStringFromName("slowSqlHits"),

  slowSqlAverage: bundle.GetStringFromName("slowSqlAverage"),

  slowSqlStatement: bundle.GetStringFromName("slowSqlStatement"),

  mainThreadTitle: bundle.GetStringFromName("slowSqlMain"),

  otherThreadTitle: bundle.GetStringFromName("slowSqlOther"),

  


  render: function SlowSQL_render(aPing) {
    
    
    
    
    
    let debugSlowSql = PingPicker.viewCurrentPingData && Preferences.get(PREF_DEBUG_SLOW_SQL, false);
    let slowSql = debugSlowSql ? Telemetry.debugSlowSQL : aPing.payload.slowSQL;
    if (!slowSql) {
      setHasData("slow-sql-section", false);
      return;
    }

    let {mainThread, otherThreads} =
      debugSlowSql ? Telemetry.debugSlowSQL : aPing.payload.slowSQL;

    let mainThreadCount = Object.keys(mainThread).length;
    let otherThreadCount = Object.keys(otherThreads).length;
    if (mainThreadCount == 0 && otherThreadCount == 0) {
      setHasData("slow-sql-section", false);
      return;
    }

    setHasData("slow-sql-section", true);
    if (debugSlowSql) {
      document.getElementById("sql-warning").classList.remove("hidden");
    }

    let slowSqlDiv = document.getElementById("slow-sql-tables");
    removeAllChildNodes(slowSqlDiv);

    
    if (mainThreadCount > 0) {
      let table = document.createElement("table");
      this.renderTableHeader(table, this.mainThreadTitle);
      this.renderTable(table, mainThread);

      slowSqlDiv.appendChild(table);
      slowSqlDiv.appendChild(document.createElement("hr"));
    }

    
    if (otherThreadCount > 0) {
      let table = document.createElement("table");
      this.renderTableHeader(table, this.otherThreadTitle);
      this.renderTable(table, otherThreads);

      slowSqlDiv.appendChild(table);
      slowSqlDiv.appendChild(document.createElement("hr"));
    }
  },

  






  renderTableHeader: function SlowSQL_renderTableHeader(aTable, aTitle) {
    let caption = document.createElement("caption");
    caption.appendChild(document.createTextNode(aTitle + "\n"));
    aTable.appendChild(caption);

    let headings = document.createElement("tr");
    this.appendColumn(headings, "th", this.slowSqlHits + "\t");
    this.appendColumn(headings, "th", this.slowSqlAverage + "\t");
    this.appendColumn(headings, "th", this.slowSqlStatement + "\n");
    aTable.appendChild(headings);
  },

  






  renderTable: function SlowSQL_renderTable(aTable, aSql) {
    for (let [sql, [hitCount, totalTime]] of Iterator(aSql)) {
      let averageTime = totalTime / hitCount;

      let sqlRow = document.createElement("tr");

      this.appendColumn(sqlRow, "td", hitCount + "\t");
      this.appendColumn(sqlRow, "td", averageTime.toFixed(0) + "\t");
      this.appendColumn(sqlRow, "td", sql + "\n");

      aTable.appendChild(sqlRow);
    }
  },

  






  appendColumn: function SlowSQL_appendColumn(aRowElement, aColType, aColText) {
    let colElement = document.createElement(aColType);
    let colTextElement = document.createTextNode(aColText);
    colElement.appendChild(colTextElement);
    aRowElement.appendChild(colElement);
  }
};

let StackRenderer = {

  stackTitle: bundle.GetStringFromName("stackTitle"),

  memoryMapTitle: bundle.GetStringFromName("memoryMapTitle"),

  




  renderMemoryMap: function StackRenderer_renderMemoryMap(aDiv, memoryMap) {
    aDiv.appendChild(document.createTextNode(this.memoryMapTitle));
    aDiv.appendChild(document.createElement("br"));

    for (let currentModule of memoryMap) {
      aDiv.appendChild(document.createTextNode(currentModule.join(" ")));
      aDiv.appendChild(document.createElement("br"));
    }

    aDiv.appendChild(document.createElement("br"));
  },

  





  renderStack: function StackRenderer_renderStack(aDiv, aStack) {
    aDiv.appendChild(document.createTextNode(this.stackTitle));
    let stackText = " " + aStack.join(" ");
    aDiv.appendChild(document.createTextNode(stackText));

    aDiv.appendChild(document.createElement("br"));
    aDiv.appendChild(document.createElement("br"));
  },
  renderStacks: function StackRenderer_renderStacks(aPrefix, aStacks,
                                                    aMemoryMap, aRenderHeader) {
    let div = document.getElementById(aPrefix + '-data');
    removeAllChildNodes(div);

    let fetchE = document.getElementById(aPrefix + '-fetch-symbols');
    if (fetchE) {
      fetchE.classList.remove("hidden");
    }
    let hideE = document.getElementById(aPrefix + '-hide-symbols');
    if (hideE) {
      hideE.classList.add("hidden");
    }

    if (aStacks.length == 0) {
      return;
    }

    setHasData(aPrefix + '-section', true);

    this.renderMemoryMap(div, aMemoryMap);

    for (let i = 0; i < aStacks.length; ++i) {
      let stack = aStacks[i];
      aRenderHeader(i);
      this.renderStack(div, stack)
    }
  },

  





  renderHeader: function StackRenderer_renderHeader(aPrefix, aFormatArgs) {
    let div = document.getElementById(aPrefix + "-data");

    let titleElement = document.createElement("span");
    titleElement.className = "stack-title";

    let titleText = bundle.formatStringFromName(
      aPrefix + "-title", aFormatArgs, aFormatArgs.length);
    titleElement.appendChild(document.createTextNode(titleText));

    div.appendChild(titleElement);
    div.appendChild(document.createElement("br"));
  }
};

function SymbolicationRequest(aPrefix, aRenderHeader, aMemoryMap, aStacks) {
  this.prefix = aPrefix;
  this.renderHeader = aRenderHeader;
  this.memoryMap = aMemoryMap;
  this.stacks = aStacks;
}




SymbolicationRequest.prototype.handleSymbolResponse =
function SymbolicationRequest_handleSymbolResponse() {
  if (this.symbolRequest.readyState != 4)
    return;

  let fetchElement = document.getElementById(this.prefix + "-fetch-symbols");
  fetchElement.classList.add("hidden");
  let hideElement = document.getElementById(this.prefix + "-hide-symbols");
  hideElement.classList.remove("hidden");
  let div = document.getElementById(this.prefix + "-data");
  removeAllChildNodes(div);
  let errorMessage = bundle.GetStringFromName("errorFetchingSymbols");

  if (this.symbolRequest.status != 200) {
    div.appendChild(document.createTextNode(errorMessage));
    return;
  }

  let jsonResponse = {};
  try {
    jsonResponse = JSON.parse(this.symbolRequest.responseText);
  } catch (e) {
    div.appendChild(document.createTextNode(errorMessage));
    return;
  }

  for (let i = 0; i < jsonResponse.length; ++i) {
    let stack = jsonResponse[i];
    this.renderHeader(i);

    for (let symbol of stack) {
      div.appendChild(document.createTextNode(symbol));
      div.appendChild(document.createElement("br"));
    }
    div.appendChild(document.createElement("br"));
  }
};



SymbolicationRequest.prototype.fetchSymbols =
function SymbolicationRequest_fetchSymbols() {
  let symbolServerURI =
    Preferences.get(PREF_SYMBOL_SERVER_URI, DEFAULT_SYMBOL_SERVER_URI);
  let request = {"memoryMap" : this.memoryMap, "stacks" : this.stacks,
                 "version" : 3};
  let requestJSON = JSON.stringify(request);

  this.symbolRequest = new XMLHttpRequest();
  this.symbolRequest.open("POST", symbolServerURI, true);
  this.symbolRequest.setRequestHeader("Content-type", "application/json");
  this.symbolRequest.setRequestHeader("Content-length",
                                      requestJSON.length);
  this.symbolRequest.setRequestHeader("Connection", "close");
  this.symbolRequest.onreadystatechange = this.handleSymbolResponse.bind(this);
  this.symbolRequest.send(requestJSON);
}

let ChromeHangs = {

  symbolRequest: null,

  


  render: function ChromeHangs_render(aPing) {
    let hangs = aPing.payload.chromeHangs;
    setHasData("chrome-hangs-section", !!hangs);
    if (!hangs) {
      return;
    }

    let stacks = hangs.stacks;
    let memoryMap = hangs.memoryMap;

    StackRenderer.renderStacks("chrome-hangs", stacks, memoryMap,
			       (index) => this.renderHangHeader(aPing, index));
  },

  renderHangHeader: function ChromeHangs_renderHangHeader(aPing, aIndex) {
    let durations = aPing.payload.chromeHangs.durations;
    StackRenderer.renderHeader("chrome-hangs", [aIndex + 1, durations[aIndex]]);
  }
};

let ThreadHangStats = {

  


  render: function(aPing) {
    let div = document.getElementById("thread-hang-stats");
    removeAllChildNodes(div);

    let stats = aPing.payload.threadHangStats;
    setHasData("thread-hang-stats-section", stats && (stats.length > 0));
    if (!stats) {
      return;
    }

    stats.forEach((thread) => {
      div.appendChild(this.renderThread(thread));
    });
  },

  


  renderThread: function(aThread) {
    let div = document.createElement("div");

    let title = document.createElement("h2");
    title.textContent = aThread.name;
    div.appendChild(title);

    
    
    Histogram.render(div, aThread.name + "-Activity",
                     aThread.activity, {exponential: true});
    aThread.hangs.forEach((hang, index) => {
      let hangName = aThread.name + "-Hang-" + (index + 1);
      let hangDiv = Histogram.render(
        div, hangName, hang.histogram, {exponential: true});
      let stackDiv = document.createElement("div");
      let stack = hang.nativeStack || hang.stack;
      stack.forEach((frame) => {
        stackDiv.appendChild(document.createTextNode(frame));
        
        stackDiv.appendChild(document.createElement("br"));
      });
      
      hangDiv.insertBefore(stackDiv, hangDiv.childNodes[1]);
    });
    return div;
  },
};

let Histogram = {

  hgramSamplesCaption: bundle.GetStringFromName("histogramSamples"),

  hgramAverageCaption: bundle.GetStringFromName("histogramAverage"),

  hgramSumCaption: bundle.GetStringFromName("histogramSum"),

  hgramCopyCaption: bundle.GetStringFromName("histogramCopy"),

  








  render: function Histogram_render(aParent, aName, aHgram, aOptions) {
    let options = aOptions || {};
    let hgram = this.processHistogram(aHgram, aName);

    let outerDiv = document.createElement("div");
    outerDiv.className = "histogram";
    outerDiv.id = aName;

    let divTitle = document.createElement("div");
    divTitle.className = "histogram-title";
    divTitle.appendChild(document.createTextNode(aName));
    outerDiv.appendChild(divTitle);

    let stats = hgram.sample_count + " " + this.hgramSamplesCaption + ", " +
                this.hgramAverageCaption + " = " + hgram.pretty_average + ", " +
                this.hgramSumCaption + " = " + hgram.sum;

    let divStats = document.createElement("div");
    divStats.appendChild(document.createTextNode(stats));
    outerDiv.appendChild(divStats);

    if (isRTL()) {
      hgram.buckets.reverse();
      hgram.values.reverse();
    }

    let textData = this.renderValues(outerDiv, hgram, options);

    
    let copyButton = document.createElement("button");
    copyButton.className = "copy-node";
    copyButton.appendChild(document.createTextNode(this.hgramCopyCaption));
    copyButton.histogramText = aName + EOL + stats + EOL + EOL + textData;
    copyButton.addEventListener("click", function(){
      Cc["@mozilla.org/widget/clipboardhelper;1"].getService(Ci.nsIClipboardHelper)
                                                 .copyString(this.histogramText);
    });
    outerDiv.appendChild(copyButton);

    aParent.appendChild(outerDiv);
    return outerDiv;
  },

  processHistogram: function(aHgram, aName) {
    const values = [for (k of Object.keys(aHgram.values)) aHgram.values[k]];
    if (!values.length) {
      
      
      return {
        values: [],
        pretty_average: 0,
        max: 0,
        sample_count: 0,
        sum: 0
      };
    }

    const sample_count = values.reduceRight((a, b) => a + b);
    const average = Math.round(aHgram.sum * 10 / sample_count) / 10;
    const max_value = Math.max(...values);

    const labelledValues = [for (k of Object.keys(aHgram.values)) [Number(k), aHgram.values[k]]];

    let result = {
      values: labelledValues,
      pretty_average: average,
      max: max_value,
      sample_count: sample_count,
      sum: aHgram.sum
    };

    return result;
  },

  





  getLogValue: function(aNumber) {
    return Math.max(0, Math.log10(aNumber) + 1);
  },

  








  renderValues: function Histogram_renderValues(aDiv, aHgram, aOptions) {
    let text = "";
    
    let labelPadTo = 0;
    if (aHgram.values.length) {
      labelPadTo = String(aHgram.values[aHgram.values.length - 1][0]).length;
    }
    let maxBarValue = aOptions.exponential ? this.getLogValue(aHgram.max_value) : aHgram.max;

    for (let [label, value] of aHgram.values) {
      let barValue = aOptions.exponential ? this.getLogValue(value) : value;

      
      text += EOL
              + " ".repeat(Math.max(0, labelPadTo - String(label).length)) + label 
              + " |" + "#".repeat(Math.round(MAX_BAR_CHARS * barValue / maxBarValue)) 
              + "  " + value 
              + "  " + Math.round(100 * value / aHgram.sum) + "%"; 

      
      let belowEm = Math.round(MAX_BAR_HEIGHT * (barValue / maxBarValue) * 10) / 10;
      let aboveEm = MAX_BAR_HEIGHT - belowEm;

      let barDiv = document.createElement("div");
      barDiv.className = "bar";
      barDiv.style.paddingTop = aboveEm + "em";

      
      barDiv.appendChild(document.createTextNode(value ? value : '\u00A0'));

      
      let bar = document.createElement("div");
      bar.className = "bar-inner";
      bar.style.height = belowEm + "em";
      barDiv.appendChild(bar);

      
      barDiv.appendChild(document.createTextNode(label));

      aDiv.appendChild(barDiv);
    }

    return text.substr(EOL.length); 
  },

  






  filterHistograms: function _filterHistograms(aContainerNode, aFilterText) {
    let filter = aFilterText.toString();

    
    function isPassText(subject, filter) {
      for (let item of filter) {
        if (item.length && subject.indexOf(item) < 0) {
          return false; 
        }
      }
      return true;
    }

    function isPassRegex(subject, filter) {
      return filter.test(subject);
    }

    
    let isPassFunc; 
    filter = filter.trim();
    if (filter[0] != "/") { 
      isPassFunc = isPassText;
      filter = filter.toLowerCase().split(" ");
    } else {
      isPassFunc = isPassRegex;
      var r = filter.match(/^\/(.*)\/(i?)$/);
      try {
        filter = RegExp(r[1], r[2]);
      }
      catch (e) { 
        isPassFunc = function() {
          return false;
        };
      }
    }

    let needLower = (isPassFunc === isPassText);

    let histograms = aContainerNode.getElementsByClassName("histogram");
    for (let hist of histograms) {
      hist.classList[isPassFunc((needLower ? hist.id.toLowerCase() : hist.id), filter) ? "remove" : "add"]("filter-blocked");
    }
  },

  




  histogramFilterChanged: function _histogramFilterChanged() {
    if (this.idleTimeout) {
      clearTimeout(this.idleTimeout);
    }

    this.idleTimeout = setTimeout( () => {
      Histogram.filterHistograms(document.getElementById(this.getAttribute("target_id")), this.value);
    }, FILTER_IDLE_TIMEOUT);
  }
};







function RenderObject(aObject) {
  let output = "";
  if (Array.isArray(aObject)) {
    if (aObject.length == 0) {
      return "[]";
    }
    output = "[" + JSON.stringify(aObject[0]);
    for (let i = 1; i < aObject.length; i++) {
      output += ", " + JSON.stringify(aObject[i]);
    }
    return output + "]";
  }
  let keys = Object.keys(aObject);
  if (keys.length == 0) {
    return "{}";
  }
  output = "{\"" + keys[0] + "\":\u00A0" + JSON.stringify(aObject[keys[0]]);
  for (let i = 1; i < keys.length; i++) {
    output += ", \"" + keys[i] + "\":\u00A0" + JSON.stringify(aObject[keys[i]]);
  }
  return output + "}";
};

let KeyValueTable = {
  






  render: function KeyValueTable_render(aMeasurements, aKeysLabel, aValuesLabel) {
    let table = document.createElement("table");
    this.renderHeader(table, aKeysLabel, aValuesLabel);
    this.renderBody(table, aMeasurements);
    return table;
  },

  







  renderHeader: function KeyValueTable_renderHeader(aTable, aKeysLabel, aValuesLabel) {
    let headerRow = document.createElement("tr");
    aTable.appendChild(headerRow);

    let keysColumn = document.createElement("th");
    keysColumn.appendChild(document.createTextNode(aKeysLabel + "\t"));
    let valuesColumn = document.createElement("th");
    valuesColumn.appendChild(document.createTextNode(aValuesLabel + "\n"));

    headerRow.appendChild(keysColumn);
    headerRow.appendChild(valuesColumn);
  },

  






  renderBody: function KeyValueTable_renderBody(aTable, aMeasurements) {
    for (let [key, value] of Iterator(aMeasurements)) {
      
      if (value &&
         (typeof value == "object") &&
         (typeof value.valueOf() == "object")) {
        value = RenderObject(value);
      }

      let newRow = document.createElement("tr");
      aTable.appendChild(newRow);

      let keyField = document.createElement("td");
      keyField.appendChild(document.createTextNode(key + "\t"));
      newRow.appendChild(keyField);

      let valueField = document.createElement("td");
      valueField.appendChild(document.createTextNode(value + "\n"));
      newRow.appendChild(valueField);
    }
  }
};

let KeyedHistogram = {
  render: function(parent, id, keyedHistogram) {
    let outerDiv = document.createElement("div");
    outerDiv.className = "keyed-histogram";
    outerDiv.id = id;

    let divTitle = document.createElement("div");
    divTitle.className = "keyed-histogram-title";
    divTitle.appendChild(document.createTextNode(id));
    outerDiv.appendChild(divTitle);

    for (let [name, hgram] of Iterator(keyedHistogram)) {
      Histogram.render(outerDiv, name, hgram);
    }

    parent.appendChild(outerDiv);
    return outerDiv;
  },
};

let AddonDetails = {
  tableIDTitle: bundle.GetStringFromName("addonTableID"),
  tableDetailsTitle: bundle.GetStringFromName("addonTableDetails"),

  



  render: function AddonDetails_render(aPing) {
    let addonSection = document.getElementById("addon-details");
    removeAllChildNodes(addonSection);
    let addonDetails = aPing.payload.addonDetails;
    const hasData = addonDetails && Object.keys(addonDetails).length > 0;
    setHasData("addon-details-section", hasData);
    if (!hasData) {
      return;
    }

    for (let provider in addonDetails) {
      let providerSection = document.createElement("h2");
      let titleText = bundle.formatStringFromName("addonProvider", [provider], 1);
      providerSection.appendChild(document.createTextNode(titleText));
      addonSection.appendChild(providerSection);
      addonSection.appendChild(
        KeyValueTable.render(addonDetails[provider],
                             this.tableIDTitle, this.tableDetailsTitle));
    }
  }
};







function setHasData(aSectionID, aHasData) {
  let sectionElement = document.getElementById(aSectionID);
  sectionElement.classList[aHasData ? "add" : "remove"]("has-data");
}





function toggleSection(aEvent) {
  let parentElement = aEvent.target.parentElement;
  if (!parentElement.classList.contains("has-data")) {
    return; 
  }

  parentElement.classList.toggle("expanded");

  
  let statebox = parentElement.getElementsByClassName("statebox")[0];
  statebox.checked = parentElement.classList.contains("expanded");
}




function setupPageHeader()
{
  let serverOwner = Preferences.get(PREF_TELEMETRY_SERVER_OWNER, "Mozilla");
  let brandName = brandBundle.GetStringFromName("brandFullName");
  let subtitleText = bundle.formatStringFromName(
    "pageSubtitle", [serverOwner, brandName], 2);

  let subtitleElement = document.getElementById("page-subtitle");
  subtitleElement.appendChild(document.createTextNode(subtitleText));
}




function setupListeners() {
  Settings.attachObservers();
  PingPicker.attachObservers();

  
  window.addEventListener("unload",
    function unloadHandler(aEvent) {
      window.removeEventListener("unload", unloadHandler);
      Settings.detachObservers();
  }, false);

  document.getElementById("chrome-hangs-fetch-symbols").addEventListener("click",
    function () {
      if (!gPingData) {
        return;
      }

      let hangs = gPingData.payload.chromeHangs;
      let req = new SymbolicationRequest("chrome-hangs",
                                         ChromeHangs.renderHangHeader,
                                         hangs.memoryMap, hangs.stacks);
      req.fetchSymbols();
  }, false);

  document.getElementById("chrome-hangs-hide-symbols").addEventListener("click",
    function () {
      if (!gPingData) {
        return;
      }

      ChromeHangs.render(gPingData);
  }, false);

  document.getElementById("late-writes-fetch-symbols").addEventListener("click",
    function () {
      if (!gPingData) {
        return;
      }

      let lateWrites = gPingData.payload.lateWrites;
      let req = new SymbolicationRequest("late-writes",
                                         LateWritesSingleton.renderHeader,
                                         lateWrites.memoryMap,
                                         lateWrites.stacks);
      req.fetchSymbols();
  }, false);

  document.getElementById("late-writes-hide-symbols").addEventListener("click",
    function () {
      if (!gPingData) {
        return;
      }

      LateWritesSingleton.renderLateWrites(gPingData.payload.lateWrites);
  }, false);

  
  let sectionHeaders = document.getElementsByClassName("section-name");
  for (let sectionHeader of sectionHeaders) {
    sectionHeader.addEventListener("click", toggleSection, false);
  }

  
  let toggleLinks = document.getElementsByClassName("toggle-caption");
  for (let toggleLink of toggleLinks) {
    toggleLink.addEventListener("click", toggleSection, false);
  }
}

function onLoad() {
  window.removeEventListener("load", onLoad);

  
  setupPageHeader();

  
  setupListeners();

  
  Settings.render();

  
  Telemetry.asyncFetchTelemetryData(() => PingPicker.update());

  
  let stateboxes = document.getElementsByClassName("statebox");
  for (let box of stateboxes) {
    if (box.checked) { 
        box.parentElement.classList.add("expanded");
    }
  }
}

let LateWritesSingleton = {
  renderHeader: function LateWritesSingleton_renderHeader(aIndex) {
    StackRenderer.renderHeader("late-writes", [aIndex + 1]);
  },

  renderLateWrites: function LateWritesSingleton_renderLateWrites(lateWrites) {
    setHasData("late-writes-section", !!lateWrites);
    if (!lateWrites) {
      return;
    }

    let stacks = lateWrites.stacks;
    let memoryMap = lateWrites.memoryMap;
    StackRenderer.renderStacks('late-writes', stacks, memoryMap,
                               LateWritesSingleton.renderHeader);
  }
};








function sortStartupMilestones(aSimpleMeasurements) {
  const telemetryTimestamps = TelemetryTimestamps.get();
  let startupEvents = Services.startup.getStartupInfo();
  delete startupEvents['process'];

  function keyIsMilestone(k) {
    return (k in startupEvents) || (k in telemetryTimestamps);
  }

  let sortedKeys = Object.keys(aSimpleMeasurements);

  
  sortedKeys.sort(function keyCompare(keyA, keyB) {
    let isKeyAMilestone = keyIsMilestone(keyA);
    let isKeyBMilestone = keyIsMilestone(keyB);

    
    if (isKeyAMilestone && !isKeyBMilestone)
      return -1;
    if (!isKeyAMilestone && isKeyBMilestone)
      return 1;
    
    if (!isKeyAMilestone && !isKeyBMilestone)
      return 0;

    
    return aSimpleMeasurements[keyA] - aSimpleMeasurements[keyB];
  });

  
  let result = {};
  for (let key of sortedKeys) {
    result[key] = aSimpleMeasurements[key];
  }

  return result;
}

function displayPingData(ping) {
  gPingData = ping;

  const keysHeader = bundle.GetStringFromName("keysHeader");
  const valuesHeader = bundle.GetStringFromName("valuesHeader");

  
  GeneralData.render(ping);

  
  EnvironmentData.render(ping);

  
  TelLog.render(ping);

  
  SlowSQL.render(ping);

  
  ChromeHangs.render(ping);

  
  ThreadHangStats.render(ping);

  
  AddonDetails.render(ping);

  
  let payload = ping.payload;
  let simpleMeasurements = sortStartupMilestones(payload.simpleMeasurements);
  let hasData = Object.keys(simpleMeasurements).length > 0;
  setHasData("simple-measurements-section", hasData);
  let simpleSection = document.getElementById("simple-measurements");
  removeAllChildNodes(simpleSection);

  if (hasData) {
    simpleSection.appendChild(KeyValueTable.render(simpleMeasurements,
                                                   keysHeader, valuesHeader));
  }

  LateWritesSingleton.renderLateWrites(payload.lateWrites);

  
  hasData = Object.keys(payload.info).length > 0;
  setHasData("system-info-section", hasData);
  let infoSection = document.getElementById("system-info");
  removeAllChildNodes(infoSection);

  if (hasData) {
    infoSection.appendChild(KeyValueTable.render(payload.info,
                                                 keysHeader, valuesHeader));
  }

  
  let hgramDiv = document.getElementById("histograms");
  removeAllChildNodes(hgramDiv);

  let histograms = payload.histograms;
  hasData = Object.keys(histograms).length > 0;
  setHasData("histograms-section", hasData);

  if (hasData) {
    for (let [name, hgram] of Iterator(histograms)) {
      Histogram.render(hgramDiv, name, hgram, {unpacked: true});
    }

    let filterBox = document.getElementById("histograms-filter");
    filterBox.addEventListener("input", Histogram.histogramFilterChanged, false);
    if (filterBox.value.trim() != "") { 
      Histogram.filterHistograms(hgramDiv, filterBox.value);
    }

    setHasData("histograms-section", true);
  }

  
  let keyedDiv = document.getElementById("keyed-histograms");
  removeAllChildNodes(keyedDiv);

  let keyedHistograms = payload.keyedHistograms;
  hasData = Object.keys(keyedHistograms).length > 0;
  setHasData("keyed-histograms-section", hasData);

  if (hasData) {
    for (let [id, keyed] of Iterator(keyedHistograms)) {
      KeyedHistogram.render(keyedDiv, id, keyed, {unpacked: true});
    }
  }

  
  let addonDiv = document.getElementById("addon-histograms");
  removeAllChildNodes(addonDiv);

  let addonHistogramsRendered = false;
  let addonData = payload.addonHistograms;
  if (addonData) {
    for (let [addon, histograms] of Iterator(addonData)) {
      for (let [name, hgram] of Iterator(histograms)) {
        addonHistogramsRendered = true;
        Histogram.render(addonDiv, addon + ": " + name, hgram, {unpacked: true});
      }
    }
  }

  setHasData("addon-histograms-section", addonHistogramsRendered);
}

window.addEventListener("load", onLoad, false);
