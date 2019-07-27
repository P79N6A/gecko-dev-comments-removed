function getBlockAxisName(elem) {
  var wm = getComputedStyle(elem).writingMode;
  return (!wm || wm == 'horizontal-tb') ? 'height' : 'width';
}

function getBSize(elem) {
  return elem.getBoundingClientRect()[getBlockAxisName(elem)] + 'px';
}

function setBSize(elem, bsize) {
  elem.style[getBlockAxisName(elem)] = bsize;
  elem.style.lineHeight = bsize;
}








function makeBSizeMatchInlineBox(block, inline) {
  setBSize(block, getBSize(inline));
}

function makeBSizeOfParentMatch(elems) {
  
  
  for (var elem of elems)
    elem.dataset.bsize = getBSize(elem);
  for (var elem of elems)
    setBSize(elem.parentNode, elem.dataset.bsize);
}
