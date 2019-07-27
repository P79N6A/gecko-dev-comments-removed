









function convertEntries(entries) {
  var result = {};
  for (var i = 0; i < entries.length; ++i) {
    result[entries[i].key] = entries[i].value;
  }
  return result;
}

function convertScrollFrameData(scrollFrames) {
  var result = {};
  for (var i = 0; i < scrollFrames.length; ++i) {
    result[scrollFrames[i].scrollId] = convertEntries(scrollFrames[i].entries);
  }
  return result;
}

function convertBuckets(buckets) {
  var result = {};
  for (var i = 0; i < buckets.length; ++i) {
    result[buckets[i].sequenceNumber] = convertScrollFrameData(buckets[i].scrollFrames);
  }
  return result;
}

function convertTestData(testData) {
  var result = {};
  result.paints = convertBuckets(testData.paints);
  result.repaintRequests = convertBuckets(testData.repaintRequests);
  return result;
}







function makeNode(id) {
  return {scrollId: id, children: []};
}


function findNode(root, id) {
  if (root.scrollId == id) {
    return root;
  }
  for (var i = 0; i < root.children.length; ++i) {
    var subtreeResult = findNode(root.children[i], id);
    if (subtreeResult != null) {
      return subtreeResult;
    }
  }
  return null;
}


function addLink(root, child, parent) {
  var parentNode = findNode(root, parent);
  if (parentNode == null) {
    parentNode = makeNode(parent);
    root.children.push(parentNode);
  }
  parentNode.children.push(makeNode(child));
}



function addRoot(root, id) {
  root.children.push(makeNode(id));
}




function buildApzcTree(paint) {
  
  
  
  var root = makeNode(-1);
  for (var scrollId in paint) {
    if ("parentScrollId" in paint[scrollId]) {
      addLink(root, scrollId, paint[scrollId]["parentScrollId"]);
    } else {
      addRoot(root, scrollId);
    }
  }
  return root;
}
