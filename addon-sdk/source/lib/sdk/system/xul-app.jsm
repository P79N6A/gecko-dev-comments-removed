


"use strict";

this.EXPORTED_SYMBOLS = [ "XulApp" ];

var { classes: Cc, interfaces: Ci } = Components;

var exports = {};
this.XulApp = exports;

var appInfo;




try {
 appInfo = Cc["@mozilla.org/xre/app-info;1"]
              .getService(Ci.nsIXULAppInfo);
}
catch (e) {
  
  appInfo = {};
}
var vc = Cc["@mozilla.org/xpcom/version-comparator;1"]
         .getService(Ci.nsIVersionComparator);

var ID = exports.ID = appInfo.ID;
var name = exports.name = appInfo.name;
var version = exports.version = appInfo.version;
var platformVersion = exports.platformVersion = appInfo.platformVersion;










var ids = exports.ids = {
  Firefox: "{ec8030f7-c20a-464f-9b0e-13a3a9e97384}",
  Mozilla: "{86c18b42-e466-45a9-ae7a-9b95ba6f5640}",
  SeaMonkey: "{92650c4d-4b8e-4d2a-b7eb-24ecf4f6b63a}",
  Fennec: "{aa3c5121-dab2-40e2-81ca-7ea25febc110}",
  Thunderbird: "{3550f703-e582-4d05-9a08-453d09bdfdc6}"
};

function is(name) {
  if (!(name in ids))
    throw new Error("Unkown Mozilla Application: " + name);
  return ID == ids[name];
};
exports.is = is;

function isOneOf(names) {
  for (var i = 0; i < names.length; i++)
    if (is(names[i]))
      return true;
  return false;
};
exports.isOneOf = isOneOf;







var versionInRange = exports.versionInRange =
function versionInRange(version, lowInclusive, highExclusive) {
  return (vc.compare(version, lowInclusive) >= 0) &&
         (vc.compare(version, highExclusive) < 0);
}

const reVersionRange = /^((?:<|>)?=?)?\s*((?:\d+[\S]*)|\*)(?:\s+((?:<|>)=?)?(\d+[\S]+))?$/;
const reOnlyInifinity = /^[<>]?=?\s*[*x]$/;
const reSubInfinity = /\.[*x]/g;
const reHyphenRange = /^(\d+.*?)\s*-\s*(\d+.*?)$/;
const reRangeSeparator = /\s*\|\|\s*/;

const compares = {
  "=": function (c) { return c === 0 },
  ">=": function (c) { return c >= 0 },
  "<=": function (c) { return c <= 0},
  "<": function (c) { return c < 0 },
  ">": function (c) { return c > 0 }
}

function normalizeRange(range) {
    return range
        .replace(reOnlyInifinity, "")
        .replace(reSubInfinity, ".*")
        .replace(reHyphenRange, ">=$1 <=$2")
}

















function compareVersion(version, comparison, compareVersion) {
  let hasWildcard = compareVersion.indexOf("*") !== -1;

  comparison = comparison || "=";

  if (hasWildcard) {
    switch (comparison) {
      case "=":
        let zeroVersion = compareVersion.replace(reSubInfinity, ".0");
        return versionInRange(version, zeroVersion, compareVersion);
      case ">=":
        compareVersion = compareVersion.replace(reSubInfinity, ".0");
        break;
    }
  }

  let compare = compares[comparison];

  return typeof compare === "function" && compare(vc.compare(version, compareVersion));
}






























function satisfiesVersion(version, versionRange) {
  if (arguments.length === 1) {
    versionRange = version;
    version = appInfo.version;
  }

  let ranges = versionRange.trim().split(reRangeSeparator);

  return ranges.some(function(range) {
    range = normalizeRange(range);

    
    
    if (range === "")
      return true;

    let matches = range.match(reVersionRange);

    if (!matches)
      return false;

    let [, lowMod, lowVer, highMod, highVer] = matches;

    return compareVersion(version, lowMod, lowVer) && (highVer !== undefined
      ? compareVersion(version, highMod, highVer)
      : true);
  });
}
exports.satisfiesVersion = satisfiesVersion;










function incompatibility(module) {
  let { metadata, id } = module;

  
  
  if (!metadata || !("engines" in metadata))
    return null;

  let { engines } = metadata;

  if (engines === null || typeof(engines) !== "object")
    return new Error("Malformed engines' property in metadata");

  let applications = Object.keys(engines);

  let versionRange;
  applications.forEach(function(name) {
    if (is(name)) {
      versionRange = engines[name];
      
      
      
    }
  });

  if (typeof(versionRange) === "string") {
    if (satisfiesVersion(versionRange))
      return null;

    return new Error("Unsupported Application version: The module " + id +
            " currently supports only version " + versionRange + " of " +
            name + ".");
  }

  return new Error("Unsupported Application: The module " + id +
            " currently supports only " + applications.join(", ") + ".")
}
exports.incompatibility = incompatibility;
