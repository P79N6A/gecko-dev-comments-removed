



let tempScope = {};
Cu.import("resource://gre/modules/NetUtil.jsm", tempScope);
Cu.import("resource://gre/modules/FileUtils.jsm", tempScope);
let NetUtil = tempScope.NetUtil;
let FileUtils = tempScope.FileUtils;


let gScratchpad;


let gFile01;
let gFile02;
let gFile03;
let gFile04;


var lists = {
  recentFiles01: null,
  recentFiles02: null,
  recentFiles03: null,
  recentFiles04: null,
};


let gFileName01 = "file01_ForBug651942.tmp"
let gFileName02 = "file02_ForBug651942.tmp"
let gFileName03 = "file03_ForBug651942.tmp"
let gFileName04 = "file04_ForBug651942.tmp"


let gFileContent;
let gFileContent01 = "hello.world.01('bug651942');";
let gFileContent02 = "hello.world.02('bug651942');";
let gFileContent03 = "hello.world.03('bug651942');";
let gFileContent04 = "hello.world.04('bug651942');";

function startTest()
{
  gScratchpad = gScratchpadWindow.Scratchpad;

  gFile01 = createAndLoadTemporaryFile(gFile01, gFileName01, gFileContent01);
  gFile02 = createAndLoadTemporaryFile(gFile02, gFileName02, gFileContent02);
  gFile03 = createAndLoadTemporaryFile(gFile03, gFileName03, gFileContent03);
}



function testAddedToRecent()
{
  lists.recentFiles01 = gScratchpad.getRecentFiles();

  is(lists.recentFiles01.length, 3,
     "Temporary files created successfully and added to list of recent files.");

  
  gFile04 = createAndLoadTemporaryFile(gFile04, gFileName04, gFileContent04);
}



function testOverwriteRecent()
{
  lists.recentFiles02 = gScratchpad.getRecentFiles();

  is(lists.recentFiles02[0], lists.recentFiles01[1],
     "File02 was reordered successfully in the 'recent files'-list.");
  is(lists.recentFiles02[1], lists.recentFiles01[2],
     "File03 was reordered successfully in the 'recent files'-list.");
  isnot(lists.recentFiles02[2], lists.recentFiles01[2],
        "File04: was added successfully.");

  
  gScratchpad.openFile(0);
}



function testOpenOldestRecent()
{
  lists.recentFiles03 = gScratchpad.getRecentFiles();

  is(lists.recentFiles02[0], lists.recentFiles03[2],
     "File04 was reordered successfully in the 'recent files'-list.");
  is(lists.recentFiles02[1], lists.recentFiles03[0],
     "File03 was reordered successfully in the 'recent files'-list.");
  is(lists.recentFiles02[2], lists.recentFiles03[1],
     "File02 was reordered successfully in the 'recent files'-list.");

  Services.prefs.setIntPref("devtools.scratchpad.recentFilesMax", 0);
}




function testHideMenu()
{
  let menu = gScratchpadWindow.document.getElementById("sp-open_recent-menu");
  ok(menu.hasAttribute("hidden"), "The menu was hidden successfully.");

  Services.prefs.setIntPref("devtools.scratchpad.recentFilesMax", 1);
}




function testChangedMaxRecent()
{
  let menu = gScratchpadWindow.document.getElementById("sp-open_recent-menu");
  ok(!menu.hasAttribute("hidden"), "The menu is visible. \\o/");

  lists.recentFiles04 = gScratchpad.getRecentFiles();

  is(lists.recentFiles04.length, 1,
     "Two recent files were successfully removed from the 'recent files'-list");

  let doc = gScratchpadWindow.document;
  let popup = doc.getElementById("sp-menu-open_recentPopup");

  let menuitemLabel = popup.children[0].getAttribute("label");
  let correctMenuItem = false;
  if (menuitemLabel === lists.recentFiles03[2] &&
      menuitemLabel === lists.recentFiles04[0]) {
    correctMenuItem = true;
  }

  is(correctMenuItem, true,
     "Two recent files were successfully removed from the 'Open Recent'-menu");

  gScratchpad.clearRecentFiles();
}



function testClearedAll()
{
  let doc = gScratchpadWindow.document;
  let menu = doc.getElementById("sp-open_recent-menu");
  let popup = doc.getElementById("sp-menu-open_recentPopup");

  is(gScratchpad.getRecentFiles().length, 0,
     "All recent files removed successfully.");
  is(popup.children.length, 0, "All menuitems removed successfully.");
  ok(menu.hasAttribute("disabled"),
     "No files in the menu, it was disabled successfully.");

  finishTest();
}

function createAndLoadTemporaryFile(aFile, aFileName, aFileContent)
{
  
  aFile = FileUtils.getFile("TmpD", [aFileName]);
  aFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0666);

  
  let fout = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
  fout.init(aFile.QueryInterface(Ci.nsILocalFile), 0x02 | 0x08 | 0x20,
            0644, fout.DEFER_OPEN);

  gScratchpad.setFilename(aFile.path);
  gScratchpad.importFromFile(aFile.QueryInterface(Ci.nsILocalFile),  true,
                            fileImported);
  gScratchpad.saveFile(fileSaved);

  return aFile;
}

function fileImported(aStatus)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was imported successfully with Scratchpad");
}

function fileSaved(aStatus)
{
  ok(Components.isSuccessCode(aStatus),
     "the temporary file was saved successfully with Scratchpad");

  checkIfMenuIsPopulated();
}

function checkIfMenuIsPopulated()
{
  let doc = gScratchpadWindow.document;
  let expectedMenuitemCount = doc.getElementById("sp-menu-open_recentPopup").
                              children.length;
  
  
  let recentFilesPlusExtra = gScratchpad.getRecentFiles().length + 2;

  if (expectedMenuitemCount > 2) {
    is(expectedMenuitemCount, recentFilesPlusExtra,
       "the recent files menu was populated successfully.");
  }
}





var PreferenceObserver = {
  _initialized: false,

  _timesFired: 0,
  set timesFired(aNewValue) {
    this._timesFired = aNewValue;
  },
  get timesFired() {
    return this._timesFired;
  },

  init: function PO_init()
  {
    if (this._initialized) {
      return;
    }

    this.branch = Services.prefs.getBranch("devtools.scratchpad.");
    this.branch.addObserver("", this, false);
    this._initialized = true;
  },

  observe: function PO_observe(aMessage, aTopic, aData)
  {
    if (aTopic != "nsPref:changed") {
      return;
    }

    switch (this.timesFired) {
      case 0:
        this.timesFired = 1;
        break;
      case 1:
        this.timesFired = 2;
        break;
      case 2:
        this.timesFired = 3;
        testAddedToRecent();
        break;
      case 3:
        this.timesFired = 4;
        testOverwriteRecent();
        break;
      case 4:
        this.timesFired = 5;
        testOpenOldestRecent();
        break;
      case 5:
        this.timesFired = 6;
        testHideMenu();
        break;
      case 6:
        this.timesFired = 7;
        testChangedMaxRecent();
        break;
      case 7:
        this.timesFired = 8;
        testClearedAll();
        break;
    }
  },

  uninit: function PO_uninit () {
    this.branch.removeObserver("", this);
  }
};

function test()
{
  waitForExplicitFinish();

  registerCleanupFunction(function () {
    gFile01.remove(false);
    gFile01 = null;
    gFile02.remove(false);
    gFile02 = null;
    gFile03.remove(false);
    gFile03 = null;
    gFile04.remove(false);
    gFile04 = null;
    lists.recentFiles01 = null;
    lists.recentFiles02 = null;
    lists.recentFiles03 = null;
    lists.recentFiles04 = null;
    gScratchpad = null;

    PreferenceObserver.uninit();
    Services.prefs.clearUserPref("devtools.scratchpad.recentFilesMax");
  });

  Services.prefs.setIntPref("devtools.scratchpad.recentFilesMax", 3);

  
  
  PreferenceObserver.init();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    openScratchpad(startTest);
  }, true);

  content.location = "data:text/html,<p>test recent files in Scratchpad";
}

function finishTest()
{
  finish();
}
