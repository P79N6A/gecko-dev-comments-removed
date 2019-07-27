




const csscoverage = require("devtools/server/actors/csscoverage");

const PAGE_1 = TEST_BASE_HTTPS + "browser_cmd_csscoverage_page1.html";
const PAGE_2 = TEST_BASE_HTTPS + "browser_cmd_csscoverage_page2.html";
const PAGE_3 = TEST_BASE_HTTPS + "browser_cmd_csscoverage_page3.html";

const SHEET_A = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetA.css";
const SHEET_B = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetB.css";
const SHEET_C = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetC.css";
const SHEET_D = TEST_BASE_HTTPS + "browser_cmd_csscoverage_sheetD.css";

let test = asyncTest(function*() {
  let options = yield helpers.openTab(PAGE_3);
  yield helpers.openToolbar(options);

  let usage = yield csscoverage.getUsage(options.target);

  yield navigate(usage, options);
  yield checkPages(usage);
  yield checkEditorReport(usage);
  
  
  
  
  
  

  yield helpers.closeToolbar(options);
  yield helpers.closeTab(options);
});




function* navigate(usage, options) {
  ok(!usage.isRunning(), "csscoverage is not running");

  yield usage.oneshot();

  ok(!usage.isRunning(), "csscoverage is still not running");
}




function* checkPages(usage) {
  let expectedVisited = [ PAGE_3 ];
  let actualVisited = yield usage._testOnly_visitedPages();
  isEqualJson(actualVisited, expectedVisited, 'Visited');
}




function* checkEditorReport(usage) {
  
  let expectedPage1 = { reports: [] };
  let actualPage1 = yield usage.createEditorReport(PAGE_1 + " \u2192 <style> index 0");
  isEqualJson(actualPage1, expectedPage1, 'Page1');

  
  let expectedPage2 = { reports: [] };
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
      },
      {
        selectorText: ".sheetA-test3",
        start: { line: 12, column: 1 },
      },
      {
        selectorText: ".sheetA-test4",
        start: { line: 16, column: 1 },
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
      },
      {
        selectorText: ".sheetB-test3",
        start: { line: 10, column: 1 },
      },
      {
        selectorText: ".sheetB-test4",
        start: { line: 14, column: 1 },
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
      },
      {
        selectorText: ".sheetC-test3",
        start: { line: 10, column: 1 },
      },
      {
        selectorText: ".sheetC-test4",
        start: { line: 14, column: 1 },
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
      },
      {
        selectorText: ".sheetD-test3",
        start: { line: 10, column: 1 },
      },
      {
        selectorText: ".sheetD-test4",
        start: { line: 14, column: 1 },
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

  
  let expectedSummary = { "used": 23, "unused": 9, "preload": 0 };
  isEqualJson(actualReport.summary, expectedSummary, 'summary');

  
  isEqualJson(actualReport.preload.length, 0, 'preload length');

  
  isEqualJson(actualReport.unused.length, 6, 'unused length');

  
  isEqualJson(actualReport.unused[0].url, PAGE_3 + " \u2192 <style> index 0", "unused url 0");
  let expectedUnusedRules0 = [
    {
      "url": PAGE_3 + " \u2192 <style> index 0",
      "start": { "line": 9, "column": 5 },
      "selectorText": ".page3-test2"
    }
  ];
  isEqualJson(actualReport.unused[0].rules, expectedUnusedRules0, 'unused rules 0');

  isEqualJson(actualReport.unused[1].url, PAGE_3 + " \u2192 <style> index 1", "unused url 1");
  let expectedUnusedRules1 = [
    {
      "url": PAGE_3 + " \u2192 <style> index 1",
      "start": { "line": 3, "column": 5 },
      "selectorText": ".page3-test3"
    }
  ];
  isEqualJson(actualReport.unused[1].rules, expectedUnusedRules1, 'unused rules 1');

  isEqualJson(actualReport.unused[2].url, SHEET_A, "unused url 2");
  let expectedUnusedRules2 = [
    {
      "url": SHEET_A,
      "start": { "line": 8, "column": 1 },
      "selectorText": ".sheetA-test2"
    },
    {
      "url": SHEET_A,
      "start": { "line": 12, "column": 1 },
      "selectorText": ".sheetA-test3"
    },
    {
      "url": SHEET_A,
      "start": { "line": 16, "column": 1 },
      "selectorText": ".sheetA-test4"
    }
  ];
  isEqualJson(actualReport.unused[2].rules, expectedUnusedRules2, 'unused rules 2');

  isEqualJson(actualReport.unused[3].url, SHEET_B, "unused url 3");
  let expectedUnusedRules3 = [
    {
      "url": SHEET_B,
      "start": { "line": 6, "column": 1 },
      "selectorText": ".sheetB-test2"
    },
    {
      "url": SHEET_B,
      "start": { "line": 10, "column": 1 },
      "selectorText": ".sheetB-test3"
    },
    {
      "url": SHEET_B,
      "start": { "line": 14, "column": 1 },
      "selectorText": ".sheetB-test4"
    }
  ];
  isEqualJson(actualReport.unused[3].rules, expectedUnusedRules3, 'unused rules 3');

  isEqualJson(actualReport.unused[4].url, SHEET_D, "unused url 4");
  let expectedUnusedRules4 = [
    {
      "url": SHEET_D,
      "start": { "line": 6, "column": 1 },
      "selectorText": ".sheetD-test2"
    },
    {
      "url": SHEET_D,
      "start": { "line": 10, "column": 1 },
      "selectorText": ".sheetD-test3"
    },
    {
      "url": SHEET_D,
      "start": { "line": 14, "column": 1 },
      "selectorText": ".sheetD-test4"
    }
  ];
  isEqualJson(actualReport.unused[4].rules, expectedUnusedRules4, 'unused rules 4');

  isEqualJson(actualReport.unused[5].url, SHEET_C, "unused url 5");
  let expectedUnusedRules5 = [
    {
      "url": SHEET_C,
      "start": { "line": 6, "column": 1 },
      "selectorText": ".sheetC-test2"
    },
    {
      "url": SHEET_C,
      "start": { "line": 10, "column": 1 },
      "selectorText": ".sheetC-test3"
    },
    {
      "url": SHEET_C,
      "start": { "line": 14, "column": 1 },
      "selectorText": ".sheetC-test4"
    }
  ];
  isEqualJson(actualReport.unused[5].rules, expectedUnusedRules5, 'unused rules 5');
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
