
let test = function (isContent) {
  SimpleTest.waitForExplicitFinish();

  let { ww } = SpecialPowers.Services;
  window.chromeWindow = ww.activeWindow;

  
  
  let pairs = [
    ["screenX", 0],
    ["screenY", 0],
    ["mozInnerScreenX", 0],
    ["mozInnerScreenY", 0],
    ["screen.pixelDepth", 24],
    ["screen.colorDepth", 24],
    ["screen.availWidth", "innerWidth"],
    ["screen.availHeight", "innerHeight"],
    ["screen.left", 0],
    ["screen.top", 0],
    ["screen.availLeft", 0],
    ["screen.availTop", 0],
    ["screen.width", "innerWidth"],
    ["screen.height", "innerHeight"],
    ["screen.mozOrientation", "'landscape-primary'"],
    ["devicePixelRatio", 1]
  ];

  
  let checkPair = function (a, b) {
    is(eval(a), eval(b), a + " should be equal to " + b);
  };

  
  let prefVals = (for (prefVal of [false, true]) prefVal);

  
  let nextTest = function () {
    let {value : prefValue, done} = prefVals.next();
    if (done) {
      SimpleTest.finish();
      return;
    }
    SpecialPowers.pushPrefEnv({set : [["privacy.resistFingerprinting", prefValue]]},
      function () {
        
        
        let resisting = prefValue && isContent;
        
        pairs.map(function ([item, onVal]) {
          if (resisting) {
            checkPair("window." + item, onVal);
          } else {
            if (!item.startsWith("moz")) {
              checkPair("window." + item, "chromeWindow." + item);
            }
          }
        });
        if (!resisting) {
          
          ok(window.mozInnerScreenX >= chromeWindow.mozInnerScreenX,
             "mozInnerScreenX");
          ok(window.mozInnerScreenY >= chromeWindow.mozInnerScreenY,
             "mozInnerScreenY");
        }
      nextTest();
    });
  }

  nextTest();
}
