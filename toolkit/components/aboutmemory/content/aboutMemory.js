







































"use strict";





const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const KIND_NONHEAP           = Ci.nsIMemoryReporter.KIND_NONHEAP;
const KIND_HEAP              = Ci.nsIMemoryReporter.KIND_HEAP;
const KIND_OTHER             = Ci.nsIMemoryReporter.KIND_OTHER;
const KIND_SUMMARY           = Ci.nsIMemoryReporter.KIND_SUMMARY;
const UNITS_BYTES            = Ci.nsIMemoryReporter.UNITS_BYTES;
const UNITS_COUNT            = Ci.nsIMemoryReporter.UNITS_COUNT;
const UNITS_COUNT_CUMULATIVE = Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE;
const UNITS_PERCENTAGE       = Ci.nsIMemoryReporter.UNITS_PERCENTAGE;




let gVerbose;
{
  let split = document.location.href.split('?');
  document.title = split[0].toLowerCase();
  gVerbose = split.length == 2 && split[1].toLowerCase() == 'verbose';
}

let gChildMemoryListener = undefined;


String.prototype.startsWith =
  function(s) { return this.lastIndexOf(s, 0) === 0; }






function flipBackslashes(aUnsafeStr)
{
  return aUnsafeStr.replace(/\\/g, '/');
}

const gAssertionFailureMsgPrefix = "aboutMemory.js assertion failed: ";

function assert(aCond, aMsg)
{
  if (!aCond) {
    reportAssertionFailure(aMsg)
    throw(gAssertionFailureMsgPrefix + aMsg);
  }
}

function reportAssertionFailure(aMsg)
{
  let debug = Cc["@mozilla.org/xpcom/debug;1"].getService(Ci.nsIDebug2);
  if (debug.isDebugBuild) {
    debug.assertion(aMsg, "false", "aboutMemory.js", 0);
  }
}

function debug(x)
{
  appendElementWithText(document.body, "div", "debug", JSON.stringify(x));
}



function addChildObserversAndUpdate(aUpdateFn)
{
  let os = Cc["@mozilla.org/observer-service;1"].
      getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-memory-reporter-request", null);

  gChildMemoryListener = aUpdateFn;
  os.addObserver(gChildMemoryListener, "child-memory-reporter-update", false);
 
  gChildMemoryListener();
}

function onLoad()
{
  if (document.title === "about:memory") {
    onLoadAboutMemory();
  } else if (document.title === "about:compartments") {
    onLoadAboutCompartments();
  } else {
    assert(false, "Unknown location: " + document.title);
  }
}

function onUnload()
{
  
  
  
  if (gChildMemoryListener) {
    let os = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
    os.removeObserver(gChildMemoryListener, "child-memory-reporter-update");
  }
}




function minimizeMemoryUsage3x(fAfter)
{
  let i = 0;

  function runSoon(f)
  {
    let tm = Cc["@mozilla.org/thread-manager;1"]
              .getService(Ci.nsIThreadManager);

    tm.mainThread.dispatch({ run: f }, Ci.nsIThread.DISPATCH_NORMAL);
  }

  function sendHeapMinNotificationsInner()
  {
    let os = Cc["@mozilla.org/observer-service;1"]
             .getService(Ci.nsIObserverService);
    os.notifyObservers(null, "memory-pressure", "heap-minimize");

    if (++i < 3) {
      runSoon(sendHeapMinNotificationsInner);
    } else {
      os.notifyObservers(null, "after-minimize-memory-usage", "about:memory");
      runSoon(fAfter);
    }
  }

  sendHeapMinNotificationsInner();
}


 














function processMemoryReporters(aMgr, aIgnoreSingle, aIgnoreMulti,
                                aHandleReport)
{
  
  
  
  
  
  
  
  
  

  function handleReport(aProcess, aUnsafePath, aKind, aUnits, aAmount,
                        aDescription)
  {
    checkReport(aUnsafePath, aKind, aUnits, aAmount, aDescription);
    aHandleReport(aProcess, aUnsafePath, aKind, aUnits, aAmount, aDescription);
  }

  let e = aMgr.enumerateReporters();
  while (e.hasMoreElements()) {
    let rOrig = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
    let unsafePath;
    try {
      unsafePath = rOrig.path;
      if (!aIgnoreSingle(unsafePath)) {
        handleReport(rOrig.process, unsafePath, rOrig.kind, rOrig.units, 
                     rOrig.amount, rOrig.description);
      }
    }
    catch (ex) {
      debug("Exception thrown by memory reporter: " + unsafePath + ": " + ex);
    }
  }

  let e = aMgr.enumerateMultiReporters();
  while (e.hasMoreElements()) {
    let mrOrig = e.getNext().QueryInterface(Ci.nsIMemoryMultiReporter);
    let name = mrOrig.name;
    try {
      if (!aIgnoreMulti(name)) {
        mrOrig.collectReports(handleReport, null);
      }
    }
    catch (ex) {
      
      
      
      
      
      
      
      
      let str = ex.toString();
      if (str.search(gAssertionFailureMsgPrefix) >= 0) {
        throw(ex); 
      } else {
        debug("Exception thrown within memory multi-reporter: " + name + ": " +
              ex);
      }
    }
  }
}




const gSentenceRegExp = /^[A-Z].*\.\)?$/m;

function checkReport(aUnsafePath, aKind, aUnits, aAmount, aDescription)
{
  if (aUnsafePath.startsWith("explicit/")) {
    assert(aKind === KIND_HEAP || aKind === KIND_NONHEAP, "bad explicit kind");
    assert(aUnits === UNITS_BYTES, "bad explicit units");
    assert(aDescription.match(gSentenceRegExp),
           "non-sentence explicit description");

  } else if (aUnsafePath.startsWith("smaps/")) {
    assert(aKind === KIND_NONHEAP, "bad smaps kind");
    assert(aUnits === UNITS_BYTES, "bad smaps units");
    assert(aDescription !== "", "empty smaps description");

  } else if (aKind === KIND_SUMMARY) {
    assert(!aUnsafePath.startsWith("explicit/") &&
           !aUnsafePath.startsWith("smaps/"),
           "bad SUMMARY path");

  } else {
    assert(aUnsafePath.indexOf("/") === -1, "'other' path contains '/'");
    assert(aKind === KIND_OTHER, "bad other kind: " + aUnsafePath);
    assert(aDescription.match(gSentenceRegExp),
           "non-sentence other description " + aDescription);
  }
}



function clearBody()
{
  let oldBody = document.body;
  let body = oldBody.cloneNode(false);
  oldBody.parentNode.replaceChild(body, oldBody);
  body.classList.add(gVerbose ? 'verbose' : 'non-verbose');
  return body;
}

function appendTextNode(aP, aText)
{
  let e = document.createTextNode(aText);
  aP.appendChild(e);
  return e;
}

function appendElement(aP, aTagName, aClassName)
{
  let e = document.createElement(aTagName);
  if (aClassName) {
    e.className = aClassName;
  }
  aP.appendChild(e);
  return e;
}

function appendElementWithText(aP, aTagName, aClassName, aText)
{
  let e = appendElement(aP, aTagName, aClassName);
  appendTextNode(e, aText);
  return e;
}





const kTreeDescriptions = {
  'explicit' :
"This tree covers explicit memory allocations by the application, both at the \
operating system level (via calls to functions such as VirtualAlloc, \
vm_allocate, and mmap), and at the heap allocation level (via functions such \
as malloc, calloc, realloc, memalign, operator new, and operator new[]).\
\n\n\
It excludes memory that is mapped implicitly such as code and data segments, \
and thread stacks.  It also excludes heap memory that has been freed by the \
application but is still being held onto by the heap allocator. \
\n\n\
It is not guaranteed to cover every explicit allocation, but it does cover \
most (including the entire heap), and therefore it is the single best number \
to focus on when trying to reduce memory usage.",

  'resident':
"This tree shows how much space in physical memory each of the process's \
mappings is currently using (the mapping's 'resident set size', or 'RSS'). \
This is a good measure of the 'cost' of the mapping, although it does not \
take into account the fact that shared libraries may be mapped by multiple \
processes but appear only once in physical memory. \
\n\n\
Note that the 'resident' value here might not equal the value for 'resident' \
under 'Other Measurements' because the two measurements are not taken at \
exactly the same time.",

  'pss':
"This tree shows how much space in physical memory can be 'blamed' on this \
process.  For each mapping, its 'proportional set size' (PSS) is the \
mapping's resident size divided by the number of processes which use the \
mapping.  So if a mapping is private to this process, its PSS should equal \
its RSS.  But if a mapping is shared between three processes, its PSS in each \
of the processes would be 1/3 its RSS.",

  'vsize':
"This tree shows how much virtual addres space each of the process's mappings \
takes up (the mapping's 'vsize').  A mapping may have a large vsize but use \
only a small amount of physical memory; the resident set size of a mapping is \
a better measure of the mapping's 'cost'. \
\n\n\
Note that the 'vsize' value here might not equal the value for 'vsize' under \
'Other Measurements' because the two measurements are not taken at exactly \
the same time.",

  'swap':
"This tree shows how much space in the swap file each of the process's \
mappings is currently using. Mappings which are not in the swap file (i.e., \
nodes which would have a value of 0 in this tree) are omitted."
};

const kTreeNames = {
  'explicit': 'Explicit Allocations',
  'resident': 'Resident Set Size (RSS) Breakdown',
  'pss':      'Proportional Set Size (PSS) Breakdown',
  'vsize':    'Virtual Size Breakdown',
  'swap':     'Swap Usage Breakdown',
  'other':    'Other Measurements'
};

const kMapTreePaths =
  ['smaps/resident', 'smaps/pss', 'smaps/vsize', 'smaps/swap'];



function onLoadAboutMemory()
{
  addChildObserversAndUpdate(updateAboutMemory);
}

function doGlobalGC()
{
  Cu.forceGC();
  let os = Cc["@mozilla.org/observer-service;1"]
            .getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-gc-request", null);
  updateAboutMemory();
}

function doCC()
{
  window.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils)
        .cycleCollect();
  let os = Cc["@mozilla.org/observer-service;1"]
            .getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-cc-request", null);
  updateAboutMemory();
}






function updateAboutMemory()
{
  
  
  
  let body = clearBody();

  let mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
      getService(Ci.nsIMemoryReporterManager);

  
  
  let reportsByProcess = getReportsByProcess(mgr);
  let hasMozMallocUsableSize = mgr.hasMozMallocUsableSize;
  appendProcessReportsElements(body, "Main", reportsByProcess["Main"],
                               hasMozMallocUsableSize);
  for (let process in reportsByProcess) {
    if (process !== "Main") {
      appendProcessReportsElements(body, process, reportsByProcess[process],
                                   hasMozMallocUsableSize);
    }
  }

  appendElement(body, "hr");

  
  const UpDesc = "Re-measure.";
  const GCDesc = "Do a global garbage collection.";
  const CCDesc = "Do a cycle collection.";
  const MPDesc = "Send three \"heap-minimize\" notifications in a " +
                 "row.  Each notification triggers a global garbage " +
                 "collection followed by a cycle collection, and causes the " +
                 "process to reduce memory usage in other ways, e.g. by " +
                 "flushing various caches.";

  function appendButton(aTitle, aOnClick, aText, aId)
  {
    let b = appendElementWithText(body, "button", "", aText);
    b.title = aTitle;
    b.onclick = aOnClick
    if (aId) {
      b.id = aId;
    }
  }

  
  appendButton(UpDesc, updateAboutMemory, "Update", "updateButton");
  appendButton(GCDesc, doGlobalGC,        "GC");
  appendButton(CCDesc, doCC,              "CC");
  appendButton(MPDesc, function() { minimizeMemoryUsage3x(updateAboutMemory); },
                                          "Minimize memory usage");

  let div1 = appendElement(body, "div");
  if (gVerbose) {
    let a = appendElementWithText(div1, "a", "option", "Less verbose");
    a.href = "about:memory";
  } else {
    let a = appendElementWithText(div1, "a", "option", "More verbose");
    a.href = "about:memory?verbose";
  }

  let div2 = appendElement(body, "div");
  let a = appendElementWithText(div2, "a", "option",
                                "Troubleshooting information");
  a.href = "about:support";

  let legendText1 = "Click on a non-leaf node in a tree to expand ('++') " +
                    "or collapse ('--') its children.";
  let legendText2 = "Hover the pointer over the name of a memory report " +
                    "to see a description of what it measures.";

  appendElementWithText(body, "div", "legend", legendText1);
  appendElementWithText(body, "div", "legend", legendText2);
}



function Report(aUnsafePath, aKind, aUnits, aAmount, aDescription)
{
  this._unsafePath  = aUnsafePath;
  this._kind        = aKind;
  this._units       = aUnits;
  this._amount      = aAmount;
  this._description = aDescription;
  
  
}

Report.prototype = {
  
  
  
  merge: function(r) {
    this._amount += r._amount;
    this._nMerged = this._nMerged ? this._nMerged + 1 : 2;
  },

  treeNameMatches: function(aTreeName) {
    
    
    return this._unsafePath.startsWith(aTreeName) &&
           this._unsafePath.charAt(aTreeName.length) === '/';
  }
};

function getReportsByProcess(aMgr)
{
  
  
  
  

  function ignoreSingle(aPath) 
  {
    return (aPath.startsWith("smaps/") && !gVerbose) ||
           aPath.startsWith("compartments/") ||
           aPath.startsWith("ghost-windows/");
  }

  function ignoreMulti(aName)
  {
    return (aName === "smaps" && !gVerbose) ||
           aName === "compartments" ||
           aName === "ghost-windows";
  }

  let reportsByProcess = {};

  function handleReport(aProcess, aUnsafePath, aKind, aUnits, aAmount,
                        aDescription)
  {
    let process = aProcess === "" ? "Main" : aProcess;
    let r = new Report(aUnsafePath, aKind, aUnits, aAmount, aDescription);
    if (!reportsByProcess[process]) {
      reportsByProcess[process] = {};
    }
    let reports = reportsByProcess[process];
    let rOld = reports[r._unsafePath];
    if (rOld) {
      
      
      rOld.merge(r);
    } else {
      reports[r._unsafePath] = r;
    }
  }

  processMemoryReporters(aMgr, ignoreSingle, ignoreMulti, handleReport);

  return reportsByProcess;
}







function TreeNode(aUnsafeName)
{
  
  this._unsafeName = aUnsafeName;
  this._kids = [];
  
  
  
  
  
  
  
  
  
  
}

TreeNode.prototype = {
  findKid: function(aUnsafeName) {
    for (let i = 0; i < this._kids.length; i++) {
      if (this._kids[i]._unsafeName === aUnsafeName) {
        return this._kids[i];
      }
    }
    return undefined;
  },

  toString: function() {
    return formatBytes(this._amount);
  }
};

TreeNode.compare = function(a, b) {
  return b._amount - a._amount;
};











function buildTree(aReports, aTreeName)
{
  
  
  

  
  
  
  let foundReport = false;
  for (let unsafePath in aReports) {
    if (aReports[unsafePath].treeNameMatches(aTreeName)) {
      foundReport = true;
      break;
    }
  }
  if (!foundReport) {
    assert(aTreeName !== 'explicit', "aTreeName !== 'explicit'");
    return null;
  }

  let t = new TreeNode("falseRoot");
  for (let unsafePath in aReports) {
    
    let r = aReports[unsafePath];
    if (r.treeNameMatches(aTreeName)) {
      let unsafeNames = r._unsafePath.split('/');
      let u = t;
      for (let i = 0; i < unsafeNames.length; i++) {
        let unsafeName = unsafeNames[i];
        let uMatch = u.findKid(unsafeName);
        if (uMatch) {
          u = uMatch;
        } else {
          let v = new TreeNode(unsafeName);
          u._kids.push(v);
          u = v;
        }
      }
      
      u._amount = r._amount;
      u._description = r._description;
      u._kind = r._kind;
      if (r._nMerged) {
        u._nMerged = r._nMerged;
      }
      r._done = true;
    }
  }

  
  
  t = t._kids[0];

  
  function fillInNonLeafNodes(aT)
  {
    if (aT._kids.length === 0) {
      
      assert(aT._kind !== undefined, "aT._kind is undefined for leaf node");
    } else {
      
      
      assert(aT._kind === undefined, "aT._kind is defined for non-leaf node");
      let childrenBytes = 0;
      for (let i = 0; i < aT._kids.length; i++) {
        childrenBytes += fillInNonLeafNodes(aT._kids[i]);
      }
      aT._amount = childrenBytes;
      aT._description = "The sum of all entries below '" +
                        flipBackslashes(aT._unsafeName) + "'.";
    }
    return aT._amount;
  }

  fillInNonLeafNodes(t);

  
  
  let slashCount = 0;
  for (let i = 0; i < aTreeName.length; i++) {
    if (aTreeName[i] == '/') {
      assert(t._kids.length == 1, "Not expecting multiple kids here.");
      t = t._kids[0];
    }
  }

  
  t._description = kTreeDescriptions[t._unsafeName];

  return t;
}








function ignoreSmapsTrees(aReports)
{
  for (let unsafePath in aReports) {
    let r = aReports[unsafePath];
    if (r.treeNameMatches("smaps")) {
      r._done = true;
    }
  }
}










function fixUpExplicitTree(aT, aReports)
{
  
  function getKnownHeapUsedBytes(aT)
  {
    let n = 0;
    if (aT._kids.length === 0) {
      
      assert(aT._kind !== undefined, "aT._kind is undefined for leaf node");
      n = aT._kind === KIND_HEAP ? aT._amount : 0;
    } else {
      for (let i = 0; i < aT._kids.length; i++) {
        n += getKnownHeapUsedBytes(aT._kids[i]);
      }
    }
    return n;
  }

  
  
  
  let heapAllocatedReport = aReports["heap-allocated"];
  if (heapAllocatedReport === undefined)
    return false;

  let heapAllocatedBytes = heapAllocatedReport._amount;
  let heapUnclassifiedT = new TreeNode("heap-unclassified");
  heapUnclassifiedT._amount = heapAllocatedBytes - getKnownHeapUsedBytes(aT);
  
  
  
  heapUnclassifiedT._description = kindToString(KIND_HEAP) +
      "Memory not classified by a more specific reporter. This includes " +
      "slop bytes due to internal fragmentation in the heap allocator " +
      "(caused when the allocator rounds up request sizes).";
  aT._kids.push(heapUnclassifiedT);
  aT._amount += heapUnclassifiedT._amount;
  return true;
}










function sortTreeAndInsertAggregateNodes(aTotalBytes, aT)
{
  const kSignificanceThresholdPerc = 1;

  function isInsignificant(aT)
  {
    return !gVerbose &&
           (100 * aT._amount / aTotalBytes) < kSignificanceThresholdPerc;
  }

  if (aT._kids.length === 0) {
    return;
  }

  aT._kids.sort(TreeNode.compare);

  
  
  
  if (isInsignificant(aT._kids[0])) {
    aT._hideKids = true;
    for (let i = 0; i < aT._kids.length; i++) {
      sortTreeAndInsertAggregateNodes(aTotalBytes, aT._kids[i]);
    }
    return;
  }

  
  let i;
  for (i = 0; i < aT._kids.length - 1; i++) {
    if (isInsignificant(aT._kids[i])) {
      
      
      let i0 = i;
      let nAgg = aT._kids.length - i0;
      
      let aggT = new TreeNode("(" + nAgg + " tiny)");
      let aggBytes = 0;
      for ( ; i < aT._kids.length; i++) {
        aggBytes += aT._kids[i]._amount;
        aggT._kids.push(aT._kids[i]);
      }
      aggT._hideKids = true;
      aggT._amount = aggBytes;
      aggT._description =
        nAgg + " sub-trees that are below the " + kSignificanceThresholdPerc +
        "% significance threshold.";
      aT._kids.splice(i0, nAgg, aggT);
      aT._kids.sort(TreeNode.compare);

      
      for (i = 0; i < aggT._kids.length; i++) {
        sortTreeAndInsertAggregateNodes(aTotalBytes, aggT._kids[i]);
      }
      return;
    }

    sortTreeAndInsertAggregateNodes(aTotalBytes, aT._kids[i]);
  }

  
  
  
  sortTreeAndInsertAggregateNodes(aTotalBytes, aT._kids[i]);
}




let gUnsafePathsWithInvalidValuesForThisProcess = [];

function appendWarningElements(aP, aHasKnownHeapAllocated,
                               aHasMozMallocUsableSize)
{
  if (!aHasKnownHeapAllocated && !aHasMozMallocUsableSize) {
    appendElementWithText(aP, "p", "", 
      "WARNING: the 'heap-allocated' memory reporter and the " +
      "moz_malloc_usable_size() function do not work for this platform " +
      "and/or configuration.  This means that 'heap-unclassified' is not " +
      "shown and the 'explicit' tree shows much less memory than it should.");
    appendTextNode(aP, "\n\n");

  } else if (!aHasKnownHeapAllocated) {
    appendElementWithText(aP, "p", "", 
      "WARNING: the 'heap-allocated' memory reporter does not work for this " +
      "platform and/or configuration. This means that 'heap-unclassified' " +
      "is not shown and the 'explicit' tree shows less memory than it should.");
    appendTextNode(aP, "\n\n");

  } else if (!aHasMozMallocUsableSize) {
    appendElementWithText(aP, "p", "", 
      "WARNING: the moz_malloc_usable_size() function does not work for " +
      "this platform and/or configuration.  This means that much of the " +
      "heap-allocated memory is not measured by individual memory reporters " +
      "and so will fall under 'heap-unclassified'.");
    appendTextNode(aP, "\n\n");
  }

  if (gUnsafePathsWithInvalidValuesForThisProcess.length > 0) {
    let div = appendElement(aP, "div");
    appendElementWithText(div, "p", "", 
      "WARNING: the following values are negative or unreasonably large.");
    appendTextNode(div, "\n");  

    let ul = appendElement(div, "ul");
    for (let i = 0;
         i < gUnsafePathsWithInvalidValuesForThisProcess.length;
         i++)
    {
      appendTextNode(ul, " ");
      appendElementWithText(ul, "li", "", 
        flipBackslashes(gUnsafePathsWithInvalidValuesForThisProcess[i]));
      appendTextNode(ul, "\n");
    }

    appendElementWithText(div, "p", "",
      "This indicates a defect in one or more memory reporters.  The " +
      "invalid values are highlighted.");
    appendTextNode(div, "\n\n");  
    gUnsafePathsWithInvalidValuesForThisProcess = [];  
  }
}














function appendProcessReportsElements(aP, aProcess, aReports,
                                      aHasMozMallocUsableSize)
{
  appendElementWithText(aP, "h1", "", aProcess + " Process");
  appendTextNode(aP, "\n\n");   

  
  let warningsDiv = appendElement(aP, "div", "accuracyWarning");

  let explicitTree = buildTree(aReports, 'explicit');
  let hasKnownHeapAllocated = fixUpExplicitTree(explicitTree, aReports);
  sortTreeAndInsertAggregateNodes(explicitTree._amount, explicitTree);
  appendTreeElements(aP, explicitTree, aProcess);

  
  if (gVerbose) {
    kMapTreePaths.forEach(function(t) {
      let tree = buildTree(aReports, t);

      
      
      if (tree) {
        sortTreeAndInsertAggregateNodes(tree._amount, tree);
        tree._hideKids = true;   
        appendTreeElements(aP, tree, aProcess);
      }
    });
  } else {
    
    
    
    ignoreSmapsTrees(aReports);
  }

  
  
  appendOtherElements(aP, aReports);

  
  
  
  appendWarningElements(warningsDiv, hasKnownHeapAllocated,
                        aHasMozMallocUsableSize);
}









function hasNegativeSign(aN)
{
  if (aN === 0) {                   
    return 1 / aN === -Infinity;    
  }
  return aN < 0;
}














function formatInt(aN, aExtra)
{
  let neg = false;
  if (hasNegativeSign(aN)) {
    neg = true;
    aN = -aN;
  }
  let s = [];
  while (true) {
    let k = aN % 1000;
    aN = Math.floor(aN / 1000);
    if (aN > 0) {
      if (k < 10) {
        s.unshift(",00", k);
      } else if (k < 100) {
        s.unshift(",0", k);
      } else {
        s.unshift(",", k);
      }
    } else {
      s.unshift(k);
      break;
    }
  }
  if (neg) {
    s.unshift("-");
  }
  if (aExtra) {
    s.push(aExtra);
  }
  return s.join("");
}








function formatBytes(aBytes)
{
  let unit = gVerbose ? " B" : " MB";

  let s;
  if (gVerbose) {
    s = formatInt(aBytes, unit);
  } else {
    let mbytes = (aBytes / (1024 * 1024)).toFixed(2);
    let a = String(mbytes).split(".");
    
    s = formatInt(Number(a[0])) + "." + a[1] + unit;
  }
  return s;
}








function formatPercentage(aPerc100x)
{
  return (aPerc100x / 100).toFixed(2) + "%";
}












function pad(aS, aN, aC)
{
  let padding = "";
  let n2 = aN - aS.length;
  for (let i = 0; i < n2; i++) {
    padding += aC;
  }
  return padding + aS;
}





const kHorizontal       = "\u2500",
      kVertical         = "\u2502",
      kUpAndRight       = "\u2514",
      kVerticalAndRight = "\u251c",
      kDoubleHorizontalSep = " \u2500\u2500 ";

function appendMrValueSpan(aP, aValue, aIsInvalid)
{
  appendElementWithText(aP, "span", "mrValue" + (aIsInvalid ? " invalid" : ""),
                        aValue);
}

function kindToString(aKind)
{
  switch (aKind) {
   case KIND_NONHEAP: return "(Non-heap) ";
   case KIND_HEAP:    return "(Heap) ";
   case KIND_OTHER:
   case undefined:    return "";
   default:           assert(false, "bad kind in kindToString");
  }
}


const kNoKids   = 0;
const kHideKids = 1;
const kShowKids = 2;

function appendMrNameSpan(aP, aKind, aKidsState, aDescription, aUnsafeName,
                          aIsInvalid, aNMerged)
{
  let text = "";
  if (aKidsState === kNoKids) {
    appendElementWithText(aP, "span", "mrSep", kDoubleHorizontalSep);
  } else if (aKidsState === kHideKids) {
    appendElementWithText(aP, "span", "mrSep",        " ++ ");
    appendElementWithText(aP, "span", "mrSep hidden", " -- ");
  } else if (aKidsState === kShowKids) {
    appendElementWithText(aP, "span", "mrSep hidden", " ++ ");
    appendElementWithText(aP, "span", "mrSep",        " -- ");
  } else {
    assert(false, "bad aKidsState");
  }

  let nameSpan = appendElementWithText(aP, "span", "mrName",
                                       flipBackslashes(aUnsafeName));
  nameSpan.title = kindToString(aKind) + aDescription;

  if (aIsInvalid) {
    let noteSpan = appendElementWithText(aP, "span", "mrNote", " [?!]");
    noteSpan.title =
      "Warning: this value is invalid and indicates a bug in one or more " +
      "memory reporters. ";
  }
  if (aNMerged) {
    let noteSpan = appendElementWithText(aP, "span", "mrNote",
                                         " [" + aNMerged + "]");
    noteSpan.title =
      "This value is the sum of " + aNMerged +
      " memory reporters that all have the same path.";
  }
}








let gShowSubtreesBySafeTreeId = {};

function assertClassListContains(e, className) {
  assert(e, "undefined " + className);
  assert(e.classList.contains(className), "classname isn't " + className);
}

function toggle(aEvent)
{
  
  
  
  
  

  
  let outerSpan = aEvent.target.parentNode;
  assertClassListContains(outerSpan, "hasKids");

  
  let plusSpan  = outerSpan.childNodes[2];
  let minusSpan = outerSpan.childNodes[3];
  assertClassListContains(plusSpan,  "mrSep");
  assertClassListContains(minusSpan, "mrSep");
  let isExpansion = !plusSpan.classList.contains("hidden");
  plusSpan .classList.toggle("hidden");
  minusSpan.classList.toggle("hidden");

  
  let subTreeSpan = outerSpan.nextSibling;
  assertClassListContains(subTreeSpan, "kids");
  subTreeSpan.classList.toggle("hidden");

  
  let safeTreeId = outerSpan.id;
  if (gShowSubtreesBySafeTreeId[safeTreeId] !== undefined) {
    delete gShowSubtreesBySafeTreeId[safeTreeId];
  } else {
    gShowSubtreesBySafeTreeId[safeTreeId] = isExpansion;
  }
}

function expandPathToThisElement(aElement)
{
  if (aElement.classList.contains("kids")) {
    
    aElement.classList.remove("hidden");
    expandPathToThisElement(aElement.previousSibling);  

  } else if (aElement.classList.contains("hasKids")) {
    
    let  plusSpan = aElement.childNodes[2];
    let minusSpan = aElement.childNodes[3];
    assertClassListContains(plusSpan,  "mrSep");
    assertClassListContains(minusSpan, "mrSep");
    plusSpan.classList.add("hidden");
    minusSpan.classList.remove("hidden");
    expandPathToThisElement(aElement.parentNode);       

  } else {
    assertClassListContains(aElement, "entries");
  }
}












function appendTreeElements(aPOuter, aT, aProcess)
{
  let treeBytes = aT._amount;
  let rootStringLength = aT.toString().length;
  let isExplicitTree = aT._unsafeName == 'explicit';

  




















  function appendTreeElements2(aP, aUnsafePrePath, aT, aIndentGuide,
                               aBaseIndentText, aParentStringLength)
  {
    function repeatStr(aA, aC, aN)
    {
      for (let i = 0; i < aN; i++) {
        aA.push(aC);
      }
    }

    let unsafePath = aUnsafePrePath + aT._unsafeName;

    
    
    let tString = aT.toString();
    let extraIndentArray = [];
    let extraIndentLength = Math.max(aParentStringLength - tString.length, 0);
    if (extraIndentLength > 0) {
      repeatStr(extraIndentArray, kHorizontal, extraIndentLength);
      aIndentGuide[aIndentGuide.length - 1]._depth += extraIndentLength;
    }
    let indentText = aBaseIndentText + extraIndentArray.join("");
    appendElementWithText(aP, "span", "treeLine", indentText);

    
    
    let percText = "";
    let tIsInvalid = false;
    if (aT._amount === treeBytes) {
      percText = "100.0";
    } else {
      if (!(0 <= aT._amount && aT._amount <= treeBytes)) {
        tIsInvalid = true;
        gUnsafePathsWithInvalidValuesForThisProcess.push(unsafePath);
        reportAssertionFailure("Invalid value for " +
                               flipBackslashes(unsafePath));
      }
      percText = (100 * aT._amount / treeBytes).toFixed(2);
      percText = pad(percText, 5, '0');
    }
    percText = " (" + percText + "%)";

    
    
    let d;
    let hasKids = aT._kids.length > 0;
    let kidsState;
    let showSubtrees;
    if (hasKids) {
      
      
      let safeTreeId = flipBackslashes(aProcess + ":" + unsafePath);
      showSubtrees = !aT._hideKids;
      if (gShowSubtreesBySafeTreeId[safeTreeId] !== undefined) {
        showSubtrees = gShowSubtreesBySafeTreeId[safeTreeId];
      }
      d = appendElement(aP, "span", "hasKids");
      d.id = safeTreeId;
      d.onclick = toggle;
      kidsState = showSubtrees ? kShowKids : kHideKids;
    } else {
      assert(!aT._hideKids, "leaf node with _hideKids set")
      kidsState = kNoKids;
      d = aP;
    }

    appendMrValueSpan(d, tString, tIsInvalid);
    appendElementWithText(d, "span", "mrPerc", percText);

    
    
    let kind = isExplicitTree ? aT._kind : undefined;
    appendMrNameSpan(d, kind, kidsState, aT._description, aT._unsafeName,
                     tIsInvalid, aT._nMerged);
    appendTextNode(d, "\n");

    
    
    if (!gVerbose && tIsInvalid) {
      expandPathToThisElement(d);
    }

    if (hasKids) {
      
      d = appendElement(aP, "span", showSubtrees ? "kids" : "kids hidden");

      for (let i = 0; i < aT._kids.length; i++) {
        
        aIndentGuide.push({ _isLastKid: (i === aT._kids.length - 1), _depth: 3 });

        
        let baseIndentArray = [];
        if (aIndentGuide.length > 0) {
          let j;
          for (j = 0; j < aIndentGuide.length - 1; j++) {
            baseIndentArray.push(aIndentGuide[j]._isLastKid ? " " : kVertical);
            repeatStr(baseIndentArray, " ", aIndentGuide[j]._depth - 1);
          }
          baseIndentArray.push(aIndentGuide[j]._isLastKid ?
                               kUpAndRight : kVerticalAndRight);
          repeatStr(baseIndentArray, kHorizontal, aIndentGuide[j]._depth - 1);
        }

        let baseIndentText = baseIndentArray.join("");
        appendTreeElements2(d, unsafePath + "/", aT._kids[i], aIndentGuide,
                            baseIndentText, tString.length);
        aIndentGuide.pop();
      }
    }
  }

  appendSectionHeader(aPOuter, kTreeNames[aT._unsafeName]);
 
  let pre = appendElement(aPOuter, "pre", "entries");
  appendTreeElements2(pre, "", aT, [], "", rootStringLength);
  appendTextNode(aPOuter, "\n");  
}



function OtherReport(aUnsafePath, aUnits, aAmount, aDescription, aNMerged)
{
  
  this._unsafePath = aUnsafePath;
  this._units    = aUnits;
  this._amount = aAmount;
  this._description = aDescription;
  this._asString = this.toString();
}

OtherReport.prototype = {
  toString: function() {
    switch (this._units) {
      case UNITS_BYTES:            return formatBytes(this._amount);
      case UNITS_COUNT:
      case UNITS_COUNT_CUMULATIVE: return formatInt(this._amount);
      case UNITS_PERCENTAGE:       return formatPercentage(this._amount);
      default:
        assert(false, "bad units in OtherReport.toString");
    }
  },

  isInvalid: function() {
    let n = this._amount;
    switch (this._units) {
      case UNITS_BYTES:
      case UNITS_COUNT:
      case UNITS_COUNT_CUMULATIVE: return n < 0;
      case UNITS_PERCENTAGE:       return n < 0; 
      default:
        assert(false, "bad units in OtherReport.isInvalid");
    }
  }
};

OtherReport.compare = function(a, b) {
  return a._unsafePath < b._unsafePath ? -1 :
         a._unsafePath > b._unsafePath ?  1 :
         0;
};












function appendOtherElements(aP, aReportsByProcess)
{
  appendSectionHeader(aP, kTreeNames['other']);

  let pre = appendElement(aP, "pre", "entries");

  
  
  
  let maxStringLength = 0;
  let otherReports = [];
  for (let unsafePath in aReportsByProcess) {
    let r = aReportsByProcess[unsafePath];
    if (!r._done) {
      assert(r._nMerged === undefined, "dup'd OTHER report");
      let o = new OtherReport(r._unsafePath, r._units, r._amount,
                              r._description);
      otherReports.push(o);
      if (o._asString.length > maxStringLength) {
        maxStringLength = o._asString.length;
      }
    }
  }
  otherReports.sort(OtherReport.compare);

  
  let text = "";
  for (let i = 0; i < otherReports.length; i++) {
    let o = otherReports[i];
    let oIsInvalid = o.isInvalid();
    if (oIsInvalid) {
      gUnsafePathsWithInvalidValuesForThisProcess.push(o._unsafePath);
      reportAssertionFailure("Invalid value for " +
                             flipBackslashes(o._unsafePath));
    }
    appendMrValueSpan(pre, pad(o._asString, maxStringLength, ' '), oIsInvalid);
    appendMrNameSpan(pre, KIND_OTHER, kNoKids, o._description, o._unsafePath,
                     oIsInvalid);
    appendTextNode(pre, "\n");
  }

  appendTextNode(aP, "\n");  
}

function appendSectionHeader(aP, aText)
{
  appendElementWithText(aP, "h2", "", aText);
  appendTextNode(aP, "\n");
}





function onLoadAboutCompartments()
{
  
  
  
  
  
  updateAboutCompartments();
  minimizeMemoryUsage3x(
    function() { addChildObserversAndUpdate(updateAboutCompartments); });
}




function updateAboutCompartments()
{
  
  
  
  let body = clearBody();

  let mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
      getService(Ci.nsIMemoryReporterManager);

  let compartmentsByProcess = getCompartmentsByProcess(mgr);
  let ghostWindowsByProcess = getGhostWindowsByProcess(mgr);

  function handleProcess(aProcess) {
    appendProcessAboutCompartmentsElements(body, aProcess,
                                           compartmentsByProcess[aProcess],
                                           ghostWindowsByProcess[aProcess]);
  }

  
  
  handleProcess('Main');
  for (let process in compartmentsByProcess) {
    if (process !== "Main") {
      handleProcess(process);
    }
  }

  appendElement(body, "hr");

  let div1 = appendElement(body, "div");
  let a;
  if (gVerbose) {
    let a = appendElementWithText(div1, "a", "option", "Less verbose");
    a.href = "about:compartments";
  } else {
    let a = appendElementWithText(div1, "a", "option", "More verbose");
    a.href = "about:compartments?verbose";
  }
}



function Compartment(aUnsafeName, aIsSystemCompartment)
{
  this._unsafeName          = aUnsafeName;
  this._isSystemCompartment = aIsSystemCompartment;
  
}

Compartment.prototype = {
  merge: function(r) {
    this._nMerged = this._nMerged ? this._nMerged + 1 : 2;
  }
};

function getCompartmentsByProcess(aMgr)
{
  
  
  

  function ignoreSingle(aPath) 
  {
    return !aPath.startsWith("compartments/");
  }

  function ignoreMulti(aName)
  {
    return aName !== "compartments";
  }

  let compartmentsByProcess = {};

  function handleReport(aProcess, aUnsafePath, aKind, aUnits, aAmount,
                        aDescription)
  {
    let process = aProcess === "" ? "Main" : aProcess;
    let unsafeNames = aUnsafePath.split('/');
    let isSystemCompartment;
    if (unsafeNames[0] === "compartments" && unsafeNames[1] == "system" &&
        unsafeNames.length == 3)
    {
      isSystemCompartment = true;

    } else if (unsafeNames[0] === "compartments" && unsafeNames[1] == "user" &&
        unsafeNames.length == 3)
    {
      isSystemCompartment = false;
      
      
      
      if (unsafeNames[2].startsWith("moz-nullprincipal:{")) {
        isSystemCompartment = true;
      }

    } else {
      assert(false, "bad compartments path: " + aUnsafePath);
    }
    let c = new Compartment(unsafeNames[2], isSystemCompartment);

    if (!compartmentsByProcess[process]) {
      compartmentsByProcess[process] = {};
    }
    let compartments = compartmentsByProcess[process];
    let cOld = compartments[c._unsafeName];
    if (cOld) {
      
      
      cOld.merge(c);
    } else {
      compartments[c._unsafeName] = c;
    }
  }

  processMemoryReporters(aMgr, ignoreSingle, ignoreMulti, handleReport);

  return compartmentsByProcess;
}

function GhostWindow(aUnsafeURL)
{
  
  
  this._unsafeName = aUnsafeURL;

  
}

GhostWindow.prototype = {
  merge: function(r) {
    this._nMerged = this._nMerged ? this._nMerged + 1 : 2;
  }
};

function getGhostWindowsByProcess(aMgr)
{
  function ignoreSingle(aPath) 
  {
    return !aPath.startsWith('ghost-windows/')
  }

  function ignoreMulti(aName)
  {
    return aName !== "ghost-windows";
  }

  let ghostWindowsByProcess = {};

  function handleReport(aProcess, aUnsafePath, aKind, aUnits, aAmount,
                        aDescription)
  {
    let unsafeSplit = aUnsafePath.split('/');
    assert(unsafeSplit[0] == 'ghost-windows',
           'Unexpected path in getGhostWindowsByProcess: ' + aUnsafePath);

    let unsafeURL = unsafeSplit[1];
    let ghostWindow = new GhostWindow(unsafeURL);

    let process = aProcess === "" ? "Main" : aProcess;
    if (!ghostWindowsByProcess[process]) {
      ghostWindowsByProcess[process] = {};
    }

    if (ghostWindowsByProcess[process][unsafeURL]) {
      ghostWindowsByProcess[process][unsafeURL].merge(ghostWindow);
    }
    else {
      ghostWindowsByProcess[process][unsafeURL] = ghostWindow;
    }
  }

  processMemoryReporters(aMgr, ignoreSingle, ignoreMulti, handleReport);

  return ghostWindowsByProcess;
}



function appendProcessAboutCompartmentsElementsHelper(aP, aEntries, aKindString)
{
  
  
  aEntries = aEntries ? aEntries : {};

  appendElementWithText(aP, "h2", "", aKindString + "\n");

  let uPre = appendElement(aP, "pre", "entries");

  let lines = [];
  for (let name in aEntries) {
    let e = aEntries[name];
    let line = flipBackslashes(e._unsafeName);
    if (e._nMerged) {
      line += ' [' + e._nMerged + ']';
    }
    line += '\n';
    lines.push(line);
  }
  lines.sort();

  for (let i = 0; i < lines.length; i++) {
    appendElementWithText(uPre, "span", "", lines[i]);
  }

  appendTextNode(aP, "\n");   
}















function appendProcessAboutCompartmentsElements(aP, aProcess, aCompartments, aGhostWindows)
{
  appendElementWithText(aP, "h1", "", aProcess + " Process");
  appendTextNode(aP, "\n\n");   

  let userCompartments = {};
  let systemCompartments = {};
  for (let name in aCompartments) {
    let c = aCompartments[name];
    if (c._isSystemCompartment) {
      systemCompartments[name] = c;
    }
    else {
      userCompartments[name] = c;
    }
  }
  
  appendProcessAboutCompartmentsElementsHelper(aP, userCompartments, "User Compartments");
  appendProcessAboutCompartmentsElementsHelper(aP, systemCompartments, "System Compartments");
  appendProcessAboutCompartmentsElementsHelper(aP, aGhostWindows, "Ghost Windows");
}

