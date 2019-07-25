





































const Cu = Components.utils;
Cu.import("resource:///modules/devtools/LayoutHelpers.jsm");

function isHighlighting()
{
  let veil = InspectorUI.highlighter.veilTransparentBox;
  return !(veil.style.visibility == "hidden");
}

function getHighlitNode()
{
  let h = InspectorUI.highlighter;
  if (!isHighlighting() || !h._contentRect)
    return null;

  let a = {
    x: h._contentRect.left,
    y: h._contentRect.top
  };

  let b = {
    x: a.x + h._contentRect.width,
    y: a.y + h._contentRect.height
  };

  
  let midpoint = midPoint(a, b);

  return LayoutHelpers.getElementFromPoint(h.win.document, midpoint.x,
    midpoint.y);
}


function midPoint(aPointA, aPointB)
{
  let pointC = { };
  pointC.x = (aPointB.x - aPointA.x) / 2 + aPointA.x;
  pointC.y = (aPointB.y - aPointA.y) / 2 + aPointA.y;
  return pointC;
}
