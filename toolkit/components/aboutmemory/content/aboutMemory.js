





































"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

const gVerbose = location.href === "about:memory?verbose";

var gAddedObserver = false;

const KIND_NONHEAP           = Ci.nsIMemoryReporter.KIND_NONHEAP;
const KIND_HEAP              = Ci.nsIMemoryReporter.KIND_HEAP;
const KIND_OTHER             = Ci.nsIMemoryReporter.KIND_OTHER;
const UNITS_BYTES            = Ci.nsIMemoryReporter.UNITS_BYTES;
const UNITS_COUNT            = Ci.nsIMemoryReporter.UNITS_COUNT;
const UNITS_COUNT_CUMULATIVE = Ci.nsIMemoryReporter.UNITS_COUNT_CUMULATIVE;
const UNITS_PERCENTAGE       = Ci.nsIMemoryReporter.UNITS_PERCENTAGE;

const kUnknown = -1;    





function makeSafe(aUnsafeStr)
{
  return aUnsafeStr.replace(/\\/g, '/');
}

const kTreeUnsafeDescriptions = {
  'explicit' :
    "This tree covers explicit memory allocations by the application, " +
    "both at the operating system level (via calls to functions such as " +
    "VirtualAlloc, vm_allocate, and mmap), and at the heap allocation level " +
    "(via functions such as malloc, calloc, realloc, memalign, operator " +
    "new, and operator new[]).  It excludes memory that is mapped implicitly " +
    "such as code and data segments, and thread stacks.  It also excludes " +
    "heap memory that has been freed by the application but is still being " +
    "held onto by the heap allocator.  It is not guaranteed to cover every " +
    "explicit allocation, but it does cover most (including the entire " +
    "heap), and therefore it is the single best number to focus on when " +
    "trying to reduce memory usage.",

  'resident':
    "This tree shows how much space in physical memory each of the " +
    "process's mappings is currently using (the mapping's 'resident set size', " +
    "or 'RSS'). This is a good measure of the 'cost' of the mapping, although " +
    "it does not take into account the fact that shared libraries may be mapped " +
    "by multiple processes but appear only once in physical memory. " +
    "Note that the 'resident' value here might not equal the value for " +
    "'resident' under 'Other Measurements' because the two measurements are not " +
    "taken at exactly the same time.",

  'pss':
    "This tree shows how much space in physical memory can be 'blamed' on this " +
    "process.  For each mapping, its 'proportional set size' (PSS) is the " +
    "mapping's resident size divided by the number of processes which use the " +
    "mapping.  So if a mapping is private to this process, its PSS should equal " +
    "its RSS.  But if a mapping is shared between three processes, its PSS in " +
    "each of the processes would be 1/3 its RSS.",

  'vsize':
    "This tree shows how much virtual addres space each of the process's " +
    "mappings takes up (the mapping's 'vsize').  A mapping may have a large " +
    "vsize but use only a small amount of physical memory; the resident set size " +
    "of a mapping is a better measure of the mapping's 'cost'. Note that the " +
    "'vsize' value here might not equal the value for 'vsize' under 'Other " +
    "Measurements' because the two measurements are not taken at exactly the " +
    "same time.",

  'swap':
    "This tree shows how much space in the swap file each of the process's " +
    "mappings is currently using. Mappings which are not in the swap file " +
    "(i.e., nodes which would have a value of 0 in this tree) are omitted."
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

function onLoad()
{
  var os = Cc["@mozilla.org/observer-service;1"].
      getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-memory-reporter-request", null);

  os.addObserver(ChildMemoryListener, "child-memory-reporter-update", false);
  gAddedObserver = true;

  update();
}

function onUnload()
{
  
  
  
  if (gAddedObserver) {
    var os = Cc["@mozilla.org/observer-service;1"].
        getService(Ci.nsIObserverService);
    os.removeObserver(ChildMemoryListener, "child-memory-reporter-update");
  }
}

function ChildMemoryListener(aSubject, aTopic, aData)
{
  update();
}

function doGlobalGC()
{
  Cu.forceGC();
  var os = Cc["@mozilla.org/observer-service;1"]
            .getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-gc-request", null);
  update();
}

function doCC()
{
  window.QueryInterface(Ci.nsIInterfaceRequestor)
        .getInterface(Ci.nsIDOMWindowUtils)
        .cycleCollect();
  var os = Cc["@mozilla.org/observer-service;1"]
            .getService(Ci.nsIObserverService);
  os.notifyObservers(null, "child-cc-request", null);
  update();
}




function sendHeapMinNotifications()
{
  function runSoon(f)
  {
    var tm = Cc["@mozilla.org/thread-manager;1"]
              .getService(Ci.nsIThreadManager);

    tm.mainThread.dispatch({ run: f }, Ci.nsIThread.DISPATCH_NORMAL);
  }

  function sendHeapMinNotificationsInner()
  {
    var os = Cc["@mozilla.org/observer-service;1"]
             .getService(Ci.nsIObserverService);
    os.notifyObservers(null, "memory-pressure", "heap-minimize");

    if (++j < 3)
      runSoon(sendHeapMinNotificationsInner);
    else
      runSoon(update);
  }

  var j = 0;
  sendHeapMinNotificationsInner();
}

function Reporter(aUnsafePath, aKind, aUnits, aAmount, aUnsafeDesc)
{
  this._unsafePath  = aUnsafePath;
  this._kind        = aKind;
  this._units       = aUnits;
  this._amount      = aAmount;
  this._unsafeDescription = aUnsafeDesc;
  
  
}

Reporter.prototype = {
  
  
  
  
  merge: function(r) {
    if (this._amount !== kUnknown && r._amount !== kUnknown) {
      this._amount += r._amount;
    } else if (this._amount === kUnknown && r._amount !== kUnknown) {
      this._amount = r._amount;
    }
    this._nMerged = this._nMerged ? this._nMerged + 1 : 2;
  },

  treeNameMatches: function(aTreeName) {
    
    
    aTreeName += "/";
    return this._unsafePath.slice(0, aTreeName.length) === aTreeName;
  }
};

function getReportersByProcess(aMgr)
{
  
  
  
  
  
  
  
  
  
  
  var reportersByProcess = {};

  function addReporter(aProcess, aUnsafePath, aKind, aUnits, aAmount,
                       aUnsafeDesc)
  {
    var process = aProcess === "" ? "Main" : aProcess;
    var r = new Reporter(aUnsafePath, aKind, aUnits, aAmount, aUnsafeDesc);
    if (!reportersByProcess[process]) {
      reportersByProcess[process] = {};
    }
    var reporters = reportersByProcess[process];
    var reporter = reporters[r._unsafePath];
    if (reporter) {
      
      
      reporter.merge(r);
    } else {
      reporters[r._unsafePath] = r;
    }
  }

  
  var e = aMgr.enumerateReporters();
  while (e.hasMoreElements()) {
    var rOrig = e.getNext().QueryInterface(Ci.nsIMemoryReporter);
    try {
      addReporter(rOrig.process, rOrig.path, rOrig.kind, rOrig.units,
                  rOrig.amount, rOrig.description);
    }
    catch(e) {
      debug("An error occurred when collecting results from the memory reporter " +
            rOrig.path + ": " + e);
    }
  }
  var e = aMgr.enumerateMultiReporters();
  while (e.hasMoreElements()) {
    var mrOrig = e.getNext().QueryInterface(Ci.nsIMemoryMultiReporter);
    
    if (!gVerbose && mrOrig.name === "smaps") {
      continue;
    }

    try {
      mrOrig.collectReports(addReporter, null);
    }
    catch(e) {
      debug("An error occurred when collecting a multi-reporter's results: " + e);
    }
  }

  return reportersByProcess;
}

function appendTextNode(aP, aText)
{
  var e = document.createTextNode(aText);
  aP.appendChild(e);
  return e;
}

function appendElement(aP, aTagName, aClassName)
{
  var e = document.createElement(aTagName);
  e.className = aClassName;
  aP.appendChild(e);
  return e;
}

function appendElementWithText(aP, aTagName, aClassName, aText)
{
  var e = appendElement(aP, aTagName, aClassName);
  appendTextNode(e, aText);
  return e;
}




function update()
{
  
  
  var oldContent = document.getElementById("content");
  var content = oldContent.cloneNode(false);
  oldContent.parentNode.replaceChild(content, oldContent);
  content.classList.add(gVerbose ? 'verbose' : 'non-verbose');

  var mgr = Cc["@mozilla.org/memory-reporter-manager;1"].
      getService(Ci.nsIMemoryReporterManager);

  
  
  var reportersByProcess = getReportersByProcess(mgr);
  var hasMozMallocUsableSize = mgr.hasMozMallocUsableSize;
  appendProcessElements(content, "Main", reportersByProcess["Main"],
                        hasMozMallocUsableSize);
  for (var process in reportersByProcess) {
    if (process !== "Main") {
      appendProcessElements(content, process, reportersByProcess[process],
                            hasMozMallocUsableSize);
    }
  }

  appendElement(content, "hr");

  
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
    var b = appendElementWithText(content, "button", "", aText);
    b.title = aTitle;
    b.onclick = aOnClick
    if (aId) {
      b.id = aId;
    }
  }

  
  appendButton(UpDesc, update,                   "Update", "updateButton");
  appendButton(GCDesc, doGlobalGC,               "GC");
  appendButton(CCDesc, doCC,                     "CC");
  appendButton(MPDesc, sendHeapMinNotifications, "Minimize memory usage");

  var div1 = appendElement(content, "div", "");
  var a;
  if (gVerbose) {
    var a = appendElementWithText(div1, "a", "option", "Less verbose");
    a.href = "about:memory";
  } else {
    var a = appendElementWithText(div1, "a", "option", "More verbose");
    a.href = "about:memory?verbose";
  }

  var div2 = appendElement(content, "div", "");
  a = appendElementWithText(div2, "a", "option", "Troubleshooting information");
  a.href = "about:support";

  var legendText1 = "Click on a non-leaf node in a tree to expand ('++') " +
                    "or collapse ('--') its children.";
  var legendText2 = "Hover the pointer over the name of a memory reporter " +
                    "to see a description of what it measures.";

  appendElementWithText(content, "div", "legend", legendText1);
  appendElementWithText(content, "div", "legend", legendText2);
}





function TreeNode(aUnsafeName)
{
  
  this._unsafeName = aUnsafeName;
  this._kids = [];
  
  
  
  
  
  
  
  
  
  
  
}

TreeNode.prototype = {
  findKid: function(aUnsafeName) {
    for (var i = 0; i < this._kids.length; i++) {
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











function buildTree(aReporters, aTreeName)
{
  
  
  

  
  
  
  var foundReporter = false;
  for (var unsafePath in aReporters) {
    if (aReporters[unsafePath].treeNameMatches(aTreeName)) {
      foundReporter = true;
      break;
    }
  }
  if (!foundReporter) {
    assert(aTreeName !== 'explicit');
    return null;
  }

  var t = new TreeNode("falseRoot");
  for (var unsafePath in aReporters) {
    
    var r = aReporters[unsafePath];
    if (r.treeNameMatches(aTreeName)) {
      assert(r._kind === KIND_HEAP || r._kind === KIND_NONHEAP,
             "reporters in the tree must have KIND_HEAP or KIND_NONHEAP");
      assert(r._units === UNITS_BYTES, "r._units === UNITS_BYTES");
      var unsafeNames = r._unsafePath.split('/');
      var u = t;
      for (var i = 0; i < unsafeNames.length; i++) {
        var unsafeName = unsafeNames[i];
        var uMatch = u.findKid(unsafeName);
        if (uMatch) {
          u = uMatch;
        } else {
          var v = new TreeNode(unsafeName);
          u._kids.push(v);
          u = v;
        }
      }
      
      u._kind = r._kind;
      if (r._nMerged) {
        u._nMerged = r._nMerged;
      }
    }
  }

  
  
  t = t._kids[0];

  
  
  function fillInTree(aT, aUnsafePrePath)
  {
    var unsafePath =
      aUnsafePrePath ? aUnsafePrePath + '/' + aT._unsafeName : aT._unsafeName; 
    if (aT._kids.length === 0) {
      
      assert(aT._kind !== undefined, "aT._kind is undefined for leaf node");
      aT._unsafeDescription = getUnsafeDescription(aReporters, unsafePath);
      var amount = getBytes(aReporters, unsafePath);
      if (amount !== kUnknown) {
        aT._amount = amount;
      } else {
        aT._amount = 0;
        aT._isUnknown = true;
      }
    } else {
      
      
      assert(aT._kind === undefined, "aT._kind is defined for non-leaf node");
      var childrenBytes = 0;
      for (var i = 0; i < aT._kids.length; i++) {
        childrenBytes += fillInTree(aT._kids[i], unsafePath);
      }
      aT._amount = childrenBytes;
      aT._unsafeDescription =
        "The sum of all entries below '" + aT._unsafeName + "'.";
    }
    assert(aT._amount !== kUnknown, "aT._amount !== kUnknown");
    return aT._amount;
  }

  fillInTree(t, "");

  
  
  var slashCount = 0;
  for (var i = 0; i < aTreeName.length; i++) {
    if (aTreeName[i] == '/') {
      assert(t._kids.length == 1, "Not expecting multiple kids here.");
      t = t._kids[0];
    }
  }

  
  t._unsafeDescription = kTreeUnsafeDescriptions[t._unsafeName];

  return t;
}








function ignoreSmapsTrees(aReporters)
{
  for (var unsafePath in aReporters) {
    var r = aReporters[unsafePath];
    if (r.treeNameMatches("smaps")) {
      var dummy = getBytes(aReporters, unsafePath);
    }
  }
}










function fixUpExplicitTree(aT, aReporters)
{
  
  function getKnownHeapUsedBytes(aT)
  {
    var n = 0;
    if (aT._kids.length === 0) {
      
      assert(aT._kind !== undefined, "aT._kind is undefined for leaf node");
      n = aT._kind === KIND_HEAP ? aT._amount : 0;
    } else {
      for (var i = 0; i < aT._kids.length; i++) {
        n += getKnownHeapUsedBytes(aT._kids[i]);
      }
    }
    return n;
  }

  
  
  
  var heapAllocatedBytes = getBytes(aReporters, "heap-allocated", true);
  var heapUnclassifiedT = new TreeNode("heap-unclassified");
  var hasKnownHeapAllocated = heapAllocatedBytes !== kUnknown;
  if (hasKnownHeapAllocated) {
    heapUnclassifiedT._amount =
      heapAllocatedBytes - getKnownHeapUsedBytes(aT);
  } else {
    heapUnclassifiedT._amount = 0;
    heapUnclassifiedT._isUnknown = true;
  }
  
  
  
  heapUnclassifiedT._unsafeDescription = kindToString(KIND_HEAP) +
      "Memory not classified by a more specific reporter. This includes " +
      "slop bytes due to internal fragmentation in the heap allocator " +
      "(caused when the allocator rounds up request sizes).";

  aT._kids.push(heapUnclassifiedT);
  aT._amount += heapUnclassifiedT._amount;

  return hasKnownHeapAllocated;
}










function sortTreeAndInsertAggregateNodes(aTotalBytes, aT)
{
  const kSignificanceThresholdPerc = 1;

  function isInsignificant(aT)
  {
    return !gVerbose &&
           aTotalBytes !== kUnknown &&
           (100 * aT._amount / aTotalBytes) < kSignificanceThresholdPerc;
  }

  if (aT._kids.length === 0) {
    return;
  }

  aT._kids.sort(TreeNode.compare);

  
  
  
  if (isInsignificant(aT._kids[0])) {
    aT._hideKids = true;
    for (var i = 0; i < aT._kids.length; i++) {
      sortTreeAndInsertAggregateNodes(aTotalBytes, aT._kids[i]);
    }
    return;
  }

  
  for (var i = 0; i < aT._kids.length - 1; i++) {
    if (isInsignificant(aT._kids[i])) {
      
      
      var i0 = i;
      var nAgg = aT._kids.length - i0;
      
      var aggT = new TreeNode("(" + nAgg + " tiny)");
      var aggBytes = 0;
      for ( ; i < aT._kids.length; i++) {
        aggBytes += aT._kids[i]._amount;
        aggT._kids.push(aT._kids[i]);
      }
      aggT._hideKids = true;
      aggT._amount = aggBytes;
      aggT._unsafeDescription =
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




var gUnsafePathsWithInvalidValuesForThisProcess = [];

function appendWarningElements(aP, aHasKnownHeapAllocated,
                               aHasMozMallocUsableSize)
{
  if (!aHasKnownHeapAllocated && !aHasMozMallocUsableSize) {
    appendElementWithText(aP, "p", "", 
      "WARNING: the 'heap-allocated' memory reporter and the " +
      "moz_malloc_usable_size() function do not work for this platform " +
      "and/or configuration.  This means that 'heap-unclassified' is zero " +
      "and the 'explicit' tree shows much less memory than it should.");
    appendTextNode(aP, "\n\n");

  } else if (!aHasKnownHeapAllocated) {
    appendElementWithText(aP, "p", "", 
      "WARNING: the 'heap-allocated' memory reporter does not work for this " +
      "platform and/or configuration. This means that 'heap-unclassified' " +
      "is zero and the 'explicit' tree shows less memory than it should.");
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
    var div = appendElement(aP, "div", "");
    appendElementWithText(div, "p", "", 
      "WARNING: the following values are negative or unreasonably large.");
    appendTextNode(div, "\n");  

    var ul = appendElement(div, "ul", "");
    for (var i = 0;
         i < gUnsafePathsWithInvalidValuesForThisProcess.length;
         i++)
    {
      appendTextNode(ul, " ");
      appendElementWithText(ul, "li", "", 
        makeSafe(gUnsafePathsWithInvalidValuesForThisProcess[i]));
      appendTextNode(ul, "\n");
    }

    appendElementWithText(div, "p", "",
      "This indicates a defect in one or more memory reporters.  The " +
      "invalid values are highlighted, but you may need to expand one " +
      "or more sub-trees to see them.");
    appendTextNode(div, "\n\n");  
    gUnsafePathsWithInvalidValuesForThisProcess = [];  
  }
}














function appendProcessElements(aP, aProcess, aReporters,
                               aHasMozMallocUsableSize)
{
  appendElementWithText(aP, "h1", "", aProcess + " Process");
  appendTextNode(aP, "\n\n");   

  
  var warningsDiv = appendElement(aP, "div", "accuracyWarning");

  var explicitTree = buildTree(aReporters, 'explicit');
  var hasKnownHeapAllocated = fixUpExplicitTree(explicitTree, aReporters);
  sortTreeAndInsertAggregateNodes(explicitTree._amount, explicitTree);
  appendTreeElements(aP, explicitTree, aProcess);

  
  if (gVerbose) {
    kMapTreePaths.forEach(function(t) {
      var tree = buildTree(aReporters, t);

      
      
      if (tree) {
        sortTreeAndInsertAggregateNodes(tree._amount, tree);
        tree._hideKids = true;   
        appendTreeElements(aP, tree, aProcess);
      }
    });
  } else {
    
    
    
    ignoreSmapsTrees(aReporters);
  }

  
  
  var otherText = appendOtherElements(aP, aReporters, aProcess);

  
  
  
  var warningElements =
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








function formatInt(aN)
{
  var neg = false;
  if (hasNegativeSign(aN)) {
    neg = true;
    aN = -aN;
  }
  var s = "";
  while (true) {
    var k = aN % 1000;
    aN = Math.floor(aN / 1000);
    if (aN > 0) {
      if (k < 10) {
        s = ",00" + k + s;
      } else if (k < 100) {
        s = ",0" + k + s;
      } else {
        s = "," + k + s;
      }
    } else {
      s = k + s;
      break;
    }
  }
  return neg ? "-" + s : s;
}








function formatBytes(aBytes)
{
  var unit = gVerbose ? "B" : "MB";

  var s;
  if (gVerbose) {
    s = formatInt(aBytes) + " " + unit;
  } else {
    var mbytes = (aBytes / (1024 * 1024)).toFixed(2);
    var a = String(mbytes).split(".");
    
    s = formatInt(Number(a[0])) + "." + a[1] + " " + unit;
  }
  return s;
}








function formatPercentage(aPerc100x)
{
  return (aPerc100x / 100).toFixed(2) + "%";
}












function pad(aS, aN, aC)
{
  var padding = "";
  var n2 = aN - aS.length;
  for (var i = 0; i < n2; i++) {
    padding += aC;
  }
  return padding + aS;
}













function getBytes(aReporters, aUnsafePath, aDoNotMark)
{
  var r = aReporters[aUnsafePath];
  assert(r, "getBytes: no such Reporter: " + makeSafe(aUnsafePath));
  if (!aDoNotMark) {
    r._done = true;
  }
  return r._amount;
}










function getUnsafeDescription(aReporters, aUnsafePath)
{
  var r = aReporters[aUnsafePath];
  assert(r, "getUnsafeDescription: no such Reporter: " + makeSafe(aUnsafePath));
  return r._unsafeDescription;
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

function appendMrNameSpan(aP, aKind, aShowSubtrees, aHasKids, aUnsafeDesc,
                          aUnsafeName, aIsUnknown, aIsInvalid, aNMerged)
{
  var text = "";
  if (aHasKids) {
    if (aShowSubtrees) {
      appendElementWithText(aP, "span", "mrSep hidden", " ++ ");
      appendElementWithText(aP, "span", "mrSep",        " -- ");
    } else {
      appendElementWithText(aP, "span", "mrSep",        " ++ ");
      appendElementWithText(aP, "span", "mrSep hidden", " -- ");
    }
  } else {
    appendElementWithText(aP, "span", "mrSep", kDoubleHorizontalSep);
  }

  var nameSpan = appendElementWithText(aP, "span", "mrName",
                                       makeSafe(aUnsafeName));
  nameSpan.title = kindToString(aKind) + makeSafe(aUnsafeDesc);

  if (aIsUnknown) {
    var noteSpan = appendElementWithText(aP, "span", "mrNote", " [*]");
    noteSpan.title =
      "Warning: this memory reporter was unable to compute a useful value. ";
  }
  if (aIsInvalid) {
    var noteSpan = appendElementWithText(aP, "span", "mrNote", " [?!]");
    noteSpan.title =
      "Warning: this value is invalid and indicates a bug in one or more " +
      "memory reporters. ";
  }
  if (aNMerged) {
    var noteSpan = appendElementWithText(aP, "span", "mrNote",
                                         " [" + aNMerged + "]");
    noteSpan.title =
      "This value is the sum of " + aNMerged +
      " memory reporters that all have the same path.";
  }
}







var gTogglesBySafeTreeId = {};

function toggle(aEvent)
{
  
  
  
  
  

  function assertClassName(span, className) {
    assert(span, "undefined " + className);
    assert(span.nodeName === "span", "non-span " + className);
    assert(span.classList.contains(className), "bad " + className);
  }

  
  var outerSpan = aEvent.target.parentNode;
  assertClassName(outerSpan, "hasKids");

  
  var plusSpan  = outerSpan.childNodes[2];
  var minusSpan = outerSpan.childNodes[3];
  assertClassName(plusSpan,  "mrSep");
  assertClassName(minusSpan, "mrSep");
  plusSpan .classList.toggle("hidden");
  minusSpan.classList.toggle("hidden");

  
  var subTreeSpan = outerSpan.nextSibling;
  assertClassName(subTreeSpan, "kids");
  subTreeSpan.classList.toggle("hidden");

  
  var safeTreeId = outerSpan.id;
  if (gTogglesBySafeTreeId[safeTreeId]) {
    delete gTogglesBySafeTreeId[safeTreeId];
  } else {
    gTogglesBySafeTreeId[safeTreeId] = true;
  }
}












function appendTreeElements(aPOuter, aT, aProcess)
{
  var treeBytes = aT._amount;
  var rootStringLength = aT.toString().length;
  var isExplicitTree = aT._unsafeName == 'explicit';

  

















  function appendTreeElements2(aP, aUnsafePrePath, aT, aIndentGuide,
                               aParentStringLength)
  {
    function repeatStr(aC, aN)
    {
      var s = "";
      for (var i = 0; i < aN; i++) {
        s += aC;
      }
      return s;
    }

    
    
    var unsafePath = aUnsafePrePath + aT._unsafeName;
    var safeTreeId = makeSafe(aProcess + ":" + unsafePath);
    var showSubtrees = !aT._hideKids;
    if (gTogglesBySafeTreeId[safeTreeId]) {
      showSubtrees = !showSubtrees;
    }

    
    var indent = "";
    if (aIndentGuide.length > 0) {
      for (var i = 0; i < aIndentGuide.length - 1; i++) {
        indent += aIndentGuide[i]._isLastKid ? " " : kVertical;
        indent += repeatStr(" ", aIndentGuide[i]._depth - 1);
      }
      indent += aIndentGuide[i]._isLastKid ? kUpAndRight : kVerticalAndRight;
      indent += repeatStr(kHorizontal, aIndentGuide[i]._depth - 1);
    }
    
    
    var tString = aT.toString();
    var extraIndentLength = Math.max(aParentStringLength - tString.length, 0);
    if (extraIndentLength > 0) {
      for (var i = 0; i < extraIndentLength; i++) {
        indent += kHorizontal;
      }
      aIndentGuide[aIndentGuide.length - 1]._depth += extraIndentLength;
    }

    
    
    var percText = "";
    var tIsInvalid = false;
    if (aT._amount === treeBytes) {
      percText = "100.0";
    } else {
      var perc = (100 * aT._amount / treeBytes);
      if (!(0 <= perc && perc <= 100)) {
        tIsInvalid = true;
        gUnsafePathsWithInvalidValuesForThisProcess.push(unsafePath);
      }
      percText = (100 * aT._amount / treeBytes).toFixed(2);
      percText = pad(percText, 5, '0');
    }
    percText = " (" + percText + "%)";

    
    
    var hasKids = aT._kids.length > 0;
    if (!hasKids) {
      assert(!aT._hideKids, "leaf node with _hideKids set")
    }

    appendElementWithText(aP, "span", "treeLine", indent);

    var d;
    if (hasKids) {
      d = appendElement(aP, "span", "hasKids");
      d.id = safeTreeId;
      d.onclick = toggle;
    } else {
      d = aP;
    }

    appendMrValueSpan(d, tString, tIsInvalid);
    appendElementWithText(d, "span", "mrPerc", percText);

    
    
    var kind = isExplicitTree ? aT._kind : undefined;
    appendMrNameSpan(d, kind, showSubtrees, hasKids, aT._unsafeDescription,
                     aT._unsafeName, aT._isUnknown, tIsInvalid, aT._nMerged);
    appendTextNode(d, "\n");

    if (hasKids) {
      
      d = appendElement(aP, "span", showSubtrees ? "kids" : "kids hidden");
    } else {
      d = aP;
    }

    for (var i = 0; i < aT._kids.length; i++) {
      
      aIndentGuide.push({ _isLastKid: (i === aT._kids.length - 1), _depth: 3 });
      appendTreeElements2(d, unsafePath + "/", aT._kids[i], aIndentGuide,
                          tString.length);
      aIndentGuide.pop();
    }
  }

  appendSectionHeader(aPOuter, kTreeNames[aT._unsafeName]);
 
  var pre = appendElement(aPOuter, "pre", "tree");
  appendTreeElements2(pre, "", aT, [], rootStringLength);
  appendTextNode(aPOuter, "\n");  
}

function OtherReporter(aUnsafePath, aUnits, aAmount, aUnsafeDesc, aNMerged)
{
  
  this._unsafePath = aUnsafePath;
  this._units    = aUnits;
  if (aAmount === kUnknown) {
    this._amount     = 0;
    this._isUnknown = true;
  } else {
    this._amount = aAmount;
  }
  this._unsafeDescription = aUnsafeDesc;
  this._asString = this.toString();
}

OtherReporter.prototype = {
  toString: function() {
    switch (this._units) {
      case UNITS_BYTES:            return formatBytes(this._amount);
      case UNITS_COUNT:
      case UNITS_COUNT_CUMULATIVE: return formatInt(this._amount);
      case UNITS_PERCENTAGE:       return formatPercentage(this._amount);
      default:
        assert(false, "bad units in OtherReporter.toString");
    }
  },

  isInvalid: function() {
    var n = this._amount;
    switch (this._units) {
      case UNITS_BYTES:
      case UNITS_COUNT:
      case UNITS_COUNT_CUMULATIVE: return (n !== kUnknown && n < 0);
      case UNITS_PERCENTAGE:       return (n !== kUnknown &&
                                           !(0 <= n && n <= 10000));
      default:
        assert(false, "bad units in OtherReporter.isInvalid");
    }
  }
};

OtherReporter.compare = function(a, b) {
  return a._unsafePath < b._unsafePath ? -1 :
         a._unsafePath > b._unsafePath ?  1 :
         0;
};












function appendOtherElements(aP, aReportersByProcess, aProcess)
{
  appendSectionHeader(aP, kTreeNames['other']);

  var pre = appendElement(aP, "pre", "tree");

  
  
  
  var maxStringLength = 0;
  var otherReporters = [];
  for (var unsafePath in aReportersByProcess) {
    var r = aReportersByProcess[unsafePath];
    if (!r._done) {
      assert(r._kind === KIND_OTHER,
             "_kind !== KIND_OTHER for " + makeSafe(r._unsafePath));
      assert(r._nMerged === undefined);  
      var o = new OtherReporter(r._unsafePath, r._units, r._amount,
                                r._unsafeDescription);
      otherReporters.push(o);
      if (o._asString.length > maxStringLength) {
        maxStringLength = o._asString.length;
      }
    }
  }
  otherReporters.sort(OtherReporter.compare);

  
  var text = "";
  for (var i = 0; i < otherReporters.length; i++) {
    var o = otherReporters[i];
    var oIsInvalid = o.isInvalid();
    if (oIsInvalid) {
      gUnsafePathsWithInvalidValuesForThisProcess.push(o._unsafePath);
    }
    appendMrValueSpan(pre, pad(o._asString, maxStringLength, ' '), oIsInvalid);
    appendMrNameSpan(pre, KIND_OTHER, true,
                     false, o._unsafeDescription,
                     o._unsafePath, o._isUnknown, oIsInvalid);
    appendTextNode(pre, "\n");
  }

  appendTextNode(aP, "\n");  
}

function appendSectionHeader(aP, aText)
{
  appendElementWithText(aP, "h2", "sectionHeader", aText);
  appendTextNode(aP, "\n");
}

function assert(aCond, aMsg)
{
  if (!aCond) {
    throw("assertion failed: " + aMsg);
  }
}

function debug(x)
{
  var content = document.getElementById("content");
  appendElementWithText(content, "div", "legend", JSON.stringify(x));
}
