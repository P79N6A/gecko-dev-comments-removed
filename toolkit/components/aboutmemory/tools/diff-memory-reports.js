




































"use strict";

function assert(aCond, aFailMsg)
{
  if (!aCond) {
    throw aFailMsg;
  }
}

let kSep = "^:^:^";   

function Report(aKind, aUnits, aAmount, aDescription, aNMerged)
{
  this._kind = aKind;
  this._units = aUnits;
  this._amount = aAmount;
  this._description = aDescription;
  this._nMerged = aNMerged;
}

Report.prototype = {
  assertCompatible: function(aKind, aUnits)
  {
    assert(this._kind  == aKind,  "Mismatched kinds");
    assert(this._units == aUnits, "Mismatched units");

    
    
    
    
    
    
    
    
    
    
    
    
  },

  merge: function(aJr) {
    this.assertCompatible(aJr.kind, aJr.units);
    this._amount += aJr.amount;
    this._nMerged++;
  },

  toJSON: function(aProcess, aPath, aAmount) {
    return {
      process:     aProcess,
      path:        aPath,
      kind:        this._kind,
      units:       this._units,
      amount:      aAmount,
      description: this._description
    };
  }
};


function makeReportMap(aJSONReports)
{
  let reportMap = {};
  for (let i = 0; i < aJSONReports.length; i++) {
    let jr = aJSONReports[i];

    assert(jr.process     !== undefined, "Missing process");
    assert(jr.path        !== undefined, "Missing path");
    assert(jr.kind        !== undefined, "Missing kind");
    assert(jr.units       !== undefined, "Missing units");
    assert(jr.amount      !== undefined, "Missing amount");
    assert(jr.description !== undefined, "Missing description");

    
    
    let strippedProcess = jr.process.replace(/pid \d+/, "pid NNN");
    let strippedPath = jr.path.replace(/0x[0-9A-Fa-f]+/, "0xNNN");
    let processPath = strippedProcess + kSep + strippedPath;

    let rOld = reportMap[processPath];
    if (rOld === undefined) {
      reportMap[processPath] =
        new Report(jr.kind, jr.units, jr.amount, jr.description, 1);
    } else {
      rOld.merge(jr);
    }
  }
  return reportMap;
}



function diffReportMaps(aReportMap1, aReportMap2)
{
  let result = {};

  for (let processPath in aReportMap1) {
    let r1 = aReportMap1[processPath];
    let r2 = aReportMap2[processPath];
    let r2_amount, r2_nMerged;
    if (r2 !== undefined) {
      r1.assertCompatible(r2._kind, r2._units);
      r2_amount = r2._amount;
      r2_nMerged = r2._nMerged;
      delete aReportMap2[processPath];
    } else {
      r2_amount = 0;
      r2_nMerged = 0;
    }
    result[processPath] =
      new Report(r1._kind, r1._units, r2_amount - r1._amount, r1._description,
                 Math.max(r1._nMerged, r2_nMerged));
  }

  for (let processPath in aReportMap2) {
    let r2 = aReportMap2[processPath];
    result[processPath] = new Report(r2._kind, r2._units, r2._amount,
                                     r2._description, r2._nMerged);
  }

  return result;
}

function makeJSONReports(aReportMap)
{
  let reports = [];
  for (let processPath in aReportMap) {
    let r = aReportMap[processPath];
    if (r._amount !== 0) {
      
      
      
      
      let split = processPath.split(kSep);
      assert(split.length >= 2);
      let process = split.shift();
      let path = split.join();
      reports.push(r.toJSON(process, path, r._amount));
      for (let i = 1; i < r._nMerged; i++) {
        reports.push(r.toJSON(process, path, 0));
      }
    }
  }

  
  
  reports.sort(function(a, b) {
    if      (a.process < b.process) return -1;
    else if (a.process > b.process) return  1;
    else if (a.path    < b.path)    return -1;
    else if (a.path    > b.path)    return  1;
    else if (a.amount  < b.amount)  return -1;
    else if (a.amount  > b.amount)  return  1;
    else                            return  0;
  });

  return reports;
}



function diff(aJson1, aJson2)
{
  function simpleProp(aProp)
  {
    assert(aJson1[aProp] !== undefined && aJson1[aProp] === aJson2[aProp],
           aProp + " properties don't match");
    return aJson1[aProp];
  }

  return {
    version: simpleProp("version"),

    hasMozMallocUsableSize: simpleProp("hasMozMallocUsableSize"),

    reports: makeJSONReports(diffReportMaps(makeReportMap(aJson1.reports),
                                            makeReportMap(aJson2.reports)))
  };
}




let kTestInput1 =
{
  "version": 1,
  "hasMozMallocUsableSize": true,
  "reports": [
    
    {"process": "P", "path": "explicit/xpcom/category-manager", "kind": 1, "units": 0, "amount": 56848, "description": "Desc."},
    {"process": "P", "path": "explicit/storage/prefixset/goog-phish-shavar", "kind": 1, "units": 0, "amount": 680000, "description": "Desc."},

    
    {"process": "P", "path": "explicit/spell-check", "kind": 1, "units": 0, "amount": 4, "description": "Desc."},
    {"process": "P", "path": "explicit/spell-check", "kind": 1, "units": 0, "amount": 5, "description": "Desc."},

    
    {"process": "P", "path": "page-faults-soft", "kind": 2, "units": 2, "amount": 61013, "description": "Desc."},

    {"process": "P", "path": "foobar", "kind": 2, "units": 0, "amount": 100, "description": "Desc."},
    {"process": "P", "path": "zero1", "kind": 2, "units": 0, "amount": 0, "description": "Desc."},

    
    {"process": "P2 (pid 22)", "path": "z 0x1234", "kind": 2, "units": 0, "amount": 33, "description": "Desc."},

    
    {"process": "P3", "path": "p3", "kind": 2, "units": 0, "amount": 55, "description": "Desc."},

    
    
    {"process": "P5", "path": "p5", "kind": 2, "units": 0, "amount": 0, "description": "Desc."}
  ]
}

let kTestInput2 =
{
  "version": 1,
  "hasMozMallocUsableSize": true,
  "reports": [
    {"process": "P", "path": "explicit/xpcom/category-manager", "kind": 1, "units": 0, "amount": 56849, "description": "Desc."},
    {"process": "P", "path": "explicit/storage/prefixset/goog-phish-shavar", "kind": 1, "units": 0, "amount": 670000, "description": "Desc."},

    {"process": "P", "path": "explicit/spell-check", "kind": 1, "units": 0, "amount": 3, "description": "Desc."},

    {"process": "P", "path": "page-faults-soft", "kind": 2, "units": 2, "amount": 61013, "description": "Desc."},

    
    {"process": "P", "path": "canvas-2d-pixel-bytes", "kind": 2, "units": 0, "amount": 1000, "description": "Desc."},
    {"process": "P", "path": "canvas-2d-pixel-bytes", "kind": 2, "units": 0, "amount": 2000, "description": "Desc."},

    {"process": "P", "path": "foobaz", "kind": 2, "units": 0, "amount": 0, "description": "Desc."},

    {"process": "P2 (pid 33)", "path": "z 0x5678", "kind": 2, "units": 0, "amount": 44, "description": "Desc."},

    
    {"process": "P4", "path": "p4", "kind": 2, "units": 0, "amount": 66, "description": "Desc."},

    
    
    {"process": "P6", "path": "p6", "kind": 2, "units": 0, "amount": 0, "description": "Desc."}
  ]
}

let kTestExpectedOutput =
{
  "version": 1,
  "hasMozMallocUsableSize": true,
  "reports": [
    {"process":"P","path":"canvas-2d-pixel-bytes","kind":2,"units":0,"amount":0,"description":"Desc."},
    {"process":"P","path":"canvas-2d-pixel-bytes","kind":2,"units":0,"amount":3000,"description":"Desc."},
    {"process":"P","path":"explicit/spell-check","kind":1,"units":0,"amount":-6,"description":"Desc."},
    {"process":"P","path":"explicit/spell-check","kind":1,"units":0,"amount":0,"description":"Desc."},
    {"process":"P","path":"explicit/storage/prefixset/goog-phish-shavar","kind":1,"units":0,"amount":-10000,"description":"Desc."},
    {"process":"P","path":"explicit/xpcom/category-manager","kind":1,"units":0,"amount":1,"description":"Desc."},
    {"process":"P","path":"foobar","kind":2,"units":0,"amount":-100,"description":"Desc."},
    {"process":"P2 (pid NNN)","path":"z 0xNNN","kind":2,"units":0,"amount":11,"description":"Desc."},
    {"process":"P3","path":"p3","kind":2,"units":0,"amount":-55,"description":"Desc."},{"process":"P4","path":"p4","kind":2,"units":0,"amount":66,"description":"Desc."}]}

function matches(aA, aB)
{
  return JSON.stringify(aA) === JSON.stringify(aB);
}






let kUsageMsg =
"Usage:\n\
\n\
  diff-memory-reports.js <file1.json> <file2.json>\n\
\n\
or:\n\
\n\
  diff-memory-reports.js --test\n\
"

if (arguments.length === 1 && arguments[0] === "--test") {
  let expected = JSON.stringify(kTestExpectedOutput);
  let actual   = JSON.stringify(diff(kTestInput1, kTestInput2));
  if (expected === actual) {
    print("test PASSED");
  } else {
    print("test FAILED");
    print();
    print("expected:");
    print(expected);
    print();
    print("actual:");
    print(actual);
  }

} else if (arguments.length === 2) {
  let json1 = JSON.parse(read(arguments[0]));
  let json2 = JSON.parse(read(arguments[1]));

  print(JSON.stringify(diff(json1, json2)));

} else {
  print(kUsageMsg);
  quit(1);
}


