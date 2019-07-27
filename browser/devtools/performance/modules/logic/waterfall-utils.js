


"use strict";





loader.lazyRequireGetter(this, "getBlueprintFor",
  "devtools/performance/markers", true);







function collapseMarkersIntoNode({ markerNode, markersList, blueprint }) {
  let { getCurrentParentNode, collapseMarker, addParentNode, popParentNode } = createParentNodeFactory(markerNode);

  for (let i = 0, len = markersList.length; i < len; i++) {
    let curr = markersList[i];

    let parentNode = getCurrentParentNode();
    let definition = getBlueprintFor(curr, blueprint);

    let collapse = definition.collapseFunc || (() => null);
    let peek = distance => markersList[i + distance];
    let foundParent = false;

    let collapseInfo = collapse(parentNode, curr, peek);
    if (collapseInfo) {
      let { collapse, toParent, finalize } = collapseInfo;

      
      if (typeof toParent === "object") {
        addParentNode(toParent);
      }

      if (collapse) {
        collapseMarker(curr);
      }

      
      
      if (finalize) {
        popParentNode();
      }
    } else {
      markerNode.submarkers.push(curr);
    }
  }
}









function makeParentMarkerNode (marker) {
  let node = Object.create(null);
  for (let prop in marker) {
    node[prop] = marker[prop];
  }
  node.submarkers = [];
  return node;
}








function createParentNodeFactory (root) {
  let parentMarkers = [];
  let factory = {
    



    popParentNode: () => {
      if (parentMarkers.length === 0) {
        throw new Error("Cannot pop parent markers when none exist.");
      }

      let lastParent = parentMarkers.pop();
      
      
      if (lastParent.end == void 0) {
        lastParent.end = lastParent.submarkers[lastParent.submarkers.length - 1].end;
      }
      return lastParent;
    },

    


    getCurrentParentNode: () => parentMarkers.length ? parentMarkers[parentMarkers.length - 1] : null,

    



    addParentNode: (marker) => {
      let parentMarker = makeParentMarkerNode(marker);
      (factory.getCurrentParentNode() || root).submarkers.push(parentMarker);
      parentMarkers.push(parentMarker);
    },

    


    collapseMarker: (marker) => {
      if (parentMarkers.length === 0) {
        throw new Error("Cannot collapse marker with no parents.");
      }
      factory.getCurrentParentNode().submarkers.push(marker);
    }
  };

  return factory;
}

exports.makeParentMarkerNode = makeParentMarkerNode;
exports.collapseMarkersIntoNode = collapseMarkersIntoNode;
