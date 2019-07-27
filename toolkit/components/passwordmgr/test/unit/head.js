








"use strict";




const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/LoginRecipes.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "DownloadPaths",
                                  "resource://gre/modules/DownloadPaths.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "FileUtils",
                                  "resource://gre/modules/FileUtils.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "OS",
                                  "resource://gre/modules/osfile.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Promise",
                                  "resource://gre/modules/Promise.jsm");

const LoginInfo =
      Components.Constructor("@mozilla.org/login-manager/loginInfo;1",
                             "nsILoginInfo", "init");


XPCOMUtils.defineLazyModuleGetter(this, "LoginTestUtils",
                                  "resource://testing-common/LoginTestUtils.jsm");
LoginTestUtils.Assert = Assert;
const TestData = LoginTestUtils.testData;




function run_test()
{
  do_get_profile();
  run_next_test();
}











let gFileCounter = Math.floor(Math.random() * 1000000);















function getTempFile(aLeafName)
{
  
  let [base, ext] = DownloadPaths.splitBaseNameAndExtension(aLeafName);
  let leafName = base + "-" + gFileCounter + ext;
  gFileCounter++;

  
  let file = FileUtils.getFile("TmpD", [leafName]);
  do_check_false(file.exists());

  do_register_cleanup(function () {
    if (file.exists()) {
      file.remove(false);
    }
  });

  return file;
}











function newPropertyBag(aProperties)
{
  let propertyBag = Cc["@mozilla.org/hash-property-bag;1"]
                      .createInstance(Ci.nsIWritablePropertyBag);
  if (aProperties) {
    for (let [name, value] of Iterator(aProperties)) {
      propertyBag.setProperty(name, value);
    }
  }
  return propertyBag.QueryInterface(Ci.nsIPropertyBag)
                    .QueryInterface(Ci.nsIPropertyBag2)
                    .QueryInterface(Ci.nsIWritablePropertyBag2);
}



const RecipeHelpers = {
  initNewParent() {
    return (new LoginRecipesParent({ defaults: false })).initializationPromise;
  },

  



  createTestForm(aDocumentURL, aHTML = "<form>") {
    let parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                 createInstance(Ci.nsIDOMParser);
    parser.init();
    let parsedDoc = parser.parseFromString(aHTML, "text/html");

    
    
    let document = new Proxy(parsedDoc, {
      get(target, property, receiver) {
        
        
        if (property == "location") {
          return new URL(aDocumentURL);
        }
        return target[property];
      },
    });

    let form = parsedDoc.forms[0];

    
    Object.defineProperty(form, "ownerDocument", {
      value: document,
    });

    return form;
  }
};



add_task(function test_common_initialize()
{
  
  
  
  yield OS.File.copy(do_get_file("data/key3.db").path,
                     OS.Path.join(OS.Constants.Path.profileDir, "key3.db"));

  
  yield Services.logins.initializationPromise;

  
  LoginTestUtils.clearData();

  
  do_register_cleanup(() => LoginTestUtils.clearData());
});
