


"use strict";





loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/performance/global", true);







function collapseMarkersIntoNode({ markerNode, markersList }) {
  let [getOrCreateParentNode, getCurrentParentNode, clearParentNode] = makeParentNodeFactory();

  for (let i = 0, len = markersList.length; i < len; i++) {
    let curr = markersList[i];
    let blueprint = TIMELINE_BLUEPRINT[curr.name];

    let parentNode = getCurrentParentNode();
    let collapse = blueprint.collapseFunc || (() => null);
    let peek = distance => markersList[i + distance];
    let collapseInfo = collapse(parentNode, curr, peek);

    if (collapseInfo) {
      let { toParent, withData, forceNew, forceEnd } = collapseInfo;

      
      
      if (forceNew) {
        clearParentNode();
      }
      
      
      if (toParent) {
        let parentNode = getOrCreateParentNode(markerNode, toParent, curr.start);
        parentNode.end = curr.end;
        parentNode.submarkers.push(curr);
        for (let key in withData) {
          parentNode[key] = withData[key];
        }
      }
      
      
      if (forceEnd) {
        clearParentNode();
      }
    } else {
      clearParentNode();
      markerNode.submarkers.push(curr);
    }
  }
}









function makeEmptyMarkerNode(name, start, end) {
  return {
    name: name,
    start: start,
    end: end,
    submarkers: []
  };
}





function makeParentNodeFactory() {
  let marker;

  return [
    







    function getOrCreateParentNode(owner, name, start) {
      if (marker && marker.name == name) {
        return marker;
      } else {
        marker = makeEmptyMarkerNode(name, start);
        owner.submarkers.push(marker);
        return marker;
      }
    },

    



    function getCurrentParentNode() {
      return marker;
    },

    


    function clearParentNode() {
      marker = null;
    }
  ];
}

exports.makeEmptyMarkerNode = makeEmptyMarkerNode;
exports.collapseMarkersIntoNode = collapseMarkersIntoNode;
