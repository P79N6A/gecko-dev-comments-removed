SimpleTest.waitForExplicitFinish();


let test = function (isContent) {
  
  
  let eventDefs = [["mousedown", true],
                   ["mouseup", true],
                   ["mousedown", false],
                   ["mouseup", false]];

  let testCounter = 0;

  
  let setup;

  
  let handleEvent = function (event, prefVal) {
    let resisting = prefVal && isContent;
    if (resisting) {
      is(event.screenX, event.clientX, "event.screenX and event.clientX should be the same");
      is(event.screenY, event.clientY, "event.screenY and event.clientY should be the same");
    } else {
      
      isnot(event.screenY, event.clientY, "event.screenY !== event.clientY");
    }
    ++testCounter;
    if (testCounter < eventDefs.length) {
      nextTest();
    } else {
      SimpleTest.finish();
    }
  };

  
  
  
  
  nextTest = function () {
    let [eventType, prefVal] = eventDefs[testCounter];
    SpecialPowers.pushPrefEnv({set:[["privacy.resistFingerprinting", prefVal]]},
      function () {
        
        
        
        let div = document.createElement("div");
        div.style.width = "10px";
        div.style.height = "10px";
        div.style.backgroundColor = "red";
        
        div.id = eventType;
        document.getElementById("body").appendChild(div);
        
        
        window.setTimeout(function() {
          div.addEventListener(eventType, event => handleEvent(event, prefVal), false);
          
          
          window.setTimeout(function () {
            synthesizeMouseAtCenter(div, {type : eventType});
          }, 0);
        }, 0);
      });
  };

  
  nextTest();

};
