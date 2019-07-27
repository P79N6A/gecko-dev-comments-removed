





SimpleTest.waitForExplicitFinish();





let expected_values = [
  ["color", null, 8],
  ["color-index", null, 0],
  ["aspect-ratio", null, window.innerWidth + "/" + window.innerHeight],
  ["device-aspect-ratio", screen.width + "/" + screen.height,
                          window.innerWidth + "/" + window.innerHeight],
  ["device-height", screen.height + "px", window.innerHeight + "px"],
  ["device-width", screen.width + "px", window.innerWidth + "px"],
  ["grid", null, 0],
  ["height", window.innerHeight + "px", window.innerHeight + "px"],
  ["monochrome", null, 0],
  
  ["orientation", null,
                  window.innerWidth > window.innerHeight ?
                    "landscape" : "portrait"],
  ["resolution", null, "96dpi"],
  ["resolution", [0.999 * window.devicePixelRatio + "dppx",
                  1.001 * window.devicePixelRatio + "dppx"], "1dppx"],
  ["width", window.innerWidth + "px", window.innerWidth + "px"],
  ["-moz-device-pixel-ratio", window.devicePixelRatio, 1],
  ["-moz-device-orientation", screen.width > screen.height ?
                                "landscape" : "portrait",
                              window.innerWidth > window.innerHeight ?
                                "landscape" : "portrait"]
];



let suppressed_toggles = [
  "-moz-images-in-menus",
  "-moz-mac-graphite-theme",
  

  "-moz-scrollbar-end-backward",
  "-moz-scrollbar-end-forward",
  "-moz-scrollbar-start-backward",
  "-moz-scrollbar-start-forward",
  "-moz-scrollbar-thumb-proportional",
  "-moz-touch-enabled",
  "-moz-windows-compositor",
  "-moz-windows-default-theme",
  "-moz-windows-glass",
];


let windows_versions = [
  "windows-xp",
  "windows-vista",
  "windows-win7",
  "windows-win8"];


let windows_themes = [
  "aero",
  "luna-blue",
  "luna-olive",
  "luna-silver",
  "royale",
  "generic",
  "zune"
];


let OS = SpecialPowers.Services.appinfo.OS;



if (OS === "WINNT") {
  suppressed_toggles.push("-moz-windows-classic");
}



let keyValMatches = (key, val) => matchMedia("(" + key + ":" + val +")").matches;





let testMatch = function (key, val) {
  if (val === null) {
    return;
  } else if (Array.isArray(val)) {
    ok(keyValMatches("min-" + key, val[0]) && keyValMatches("max-" + key, val[1]),
       "Expected " + key + " between " + val[0] + " and " + val[1]);
  } else {
    ok(keyValMatches(key, val), "Expected " + key + ":" + val);
  }
};



let testToggles = function (resisting) {
  suppressed_toggles.forEach(
    function (key) {
      var exists = keyValMatches(key, 0) || keyValMatches(key, 1);
      if (resisting) {
         ok(!exists, key + " should not exist.");
      } else {
         ok(exists, key + " should exist.");
      }
    });
};



let testWindowsSpecific = function (resisting, queryName, possibleValues) {
  let found = false;
  possibleValues.forEach(function (val) {
    found = found || keyValMatches(queryName, val);
  });
  if (resisting) {
    ok(!found, queryName + " should have no match");
  } else {
    ok(found, queryName + " should match");
  }
};





let generateHtmlLines = function (resisting) {
  let lines = "";
  expected_values.forEach(
    function ([key, offVal, onVal]) {
      let val = resisting ? onVal : offVal;
      if (val) {
        lines += "<div class='spoof' id='" + key + "'>" + key + "</div>\n";
      }
    });
  suppressed_toggles.forEach(
    function (key) {
      lines += "<div class='suppress' id='" + key + "'>" + key + "</div>\n";
    });
  if (OS === "WINNT") {
    lines += "<div class='windows' id='-moz-os-version'>-moz-os-version</div>";
    lines += "<div class='windows' id='-moz-windows-theme'>-moz-windows-theme</div>";
  }
  return lines;
};




let cssLine = function (query, clazz, id, color) {
  return "@media " + query + " { ." + clazz +  "#" + id +
         " { background-color: " + color + "; } }\n";
};



let mediaQueryCSSLine = function (key, val, color) {
  if (val === null) {
    return "";
  }
  let query;
  if (Array.isArray(val)) {
    query = "(min-" + key + ": " + val[0] + ") and (max-" +  key + ": " + val[1] + ")";
  } else {
    query = "(" + key + ": " + val + ")";
  }
  return cssLine(query, "spoof", key, color);
};




let suppressedMediaQueryCSSLine = function (key, color, suppressed) {
  let query = "(" + key + ": 0), (" + key + ": 1)";
  return cssLine(query, "suppress", key, color);
};





let generateCSSLines = function (resisting) {
  let lines = ".spoof { background-color: red;}\n";
  expected_values.forEach(
    function ([key, offVal, onVal]) {
      lines += mediaQueryCSSLine(key, resisting ? onVal : offVal, "green");
    });
  lines += ".suppress { background-color: " + (resisting ? "green" : "red") + ";}\n";
  suppressed_toggles.forEach(
    function (key) {
      lines += suppressedMediaQueryCSSLine(key, resisting ? "red" : "green");
    });
  if (OS === "WINNT") {
    lines += ".windows { background-color: " + (resisting ? "green" : "red") + ";}\n";
    lines += windows_versions.map(val => "(-moz-os-version: " + val + ")").join(", ") +
             " { #-moz-os-version { background-color: " + (resisting ? "red" : "green") + ";} }\n";
    lines += windows_themes.map(val => "(-moz-windows-theme: " + val + ")").join(",") +
             " { #-moz-windows-theme { background-color: " + (resisting ? "red" : "green") + ";} }\n";
  }
  return lines;
};



let green = (function () {
  let temp = document.createElement("span");
  temp.style.backgroundColor = "green";
  return getComputedStyle(temp).backgroundColor;
})();





let testCSS = function (resisting) {
  document.getElementById("display").innerHTML = generateHtmlLines(resisting);
  document.getElementById("test-css").innerHTML = generateCSSLines(resisting);
  let cssTestDivs = document.querySelectorAll(".spoof,.suppress");
  for (let div of cssTestDivs) {
    let color = window.getComputedStyle(div).backgroundColor;
    ok(color === green, "CSS for '" + div.id + "'");
  }
};




let testOSXFontSmoothing = function (resisting) {
  let div = document.createElement("div");
  div.style.MozOsxFontSmoothing = "unset";
  let readBack = window.getComputedStyle(div).MozOsxFontSmoothing;
  let smoothingPref = SpecialPowers.getBoolPref("layout.css.osx-font-smoothing.enabled", false);
  is(readBack, resisting ? "" : (smoothingPref ? "auto" : ""),
               "-moz-osx-font-smoothing");
};


let prefVals = (for (prefVal of [false, true]) prefVal);



let test = function(isContent) {
  let {value: prefValue, done} = prefVals.next();
  if (done) {
    SimpleTest.finish();
    return;
  }
  SpecialPowers.pushPrefEnv({set: [["privacy.resistFingerprinting", prefValue]]},
    function () {
      let resisting = prefValue && isContent;
      expected_values.forEach(
        function ([key, offVal, onVal]) {
          testMatch(key, resisting ? onVal : offVal);
        });
      testToggles(resisting);
      if (OS === "WINNT") {
        testWindowsSpecific(resisting, "-moz-os-version", windows_versions);
        testWindowsSpecific(resisting, "-moz-windows-theme", windows_themes);
      }
      testCSS(resisting);
      if (OS === "Darwin") {
        testOSXFontSmoothing(resisting);
      }
      test(isContent);
    });
};
