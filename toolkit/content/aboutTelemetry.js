



'use strict';

const Ci = Components.interfaces;
const Cc = Components.classes;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

const Telemetry = Services.telemetry;
const bundle = Services.strings.createBundle(
  "chrome://global/locale/aboutTelemetry.properties");
const brandBundle = Services.strings.createBundle(
  "chrome://branding/locale/brand.properties");
const TelemetryPing = Cc["@mozilla.org/base/telemetry-ping;1"].
  getService(Ci.nsITelemetryPing);


const MAX_BAR_HEIGHT = 18;
const PREF_TELEMETRY_SERVER_OWNER = "toolkit.telemetry.server_owner";
#ifdef MOZ_TELEMETRY_ON_BY_DEFAULT
const PREF_TELEMETRY_ENABLED = "toolkit.telemetry.enabledPreRelease";
#else
const PREF_TELEMETRY_ENABLED = "toolkit.telemetry.enabled";
#endif
const PREF_DEBUG_SLOW_SQL = "toolkit.telemetry.debugSlowSql";
const PREF_SYMBOL_SERVER_URI = "profiler.symbolicationUrl";
const DEFAULT_SYMBOL_SERVER_URI = "http://symbolapi.mozilla.org";


let documentRTLMode = "";








function getPref(aPrefName, aDefault) {
  let result = aDefault;

  try {
    let prefType = Services.prefs.getPrefType(aPrefName);
    if (prefType == Ci.nsIPrefBranch.PREF_BOOL) {
      result = Services.prefs.getBoolPref(aPrefName);
    } else if (prefType == Ci.nsIPrefBranch.PREF_STRING) {
      result = Services.prefs.getCharPref(aPrefName);
    }
  } catch (e) {
    
  }

  return result;
}





function isRTL() {
  if (!documentRTLMode)
    documentRTLMode = window.getComputedStyle(document.body).direction;
  return (documentRTLMode == "rtl");
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
    if (getPref(PREF_TELEMETRY_ENABLED, false)) {
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

let SlowSQL = {

  slowSqlHits: bundle.GetStringFromName("slowSqlHits"),

  slowSqlAverage: bundle.GetStringFromName("slowSqlAverage"),

  slowSqlStatement: bundle.GetStringFromName("slowSqlStatement"),

  mainThreadTitle: bundle.GetStringFromName("slowSqlMain"),

  otherThreadTitle: bundle.GetStringFromName("slowSqlOther"),

  


  render: function SlowSQL_render() {
    let debugSlowSql = getPref(PREF_DEBUG_SLOW_SQL, false);
    let {mainThread, otherThreads} =
      Telemetry[debugSlowSql ? "debugSlowSQL" : "slowSQL"];

    let mainThreadCount = Object.keys(mainThread).length;
    let otherThreadCount = Object.keys(otherThreads).length;
    if (mainThreadCount == 0 && otherThreadCount == 0) {
      showEmptySectionMessage("slow-sql-section");
      return;
    }

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
      showEmptySectionMessage(aPrefix + '-section');
      return;
    }

    this.renderMemoryMap(div, aMemoryMap);

    for (let i = 0; i < aStacks.length; ++i) {
      let stack = aStacks[i];
      aRenderHeader(i);
      this.renderStack(div, stack)
    }
  },
  renderSymbolicatedStacks:
    function StackRenderer_renderSymbolicatedStacks(aPrefix, aRequest,
                                                    aRenderHeader) {
    if (aRequest.readyState != 4)
      return;

    document.getElementById(aPrefix + "-fetch-symbols").classList.add("hidden");
    document.getElementById(aPrefix + "-hide-symbols").classList.remove("hidden");
    let div = document.getElementById(aPrefix + "-data");
    clearDivData(div);
    let errorMessage = bundle.GetStringFromName("errorFetchingSymbols");

    if (aRequest.status != 200) {
      div.appendChild(document.createTextNode(errorMessage));
      return;
    }

    let jsonResponse = {};
    try {
      jsonResponse = JSON.parse(aRequest.responseText);
    } catch (e) {
      div.appendChild(document.createTextNode(errorMessage));
      return;
    }

    for (let i = 0; i < jsonResponse.length; ++i) {
      let stack = jsonResponse[i];
      aRenderHeader(i);

      for (let symbol of stack) {
        div.appendChild(document.createTextNode(symbol));
        div.appendChild(document.createElement("br"));
      }
      div.appendChild(document.createElement("br"));
    }
  }
};

function SymbolicationRequest(aPrefix, aRenderHeader, aMemoryMap, aStacks) {
  this.prefix = aPrefix;
  this.renderHeader = aRenderHeader;
  this.memoryMap = aMemoryMap;
  this.stacks = aStacks;
}
SymbolicationRequest.prototype.handleSymbolResponse = function() {
  StackRenderer.renderSymbolicatedStacks(this.prefix, this.symbolRequest,
                                         this.renderHeader);
};
SymbolicationRequest.prototype.fetchSymbols = function() {
  let symbolServerURI =
    getPref(PREF_SYMBOL_SERVER_URI, DEFAULT_SYMBOL_SERVER_URI);
  let request = {"memoryMap" : this.memoryMap, "stacks" : this.stacks,
                 "version" : 3};
  let requestJSON = JSON.stringify(request);

  this.symbolRequest = XMLHttpRequest();
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

  


  render: function ChromeHangs_render() {
    let hangs = Telemetry.chromeHangs;
    let stacks = hangs.stacks;
    let memoryMap = hangs.memoryMap;

    StackRenderer.renderStacks("chrome-hangs", stacks, memoryMap,
			       this.renderHangHeader);
  },

  






  renderHangHeader: function ChromeHangs_renderHangHeader(aIndex) {
    let div = document.getElementById("chrome-hangs-data");

    let titleElement = document.createElement("span");
    titleElement.className = "hang-title";

    let durations = Telemetry.chromeHangs.durations;
    let titleText = bundle.formatStringFromName(
      "hangTitle", [aIndex + 1, durations[aIndex]], 2);
    titleElement.appendChild(document.createTextNode(titleText));

    div.appendChild(titleElement);
    div.appendChild(document.createElement("br"));
  }
};

let Histogram = {

  hgramSamplesCaption: bundle.GetStringFromName("histogramSamples"),

  hgramAverageCaption: bundle.GetStringFromName("histogramAverage"),

  hgramSumCaption: bundle.GetStringFromName("histogramSum"),

  






  render: function Histogram_render(aParent, aName, aHgram) {
    let hgram = this.unpack(aHgram);

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

    if (isRTL())
      hgram.values.reverse();

    this.renderValues(outerDiv, hgram.values, hgram.max);

    aParent.appendChild(outerDiv);
  },

  






  unpack: function Histogram_unpack(aHgram) {
    let sample_count = aHgram.counts.reduceRight(function (a, b) a + b);
    let buckets = [0, 1];
    if (aHgram.histogram_type != Telemetry.HISTOGRAM_BOOLEAN) {
      buckets = aHgram.ranges;
    }

    let average =  Math.round(aHgram.sum * 10 / sample_count) / 10;
    let max_value = Math.max.apply(Math, aHgram.counts);

    let first = true;
    let last = 0;
    let values = [];
    for (let i = 0; i < buckets.length; i++) {
      let count = aHgram.counts[i];
      if (!count)
        continue;
      if (first) {
        first = false;
        if (i) {
          values.push([buckets[i - 1], 0]);
        }
      }
      last = i + 1;
      values.push([buckets[i], count]);
    }
    if (last && last < buckets.length) {
      values.push([buckets[last], 0]);
    }

    let result = {
      values: values,
      pretty_average: average,
      max: max_value,
      sample_count: sample_count,
      sum: aHgram.sum
    };

    return result;
  },

  






  renderValues: function Histogram_renderValues(aDiv, aValues, aMaxValue) {
    for (let [label, value] of aValues) {
      let belowEm = Math.round(MAX_BAR_HEIGHT * (value / aMaxValue) * 10) / 10;
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
  }
};

let KeyValueTable = {

  keysHeader: bundle.GetStringFromName("keysHeader"),

  valuesHeader: bundle.GetStringFromName("valuesHeader"),

  


  render: function KeyValueTable_render(aTableID, aMeasurements) {
    let table = document.getElementById(aTableID);
    this.renderHeader(table);
    this.renderBody(table, aMeasurements);
  },

  





  renderHeader: function KeyValueTable_renderHeader(aTable) {
    let headerRow = document.createElement("tr");
    aTable.appendChild(headerRow);

    let keysColumn = document.createElement("th");
    keysColumn.appendChild(document.createTextNode(this.keysHeader + "\t"));
    let valuesColumn = document.createElement("th");
    valuesColumn.appendChild(document.createTextNode(this.valuesHeader + "\n"));

    headerRow.appendChild(keysColumn);
    headerRow.appendChild(valuesColumn);
  },

  






  renderBody: function KeyValueTable_renderBody(aTable, aMeasurements) {
    for (let [key, value] of Iterator(aMeasurements)) {
      if (typeof value == "object") {
        value = JSON.stringify(value);
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






function showEmptySectionMessage(aSectionID) {
  let sectionElement = document.getElementById(aSectionID);

  
  let toggleElements = sectionElement.getElementsByClassName("toggle-caption");
  toggleElements[0].classList.add("hidden");
  toggleElements[1].classList.add("hidden");

  
  let messageElement = sectionElement.getElementsByClassName("empty-caption")[0];
  messageElement.classList.remove("hidden");

  
  let sectionHeaders = sectionElement.getElementsByClassName("section-name");
  for (let sectionHeader of sectionHeaders) {
    sectionHeader.removeEventListener("click", toggleSection);
    sectionHeader.style.cursor = "auto";
  }

  
  let toggleLinks = sectionElement.getElementsByClassName("toggle-caption");
  for (let toggleLink of toggleLinks) {
    toggleLink.removeEventListener("click", toggleSection);
  }
}





function toggleSection(aEvent) {
  let parentElement = aEvent.target.parentElement;
  let sectionDiv = parentElement.getElementsByTagName("div")[0];
  sectionDiv.classList.toggle("hidden");

  let toggleLinks = parentElement.getElementsByClassName("toggle-caption");
  toggleLinks[0].classList.toggle("hidden");
  toggleLinks[1].classList.toggle("hidden");
}




function setupPageHeader()
{
  let serverOwner = getPref(PREF_TELEMETRY_SERVER_OWNER, "Mozilla");
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
      let value = getPref(PREF_TELEMETRY_ENABLED, false);
      Services.prefs.setBoolPref(PREF_TELEMETRY_ENABLED, !value);
  }, false);

  document.getElementById("chrome-hangs-fetch-symbols").addEventListener("click",
    function () {
      let hangs = Telemetry.chromeHangs;
      let req = new SymbolicationRequest("chrome-hangs",
                                         ChromeHangs.renderHangHeader,
                                         hangs.memoryMap, hangs.stacks);
      req.fetchSymbols();
  }, false);

  document.getElementById("chrome-hangs-hide-symbols").addEventListener("click",
    function () {
      ChromeHangs.render();
  }, false);

  document.getElementById("late-writes-fetch-symbols").addEventListener("click",
    function () {
      let lateWrites = TelemetryPing.getPayload().lateWrites;
      let req = new SymbolicationRequest("late-writes", function() {},
                                         lateWrites.memoryMap,
                                         lateWrites.stacks);
      req.fetchSymbols();
  }, false);

  document.getElementById("late-writes-hide-symbols").addEventListener("click",
    function () {
      let ping = TelemetryPing.getPayload();
      LateWritesSingleton.renderLateWrites(ping.lateWrites);
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

  
  SlowSQL.render();

  
  ChromeHangs.render();

  
  let histograms = Telemetry.histogramSnapshots;
  if (Object.keys(histograms).length) {
    let hgramDiv = document.getElementById("histograms");
    for (let [name, hgram] of Iterator(histograms)) {
      Histogram.render(hgramDiv, name, hgram);
    }
  } else {
    showEmptySectionMessage("histograms-section");
  }

  
  let addonDiv = document.getElementById("addon-histograms");
  let addonHistogramsRendered = false;
  let addonData = Telemetry.addonHistogramSnapshots;
  for (let [addon, histograms] of Iterator(addonData)) {
    for (let [name, hgram] of Iterator(histograms)) {
      addonHistogramsRendered = true;
      Histogram.render(addonDiv, addon + ": " + name, hgram);
    }
  }

  if (!addonHistogramsRendered) {
    showEmptySectionMessage("addon-histograms-section");
  }

  
  Telemetry.asyncFetchTelemetryData(displayPingData);
};

let LateWritesSingleton = {
  renderLateWrites: function LateWritesSingleton_renderLateWrites(lateWrites) {
    let stacks = lateWrites.stacks;
    let memoryMap = lateWrites.memoryMap;
    function f() {}
    StackRenderer.renderStacks('late-writes', stacks, memoryMap, f);
  }
};








function sortStartupMilestones(aSimpleMeasurements) {
  
  const startupMilestones =
    ["start", "main", "startupCrashDetectionBegin", "createTopLevelWindow",
     "firstPaint", "delayedStartupStarted", "firstLoadURI",
     "sessionRestoreInitialized", "sessionRestoreRestoring", "sessionRestored",
     "delayedStartupFinished", "startupCrashDetectionEnd",
     "AMI_startup_begin", "AMI_startup_end", "XPI_startup_begin", "XPI_startup_end",
     "XPI_bootstrap_addons_begin", "XPI_bootstrap_addons_end"];

  let sortedKeys = Object.keys(aSimpleMeasurements);

  
  sortedKeys.sort(function keyCompare(keyA, keyB) {
    let isKeyAMilestone = (startupMilestones.indexOf(keyA) > -1);
    let isKeyBMilestone = (startupMilestones.indexOf(keyB) > -1);

    
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
  let ping = TelemetryPing.getPayload();

  
  let simpleMeasurements = sortStartupMilestones(ping.simpleMeasurements);
  if (Object.keys(simpleMeasurements).length) {
    KeyValueTable.render("simple-measurements-table", simpleMeasurements);
  } else {
    showEmptySectionMessage("simple-measurements-section");
  }

  LateWritesSingleton.renderLateWrites(ping.lateWrites);

  
  if (Object.keys(ping.info).length) {
    KeyValueTable.render("system-info-table", ping.info);
  } else {
    showEmptySectionMessage("system-info-section");
  }
}

window.addEventListener("load", onLoad, false);
