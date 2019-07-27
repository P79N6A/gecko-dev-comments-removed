




const csscoverage = require("devtools/server/actors/csscoverage");

const PAGE_1 = TEST_BASE_HTTPS + "browser_cmd_csscoverage_page1.html";
const PAGE_2 = TEST_BASE_HTTPS + "browser_cmd_csscoverage_page2.html";
const PAGE_3 = TEST_BASE_HTTPS + "browser_cmd_csscoverage_page3.html";

const SHEET_A = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetA.css";
const SHEET_B = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetB.css";
const SHEET_C = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetC.css";
const SHEET_D = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetD.css";

add_task(function*() {
  let options = yield helpers.openTab("about:blank");
  yield helpers.openToolbar(options);

  let usage = yield csscoverage.getUsage(options.target);

  yield navigate(usage, options);
  yield checkPages(usage);
  yield checkEditorReport(usage);
  yield checkPageReport(usage);

  yield helpers.closeToolbar(options);
  yield helpers.closeTab(options);
});




function* navigate(usage, options) {
  yield usage.start(options.chromeWindow, options.target);

  ok(usage.isRunning(), "csscoverage is running");

  let load1Promise = helpers.listenOnce(options.browser, "load", true);

  yield helpers.navigate(PAGE_1, options);

  
  yield load1Promise;
  is(options.window.location.href, PAGE_1, "page 1 loaded");

  
  yield helpers.listenOnce(options.browser, "load", true);
  is(options.window.location.href, PAGE_3, "page 3 loaded");

  yield usage.stop();

  ok(!usage.isRunning(), "csscoverage not is running");
}




function* checkPages(usage) {
  
  let expectedVisited = [ 'null', PAGE_2, PAGE_1, PAGE_3 ];
  let actualVisited = yield usage._testOnly_visitedPages();
  isEqualJson(actualVisited, expectedVisited, 'Visited');
}




function* checkEditorReport(usage) {
  
  let expectedPage1 = {
    reports: [
      {
        selectorText: ".page1-test2",
        start: { line: 8, column: 5 },
      }
    ]
  };
  let actualPage1 = yield usage.createEditorReport(PAGE_1 + " \u2192 <style> index 0");
  isEqualJson(actualPage1, expectedPage1, 'Page1');

  
  let expectedPage2 = {
    reports: [
      {
        selectorText: ".page2-test2",
        start: { line: 9, column: 5 },
      },
    ]
  };
  let actualPage2 = yield usage.createEditorReport(PAGE_2 + " \u2192 <style> index 0");
  isEqualJson(actualPage2, expectedPage2, 'Page2');

  
  let expectedPage3a = {
    reports: [
      {
        selectorText: ".page3-test2",
        start: { line: 9, column: 5 },
      }
    ]
  };
  let actualPage3a = yield usage.createEditorReport(PAGE_3 + " \u2192 <style> index 0");
  isEqualJson(actualPage3a, expectedPage3a, 'Page3a');

  
  let expectedPage3b = {
    reports: [
      {
        selectorText: ".page3-test3",
        start: { line: 3, column: 5 },
      }
    ]
  };
  let actualPage3b = yield usage.createEditorReport(PAGE_3 + " \u2192 <style> index 1");
  isEqualJson(actualPage3b, expectedPage3b, 'Page3b');

  
  let expectedSheetA = {
    reports: [
      {
        selectorText: ".sheetA-test2",
        start: { line: 8, column: 1 },
      }
    ]
  };
  let actualSheetA = yield usage.createEditorReport(SHEET_A);
  isEqualJson(actualSheetA, expectedSheetA, 'SheetA');

  
  let expectedSheetB = {
    reports: [
      {
        selectorText: ".sheetB-test2",
        start: { line: 6, column: 1 },
      }
    ]
  };
  let actualSheetB = yield usage.createEditorReport(SHEET_B);
  isEqualJson(actualSheetB, expectedSheetB, 'SheetB');

  
  let expectedSheetC = {
    reports: [
      {
        selectorText: ".sheetC-test2",
        start: { line: 6, column: 1 },
      }
    ]
  };
  let actualSheetC = yield usage.createEditorReport(SHEET_C);
  isEqualJson(actualSheetC, expectedSheetC, 'SheetC');

  
  let expectedSheetD = {
    reports: [
      {
        selectorText: ".sheetD-test2",
        start: { line: 6, column: 1 },
      }
    ]
  };
  let actualSheetD = yield usage.createEditorReport(SHEET_D);
  isEqualJson(actualSheetD, expectedSheetD, 'SheetD');
}




function* checkPageReport(usage) {
  let actualReport = yield usage.createPageReport();

  
  actualReport.preload.forEach(page => page.rules.forEach(checkRuleProperties));
  actualReport.unused.forEach(page => page.rules.forEach(checkRuleProperties));

  
  let expectedSummary = { "used": 92, "unused": 22, "preload": 28 };
  isEqualJson(actualReport.summary, expectedSummary, 'summary');

  checkPageReportPreload(actualReport);
  checkPageReportUnused(actualReport);
}




function checkPageReportPreload(actualReport) {
  
  isEqualJson(actualReport.preload.length, 3, 'preload length');

  
  isEqualJson(actualReport.preload[0].url, PAGE_2, 'preload url 0');
  let expectedPreloadRules0 = [
    
    {
      url: PAGE_2 + " \u2192 <style> index 0",
      start: { line: 5, column: 5 },
      selectorText: ".page2-test1"
    },
    {
      url: SHEET_A,
      start: { line: 4, column: 1 },
      selectorText: ".sheetA-test1"
    },
    {
      url: SHEET_A,
      start: { line: 16, column: 1 },
      selectorText: ".sheetA-test4"
    },
    {
      url: SHEET_B,
      start: { line: 2, column: 1 },
      selectorText: ".sheetB-test1"
    },
    {
      url: SHEET_B,
      start: { line: 14, column: 1 },
      selectorText: ".sheetB-test4"
    },
    {
      url: SHEET_D,
      start: { line: 2, column: 1 },
      selectorText: ".sheetD-test1"
    },
    {
      url: SHEET_D,
      start: { line: 14, column: 1 },
      selectorText: ".sheetD-test4"
    },
    {
      url: SHEET_C,
      start: { line: 2, column: 1 },
      selectorText: ".sheetC-test1"
    },
    {
      url: SHEET_C,
      start: { line: 14, column: 1 },
      selectorText: ".sheetC-test4"
    }
  ];
  isEqualJson(actualReport.preload[0].rules, expectedPreloadRules0, 'preload rules 0');

  isEqualJson(actualReport.preload[1].url, PAGE_1, 'preload url 1');
  let expectedPreloadRules1 = [
    {
      url:  SHEET_A,
      start: { line: 4, column: 1 },
      selectorText: ".sheetA-test1"
    },
    {
      url: SHEET_A,
      start: { line: 12, column: 1 },
      selectorText: ".sheetA-test3"
    },
    {
      url: SHEET_B,
      start: { line: 2, column: 1 },
      selectorText: ".sheetB-test1"
    },
    {
      url: SHEET_B,
      start: { line: 10, column: 1 },
      selectorText: ".sheetB-test3"
    },
    {
      url: SHEET_D,
      start: { line: 2, column: 1 },
      selectorText: ".sheetD-test1"
    },
    {
      url: SHEET_D,
      start: { line: 10, column: 1 },
      selectorText: ".sheetD-test3"
    },
    {
      url: SHEET_C,
      start: { line: 2, column: 1 },
      selectorText: ".sheetC-test1"
    },
    {
      url: SHEET_C,
      start: { line: 10, column: 1 },
      selectorText: ".sheetC-test3"
    },
    {
      url: PAGE_1 + " \u2192 <style> index 0",
      start: { line: 4, column: 5 },
      selectorText: ".page1-test1"
    },
    {
      url: PAGE_1 + " \u2192 <style> index 0",
      start: { line: 12, column: 5 },
      selectorText: ".page1-test3:hover"
    }
  ];
  isEqualJson(actualReport.preload[1].rules, expectedPreloadRules1, 'preload rules 1');

  isEqualJson(actualReport.preload[2].url, PAGE_3, 'preload url 2');
  let expectedPreloadRules2 = [
    {
      url: SHEET_A,
      start: { line: 4, column: 1 },
      selectorText: ".sheetA-test1"
    },
    {
      url: SHEET_A,
      start: { line: 20, column: 1 },
      selectorText: ".sheetA-test5"
    },
    {
      url: SHEET_B,
      start: { line: 2, column: 1 },
      selectorText: ".sheetB-test1"
    },
    {
      url: SHEET_B,
      start: { line: 18, column: 1 },
      selectorText: ".sheetB-test5"
    },
    {
      url: SHEET_D,
      start: { line: 2, column: 1 },
      selectorText: ".sheetD-test1"
    },
    {
      url: SHEET_D,
      start: { line: 18, column: 1 },
      selectorText: ".sheetD-test5"
    },
    {
      url: SHEET_C,
      start: { line: 2, column: 1 },
      selectorText: ".sheetC-test1"
    },
    {
      url: SHEET_C,
      start: { line: 18, column: 1 },
      selectorText: ".sheetC-test5"
    },
    {
      url: PAGE_3 + " \u2192 <style> index 0",
      start: { line: 5, column: 5 },
      selectorText: ".page3-test1"
    },
  ];
  isEqualJson(actualReport.preload[2].rules, expectedPreloadRules2, 'preload rules 2');
}




function checkPageReportUnused(actualReport) {
  
  isEqualJson(actualReport.unused.length, 8, 'unused length');

  
  isEqualJson(actualReport.unused[0].url, PAGE_2 + " \u2192 <style> index 0", "unused url 0");
  let expectedUnusedRules0 = [
    {
      url: PAGE_2 + " \u2192 <style> index 0",
      start: { line: 9, column: 5 },
      selectorText: ".page2-test2"
    }
  ];
  isEqualJson(actualReport.unused[0].rules, expectedUnusedRules0, 'unused rules 0');

  isEqualJson(actualReport.unused[1].url, SHEET_A, "unused url 1");
  let expectedUnusedRules1 = [
    {
      url: SHEET_A,
      start: { line: 8, column: 1 },
      selectorText: ".sheetA-test2"
    }
  ];
  isEqualJson(actualReport.unused[1].rules, expectedUnusedRules1, 'unused rules 1');

  isEqualJson(actualReport.unused[2].url, SHEET_B, "unused url 2");
  let expectedUnusedRules2 = [
    {
      url: SHEET_B,
      start: { line: 6, column: 1 },
      selectorText: ".sheetB-test2"
    }
  ];
  isEqualJson(actualReport.unused[2].rules, expectedUnusedRules2, 'unused rules 2');

  isEqualJson(actualReport.unused[3].url, SHEET_D, "unused url 3");
  let expectedUnusedRules3 = [
    {
      url: SHEET_D,
      start: { line: 6, column: 1 },
      selectorText: ".sheetD-test2"
    }
  ];
  isEqualJson(actualReport.unused[3].rules, expectedUnusedRules3, 'unused rules 3');

  isEqualJson(actualReport.unused[4].url, SHEET_C, "unused url 4");
  let expectedUnusedRules4 = [
    {
      url: SHEET_C,
      start: { line: 6, column: 1 },
      selectorText: ".sheetC-test2"
    }
  ];
  isEqualJson(actualReport.unused[4].rules, expectedUnusedRules4, 'unused rules 4');

  isEqualJson(actualReport.unused[5].url, PAGE_1 + " \u2192 <style> index 0", "unused url 5");
  let expectedUnusedRules5 = [
    {
      url: PAGE_1 + " \u2192 <style> index 0",
      start: { line: 8, column: 5 },
      selectorText: ".page1-test2"
    }
  ];
  isEqualJson(actualReport.unused[5].rules, expectedUnusedRules5, 'unused rules 5');

  isEqualJson(actualReport.unused[6].url, PAGE_3 + " \u2192 <style> index 0", "unused url 6");
  let expectedUnusedRules6 = [
    {
      url: PAGE_3 + " \u2192 <style> index 0",
      start: { line: 9, column: 5 },
      selectorText: ".page3-test2"
    }
  ];
  isEqualJson(actualReport.unused[6].rules, expectedUnusedRules6, 'unused rules 6');

  isEqualJson(actualReport.unused[7].url, PAGE_3 + " \u2192 <style> index 1", "unused url 7");
  let expectedUnusedRules7 = [
    {
      url: PAGE_3 + " \u2192 <style> index 1",
      start: { line: 3, column: 5 },
      selectorText: ".page3-test3"
    }
  ];
  isEqualJson(actualReport.unused[7].rules, expectedUnusedRules7, 'unused rules 7');
}






function checkRuleProperties(rule, index) {
  is(typeof rule.shortUrl, "string", "typeof rule.shortUrl for " + index);
  is(rule.shortUrl.indexOf("http://"), -1, "http not in rule.shortUrl for" + index);
  delete rule.shortUrl;

  is(typeof rule.formattedCssText, "string", "typeof rule.formattedCssText for " + index);
  ok(rule.formattedCssText.indexOf("{") > 0, "{ in rule.formattedCssText for " + index);
  delete rule.formattedCssText;
}




function isEqualJson(o1, o2, msg) {
  is(JSON.stringify(o1), JSON.stringify(o2), msg);
}
