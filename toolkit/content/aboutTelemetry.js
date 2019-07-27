



'use strict';

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/TelemetryTimestamps.jsm");
Cu.import("resource://gre/modules/TelemetryController.jsm");
Cu.import("resource://gre/modules/TelemetrySession.jsm");
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

let observer = {

  enableTelemetry: bundle.GetStringFromName("enableTelemetry"),

  disableTelemetry: bundle.GetStringFromName("disableTelemetry"),

  


  observe: function observe(aSubject, aTopic, aData) {
    if (aData == PREF_TELEMETRY_ENABLED) {
      this.updatePrefStatus();
    }
  },

  


  updatePrefStatus: function updatePrefStatus() {
    
    let enabledElement = document.getElementById("description-enabled");
    let disabledElement = document.getElementById("description-disabled");
    let toggleElement = document.getElementById("toggle-telemetry");
    if (Preferences.get(PREF_TELEMETRY_ENABLED, false)) {
      enabledElement.classList.remove("hidden");
      disabledElement.classList.add("hidden");
      toggleElement.innerHTML = this.disableTelemetry;
    } else {
      enabledElement.classList.add("hidden");
      disabledElement.classList.remove("hidden");
      toggleElement.innerHTML = this.enableTelemetry;
    }
  }
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
    setHasData("environment-data-section", true);
    let dataDiv = document.getElementById("environment-data");
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

    if(entries.length == 0) {
        return;
    }
    setHasData("telemetry-log-section", true);
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
    
    
    
    let debugSlowSql = Preferences.get(PREF_DEBUG_SLOW_SQL, false);
    let {mainThread, otherThreads} =
      debugSlowSql ? Telemetry.debugSlowSQL : aPing.payload.slowSQL;

    let mainThreadCount = Object.keys(mainThread).length;
    let otherThreadCount = Object.keys(otherThreads).length;
    if (mainThreadCount == 0 && otherThreadCount == 0) {
      return;
    }

    setHasData("slow-sql-section", true);

    if (debugSlowSql) {
      document.getElementById("sql-warning").classList.remove("hidden");
    }

    let slowSqlDiv = document.getElementById("slow-sql-tables");

    
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






function clearDivData(aDiv) {
  while (aDiv.hasChildNodes()) {
    aDiv.removeChild(aDiv.lastChild);
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
    clearDivData(div);

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
  clearDivData(div);
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
    clearDivData(div);

    let stats = aPing.payload.threadHangStats;
    stats.forEach((thread) => {
      div.appendChild(this.renderThread(thread));
    });
    if (stats.length) {
      setHasData("thread-hang-stats-section", true);
    }
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

  



  render: function AddonDetails_render(aSections) {
    let addonSection = document.getElementById("addon-details");
    for (let provider in aSections) {
      let providerSection = document.createElement("h2");
      let titleText = bundle.formatStringFromName("addonProvider", [provider], 1);
      providerSection.appendChild(document.createTextNode(titleText));
      addonSection.appendChild(providerSection);
      addonSection.appendChild(
        KeyValueTable.render(aSections[provider],
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
  Services.prefs.addObserver(PREF_TELEMETRY_ENABLED, observer, false);
  observer.updatePrefStatus();

  
  window.addEventListener("unload",
    function unloadHandler(aEvent) {
      window.removeEventListener("unload", unloadHandler);
      Services.prefs.removeObserver(PREF_TELEMETRY_ENABLED, observer);
  }, false);

  document.getElementById("toggle-telemetry").addEventListener("click",
    function () {
      let value = Preferences.get(PREF_TELEMETRY_ENABLED, false);
      Preferences.set(PREF_TELEMETRY_ENABLED, !value);
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

  
  Telemetry.asyncFetchTelemetryData(displayPingData);

  
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

function displayPingData() {
  gPingData = TelemetryController.getCurrentPingData(false);
  let ping = gPingData;

  const keysHeader = bundle.GetStringFromName("keysHeader");
  const valuesHeader = bundle.GetStringFromName("valuesHeader");

  
  GeneralData.render(ping);

  
  EnvironmentData.render(ping);

  
  TelLog.render(ping);

  
  SlowSQL.render(ping);

  
  ChromeHangs.render(ping);

  
  ThreadHangStats.render(ping);

  
  let payload = ping.payload;
  let simpleMeasurements = sortStartupMilestones(payload.simpleMeasurements);
  if (Object.keys(simpleMeasurements).length) {
    let simpleSection = document.getElementById("simple-measurements");
    simpleSection.appendChild(KeyValueTable.render(simpleMeasurements,
                                                   keysHeader, valuesHeader));
    setHasData("simple-measurements-section", true);
  }

  LateWritesSingleton.renderLateWrites(payload.lateWrites);

  
  if (Object.keys(payload.info).length) {
    let infoSection = document.getElementById("system-info");
    infoSection.appendChild(KeyValueTable.render(payload.info,
                                                 keysHeader, valuesHeader));
    setHasData("system-info-section", true);
  }

  let addonDetails = payload.addonDetails;
  if (Object.keys(addonDetails).length) {
    AddonDetails.render(addonDetails);
    setHasData("addon-details-section", true);
  }

  
  let histograms = payload.histograms;
  if (Object.keys(histograms).length) {
    let hgramDiv = document.getElementById("histograms");
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

  
  let keyedHistograms = payload.keyedHistograms;
  if (Object.keys(keyedHistograms).length) {
    let keyedDiv = document.getElementById("keyed-histograms");
    for (let [id, keyed] of Iterator(keyedHistograms)) {
      KeyedHistogram.render(keyedDiv, id, keyed, {unpacked: true});
    }

    setHasData("keyed-histograms-section", true);
  }

  
  let addonDiv = document.getElementById("addon-histograms");
  let addonHistogramsRendered = false;
  let addonData = payload.addonHistograms;
  for (let [addon, histograms] of Iterator(addonData)) {
    for (let [name, hgram] of Iterator(histograms)) {
      addonHistogramsRendered = true;
      Histogram.render(addonDiv, addon + ": " + name, hgram, {unpacked: true});
    }
  }

  if (addonHistogramsRendered) {
   setHasData("addon-histograms-section", true);
  }
}

window.addEventListener("load", onLoad, false);
