


function checkDOM(target, data) {
  
  var piNode = document.firstChild;
  if (!piNode || piNode.nodeType != Node.PROCESSING_INSTRUCTION_NODE ||
      piNode.target != target || piNode.data != data) {
    document.documentElement.style.backgroundColor = "red";
  }
}
