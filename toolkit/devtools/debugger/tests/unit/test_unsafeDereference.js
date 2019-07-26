










Components.utils.import("resource://gre/modules/jsdebugger.jsm");
addDebuggerToGlobal(this);



Debugger.Object.prototype.getProperty = function (aName) {
  let desc = this.getOwnPropertyDescriptor(aName);
  if (!desc)
    return undefined;
  if (!desc.value) {
    throw Error("Debugger.Object.prototype.getProperty: " +
                "not a value property: " + aName);
  }
  return desc.value;
};

function run_test() {
  
  let contentBox = Components.utils.Sandbox('http://www.example.com');
  let chromeBox = Components.utils.Sandbox(this);

  
  
  
  var mainObj = { name: "mainObj" };
  Components.utils.evalInSandbox('var contentObj = { name: "contentObj" };',
                                 contentBox);
  Components.utils.evalInSandbox('var chromeObj = { name: "chromeObj" };',
                                 chromeBox);

  
  contentBox.mainObj = chromeBox.mainObj = mainObj;
  var contentObj = chromeBox.contentObj = contentBox.contentObj;
  var chromeObj  = contentBox.chromeObj = chromeBox.chromeObj;

  
  
  

  
  
  do_check_true(Components.utils.evalInSandbox('mainObj', contentBox)
                === contentBox.mainObj);
  do_check_true(Components.utils.evalInSandbox('contentObj', contentBox)
                === contentBox.contentObj);
  do_check_true(Components.utils.evalInSandbox('chromeObj', contentBox)
                === contentBox.chromeObj);
  do_check_true(Components.utils.evalInSandbox('mainObj', chromeBox)
                === chromeBox.mainObj);
  do_check_true(Components.utils.evalInSandbox('contentObj', chromeBox)
                === chromeBox.contentObj);
  do_check_true(Components.utils.evalInSandbox('chromeObj', chromeBox)
                === chromeBox.chromeObj);

  
  do_check_true(contentBox.mainObj.name === "mainObj");
  do_check_true(contentBox.contentObj.name === "contentObj");
  do_check_true(contentBox.chromeObj.name === "chromeObj");

  
  do_check_eq(Components.utils.evalInSandbox('mainObj.name', chromeBox),
              'mainObj');
  do_check_eq(Components.utils.evalInSandbox('contentObj.name', chromeBox),
              'contentObj');
  do_check_eq(Components.utils.evalInSandbox('chromeObj.name', chromeBox),
              'chromeObj');

  
  
  
  do_check_eq(Components.utils.evalInSandbox('mainObj.name', contentBox),
              undefined);
  do_check_eq(Components.utils.evalInSandbox('contentObj.name', contentBox),
              'contentObj');
  do_check_eq(Components.utils.evalInSandbox('chromeObj.name', contentBox),
              undefined);

  
  
  

  
  let dbg = new Debugger;

  
  
  let contentBoxDO = dbg.addDebuggee(contentBox);
  let chromeBoxDO = dbg.addDebuggee(chromeBox);

  
  
  
  let mainFromContentDO = contentBoxDO.getProperty('mainObj');
  do_check_eq(mainFromContentDO, contentBoxDO.makeDebuggeeValue(mainObj));
  do_check_eq(mainFromContentDO.getProperty('name'), undefined);
  do_check_eq(mainFromContentDO.unsafeDereference(), mainObj);

  let contentFromContentDO = contentBoxDO.getProperty('contentObj');
  do_check_eq(contentFromContentDO, contentBoxDO.makeDebuggeeValue(contentObj));
  do_check_eq(contentFromContentDO.getProperty('name'), 'contentObj');
  do_check_eq(contentFromContentDO.unsafeDereference(), contentObj);

  let chromeFromContentDO = contentBoxDO.getProperty('chromeObj');
  do_check_eq(chromeFromContentDO, contentBoxDO.makeDebuggeeValue(chromeObj));
  do_check_eq(chromeFromContentDO.getProperty('name'), undefined);
  do_check_eq(chromeFromContentDO.unsafeDereference(), chromeObj);

  
  let mainFromChromeDO = chromeBoxDO.getProperty('mainObj');
  do_check_eq(mainFromChromeDO, chromeBoxDO.makeDebuggeeValue(mainObj));
  do_check_eq(mainFromChromeDO.getProperty('name'), 'mainObj');
  do_check_eq(mainFromChromeDO.unsafeDereference(), mainObj);

  let contentFromChromeDO = chromeBoxDO.getProperty('contentObj');
  do_check_eq(contentFromChromeDO, chromeBoxDO.makeDebuggeeValue(contentObj));
  do_check_eq(contentFromChromeDO.getProperty('name'), 'contentObj');
  do_check_eq(contentFromChromeDO.unsafeDereference(), contentObj);

  let chromeFromChromeDO = chromeBoxDO.getProperty('chromeObj');
  do_check_eq(chromeFromChromeDO, chromeBoxDO.makeDebuggeeValue(chromeObj));
  do_check_eq(chromeFromChromeDO.getProperty('name'), 'chromeObj');
  do_check_eq(chromeFromChromeDO.unsafeDereference(), chromeObj);
}
