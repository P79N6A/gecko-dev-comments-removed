


const PREF_NEWTAB_ROWS = "browser.newtabpage.rows";
const PREF_NEWTAB_COLUMNS = "browser.newtabpage.columns";

function runTests() {
  let testValues = [
    {row: 0, column: 0},
    {row: -1, column: -1},
    {row: -1, column: 0},
    {row: 0, column: -1},
    {row: 2, column: 4},
    {row: 2, column: 5},
  ];

  
  let expectedValues = [1, 1, 1, 1, 8, 10];

   
  let previousValues = [24, 1, 1, 1, 1, 8];

  let existingTab, existingTabGridLength, newTab, newTabGridLength;
  yield addNewTabPageTab();
  existingTab = gBrowser.selectedTab;

  for (let i = 0; i < expectedValues.length; i++) {
    gBrowser.selectedTab = existingTab;
    existingTabGridLength = getGrid().cells.length;
    is(existingTabGridLength, previousValues[i],
      "Grid length of existing page before update is correctly.");

    Services.prefs.setIntPref(PREF_NEWTAB_ROWS, testValues[i].row);
    Services.prefs.setIntPref(PREF_NEWTAB_COLUMNS, testValues[i].column);

    existingTabGridLength = getGrid().cells.length;
    is(existingTabGridLength, expectedValues[i],
      "Existing page grid is updated correctly.");

    yield addNewTabPageTab();
    newTab = gBrowser.selectedTab;
    newTabGridLength = getGrid().cells.length;
    is(newTabGridLength, expectedValues[i],
      "New page grid is updated correctly.");

    gBrowser.removeTab(newTab);
  }

  gBrowser.removeTab(existingTab);

  Services.prefs.clearUserPref(PREF_NEWTAB_ROWS);
  Services.prefs.clearUserPref(PREF_NEWTAB_COLUMNS);
}
