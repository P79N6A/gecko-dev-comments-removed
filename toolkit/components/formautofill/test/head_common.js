






"use strict";

XPCOMUtils.defineLazyModuleGetter(this, "DownloadPaths",
                                  "resource://gre/modules/DownloadPaths.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FormAutofill",
                                  "resource://gre/modules/FormAutofill.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "NetUtil",
                                  "resource://gre/modules/NetUtil.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Task",
                                  "resource://gre/modules/Task.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");

let gTerminationTasks = [];
let add_termination_task = taskFn => gTerminationTasks.push(taskFn);









let terminationTaskFn = function* test_common_terminate() {
  for (let taskFn of gTerminationTasks) {
    try {
      yield Task.spawn(taskFn);
    } catch (ex) {
      Output.print(ex);
      Assert.ok(false);
    }
  }
};






let TestUtils = {
  








  waitForTick: function () {
    return new Promise(resolve => executeSoon(resolve));
  },

  











  waitMs: function (aTimeMs) {
    return new Promise(resolve => setTimeout(resolve, aTimeMs));
  },

  









  waitForNotification: function (aTopic) {
    Output.print("Waiting for notification: '" + aTopic + "'.");

    return new Promise(resolve => Services.obs.addObserver(
      function observe(aSubject, aTopic, aData) {
        Services.obs.removeObserver(observe, aTopic);
        resolve([aSubject, aData]);
      }, aTopic, false));
  },

  













  waitForEvent: function (aTarget, aEventName, aUseCapture = false) {
    Output.print("Waiting for event: '" + aEventName + "' on " + aTarget + ".");

    return new Promise(resolve => aTarget.addEventListener(aEventName,
      function onEvent(...aArgs) {
        aTarget.removeEventListener(aEventName, onEvent, aUseCapture);
        resolve(...aArgs);
      }, aUseCapture));
  },

  
  
  
  
  _fileCounter: Math.floor(Math.random() * 1000000),

  














  getTempFile: Task.async(function* (aLeafName) {
    
    let [base, ext] = DownloadPaths.splitBaseNameAndExtension(aLeafName);
    let leafName = base + "-" + this._fileCounter + ext;
    this._fileCounter++;

    
    let path = OS.Path.join(OS.Constants.Path.tmpDir, leafName);
    Assert.ok(!(yield OS.File.exists(path)));

    
    add_termination_task(function* () {
      if (yield OS.File.exists(path)) {
        yield OS.File.remove(path);
      }
    });

    return path;
  }),
};



let FormAutofillTest = {
  



  requestAutocompleteResponse: null,

  












  showUI: Task.async(function* (aFormAutofillData) {
    Output.print("Opening UI with data: " + JSON.stringify(aFormAutofillData));

    
    let promiseUIWindow =
        TestUtils.waitForNotification("formautofill-window-initialized");
    let ui = yield FormAutofill.integration.createRequestAutocompleteUI(
                                                         aFormAutofillData);
    let promiseResult = ui.show();

    
    return {
      uiWindow: (yield promiseUIWindow)[0],
      promiseResult: promiseResult,
    };
  }),
};

let TestData = {
  


  get requestEmailOnly() {
    return {
      sections: [{
        name: "",
        addressSections: [{
          addressType: "",
          fields: [{
            fieldName: "email",
            contactType: "",
          }],
        }],
      }],
    };
  },
};



add_task(function* test_common_initialize() {
  
  Services.prefs.setBoolPref("dom.forms.requestAutocomplete", true);
  add_termination_task(function* () {
    Services.prefs.clearUserPref("dom.forms.requestAutocomplete");
  });

  
  let mockIntegrationFn = base => ({
    createRequestAutocompleteUI: Task.async(function* () {
      
      if (FormAutofillTest.requestAutocompleteResponse === null) {
        return yield base.createRequestAutocompleteUI.apply(this, arguments);
      }

      
      return {
        show: Task.async(function* () {
          let response = FormAutofillTest.requestAutocompleteResponse;
          Output.print("Mock UI response: " + JSON.stringify(response));
          return response;
        }),
      };
    }),
  });

  FormAutofill.registerIntegration(mockIntegrationFn);
  add_termination_task(function* () {
    FormAutofill.unregisterIntegration(mockIntegrationFn);
  });
});
