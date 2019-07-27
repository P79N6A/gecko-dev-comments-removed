





const TEST_URI = URL_ROOT +
                 "browser_toolbox_options_enable_serviceworkers_testing.html";

const ELEMENT_ID = "devtools-enable-serviceWorkersTesting";

let toolbox;
let doc;

function test() {
  
  
  SpecialPowers.pushPrefEnv({"set": [
    ["dom.serviceWorkers.exemptFromPerDomainMax", true],
    ["dom.serviceWorkers.enabled", true],
    ["dom.serviceWorkers.testing.enabled", false]
  ]}, start);
}

function start() {
  gBrowser.selectedTab = gBrowser.addTab();
  let target = TargetFactory.forTab(gBrowser.selectedTab);

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    doc = content.document;
    gDevTools.showToolbox(target).then(testSelectTool);
  }, true);

  content.location = TEST_URI;
}

function testSelectTool(aToolbox) {
  toolbox = aToolbox;
  toolbox.once("options-selected", testRegisterFails);
  toolbox.selectTool("options");
}

function testRegisterFails() {
  let output = doc.getElementById("output");
  let button = doc.getElementById("button");

  function doTheCheck() {
    info("Testing it doesn't registers correctly until enable testing");
    is(output.textContent,
       "SecurityError",
       "SecurityError expected");
    testRegisterInstallingWorker();
  }

  if (output.textContent !== "No output") {
    doTheCheck();
  }

  button.addEventListener('click', function onClick() {
    button.removeEventListener('click', onClick);
    doTheCheck();
  });
}

function testRegisterInstallingWorker() {
  toggleServiceWorkersTestingCheckbox().then(() => {
    let output = doc.getElementById("output");
    let button = doc.getElementById("button");

    function doTheCheck() {
      info("Testing it registers correctly and there is an installing worker");
      is(output.textContent,
         "Installing worker/",
         "Installing worker expected");
      toggleServiceWorkersTestingCheckbox().then(finishUp);
    }

    if (output.textContent !== "No output") {
      doTheCheck();
    }

    button.addEventListener('click', function onClick() {
      button.removeEventListener('click', onClick);
      doTheCheck();
    });
  });
}

function toggleServiceWorkersTestingCheckbox() {
  let deferred = promise.defer();

  let panel = toolbox.getCurrentPanel();
  let cbx = panel.panelDoc.getElementById(ELEMENT_ID);

  cbx.scrollIntoView();

  if (cbx.checked) {
    info("Clearing checkbox to disable service workers testing");
  } else {
    info("Checking checkbox to enable service workers testing");
  }

  gBrowser.selectedBrowser.addEventListener("load", function onLoad(evt) {
    gBrowser.selectedBrowser.removeEventListener(evt.type, onLoad, true);
    doc = content.document;
    deferred.resolve();
  }, true);

  cbx.click();

  let mm = getFrameScript();
  mm.sendAsyncMessage("devtools:test:reload");

  return deferred.promise;
}

function finishUp() {
  toolbox.destroy().then(function() {
    gBrowser.removeCurrentTab();
    toolbox = doc = null;
    finish();
  });
}
