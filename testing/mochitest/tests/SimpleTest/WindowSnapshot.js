var gWindowUtils;

try {
  gWindowUtils = SpecialPowers.getDOMWindowUtils(window);
  if (gWindowUtils && !gWindowUtils.compareCanvases)
    gWindowUtils = null;
} catch (e) {
  gWindowUtils = null;
}

function snapshotWindow(win, withCaret) {
  return SpecialPowers.snapshotWindow(win, withCaret);
}




function compareSnapshots(s1, s2, expected, fuzz) {
  var s1Str, s2Str;
  var correct = false;
  if (gWindowUtils) {
    
    var equal;
    if (s1.width != s2.width || s1.height != s2.height) {
      equal = false;
    } else {
      try {
        var maxDifference = {};
        var numDifferentPixels = gWindowUtils.compareCanvases(s1, s2, maxDifference);
        if (!fuzz) {
          equal = (numDifferentPixels == 0);
        } else {
          equal = (numDifferentPixels <= fuzz.numDifferentPixels &&
                   maxDifference.value <= fuzz.maxDifference);
        }
      } catch (e) {
        equal = false;
        ok(false, "Exception thrown from compareCanvases: " + e);
      }
    }
    correct = (equal == expected);
  }

  if (!correct) {
    s1Str = s1.toDataURL();
    s2Str = s2.toDataURL();

    if (!gWindowUtils) {
	correct = ((s1Str == s2Str) == expected);
    }
  }

  return [correct, s1Str, s2Str];
}

function assertSnapshots(s1, s2, expected, fuzz, s1name, s2name) {
  var [correct, s1Str, s2Str] = compareSnapshots(s1, s2, expected, fuzz);
  var sym = expected ? "==" : "!=";
  ok(correct, "reftest comparison: " + sym + " " + s1name + " " + s2name);
  if (!correct) {
    var report = "REFTEST TEST-UNEXPECTED-FAIL | " + s1name + " | image comparison (" + sym + ")\n";
    if (expected) {
      report += "REFTEST   IMAGE 1 (TEST): " + s1Str + "\n";
      report += "REFTEST   IMAGE 2 (REFERENCE): " + s2Str + "\n";
    } else {
      report += "REFTEST   IMAGE: " + s1Str + "\n";
    }
    dump(report);
  }
  return correct;
}
