











"use strict";

var HTML5_ELEMENT_NAMES = [
    'a', 'abbr', 'address', 'area', 'article', 'aside', 'audio',
    'b', 'base', 'bdi', 'bdo', 'blockquote', 'body', 'br', 'button',
    'canvas', 'caption', 'cite', 'code', 'col', 'colgroup', 'command',
    'datalist', 'dd', 'del', 'details', 'dfn', 'dialog', 'div', 'dl', 'dt',
    'em', 'embed',
    'fieldset', 'figcaption', 'figure', 'footer', 'form',
    'h1', 'h2', 'h3', 'h4', 'h5', 'h6', 'head', 'header', 'hgroup', 'hr',
    'html',
    'i', 'iframe', 'img', 'input', 'ins', 'kbd', 'keygen',
    'label', 'legend', 'li', 'link',
    'map', 'mark', 'menu', 'meta', 'meter',
    'nav', 'noscript',
    'object', 'ol', 'optgroup', 'option', 'output',
    'p', 'param', 'pre', 'progress',
    'q',
    'rp', 'rt', 'ruby',
    's', 'samp', 'script', 'section', 'select', 'small', 'source', 'span',
    'strong', 'style', 'sub',
    'table', 'tbody', 'td', 'textarea', 'tfoot', 'th', 'thead', 'time',
    'title', 'tr', 'track',
    'u', 'ul',
    'var', 'video',
    'wbr'
];


var HTML5_FORM_ASSOCIATED_ELEMENTS = ['button', 'fieldset', 'input', 'keygen', 'label',
                                      'object', 'output', 'select', 'textarea'];


var USE_VENDOR_SPECIFIC_WORKAROUND = true;

function activateVendorSpecificWorkaround() {
    if (Element.prototype.webkitCreateShadowRoot &&
        !Element.prototype.createShadowRoot) {
        Element.prototype.createShadowRoot =
            Element.prototype.webkitCreateShadowRoot;

        Object.defineProperty(Element.prototype, 'pseudo', {
            get: function () { return this.webkitPseudo; },
            set: function (value) { return this.webkitPseudo = value; }
        });

        Object.defineProperty(Element.prototype, 'shadowRoot', {
            get: function () { return this.webkitShadowRoot; }
        });
    }
}

if (USE_VENDOR_SPECIFIC_WORKAROUND)
    activateVendorSpecificWorkaround();









function ShadowDomNotSupportedError() {
    this.message = "Shadow DOM is not supported";
}



function addPrefixed(element) {
	if (element && !element.pseudo) {
		Object.defineProperty(element, 'pseudo', {
			  get: function () { return element.webkitPseudo; },
			  set: function (value) { return element.webkitPseudo = value; }
		});
	}
}

function addDocumentPrefixed(d) {
	if (d) {
		if (d.body) {
		    addPrefixed(d.body);
		}
		if (d.head) {
		    addPrefixed(d.head);			
		}
		if (d.documentElement) {
			addPrefixed(d.documentElement);
		}
		d.oldCreate = d.createElement;
		d.createElement = function(tagName) {
			var el = d.oldCreate(tagName);
			addPrefixed(el);
			return el;
		};		
	}	
}


function rethrowInternalErrors(e) {
    if (e instanceof ShadowDomNotSupportedError) {
        throw e;
    }

}

function newDocument() {
    var d = document.implementation.createDocument(
        'http://www.w3.org/1999/xhtml', 'html');
    
    addDocumentPrefixed(d);
    return d;        
}

function newHTMLDocument() {
	var d = document.implementation.createHTMLDocument('Test Document');
    
    addDocumentPrefixed(d);
    return d;
}

function newIFrame(ctx, src) {
    if (typeof(ctx) == 'undefined' || typeof (ctx.iframes) != 'object') {
        assert_unreached('Illegal context object in newIFrame');
    }

    var iframe = document.createElement('iframe');
    if (!ctx.debug) {
        iframe.style.display = 'none';
    }
    if (typeof(src) != 'undefined') {
        iframe.src = src;
    }
    document.body.appendChild(iframe);
    ctx.iframes.push(iframe);

    assert_true(typeof(iframe.contentWindow) != 'undefined'
        && typeof(iframe.contentWindow.document) != 'undefined'
        && iframe.contentWindow.document != document, 'Failed to create new rendered document'
    );
    return iframe;
}
function newRenderedHTMLDocument(ctx) {
    var frame = newIFrame(ctx);
    var d = frame.contentWindow.document;
    
    addDocumentPrefixed(d);
    return d;    
}




function newContext() {
    return {iframes:[]};
}

function cleanContext(ctx) {
    if (!ctx.debug) {
        ctx.iframes.forEach(function (e) {
            e.parentNode.removeChild(e);
        });
    }
}

function unit(f) {
    return function () {
        var ctx = newContext();
        try {
            f(ctx);
        } catch(e) {
            console.log(e.getMessage());
        } finally {
            cleanContext(ctx);
        }
    }
}

function step_unit(f, ctx, t) {
    return function () {
        var done = false;
        try {
            f();
            done = true;
        } finally {
            if (done) {
                t.done();
            }
            cleanContext(ctx);
        }
    }
}

function assert_nodelist_contents_equal_noorder(actual, expected, message) {
    assert_equals(actual.length, expected.length, message);
    var used = [];
    for (var i = 0; i < expected.length; i++) {
        used.push(false);
    }
    for (i = 0; i < expected.length; i++) {
        var found = false;
        for (var j = 0; j < actual.length; j++) {
            if (used[j] == false && expected[i] == actual[j]) {
                used[j] = true;
                found = true;
                break;
            }
        }
        if (!found) {
            assert_unreached(message + ". Fail reason:  element not found: " + expected[i]);
        }
    }
}



function createTestMediaPlayer(d) {
    d.body.innerHTML = '' +
	'<div id="player">' +
		'<input type="checkbox" id="outside-control">' +
		'<div id="player-shadow-root">' +
	    '</div>' +
	'</div>';

	var playerShadowRoot = d.querySelector('#player-shadow-root').createShadowRoot();
	playerShadowRoot.innerHTML = '' +
		'<div id="controls">' +
			'<button class="play-button">PLAY</button>' +
			'<input type="range" id="timeline">' +
				'<div id="timeline-shadow-root">' +
				'</div>' +
			'</input>' +
		    '<div class="volume-slider-container" id="volume-slider-container">' +
		        '<input type="range" class="volume-slider" id="volume-slider">' +
		            '<div id="volume-shadow-root">' +
		            '</div>' +
		        '</input>' +
		    '</div>' +
		'</div>';

	var timeLineShadowRoot = playerShadowRoot.querySelector('#timeline-shadow-root').createShadowRoot();
	timeLineShadowRoot.innerHTML =  '<div class="slider-thumb" id="timeline-slider-thumb"></div>';

	var volumeShadowRoot = playerShadowRoot.querySelector('#volume-shadow-root').createShadowRoot();
	volumeShadowRoot.innerHTML = '<div class="slider-thumb" id="volume-slider-thumb"></div>';

	return {
		'playerShadowRoot': playerShadowRoot,
		'timeLineShadowRoot': timeLineShadowRoot,
		'volumeShadowRoot': volumeShadowRoot
		};
}




function fireKeyboardEvent(doc, element, key) {
    var event = doc.createEvent('KeyboardEvent');
    event.initKeyboardEvent("keydown", true, true, doc.defaultView, key, 0, false, false, false, false);
    element.dispatchEvent(event);
}
