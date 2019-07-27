









"use strict";

var ANIMATION_END_TIME = 1000;
var ANIMATION_TOP_DEFAULT = 300;
var ANIMATION_TOP_0 = 10;
var ANIMATION_TOP_0_5 = 100;
var ANIMATION_TOP_1 = 200;

var KEYFRAMES = [ {
    top : ANIMATION_TOP_0 + 'px',
    offset : 0
}, {
    top : ANIMATION_TOP_0_5 + 'px',
    offset : 1 / 2
}, {
    top : ANIMATION_TOP_1 + 'px',
    offset : 1
} ];


function newAnimation(animationTarget) {
    animationTarget.style.top = ANIMATION_TOP_DEFAULT + 'px';
    return new Animation(animationTarget, KEYFRAMES, ANIMATION_END_TIME);
}



function createDiv(test, doc) {
    if (!doc) {
        doc = document;
    }
    var div = doc.createElement('div');
    doc.body.appendChild(div);
    test.add_cleanup(function() {
        removeElement(div);
    });
    return div;
}


function removeElement(element) {
    element.parentNode.removeChild(element);
}


function type(object) {
    return Object.prototype.toString.call(object).slice(8, -1);
}
