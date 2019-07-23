




function checkDOM(target, data) {
  
  var piNode = document.documentElement.childNodes[1];
  if (!piNode || piNode.nodeType != Node.PROCESSING_INSTRUCTION_NODE ||
      piNode.target != target || piNode.data != data) {
    document.documentElement.style.backgroundColor = "red";
  }
}
