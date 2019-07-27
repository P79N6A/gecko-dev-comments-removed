




function test()
{
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.selectedBrowser.addEventListener("load", function browserLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", browserLoad, true);
    openScratchpad(runTests, {"state":{"text":""}});
  }, true);

  content.location = "data:text/html, test that exceptions are output as " +
      "comments correctly in Scratchpad";
}

function runTests()
{
  var scratchpad = gScratchpadWindow.Scratchpad;

  var message = "\"Hello World!\""
  var openComment = "\n/*\n";
  var closeComment = "\n*/";
  var error1 = "throw new Error(\"Ouch!\")";
  var error2 = "throw \"A thrown string\"";
  var error3 = "throw {}";
  var error4 = "document.body.appendChild(document.body)";

  let tests = [{
    
    method: "display",
    code: message,
    result: message + openComment + "Hello World!" + closeComment,
    label: "message display output"
  },
  {
    
    method: "display",
    code: error1,
    result: error1 + openComment +
            "Exception: Ouch!\n@" + scratchpad.uniqueName + ":1:7" + closeComment,
    label: "error display output"
  },
  {
    
    method: "display",
    code: error2,
    result: error2 + openComment + "Exception: A thrown string" + closeComment,
    label: "thrown string display output"
  },
  {
    
    method: "display",
    code: error3,
    result: error3 + openComment + "Exception: [object Object]" + closeComment,
    label: "thrown object display output"
  },
  {
    
    method: "display",
    code: error4,
    result: error4 + openComment + "Exception: Node cannot be inserted " +
            "at the specified point in the hierarchy\n@" +
            scratchpad.uniqueName + ":1:0" + closeComment,
    label: "Alternative format error display output"
  },
  {
    
    method: "run",
    code: message,
    result: message,
    label: "message run output"
  },
  {
    
    method: "run",
    code: error1,
    result: error1 + openComment +
            "Exception: Ouch!\n@" + scratchpad.uniqueName + ":1:7" + closeComment,
    label: "error run output"
  },
  {
    
    method: "run",
    code: error2,
    result: error2 + openComment + "Exception: A thrown string" + closeComment,
    label: "thrown string run output"
  },
  {
    
    method: "run",
    code: error3,
    result: error3 + openComment + "Exception: [object Object]" + closeComment,
    label: "thrown object run output"
  },
  {
    
    method: "run",
    code: error4,
    result: error4 + openComment + "Exception: Node cannot be inserted " +
            "at the specified point in the hierarchy\n@" +
            scratchpad.uniqueName + ":1:0" + closeComment,
    label: "Alternative format error run output"
  }];

  runAsyncTests(scratchpad, tests).then(finish);
}
