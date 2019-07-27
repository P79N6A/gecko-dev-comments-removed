


"use strict";





loader.lazyRequireGetter(this, "extend",
  "sdk/util/object", true);
loader.lazyRequireGetter(this, "MarkerUtils",
  "devtools/performance/marker-utils");









function createParentNode (marker) {
  return extend(marker, { submarkers: [] });
}








function collapseMarkersIntoNode({ rootNode, markersList, filter }) {
  let { getCurrentParentNode, pushNode, popParentNode } = createParentNodeFactory(rootNode);

  for (let i = 0, len = markersList.length; i < len; i++) {
    let curr = markersList[i];

    
    if (!MarkerUtils.isMarkerValid(curr, filter)) {
      continue;
    }

    let parentNode = getCurrentParentNode();
    let blueprint = MarkerUtils.getBlueprintFor(curr);

    let nestable = "nestable" in blueprint ? blueprint.nestable : true;
    let collapsible = "collapsible" in blueprint ? blueprint.collapsible : true;

    let finalized = null;

    
    
    
    if (collapsible) {
      curr = createParentNode(curr);
    }

    
    
    if (!nestable) {
      pushNode(rootNode, curr);
      continue;
    }

    
    
    while (!finalized && parentNode) {
      
      
      
      if (nestable && curr.end <= parentNode.end) {
        pushNode(parentNode, curr);
        finalized = true;
        break;
      }

      
      
      
      if (nestable) {
        popParentNode();
        parentNode = getCurrentParentNode();
        continue;
      }
    }

    if (!finalized) {
      pushNode(rootNode, curr);
    }
  }
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

      
      
      
      if (!lastParent.submarkers.length) {
        delete lastParent.submarkers;
      }

      return lastParent;
    },

    


    getCurrentParentNode: () => parentMarkers.length ? parentMarkers[parentMarkers.length - 1] : null,

    


    pushNode: (parent, marker) => {
      parent.submarkers.push(marker);

      
      
      if (marker.submarkers) {
        parentMarkers.push(marker);
      }
    }
  };

  return factory;
}

exports.createParentNode = createParentNode;
exports.collapseMarkersIntoNode = collapseMarkersIntoNode;
