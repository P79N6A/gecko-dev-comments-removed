




const TESTCASE_URI = TEST_BASE_HTTP + "simple.gz.html";

const Cc = Components.classes;
const Ci = Components.interfaces;
Components.utils.import("resource://gre/modules/FileUtils.jsm");


function test()
{
  waitForExplicitFinish();

  addTabAndLaunchStyleEditorChromeWhenLoaded(function (aChrome) {
    aChrome.addChromeListener({
      onEditorAdded: function (aChrome, aEditor) {
        if (aEditor.styleSheetIndex != 0) {
          return; 
        }

        if (aEditor.sourceEditor) {
          run(aEditor); 
        } else {
          aEditor.addActionListener({
            onAttach: run
          });
        }
      }
    });

    gChromeWindow.addEventListener("unload", function onClose() {
      gChromeWindow.removeEventListener("unload", onClose, true);
      gChromeWindow = null;
      executeSoon(function () {
        waitForFocus(function () {
          
          
          launchStyleEditorChrome(function (aChrome) {
            is(gChromeWindow.document.documentElement.hasAttribute("data-marker"),
               false,
               "opened a completely new StyleEditorChrome window");

            aChrome.addChromeListener({
              onEditorAdded: function (aChrome, aEditor) {
                if (aEditor.styleSheetIndex != 0) {
                  return; 
                }

                if (aEditor.sourceEditor) {
                  testNewChrome(aEditor); 
                } else {
                  aEditor.addActionListener({
                    onAttach: testNewChrome
                  });
                }
              }
            });
          });
        });
      });
    }, true);
  });

  content.location = TESTCASE_URI;
}

let gFilename;

function run(aEditor)
{
  gFilename = FileUtils.getFile("ProfD", ["styleeditor-test.css"])

  aEditor.saveToFile(gFilename, function (aFile) {
    ok(aFile, "file got saved successfully");

    aEditor.addActionListener({
      onFlagChange: function (aEditor, aFlag) {
        if (aFlag != "unsaved") {
          return;
        }

        ok(aEditor.hasFlag("unsaved"),
           "first stylesheet has UNSAVED flag after making a change");

        
        
        gChromeWindow.document.documentElement.setAttribute("data-marker", "true");
        gChromeWindow.close();
      }
    });

    waitForFocus(function () {
      
      EventUtils.synthesizeKey("x", {}, gChromeWindow);
    }, gChromeWindow);
  });
}

function testNewChrome(aEditor)
{
  ok(aEditor.savedFile,
     "first stylesheet editor will save directly into the same file");

  is(aEditor.getFriendlyName(), gFilename.leafName,
     "first stylesheet still has the filename as it was saved");
  gFilename = null;

  ok(aEditor.hasFlag("unsaved"),
     "first stylesheet still has UNSAVED flag at reopening");

  ok(!aEditor.hasFlag("inline"),
     "first stylesheet does not have INLINE flag");

  ok(!aEditor.hasFlag("error"),
     "editor does not have error flag initially");
  let hadError = false;

  let onSaveCallback = function (aFile) {
    aEditor.addActionListener({
      onFlagChange: function (aEditor, aFlag) {
        if (!hadError && aFlag == "error") {
          ok(aEditor.hasFlag("error"),
             "editor has ERROR flag after attempting to save with invalid path");
          hadError = true;

          
          waitForFocus(function () {
            EventUtils.synthesizeKey("S", {accelKey: true}, gChromeWindow);
          }, gChromeWindow);
          return;
        }

        if (hadError && aFlag == "unsaved") {
          executeSoon(function () {
            ok(!aEditor.hasFlag("unsaved"),
               "first stylesheet has no UNSAVED flag after successful save");
            ok(!aEditor.hasFlag("error"),
               "ERROR flag has been removed since last operation succeeded");
            finish();
          });
        }
      }
    });
  }

  let os = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime).OS;
  if (os == "WINNT") {
    aEditor.saveToFile("C:\\I_DO_NOT_EXIST_42\\bogus.css", onSaveCallback);
  } else {
    aEditor.saveToFile("/I_DO_NOT_EXIST_42/bogos.css", onSaveCallback);
  }
}
