










































const START_TIMES = [ "4.0s",  "3.0s",  "2.7s",
                      "2.25s", "2.01s", "1.5s",
                      "1.4s",  "1.0s",  "0.5s" ];

const X_POSNS = [ "20pt",  "70pt",  "120pt",
                  "20pt",  "70pt",  "120pt",
                  "20pt",  "70pt",  "120pt" ];

const Y_POSNS = [ "20pt",  "20pt",  "20pt",
                  "70pt",  "70pt",  "70pt",
                 "120pt", "120pt", "120pt"  ];

const DURATION = "2s";
const SNAPSHOT_TIME ="3";
const SVGNS = "http://www.w3.org/2000/svg";


function testAnimatedRectGrid(animationTagName, animationAttrHashList) {
  var targetTagName = "rect";
  var targetAttrHash = {"width"  : "15pt",
                        "height" : "15pt" };
  testAnimatedGrid(targetTagName,    targetAttrHash,
                   animationTagName, animationAttrHashList);
}


function testAnimatedTextGrid(animationTagName, animationAttrHashList) {
  var targetTagName = "text";
  var targetAttrHash = { };
  testAnimatedGrid(targetTagName,    targetAttrHash,
                   animationTagName, animationAttrHashList);
}





function testAnimatedGrid(targetTagName,    targetAttrHash,
                          animationTagName, animationAttrHashList) {
    
  const numElementsToMake = START_TIMES.length;
  if (X_POSNS.length != numElementsToMake ||
      Y_POSNS.length != numElementsToMake) {
    return;
  }
  
  for (var i = 0; i < animationAttrHashList.length; i++) {
    var animationAttrHash = animationAttrHashList[i];
    
    if (!animationAttrHash["fill"]) {
      animationAttrHash["fill"] = "freeze";
    }
  }

  
  var svg = document.documentElement;
  for (var i = 0; i < numElementsToMake; i++) {
    
    var targetElem = buildElement(targetTagName, targetAttrHash);
    for (var j = 0; j < animationAttrHashList.length; j++) {
      var animationAttrHash = animationAttrHashList[j];
      var animElem = buildElement(animationTagName, animationAttrHash);

      
      targetElem.setAttribute("x", X_POSNS[i]);
      targetElem.setAttribute("y", Y_POSNS[i]);
      animElem.setAttribute("begin", START_TIMES[i]);
      animElem.setAttribute("dur", DURATION);

      
      targetElem.appendChild(animElem);
    }
    
    svg.appendChild(targetElem);
  }

  
  setTimeAndSnapshot(SNAPSHOT_TIME, true);
}

function buildElement(tagName, attrHash) {
  var elem = document.createElementNS(SVGNS, tagName);
  for (var attrName in attrHash) {
    var attrValue = attrHash[attrName];
    elem.setAttribute(attrName, attrValue);
  }
  
  if (tagName == "text") {
    elem.appendChild(document.createTextNode("abc"));
  }
  return elem;
}
