












function addDiv(t, attrs) {
  var div = document.createElement('div');
  if (attrs) {
    for (var attrName in attrs) {
      div.setAttribute(attrName, attrs[attrName]);
    }
  }
  document.body.appendChild(div);
  if (t && typeof t.add_cleanup === 'function') {
    t.add_cleanup(function() {
      if (div.parentNode) {
        div.parentNode.removeChild(div);
      }
    });
  }
  return div;
}




function waitForFrame() {
  return new Promise(function(resolve, reject) {
    window.requestAnimationFrame(resolve);
  });
}





function waitForAnimationFrames(frameCount) {
  return new Promise(function(resolve, reject) {
    function handleFrame() {
      if (--frameCount <= 0) {
        resolve();
      } else {
        window.requestAnimationFrame(handleFrame); 
      }
    }
    window.requestAnimationFrame(handleFrame);
  });
}






function waitForAllAnimations(animations) {
  return Promise.all(animations.map(function(animation) {
    return animation.ready;
  }));
}







function flushComputedStyle(elem) {
  var cs = window.getComputedStyle(elem);
  cs.marginLeft;
}

