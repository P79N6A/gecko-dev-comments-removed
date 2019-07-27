


"use strict";





loader.lazyRequireGetter(this, "TIMELINE_BLUEPRINT",
  "devtools/performance/markers", true);







function collapseMarkersIntoNode({ markerNode, markersList }) {
  let [getOrCreateParentNode, getCurrentParentNode, clearParentNode] = makeParentNodeFactory();
  let uid = 0;

  for (let i = 0, len = markersList.length; i < len; i++) {
    let curr = markersList[i];

    
    
    curr.uid = ++uid;

    let parentNode = getCurrentParentNode();
    let blueprint = TIMELINE_BLUEPRINT[curr.name];
    let collapse = blueprint.collapseFunc || (() => null);
    let peek = distance => markersList[i + distance];
    let collapseInfo = collapse(parentNode, curr, peek);

    if (collapseInfo) {
      let { toParent, withData, forceNew, forceEnd } = collapseInfo;

      
      
      if (forceNew) {
        clearParentNode();
      }
      
      
      if (toParent) {
        let parentNode = getOrCreateParentNode({
          uid: ++uid,
          owner: markerNode,
          name: toParent,
          start: curr.start,
          end: curr.end
        });

        
        
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










function makeEmptyMarkerNode(name, uid, start, end) {
  return {
    name: name,
    uid: uid,
    start: start,
    end: end,
    submarkers: []
  };
}





function makeParentNodeFactory() {
  let marker;

  return [
    








    function getOrCreateParentNode({ owner, name, uid, start, end }) {
      if (marker && marker.name == name) {
        marker.end = end;
        return marker;
      } else {
        marker = makeEmptyMarkerNode(name, uid, start, end);
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
