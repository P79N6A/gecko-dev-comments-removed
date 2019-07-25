






































const ALIGN_VALS = ["none",
                    "xMinYMin", "xMinYMid", "xMinYMax",
                    "xMidYMin", "xMidYMid", "xMidYMax",
                    "xMaxYMin", "xMaxYMid", "xMaxYMax"];


const MEETORSLICE_VALS = [ "meet", "slice" ];

const SVGNS   = "http://www.w3.org/2000/svg";
const XLINKNS = "http://www.w3.org/1999/xlink";



const IMAGE_OFFSET = 50;

function generateBorderRect(aX, aY, aWidth, aHeight) {
  var rect = document.createElementNS(SVGNS, "rect");
  rect.setAttribute("x", aX);
  rect.setAttribute("y", aY);
  rect.setAttribute("width", aWidth);
  rect.setAttribute("height", aHeight);
  rect.setAttribute("fill", "none");
  rect.setAttribute("stroke", "black");
  rect.setAttribute("stroke-width", "2");
  rect.setAttribute("stroke-dasharray", "3 2");
  return rect;
}



function generateImageElementForParams(aX, aY, aWidth, aHeight,
                                       aHref, aAlign, aMeetOrSlice) {
  var image = document.createElementNS(SVGNS, "image");
  image.setAttribute("x", aX);
  image.setAttribute("y", aY);
  image.setAttribute("width", aWidth);
  image.setAttribute("height", aHeight);
  image.setAttributeNS(XLINKNS, "href", aHref);
  image.setAttribute("preserveAspectRatio", aAlign + " " + aMeetOrSlice);
  return image;
}







function generateImageGrid(aHref, aWidth, aHeight, aBonusPARVal) {
  var grid = document.createElementNS(SVGNS, "g");
  var y = 0;
  var x = 0;
  for (var i = 0; i < ALIGN_VALS.length; i++) {
    
    
    if (i && i % 2 == 0) {
      y += IMAGE_OFFSET;
      x = 0;
    }
    var alignVal = ALIGN_VALS[i];
    for (var j = 0; j < MEETORSLICE_VALS.length; j++) {
      var meetorsliceVal = MEETORSLICE_VALS[j];
      var border = generateBorderRect(x, y, aWidth, aHeight);
      var image  = generateImageElementForParams(x, y, aWidth, aHeight,
                                                 aHref, alignVal,
                                                 meetorsliceVal);
      grid.appendChild(border);
      grid.appendChild(image);
      x += IMAGE_OFFSET;
    }
  }

  if (aBonusPARVal) {
    
    y += IMAGE_OFFSET;
    x = 0;
    var border = generateBorderRect(x, y, aWidth, aHeight);
    var image  = generateImageElementForParams(x, y, aWidth, aHeight,
                                               aHref, aBonusPARVal, "");
    grid.appendChild(border);
    grid.appendChild(image);
  }

  return grid;
}






function generateSymbolElementForParams(aSymbolID, aHref,
                                        aAlign, aMeetOrSlice) {
  var use = document.createElementNS(SVGNS, "use");
  use.setAttributeNS(XLINKNS, "href", aHref);

  var symbol = document.createElementNS(SVGNS, "symbol");
  symbol.setAttribute("id", aSymbolID);
  symbol.setAttribute("viewBox", "0 0 10 10");
  symbol.setAttribute("preserveAspectRatio", aAlign + " " + aMeetOrSlice);

  symbol.appendChild(use);
  return symbol;
}

function generateUseElementForParams(aTargetURI, aX, aY, aWidth, aHeight) {
  var use = document.createElementNS(SVGNS, "use");
  use.setAttributeNS(XLINKNS, "href", aTargetURI);
  use.setAttribute("x", aX);
  use.setAttribute("y", aY);
  use.setAttribute("width", aWidth);
  use.setAttribute("height", aHeight);
  return use;
}








function generateSymbolGrid(aHref, aWidth, aHeight, aBonusPARVal) {
  var grid = document.createElementNS(SVGNS, "g");
  var y = 0;
  var x = 0;
  for (var i = 0; i < ALIGN_VALS.length; i++) {
    
    
    if (i && i % 2 == 0) {
      y += IMAGE_OFFSET;
      x = 0;
    }
    var alignVal = ALIGN_VALS[i];
    for (var j = 0; j < MEETORSLICE_VALS.length; j++) {
      var meetorsliceVal = MEETORSLICE_VALS[j];
      var border = generateBorderRect(x, y, aWidth, aHeight);

      var symbolID = "symbol_" + alignVal + "_" + meetorsliceVal;
      var symbol = generateSymbolElementForParams(symbolID, aHref,
                                                  alignVal, meetorsliceVal);
      var use = generateUseElementForParams("#" + symbolID,
                                            x, y, aWidth, aHeight);
      grid.appendChild(symbol); 
      grid.appendChild(border);
      grid.appendChild(use);
      x += IMAGE_OFFSET;
    }
  }

  if (aBonusPARVal) {
    
    y += IMAGE_OFFSET;
    x = 0;
    var border = generateBorderRect(x, y, aWidth, aHeight);
    var symbolID = "symbol_Bonus";
    var symbol = generateSymbolElementForParams(symbolID, aHref,
                                                aBonusPARVal, "");
    var use = generateUseElementForParams("#" + symbolID,
                                          x, y, aWidth, aHeight);
    grid.appendChild(symbol); 
    grid.appendChild(border);
    grid.appendChild(use);
  }

  return grid;
}
