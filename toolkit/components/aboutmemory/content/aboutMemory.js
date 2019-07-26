






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
  
  return (aUnsafeStr.indexOf('\\') === -1)
         ? aUnsafeStr
         : aUnsafeStr.replace(/\\/g, '/');
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
    let mr = e.getNext().QueryInterface(Ci.nsIMemoryMultiReporter);
    let name = mr.name;
    try {
      if (!aIgnoreMulti(name)) {
        mr.collectReports(handleReport, null);
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
    assert(gSentenceRegExp.test(aDescription),
           "non-sentence explicit description");

  } else if (isSmapsPath(aUnsafePath)) {
    assert(aKind === KIND_NONHEAP, "bad smaps kind");
    assert(aUnits === UNITS_BYTES, "bad smaps units");
    assert(aDescription !== "", "empty smaps description");

  } else if (aKind === KIND_SUMMARY) {
    assert(!aUnsafePath.startsWith("explicit/") && !isSmapsPath(aUnsafePath),
           "bad SUMMARY path");

  } else {
    assert(aUnsafePath.indexOf("/") === -1, "'other' path contains '/'");
    assert(aKind === KIND_OTHER, "bad other kind");
    assert(gSentenceRegExp.test(aDescription),
           "non-sentence other description");
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
  
  
  
  e.textContent = aText;
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

const kSectionNames = {
  'explicit': 'Explicit Allocations',
  'resident': 'Resident Set Size (RSS) Breakdown',
  'pss':      'Proportional Set Size (PSS) Breakdown',
  'vsize':    'Virtual Size Breakdown',
  'swap':     'Swap Usage Breakdown',
  'other':    'Other Measurements'
};

const kSmapsTreePrefixes = ['resident/', 'pss/', 'vsize/', 'swap/'];

function isSmapsPath(aUnsafePath)
{
  for (let i = 0; i < kSmapsTreePrefixes.length; i++) {
    if (aUnsafePath.startsWith(kSmapsTreePrefixes[i])) {
      return true;
    }
  }
  return false;
}



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

  let treesByProcess = {}, othersByProcess = {}, heapTotalByProcess = {};
  getTreesAndOthersByProcess(mgr, treesByProcess, othersByProcess,
                             heapTotalByProcess);

  
  
  let hasMozMallocUsableSize = mgr.hasMozMallocUsableSize;
  appendProcessAboutMemoryElements(body, "Main", treesByProcess["Main"],
                                   othersByProcess["Main"],
                                   heapTotalByProcess["Main"],
                                   hasMozMallocUsableSize);
  for (let process in treesByProcess) {
    if (process !== "Main") {
      appendProcessAboutMemoryElements(body, process, treesByProcess[process],
                                       othersByProcess[process],
                                       heapTotalByProcess[process],
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


















function getTreesAndOthersByProcess(aMgr, aTreesByProcess, aOthersByProcess,
                                    aHeapTotalByProcess)
{
  
  
  
  

  function ignoreSingle(aUnsafePath) 
  {
    return (isSmapsPath(aUnsafePath) && !gVerbose) ||
           aUnsafePath.startsWith("compartments/") ||
           aUnsafePath.startsWith("ghost-windows/");
  }

  function ignoreMulti(aMRName)
  {
    return (aMRName === "smaps" && !gVerbose) ||
           aMRName === "compartments" ||
           aMRName === "ghost-windows";
  }

  function handleReport(aProcess, aUnsafePath, aKind, aUnits, aAmount,
                        aDescription)
  {
    let process = aProcess === "" ? "Main" : aProcess;

    if (aUnsafePath.indexOf('/') !== -1) {
      
      
      
      if (!aTreesByProcess[process]) {
        aTreesByProcess[process] = new TreeNode("tree-of-trees");
      }
      let t = aTreesByProcess[process]; 

      
      
      let unsafeNames = aUnsafePath.split('/');
      let u = t;
      for (let i = 0; i < unsafeNames.length; i++) {
        let unsafeName = unsafeNames[i];
        let uMatch = u.findKid(unsafeName);
        if (uMatch) {
          u = uMatch;
        } else {
          let v = new TreeNode(unsafeName);
          if (!u._kids) {
            u._kids = [];
          }
          u._kids.push(v);
          u = v;
        }
      }
    
      if (u._amount) {
        
        u._amount += aAmount;
        u._nMerged = u._nMerged ? u._nMerged + 1 : 2;
      } else {
        
        u._amount = aAmount;
        u._description = aDescription;
      }

      if (unsafeNames[0] === "explicit" && aKind == KIND_HEAP) {
        if (!aHeapTotalByProcess[process]) {
          aHeapTotalByProcess[process] = 0;
        }
        aHeapTotalByProcess[process] += aAmount;
      }

    } else {
      
      
      if (!aOthersByProcess[process]) {
        aOthersByProcess[process] = {};
      }
      let others = aOthersByProcess[process]; 

      
      assert(!others[aUnsafePath], "dup'd OTHER report");
      others[aUnsafePath] =
        new OtherReport(aUnsafePath, aUnits, aAmount, aDescription);
    }
  }

  processMemoryReporters(aMgr, ignoreSingle, ignoreMulti, handleReport);
}







function TreeNode(aUnsafeName)
{
  
  this._unsafeName = aUnsafeName;
  
  
  
  
  
  
  
  
  
  
}

TreeNode.prototype = {
  findKid: function(aUnsafeName) {
    if (this._kids) {
      for (let i = 0; i < this._kids.length; i++) {
        if (this._kids[i]._unsafeName === aUnsafeName) {
          return this._kids[i];
        }
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











function fillInTree(aTreeOfTrees, aTreePrefix)
{
  assert(aTreePrefix.indexOf('/') == aTreePrefix.length - 1,
         "aTreePrefix doesn't end in '/'");

  
  
  let t = aTreeOfTrees.findKid(aTreePrefix.replace(/\//g, ''));
  if (!t) {
    assert(aTreePrefix !== 'explicit/', "missing explicit tree");
    return null;
  }

  
  function fillInNonLeafNodes(aT, aCannotMerge)
  {
    if (!aT._kids) {
      

    } else if (aT._kids.length === 1 && !aCannotMerge) {
      
      
      let kid = aT._kids[0];
      let kidBytes = fillInNonLeafNodes(kid);
      aT._unsafeName += '/' + kid._unsafeName;
      if (kid._kids) {
        aT._kids = kid._kids;
      } else {
        delete aT._kids;
      }
      aT._amount = kid._amount;
      aT._description = kid._description;
      if (kid._nMerged !== undefined) {
        aT._nMerged = kid._nMerged
      }
      assert(!aT._hideKids && !kid._hideKids, "_hideKids set when merging");

    } else {
      
      
      let kidsBytes = 0;
      for (let i = 0; i < aT._kids.length; i++) {
        kidsBytes += fillInNonLeafNodes(aT._kids[i]);
      }
      aT._amount = kidsBytes;
      aT._description = "The sum of all entries below this one.";
    }
    return aT._amount;
  }

  
  fillInNonLeafNodes(t, true);

  
  t._description = kTreeDescriptions[t._unsafeName];

  return t;
}













function addHeapUnclassifiedNode(aT, aOthers, aHeapTotal)
{
  let heapAllocatedReport = aOthers["heap-allocated"];
  if (heapAllocatedReport === undefined)
    return false;

  let heapAllocatedBytes = heapAllocatedReport._amount;
  let heapUnclassifiedT = new TreeNode("heap-unclassified");
  heapUnclassifiedT._amount = heapAllocatedBytes - aHeapTotal;
  heapUnclassifiedT._description =
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

  if (!aT._kids) {
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
      aggT._kids = [];
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
      "shown and the 'explicit' tree shows much less memory than it should.\n\n");

  } else if (!aHasKnownHeapAllocated) {
    appendElementWithText(aP, "p", "", 
      "WARNING: the 'heap-allocated' memory reporter does not work for this " +
      "platform and/or configuration. This means that 'heap-unclassified' " +
      "is not shown and the 'explicit' tree shows less memory than it should.\n\n");

  } else if (!aHasMozMallocUsableSize) {
    appendElementWithText(aP, "p", "", 
      "WARNING: the moz_malloc_usable_size() function does not work for " +
      "this platform and/or configuration.  This means that much of the " +
      "heap-allocated memory is not measured by individual memory reporters " +
      "and so will fall under 'heap-unclassified'.\n\n");
  }

  if (gUnsafePathsWithInvalidValuesForThisProcess.length > 0) {
    let div = appendElement(aP, "div");
    appendElementWithText(div, "p", "", 
      "WARNING: the following values are negative or unreasonably large.\n");

    let ul = appendElement(div, "ul");
    for (let i = 0;
         i < gUnsafePathsWithInvalidValuesForThisProcess.length;
         i++)
    {
      appendTextNode(ul, " ");
      appendElementWithText(ul, "li", "", 
        flipBackslashes(gUnsafePathsWithInvalidValuesForThisProcess[i]) + "\n");
    }

    appendElementWithText(div, "p", "",
      "This indicates a defect in one or more memory reporters.  The " +
      "invalid values are highlighted.\n\n");
    gUnsafePathsWithInvalidValuesForThisProcess = [];  
  }
}
















function appendProcessAboutMemoryElements(aP, aProcess, aTreeOfTrees, aOthers,
                                          aHeapTotal, aHasMozMallocUsableSize)
{
  appendElementWithText(aP, "h1", "", aProcess + " Process\n\n");

  
  let warningsDiv = appendElement(aP, "div", "accuracyWarning");

  let explicitTree = fillInTree(aTreeOfTrees, "explicit/");
  let hasKnownHeapAllocated =
    addHeapUnclassifiedNode(explicitTree, aOthers, aHeapTotal);
  sortTreeAndInsertAggregateNodes(explicitTree._amount, explicitTree);
  appendTreeElements(aP, explicitTree, aProcess);

  
  if (gVerbose) {
    kSmapsTreePrefixes.forEach(function(aTreePrefix) {
      let t = fillInTree(aTreeOfTrees, aTreePrefix);

      
      
      if (t) {
        sortTreeAndInsertAggregateNodes(t._amount, t);
        t._hideKids = true;   
        appendTreeElements(aP, t, aProcess);
      }
    });
  }

  
  
  appendOtherElements(aP, aOthers);

  
  
  
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





const kHorizontal                   = "\u2500",
      kVertical                     = "\u2502",
      kUpAndRight                   = "\u2514",
      kUpAndRight_Right_Right       = "\u2514\u2500\u2500",
      kVerticalAndRight             = "\u251c",
      kVerticalAndRight_Right_Right = "\u251c\u2500\u2500",
      kVertical_Space_Space         = "\u2502  ";

const kNoKidsSep                    = " \u2500\u2500 ",
      kHideKidsSep                  = " ++ ",
      kShowKidsSep                  = " -- ";

function appendMrValueSpan(aP, aValue, aIsInvalid)
{
  appendElementWithText(aP, "span", "mrValue" + (aIsInvalid ? " invalid" : ""),
                        aValue);
}

function appendMrNameSpan(aP, aDescription, aUnsafeName, aIsInvalid, aNMerged)
{
  let safeName = flipBackslashes(aUnsafeName);
  if (!aIsInvalid && !aNMerged) {
    safeName += "\n";
  }
  let nameSpan = appendElementWithText(aP, "span", "mrName", safeName);
  nameSpan.title = aDescription;

  if (aIsInvalid) {
    let noteText = " [?!]";
    if (!aNMerged) {
      noteText += "\n";
    }
    let noteSpan = appendElementWithText(aP, "span", "mrNote", noteText);
    noteSpan.title =
      "Warning: this value is invalid and indicates a bug in one or more " +
      "memory reporters. ";
  }

  if (aNMerged) {
    let noteSpan = appendElementWithText(aP, "span", "mrNote",
                                         " [" + aNMerged + "]\n");
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

  
  let isExpansion;
  let sepSpan = outerSpan.childNodes[2];
  assertClassListContains(sepSpan, "mrSep");
  if (sepSpan.textContent === kHideKidsSep) {
    isExpansion = true;
    sepSpan.textContent = kShowKidsSep;
  } else if (sepSpan.textContent === kShowKidsSep) {
    isExpansion = false;
    sepSpan.textContent = kHideKidsSep;
  } else {
    assert(false, "bad sepSpan textContent");
  }

  
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
    
    let sepSpan = aElement.childNodes[2];
    assertClassListContains(sepSpan, "mrSep");
    sepSpan.textContent = kShowKidsSep;
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

  


















  function appendTreeElements2(aP, aUnsafeNames, aT, aTreelineText1,
                               aTreelineText2a, aTreelineText2b,
                               aParentStringLength)
  {
    function appendN(aS, aC, aN)
    {
      for (let i = 0; i < aN; i++) {
        aS += aC;
      }
      return aS;
    }

    
    let valueText = aT.toString();
    let extraTreelineLength =
      Math.max(aParentStringLength - valueText.length, 0);
    if (extraTreelineLength > 0) {
      aTreelineText2a =
        appendN(aTreelineText2a, kHorizontal, extraTreelineLength);
      aTreelineText2b =
        appendN(aTreelineText2b, " ",         extraTreelineLength);
    }
    let treelineText = aTreelineText1 + aTreelineText2a;
    appendElementWithText(aP, "span", "treeline", treelineText);

    
    
    let percText;
    let tIsInvalid = false;
    if (aT._amount === treeBytes) {
      percText = " (100.0%)";
    } else {
      if (!(0 <= aT._amount && aT._amount <= treeBytes)) {
        tIsInvalid = true;
        let unsafePath = aUnsafeNames.join("/");
        gUnsafePathsWithInvalidValuesForThisProcess.push(unsafePath);
        reportAssertionFailure("Invalid value for " +
                               flipBackslashes(unsafePath));
      }
      let num = 100 * aT._amount / treeBytes;
      let numText = num.toFixed(2);
      percText = (0 <= num && num < 10 ? " (0" : " (") + numText + "%)";
    }

    
    
    let d;
    let sep;
    let showSubtrees;
    if (aT._kids) {
      
      
      let unsafePath = aUnsafeNames.join("/");
      let safeTreeId = aProcess + ":" + flipBackslashes(unsafePath);
      showSubtrees = !aT._hideKids;
      if (gShowSubtreesBySafeTreeId[safeTreeId] !== undefined) {
        showSubtrees = gShowSubtreesBySafeTreeId[safeTreeId];
      }
      d = appendElement(aP, "span", "hasKids");
      d.id = safeTreeId;
      d.onclick = toggle;
      sep = showSubtrees ? kShowKidsSep : kHideKidsSep;
    } else {
      assert(!aT._hideKids, "leaf node with _hideKids set")
      sep = kNoKidsSep;
      d = aP;
    }

    appendMrValueSpan(d, valueText, tIsInvalid);
    appendElementWithText(d, "span", "mrPerc", percText);
    appendElementWithText(d, "span", "mrSep", sep);

    appendMrNameSpan(d, aT._description, aT._unsafeName,
                     tIsInvalid, aT._nMerged);

    
    
    if (!gVerbose && tIsInvalid) {
      expandPathToThisElement(d);
    }

    if (aT._kids) {
      
      d = appendElement(aP, "span", showSubtrees ? "kids" : "kids hidden");

      let kidTreelineText1 = aTreelineText1 + aTreelineText2b;
      for (let i = 0; i < aT._kids.length; i++) {
        let kidTreelineText2a, kidTreelineText2b;
        if (i < aT._kids.length - 1) {
          kidTreelineText2a = kVerticalAndRight_Right_Right;
          kidTreelineText2b = kVertical_Space_Space;
        } else {
          kidTreelineText2a = kUpAndRight_Right_Right;
          kidTreelineText2b = "   ";
        }
        aUnsafeNames.push(aT._kids[i]._unsafeName);
        appendTreeElements2(d, aUnsafeNames, aT._kids[i], kidTreelineText1,
                            kidTreelineText2a, kidTreelineText2b,
                            valueText.length);
        aUnsafeNames.pop();
      }
    }
  }

  appendSectionHeader(aPOuter, kSectionNames[aT._unsafeName]);
 
  let pre = appendElement(aPOuter, "pre", "entries");
  appendTreeElements2(pre, [aT._unsafeName], aT, "", "", "", rootStringLength);
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









function appendOtherElements(aP, aOthers)
{
  appendSectionHeader(aP, kSectionNames['other']);

  let pre = appendElement(aP, "pre", "entries");

  
  
  let maxStringLength = 0;
  let otherReports = [];
  for (let unsafePath in aOthers) {
    let o = aOthers[unsafePath];
    otherReports.push(o);
    if (o._asString.length > maxStringLength) {
      maxStringLength = o._asString.length;
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
    appendElementWithText(pre, "span", "mrSep", kNoKidsSep);
    appendMrNameSpan(pre, o._description, o._unsafePath, oIsInvalid);
  }

  appendTextNode(aP, "\n");  
}

function appendSectionHeader(aP, aText)
{
  appendElementWithText(aP, "h2", "", aText + "\n");
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
  
  
  

  function ignoreSingle(aUnsafePath) 
  {
    return !aUnsafePath.startsWith("compartments/");
  }

  function ignoreMulti(aMRName)
  {
    return aMRName !== "compartments";
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
  function ignoreSingle(aUnsafePath) 
  {
    return !aUnsafePath.startsWith('ghost-windows/')
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
  appendElementWithText(aP, "h1", "", aProcess + " Process\n\n");

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

