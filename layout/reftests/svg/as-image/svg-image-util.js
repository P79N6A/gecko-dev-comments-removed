






































var HOST_NODE_HEIGHT = "20";
var HOST_NODE_WIDTH =  "30";


const ALIGN_VALS = ["none",
                    "xMinYMin", "xMinYMid", "xMinYMax",
                    "xMidYMin", "xMidYMid", "xMidYMax",
                    "xMaxYMin", "xMaxYMid", "xMaxYMax"];


const MEETORSLICE_VALS = [ "meet", "slice" ];
















function generateSVGDataURI(aViewboxArr, aWidth, aHeight,
                            aAlign, aMeetOrSlice) {
  
  var datauri = "data:image/svg+xml,"
  
  datauri += "%3Csvg%20xmlns%3D%22http%3A%2F%2Fwww.w3.org%2F2000%2Fsvg%22%20shape-rendering%3D%22crispEdges%22";

  
  datauri += generateSVGAttrsForParams(aViewboxArr, aWidth, aHeight,
                                       aAlign, aMeetOrSlice);

  
  datauri += "%3E";

  
  datauri += "%3Crect%20x%3D%221%22%20y%3D%221%22%20height%3D%2218%22%20width%3D%2218%22%20stroke-width%3D%222%22%20stroke%3D%22black%22%20fill%3D%22yellow%22%2F%3E%3Ccircle%20cx%3D%2210%22%20cy%3D%2210%22%20r%3D%228%22%20style%3D%22fill%3A%20blue%22%2F%3E%3C%2Fsvg%3E";

  return datauri;
}



function generateSVGAttrsForParams(aViewboxArr, aWidth, aHeight,
                                   aAlign, aMeetOrSlice) {
  var str = "";
  if (aViewboxArr) {
    str += "%20viewBox%3D%22";
    for (var i in aViewboxArr) {
        var curVal = aViewboxArr[i];
        str += curVal + "%20";
    }
    str += "%22";
  }
  if (aWidth) {
    str += "%20width%3D%22"  + aWidth  + "%22";
  }
  if (aHeight) {
    str += "%20height%3D%22" + aHeight + "%22";
  }
  if (aAlign) {
    str += "%20preserveAspectRatio%3D%22" + aAlign;
    if (aMeetOrSlice) {
      str += "%20" + aMeetOrSlice;
    }
    str += "%22";
  }

  
  str += "%20font-size%3D%22" + "10px" + "%22";

  return str;
}



function generateHostNode(aHostNodeTagName, aUri,
                          aHostNodeWidth, aHostNodeHeight) {
  var elem = document.createElement(aHostNodeTagName);
  elem.setAttribute("src", aUri);

  if (aHostNodeWidth) {
    elem.setAttribute("width", aHostNodeWidth);
  }
  if (aHostNodeHeight) {
    elem.setAttribute("height", aHostNodeHeight);
  }

  return elem;
}


function appendSVGArrayWithParams(aSVGParams, aHostNodeTagName) {
  
  
  var hostNodeWidthVals  = [ null, HOST_NODE_WIDTH  ];
  var hostNodeHeightVals = [ null, HOST_NODE_HEIGHT ];

  for (var i = 0; i < hostNodeWidthVals.length; i++) {
    var hostNodeWidth = hostNodeWidthVals[i];
    for (var j = 0; j < hostNodeHeightVals.length; j++) {
      var hostNodeHeight = hostNodeHeightVals[j];
      appendSVGSubArrayWithParams(aSVGParams, aHostNodeTagName,
                                  hostNodeWidth, hostNodeHeight);
    }
  }
}


function appendSVGSubArrayWithParams(aSVGParams, aHostNodeTagName,
                                     aHostNodeWidth, aHostNodeHeight) {
  var rootNode = document.getElementsByTagName("body")[0];
  for (var k = 0; k < ALIGN_VALS.length; k++) {
    var alignVal = ALIGN_VALS[k];
    if (!aSVGParams.meetOrSlice) {
      alignVal = "none";
    }

    
    var uri = generateSVGDataURI(aSVGParams.viewBox,
                                 aSVGParams.width, aSVGParams.height,
                                 alignVal,
                                 aSVGParams.meetOrSlice);

    
    var hostNode = generateHostNode(aHostNodeTagName, uri,
                                    aHostNodeWidth, aHostNodeHeight);
    rootNode.appendChild(hostNode);

    
    
    if (k + 1 == ALIGN_VALS.length / 2 ||
        k + 1 == ALIGN_VALS.length) {
      rootNode.appendChild(document.createElement("br"));
    }
  }
}
