



let testPage = "data:text/html,<body><style>:-moz-window-inactive { background-color: red; }</style><div id='area'></div></body>";

let colorChangeNotifications = 0;
let otherWindow;

let browser1, browser2;

function test() {
  waitForExplicitFinish();

  let tab1 = gBrowser.addTab();
  let tab2 = gBrowser.addTab();
  browser1 = gBrowser.getBrowserForTab(tab1);
  browser2 = gBrowser.getBrowserForTab(tab2);

  gURLBar.focus();

  var loadCount = 0;
  function check()
  {
    
    if (++loadCount != 2) {
      return;
    }

    browser1.removeEventListener("load", check, true);
    browser2.removeEventListener("load", check, true);

    sendGetBackgroundRequest(true);
  }

  
  
  
  
  
  window.messageManager.addMessageListener("Test:BackgroundColorChanged", function(message) {
    colorChangeNotifications++;

    switch (colorChangeNotifications) {
      case 1:
        is(message.data.color, "transparent", "first window initial");
        break;
      case 2:
        is(message.data.color, "transparent", "second window initial");
        runOtherWindowTests();
        break;
      case 3:
        is(message.data.color, "rgb(255, 0, 0)", "first window lowered");
        break;
      case 4:
        is(message.data.color, "rgb(255, 0, 0)", "second window lowered");
        sendGetBackgroundRequest(true);
        otherWindow.close();
        break;
      case 5:
        is(message.data.color, "transparent", "first window raised");
        break;
      case 6:
        is(message.data.color, "transparent", "second window raised");
        gBrowser.selectedTab = tab2;
        break;
      case 7:
        is(message.data.color, "transparent", "first window after tab switch");
        break;
      case 8:
        is(message.data.color, "transparent", "second window after tab switch");
        finishTest();
        break;
      case 9:
        ok(false, "too many color change notifications");
        break;
    }
  });

  window.messageManager.addMessageListener("Test:FocusReceived", function(message) {
    
    if (colorChangeNotifications == 6) {
      sendGetBackgroundRequest(false);
    }
  });

  browser1.addEventListener("load", check, true);
  browser2.addEventListener("load", check, true);
  browser1.contentWindow.location = testPage;
  browser2.contentWindow.location = testPage;

  browser1.messageManager.loadFrameScript("data:,(" + childFunction.toString() + ")();", true);
  browser2.messageManager.loadFrameScript("data:,(" + childFunction.toString() + ")();", true);

  gBrowser.selectedTab = tab1;
}

function sendGetBackgroundRequest(ifChanged)
{
  browser1.messageManager.sendAsyncMessage("Test:GetBackgroundColor", { ifChanged: ifChanged });
  browser2.messageManager.sendAsyncMessage("Test:GetBackgroundColor", { ifChanged: ifChanged });
}

function runOtherWindowTests() {
  otherWindow = window.open("data:text/html,<body>Hi</body>", "", "chrome");
  waitForFocus(function () {
    sendGetBackgroundRequest(true);
  }, otherWindow);
}

function finishTest()
{
  gBrowser.removeCurrentTab();
  gBrowser.removeCurrentTab();
  otherWindow = null;
  finish();
}

function childFunction()
{
  let oldColor = null;

  let expectingResponse = false;
  let ifChanged = true;

  addMessageListener("Test:GetBackgroundColor", function(message) {
    expectingResponse = true;
    ifChanged = message.data.ifChanged;
  });

  content.addEventListener("focus", function () {
    sendAsyncMessage("Test:FocusReceived", { });
  }, false);

  content.setInterval(function () {
    if (!expectingResponse) {
      return;
    }

    let area = content.document.getElementById("area");
    if (!area) {
      return; 
    }

    let color = content.getComputedStyle(area, "").backgroundColor;
    if (oldColor != color || !ifChanged) {
      expectingResponse = false;
      oldColor = color;
      sendAsyncMessage("Test:BackgroundColorChanged", { color: color });
    }
  }, 20);
}
