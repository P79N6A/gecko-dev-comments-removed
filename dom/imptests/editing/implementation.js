"use strict";

var htmlNamespace = "http://www.w3.org/1999/xhtml";

var cssStylingFlag = false;

var defaultSingleLineContainerName = "p";


var globalRange = null;


var commands = {};






function nextNode(node) {
	if (node.hasChildNodes()) {
		return node.firstChild;
	}
	return nextNodeDescendants(node);
}

function previousNode(node) {
	if (node.previousSibling) {
		node = node.previousSibling;
		while (node.hasChildNodes()) {
			node = node.lastChild;
		}
		return node;
	}
	if (node.parentNode
	&& node.parentNode.nodeType == Node.ELEMENT_NODE) {
		return node.parentNode;
	}
	return null;
}

function nextNodeDescendants(node) {
	while (node && !node.nextSibling) {
		node = node.parentNode;
	}
	if (!node) {
		return null;
	}
	return node.nextSibling;
}




function isAncestor(ancestor, descendant) {
	return ancestor
		&& descendant
		&& Boolean(ancestor.compareDocumentPosition(descendant) & Node.DOCUMENT_POSITION_CONTAINED_BY);
}





function isAncestorContainer(ancestor, descendant) {
	return (ancestor || descendant)
		&& (ancestor == descendant || isAncestor(ancestor, descendant));
}




function isDescendant(descendant, ancestor) {
	return ancestor
		&& descendant
		&& Boolean(ancestor.compareDocumentPosition(descendant) & Node.DOCUMENT_POSITION_CONTAINED_BY);
}




function isBefore(node1, node2) {
	return Boolean(node1.compareDocumentPosition(node2) & Node.DOCUMENT_POSITION_FOLLOWING);
}




function isAfter(node1, node2) {
	return Boolean(node1.compareDocumentPosition(node2) & Node.DOCUMENT_POSITION_PRECEDING);
}

function getAncestors(node) {
	var ancestors = [];
	while (node.parentNode) {
		ancestors.unshift(node.parentNode);
		node = node.parentNode;
	}
	return ancestors;
}

function getInclusiveAncestors(node) {
	return getAncestors(node).concat(node);
}

function getDescendants(node) {
	var descendants = [];
	var stop = nextNodeDescendants(node);
	while ((node = nextNode(node))
	&& node != stop) {
		descendants.push(node);
	}
	return descendants;
}

function getInclusiveDescendants(node) {
	return [node].concat(getDescendants(node));
}

function convertProperty(property) {
	
	var map = {
		"fontFamily": "font-family",
		"fontSize": "font-size",
		"fontStyle": "font-style",
		"fontWeight": "font-weight",
		"textDecoration": "text-decoration",
	};
	if (typeof map[property] != "undefined") {
		return map[property];
	}

	return property;
}



function cssSizeToLegacy(cssVal) {
	return {
		"x-small": 1,
		"small": 2,
		"medium": 3,
		"large": 4,
		"x-large": 5,
		"xx-large": 6,
		"xxx-large": 7
	}[cssVal];
}


function legacySizeToCss(legacyVal) {
	return {
		1: "x-small",
		2: "small",
		3: "medium",
		4: "large",
		5: "x-large",
		6: "xx-large",
		7: "xxx-large",
	}[legacyVal];
}


function isHtmlNamespace(ns) {
	return ns === null
		|| ns === htmlNamespace;
}







function getDirectionality(element) {
	
	
	if (element.dir == "ltr") {
		return "ltr";
	}

	
	
	if (element.dir == "rtl") {
		return "rtl";
	}

	
	
	
	
	
	

	
	
	
	if (!isHtmlElement(element.parentNode)) {
		return "ltr";
	}

	
	
	
	
	return getDirectionality(element.parentNode);
}








function getNodeIndex(node) {
	var ret = 0;
	while (node.previousSibling) {
		ret++;
		node = node.previousSibling;
	}
	return ret;
}











function getNodeLength(node) {
	switch (node.nodeType) {
		case Node.PROCESSING_INSTRUCTION_NODE:
		case Node.DOCUMENT_TYPE_NODE:
			return 0;

		case Node.TEXT_NODE:
		case Node.COMMENT_NODE:
			return node.length;

		default:
			return node.childNodes.length;
	}
}





function getPosition(nodeA, offsetA, nodeB, offsetB) {
	
	
	
	if (nodeA == nodeB) {
		if (offsetA == offsetB) {
			return "equal";
		}
		if (offsetA < offsetB) {
			return "before";
		}
		if (offsetA > offsetB) {
			return "after";
		}
	}

	
	
	
	if (nodeB.compareDocumentPosition(nodeA) & Node.DOCUMENT_POSITION_FOLLOWING) {
		var pos = getPosition(nodeB, offsetB, nodeA, offsetA);
		if (pos == "before") {
			return "after";
		}
		if (pos == "after") {
			return "before";
		}
	}

	
	if (nodeB.compareDocumentPosition(nodeA) & Node.DOCUMENT_POSITION_CONTAINS) {
		
		var child = nodeB;

		
		while (child.parentNode != nodeA) {
			child = child.parentNode;
		}

		
		if (getNodeIndex(child) < offsetA) {
			return "after";
		}
	}

	
	return "before";
}




function getFurthestAncestor(node) {
	var root = node;
	while (root.parentNode != null) {
		root = root.parentNode;
	}
	return root;
}






function isContained(node, range) {
	var pos1 = getPosition(node, 0, range.startContainer, range.startOffset);
	var pos2 = getPosition(node, getNodeLength(node), range.endContainer, range.endOffset);

	return getFurthestAncestor(node) == getFurthestAncestor(range.startContainer)
		&& pos1 == "after"
		&& pos2 == "before";
}





function getContainedNodes(range, condition) {
	if (typeof condition == "undefined") {
		condition = function() { return true };
	}
	var node = range.startContainer;
	if (node.hasChildNodes()
	&& range.startOffset < node.childNodes.length) {
		
		node = node.childNodes[range.startOffset];
	} else if (range.startOffset == getNodeLength(node)) {
		
		node = nextNodeDescendants(node);
	} else {
		
		node = nextNode(node);
	}

	var stop = range.endContainer;
	if (stop.hasChildNodes()
	&& range.endOffset < stop.childNodes.length) {
		
		stop = stop.childNodes[range.endOffset];
	} else {
		
		stop = nextNodeDescendants(stop);
	}

	var nodeList = [];
	while (isBefore(node, stop)) {
		if (isContained(node, range)
		&& condition(node)) {
			nodeList.push(node);
			node = nextNodeDescendants(node);
			continue;
		}
		node = nextNode(node);
	}
	return nodeList;
}




function getAllContainedNodes(range, condition) {
	if (typeof condition == "undefined") {
		condition = function() { return true };
	}
	var node = range.startContainer;
	if (node.hasChildNodes()
	&& range.startOffset < node.childNodes.length) {
		
		node = node.childNodes[range.startOffset];
	} else if (range.startOffset == getNodeLength(node)) {
		
		node = nextNodeDescendants(node);
	} else {
		
		node = nextNode(node);
	}

	var stop = range.endContainer;
	if (stop.hasChildNodes()
	&& range.endOffset < stop.childNodes.length) {
		
		stop = stop.childNodes[range.endOffset];
	} else {
		
		stop = nextNodeDescendants(stop);
	}

	var nodeList = [];
	while (isBefore(node, stop)) {
		if (isContained(node, range)
		&& condition(node)) {
			nodeList.push(node);
		}
		node = nextNode(node);
	}
	return nodeList;
}



function normalizeColor(color) {
	if (color.toLowerCase() == "currentcolor") {
		return null;
	}

	if (normalizeColor.resultCache === undefined) {
		normalizeColor.resultCache = {};
	}

	if (normalizeColor.resultCache[color] !== undefined) {
		return normalizeColor.resultCache[color];
	}

	var originalColor = color;

	var outerSpan = document.createElement("span");
	document.body.appendChild(outerSpan);
	outerSpan.style.color = "black";

	var innerSpan = document.createElement("span");
	outerSpan.appendChild(innerSpan);
	innerSpan.style.color = color;
	color = getComputedStyle(innerSpan).color;

	if (color == "rgb(0, 0, 0)") {
		
		outerSpan.color = "white";
		color = getComputedStyle(innerSpan).color;
		if (color != "rgb(0, 0, 0)") {
			return normalizeColor.resultCache[originalColor] = null;
		}
	}

	document.body.removeChild(outerSpan);

	
	
	
	if (/^rgba\([0-9]+, [0-9]+, [0-9]+, 1\)$/.test(color)) {
		
		return normalizeColor.resultCache[originalColor] =
			color.replace("rgba", "rgb").replace(", 1)", ")");
	}
	if (color == "transparent") {
		
		
		return normalizeColor.resultCache[originalColor] =
			"rgba(0, 0, 0, 0)";
	}
	
	
	color = color.replace(/, 0.496094\)$/, ", 0.5)");
	return normalizeColor.resultCache[originalColor] = color;
}


function parseSimpleColor(color) {
	color = normalizeColor(color);
	var matches = /^rgb\(([0-9]+), ([0-9]+), ([0-9]+)\)$/.exec(color);
	if (matches) {
		return "#"
			+ parseInt(matches[1]).toString(16).replace(/^.$/, "0$&")
			+ parseInt(matches[2]).toString(16).replace(/^.$/, "0$&")
			+ parseInt(matches[3]).toString(16).replace(/^.$/, "0$&");
	}
	return null;
}












var executionStackDepth = 0;


function editCommandMethod(command, range, callback) {
	
	if (executionStackDepth == 0 && typeof range != "undefined") {
		globalRange = range;
	} else if (executionStackDepth == 0) {
		globalRange = null;
		globalRange = getActiveRange();
	}

	executionStackDepth++;
	try {
		var ret = callback();
	} catch(e) {
		executionStackDepth--;
		throw e;
	}
	executionStackDepth--;
	return ret;
}

function myExecCommand(command, showUi, value, range) {
	
	
	command = command.toLowerCase();

	
	
	
	
	if (arguments.length == 1
	|| (arguments.length >=4 && typeof showUi == "undefined")) {
		showUi = false;
	}

	
	
	if (arguments.length <= 2
	|| (arguments.length >=4 && typeof value == "undefined")) {
		value = "";
	}

	return editCommandMethod(command, range, (function(command, showUi, value) { return function() {
		
		if (!(command in commands) || !myQueryCommandEnabled(command)) {
			return false;
		}

		
		
		var ret = commands[command].action(value);

		
		if (ret !== true && ret !== false) {
			throw "execCommand() didn't return true or false: " + ret;
		}

		
		if (ret === false) {
			return false;
		}

		
		return true;
	}})(command, showUi, value));
}

function myQueryCommandEnabled(command, range) {
	
	
	command = command.toLowerCase();

	return editCommandMethod(command, range, (function(command) { return function() {
		
		
		if (!(command in commands)) {
			return false;
		}

		
		
		
		
		
		
		
		return ["copy", "defaultparagraphseparator", "selectall", "stylewithcss",
		"usecss"].indexOf(command) != -1
			|| (
				getActiveRange() !== null
				&& (isEditable(getActiveRange().startContainer) || isEditingHost(getActiveRange().startContainer))
				&& (isEditable(getActiveRange().endContainer) || isEditingHost(getActiveRange().endContainer))
				&& (getInclusiveAncestors(getActiveRange().commonAncestorContainer).some(isEditingHost))
			);
	}})(command));
}

function myQueryCommandIndeterm(command, range) {
	
	
	command = command.toLowerCase();

	return editCommandMethod(command, range, (function(command) { return function() {
		
		if (!(command in commands) || !("indeterm" in commands[command])) {
			return false;
		}

		
		return commands[command].indeterm();
	}})(command));
}

function myQueryCommandState(command, range) {
	
	
	command = command.toLowerCase();

	return editCommandMethod(command, range, (function(command) { return function() {
		
		if (!(command in commands) || !("state" in commands[command])) {
			return false;
		}

		
		if (typeof getStateOverride(command) != "undefined") {
			return getStateOverride(command);
		}

		
		return commands[command].state();
	}})(command));
}




function myQueryCommandSupported(command) {
	
	
	command = command.toLowerCase();

	return command in commands;
}

function myQueryCommandValue(command, range) {
	
	
	command = command.toLowerCase();

	return editCommandMethod(command, range, function() {
		
		if (!(command in commands) || !("value" in commands[command])) {
			return "";
		}

		
		
		
		if (command == "fontsize"
		&& getValueOverride("fontsize") !== undefined) {
			return getLegacyFontSize(getValueOverride("fontsize"));
		}

		
		if (typeof getValueOverride(command) != "undefined") {
			return getValueOverride(command);
		}

		
		return commands[command].value();
	});
}












function isHtmlElement(node, tags) {
	if (typeof tags == "string") {
		tags = [tags];
	}
	if (typeof tags == "object") {
		tags = tags.map(function(tag) { return tag.toUpperCase() });
	}
	return node
		&& node.nodeType == Node.ELEMENT_NODE
		&& isHtmlNamespace(node.namespaceURI)
		&& (typeof tags == "undefined" || tags.indexOf(node.tagName) != -1);
}








var prohibitedParagraphChildNames = ["address", "article", "aside",
	"blockquote", "caption", "center", "col", "colgroup", "dd", "details",
	"dir", "div", "dl", "dt", "fieldset", "figcaption", "figure", "footer",
	"form", "h1", "h2", "h3", "h4", "h5", "h6", "header", "hgroup", "hr", "li",
	"listing", "menu", "nav", "ol", "p", "plaintext", "pre", "section",
	"summary", "table", "tbody", "td", "tfoot", "th", "thead", "tr", "ul",
	"xmp"];



function isProhibitedParagraphChild(node) {
	return isHtmlElement(node, prohibitedParagraphChildNames);
}




function isBlockNode(node) {
	return node
		&& ((node.nodeType == Node.ELEMENT_NODE && ["inline", "inline-block", "inline-table", "none"].indexOf(getComputedStyle(node).display) == -1)
		|| node.nodeType == Node.DOCUMENT_NODE
		|| node.nodeType == Node.DOCUMENT_FRAGMENT_NODE);
}


function isInlineNode(node) {
	return node && !isBlockNode(node);
}




function isEditingHost(node) {
	return node
		&& isHtmlElement(node)
		&& (node.contentEditable == "true"
		|| (node.parentNode
		&& node.parentNode.nodeType == Node.DOCUMENT_NODE
		&& node.parentNode.designMode == "on"));
}






function isEditable(node) {
	return node
		&& !isEditingHost(node)
		&& (node.nodeType != Node.ELEMENT_NODE || node.contentEditable != "false")
		&& (isEditingHost(node.parentNode) || isEditable(node.parentNode))
		&& (isHtmlElement(node)
		|| (node.nodeType == Node.ELEMENT_NODE && node.namespaceURI == "http://www.w3.org/2000/svg" && node.localName == "svg")
		|| (node.nodeType == Node.ELEMENT_NODE && node.namespaceURI == "http://www.w3.org/1998/Math/MathML" && node.localName == "math")
		|| (node.nodeType != Node.ELEMENT_NODE && isHtmlElement(node.parentNode)));
}


function hasEditableDescendants(node) {
	for (var i = 0; i < node.childNodes.length; i++) {
		if (isEditable(node.childNodes[i])
		|| hasEditableDescendants(node.childNodes[i])) {
			return true;
		}
	}
	return false;
}




function getEditingHostOf(node) {
	if (isEditingHost(node)) {
		return node;
	} else if (isEditable(node)) {
		var ancestor = node.parentNode;
		while (!isEditingHost(ancestor)) {
			ancestor = ancestor.parentNode;
		}
		return ancestor;
	} else {
		return null;
	}
}



function inSameEditingHost(node1, node2) {
	return getEditingHostOf(node1)
		&& getEditingHostOf(node1) == getEditingHostOf(node2);
}



function isCollapsedLineBreak(br) {
	if (!isHtmlElement(br, "br")) {
		return false;
	}

	
	
	
	var ref = br.parentNode;
	while (getComputedStyle(ref).display == "inline") {
		ref = ref.parentNode;
	}
	var refStyle = ref.hasAttribute("style") ? ref.getAttribute("style") : null;
	ref.style.height = "auto";
	ref.style.maxHeight = "none";
	ref.style.minHeight = "0";
	var space = document.createTextNode("\u200b");
	var origHeight = ref.offsetHeight;
	if (origHeight == 0) {
		throw "isCollapsedLineBreak: original height is zero, bug?";
	}
	br.parentNode.insertBefore(space, br.nextSibling);
	var finalHeight = ref.offsetHeight;
	space.parentNode.removeChild(space);
	if (refStyle === null) {
		
		
		ref.setAttribute("style", "");
		ref.removeAttribute("style");
	} else {
		ref.setAttribute("style", refStyle);
	}

	
	
	
	return origHeight < finalHeight - 5;
}







function isExtraneousLineBreak(br) {
	if (!isHtmlElement(br, "br")) {
		return false;
	}

	if (isHtmlElement(br.parentNode, "li")
	&& br.parentNode.childNodes.length == 1) {
		return false;
	}

	
	
	
	
	var ref = br.parentNode;
	while (getComputedStyle(ref).display == "inline") {
		ref = ref.parentNode;
	}
	var refStyle = ref.hasAttribute("style") ? ref.getAttribute("style") : null;
	ref.style.height = "auto";
	ref.style.maxHeight = "none";
	ref.style.minHeight = "0";
	var brStyle = br.hasAttribute("style") ? br.getAttribute("style") : null;
	var origHeight = ref.offsetHeight;
	if (origHeight == 0) {
		throw "isExtraneousLineBreak: original height is zero, bug?";
	}
	br.setAttribute("style", "display:none");
	var finalHeight = ref.offsetHeight;
	if (refStyle === null) {
		
		
		ref.setAttribute("style", "");
		ref.removeAttribute("style");
	} else {
		ref.setAttribute("style", refStyle);
	}
	if (brStyle === null) {
		br.removeAttribute("style");
	} else {
		br.setAttribute("style", brStyle);
	}

	return origHeight == finalHeight;
}








function isWhitespaceNode(node) {
	return node
		&& node.nodeType == Node.TEXT_NODE
		&& (node.data == ""
		|| (
			/^[\t\n\r ]+$/.test(node.data)
			&& node.parentNode
			&& node.parentNode.nodeType == Node.ELEMENT_NODE
			&& ["normal", "nowrap"].indexOf(getComputedStyle(node.parentNode).whiteSpace) != -1
		) || (
			/^[\t\r ]+$/.test(node.data)
			&& node.parentNode
			&& node.parentNode.nodeType == Node.ELEMENT_NODE
			&& getComputedStyle(node.parentNode).whiteSpace == "pre-line"
		));
}



function isCollapsedWhitespaceNode(node) {
	
	if (!isWhitespaceNode(node)) {
		return false;
	}

	
	if (node.data == "") {
		return true;
	}

	
	var ancestor = node.parentNode;

	
	if (!ancestor) {
		return true;
	}

	
	
	if (getAncestors(node).some(function(ancestor) {
		return ancestor.nodeType == Node.ELEMENT_NODE
			&& getComputedStyle(ancestor).display == "none";
	})) {
		return true;
	}

	
	
	while (!isBlockNode(ancestor)
	&& ancestor.parentNode) {
		ancestor = ancestor.parentNode;
	}

	
	var reference = node;

	
	while (reference != ancestor) {
		
		reference = previousNode(reference);

		
		if (isBlockNode(reference)
		|| isHtmlElement(reference, "br")) {
			return true;
		}

		
		
		if ((reference.nodeType == Node.TEXT_NODE && !isWhitespaceNode(reference))
		|| isHtmlElement(reference, "img")) {
			break;
		}
	}

	
	reference = node;

	
	var stop = nextNodeDescendants(ancestor);
	while (reference != stop) {
		
		
		reference = nextNode(reference);

		
		if (isBlockNode(reference)
		|| isHtmlElement(reference, "br")) {
			return true;
		}

		
		
		if ((reference && reference.nodeType == Node.TEXT_NODE && !isWhitespaceNode(reference))
		|| isHtmlElement(reference, "img")) {
			break;
		}
	}

	
	return false;
}






function isVisible(node) {
	if (!node) {
		return false;
	}

	if (getAncestors(node).concat(node)
	.filter(function(node) { return node.nodeType == Node.ELEMENT_NODE })
	.some(function(node) { return getComputedStyle(node).display == "none" })) {
		return false;
	}

	if (isBlockNode(node)
	|| (node.nodeType == Node.TEXT_NODE && !isCollapsedWhitespaceNode(node))
	|| isHtmlElement(node, "img")
	|| (isHtmlElement(node, "br") && !isExtraneousLineBreak(node))) {
		return true;
	}

	for (var i = 0; i < node.childNodes.length; i++) {
		if (isVisible(node.childNodes[i])) {
			return true;
		}
	}

	return false;
}


function isInvisible(node) {
	return node && !isVisible(node);
}





function isCollapsedBlockProp(node) {
	if (isCollapsedLineBreak(node)
	&& !isExtraneousLineBreak(node)) {
		return true;
	}

	if (!isInlineNode(node)
	|| node.nodeType != Node.ELEMENT_NODE) {
		return false;
	}

	var hasCollapsedBlockPropChild = false;
	for (var i = 0; i < node.childNodes.length; i++) {
		if (!isInvisible(node.childNodes[i])
		&& !isCollapsedBlockProp(node.childNodes[i])) {
			return false;
		}
		if (isCollapsedBlockProp(node.childNodes[i])) {
			hasCollapsedBlockPropChild = true;
		}
	}

	return hasCollapsedBlockPropChild;
}








function getActiveRange() {
	var ret;
	if (globalRange) {
		ret = globalRange;
	} else if (getSelection().rangeCount) {
		ret = getSelection().getRangeAt(0);
	} else {
		return null;
	}
	if ([Node.TEXT_NODE, Node.ELEMENT_NODE].indexOf(ret.startContainer.nodeType) == -1
	|| [Node.TEXT_NODE, Node.ELEMENT_NODE].indexOf(ret.endContainer.nodeType) == -1
	|| !ret.startContainer.ownerDocument
	|| !ret.endContainer.ownerDocument
	|| !isDescendant(ret.startContainer, ret.startContainer.ownerDocument)
	|| !isDescendant(ret.endContainer, ret.endContainer.ownerDocument)) {
		throw "Invalid active range; test bug?";
	}
	return ret;
}

















var getStateOverride, setStateOverride, unsetStateOverride,
	getValueOverride, setValueOverride, unsetValueOverride;
(function() {
	var stateOverrides = {};
	var valueOverrides = {};
	var storedRange = null;

	function resetOverrides() {
		if (!storedRange
		|| storedRange.startContainer != getActiveRange().startContainer
		|| storedRange.endContainer != getActiveRange().endContainer
		|| storedRange.startOffset != getActiveRange().startOffset
		|| storedRange.endOffset != getActiveRange().endOffset) {
			stateOverrides = {};
			valueOverrides = {};
			storedRange = getActiveRange().cloneRange();
		}
	}

	getStateOverride = function(command) {
		resetOverrides();
		return stateOverrides[command];
	};

	setStateOverride = function(command, newState) {
		resetOverrides();
		stateOverrides[command] = newState;
	};

	unsetStateOverride = function(command) {
		resetOverrides();
		delete stateOverrides[command];
	}

	getValueOverride = function(command) {
		resetOverrides();
		return valueOverrides[command];
	}

	
	
	
	setValueOverride = function(command, newValue) {
		resetOverrides();
		valueOverrides[command] = newValue;
		if (command == "backcolor") {
			valueOverrides.hilitecolor = newValue;
		} else if (command == "hilitecolor") {
			valueOverrides.backcolor = newValue;
		}
	}

	unsetValueOverride = function(command) {
		resetOverrides();
		delete valueOverrides[command];
		if (command == "backcolor") {
			delete valueOverrides.hilitecolor;
		} else if (command == "hilitecolor") {
			delete valueOverrides.backcolor;
		}
	}
})();











var extraRanges = [];

function movePreservingRanges(node, newParent, newIndex) {
	
	if (newIndex == -1) {
		newIndex = newParent.childNodes.length;
	}

	
	
	
	

	
	
	
	var oldParent = node.parentNode;
	var oldIndex = getNodeIndex(node);

	
	
	
	var ranges = [globalRange].concat(extraRanges);
	for (var i = 0; i < getSelection().rangeCount; i++) {
		ranges.push(getSelection().getRangeAt(i));
	}
	var boundaryPoints = [];
	ranges.forEach(function(range) {
		boundaryPoints.push([range.startContainer, range.startOffset]);
		boundaryPoints.push([range.endContainer, range.endOffset]);
	});

	boundaryPoints.forEach(function(boundaryPoint) {
		
		
		
		

		
		
		if (boundaryPoint[0] == newParent
		&& boundaryPoint[1] > newIndex) {
			boundaryPoint[1]++;
		}

		
		
		
		if (boundaryPoint[0] == oldParent
		&& (boundaryPoint[1] == oldIndex
		|| boundaryPoint[1] == oldIndex + 1)) {
			boundaryPoint[0] = newParent;
			boundaryPoint[1] += newIndex - oldIndex;
		}

		
		
		if (boundaryPoint[0] == oldParent
		&& boundaryPoint[1] > oldIndex + 1) {
			boundaryPoint[1]--;
		}
	});

	
	if (newParent.childNodes.length == newIndex) {
		newParent.appendChild(node);
	} else {
		newParent.insertBefore(node, newParent.childNodes[newIndex]);
	}

	globalRange.setStart(boundaryPoints[0][0], boundaryPoints[0][1]);
	globalRange.setEnd(boundaryPoints[1][0], boundaryPoints[1][1]);

	for (var i = 0; i < extraRanges.length; i++) {
		extraRanges[i].setStart(boundaryPoints[2*i + 2][0], boundaryPoints[2*i + 2][1]);
		extraRanges[i].setEnd(boundaryPoints[2*i + 3][0], boundaryPoints[2*i + 3][1]);
	}

	getSelection().removeAllRanges();
	for (var i = 1 + extraRanges.length; i < ranges.length; i++) {
		var newRange = document.createRange();
		newRange.setStart(boundaryPoints[2*i][0], boundaryPoints[2*i][1]);
		newRange.setEnd(boundaryPoints[2*i + 1][0], boundaryPoints[2*i + 1][1]);
		getSelection().addRange(newRange);
	}
}

function setTagName(element, newName) {
	
	
	if (isHtmlElement(element, newName.toUpperCase())) {
		return element;
	}

	
	if (!element.parentNode) {
		return element;
	}

	
	
	var replacementElement = element.ownerDocument.createElement(newName);

	
	
	element.parentNode.insertBefore(replacementElement, element);

	
	for (var i = 0; i < element.attributes.length; i++) {
		replacementElement.setAttributeNS(element.attributes[i].namespaceURI, element.attributes[i].name, element.attributes[i].value);
	}

	
	
	while (element.childNodes.length) {
		movePreservingRanges(element.firstChild, replacementElement, replacementElement.childNodes.length);
	}

	
	element.parentNode.removeChild(element);

	
	return replacementElement;
}

function removeExtraneousLineBreaksBefore(node) {
	
	var ref = node.previousSibling;

	
	if (!ref) {
		return;
	}

	
	while (ref.hasChildNodes()) {
		ref = ref.lastChild;
	}

	
	
	while (isInvisible(ref)
	&& !isExtraneousLineBreak(ref)
	&& ref != node.parentNode) {
		ref = previousNode(ref);
	}

	
	
	if (isEditable(ref)
	&& isExtraneousLineBreak(ref)) {
		ref.parentNode.removeChild(ref);
	}
}

function removeExtraneousLineBreaksAtTheEndOf(node) {
	
	var ref = node;

	
	while (ref.hasChildNodes()) {
		ref = ref.lastChild;
	}

	
	
	while (isInvisible(ref)
	&& !isExtraneousLineBreak(ref)
	&& ref != node) {
		ref = previousNode(ref);
	}

	
	if (isEditable(ref)
	&& isExtraneousLineBreak(ref)) {
		
		
		while (isEditable(ref.parentNode)
		&& isInvisible(ref.parentNode)) {
			ref = ref.parentNode;
		}

		
		ref.parentNode.removeChild(ref);
	}
}



function removeExtraneousLineBreaksFrom(node) {
	removeExtraneousLineBreaksBefore(node);
	removeExtraneousLineBreaksAtTheEndOf(node);
}





function wrap(nodeList, siblingCriteria, newParentInstructions) {
	
	
	if (typeof siblingCriteria == "undefined") {
		siblingCriteria = function() { return false };
	}
	if (typeof newParentInstructions == "undefined") {
		newParentInstructions = function() { return null };
	}

	
	
	if (nodeList.every(isInvisible)
	&& !nodeList.some(function(node) { return isHtmlElement(node, "br") })) {
		return null;
	}

	
	
	if (!nodeList[0].parentNode) {
		return null;
	}

	
	
	if (isInlineNode(nodeList[nodeList.length - 1])
	&& !isHtmlElement(nodeList[nodeList.length - 1], "br")
	&& isHtmlElement(nodeList[nodeList.length - 1].nextSibling, "br")) {
		nodeList.push(nodeList[nodeList.length - 1].nextSibling);
	}

	
	
	while (isInvisible(nodeList[0].previousSibling)) {
		nodeList.unshift(nodeList[0].previousSibling);
	}

	
	
	while (isInvisible(nodeList[nodeList.length - 1].nextSibling)) {
		nodeList.push(nodeList[nodeList.length - 1].nextSibling);
	}

	
	
	
	var newParent;
	if (isEditable(nodeList[0].previousSibling)
	&& siblingCriteria(nodeList[0].previousSibling)) {
		newParent = nodeList[0].previousSibling;

	
	
	
	} else if (isEditable(nodeList[nodeList.length - 1].nextSibling)
	&& siblingCriteria(nodeList[nodeList.length - 1].nextSibling)) {
		newParent = nodeList[nodeList.length - 1].nextSibling;

	
	
	} else {
		newParent = newParentInstructions();
	}

	
	if (!newParent) {
		return null;
	}

	
	if (!newParent.parentNode) {
		
		
		nodeList[0].parentNode.insertBefore(newParent, nodeList[0]);

		
		
		
		
		
		if (globalRange.startContainer == newParent.parentNode
		&& globalRange.startOffset == getNodeIndex(newParent)) {
			globalRange.setStart(globalRange.startContainer, globalRange.startOffset + 1);
		}
		if (globalRange.endContainer == newParent.parentNode
		&& globalRange.endOffset == getNodeIndex(newParent)) {
			globalRange.setEnd(globalRange.endContainer, globalRange.endOffset + 1);
		}
	}

	
	var originalParent = nodeList[0].parentNode;

	
	if (isBefore(newParent, nodeList[0])) {
		
		
		
		
		
		if (!isInlineNode(newParent)
		&& isInlineNode([].filter.call(newParent.childNodes, isVisible).slice(-1)[0])
		&& isInlineNode(nodeList.filter(isVisible)[0])
		&& !isHtmlElement(newParent.lastChild, "BR")) {
			newParent.appendChild(newParent.ownerDocument.createElement("br"));
		}

		
		
		for (var i = 0; i < nodeList.length; i++) {
			movePreservingRanges(nodeList[i], newParent, -1);
		}

	
	} else {
		
		
		
		
		
		if (!isInlineNode(newParent)
		&& isInlineNode([].filter.call(newParent.childNodes, isVisible)[0])
		&& isInlineNode(nodeList.filter(isVisible).slice(-1)[0])
		&& !isHtmlElement(nodeList[nodeList.length - 1], "BR")) {
			newParent.insertBefore(newParent.ownerDocument.createElement("br"), newParent.firstChild);
		}

		
		
		for (var i = nodeList.length - 1; i >= 0; i--) {
			movePreservingRanges(nodeList[i], newParent, 0);
		}
	}

	
	
	if (isEditable(originalParent) && !originalParent.hasChildNodes()) {
		originalParent.parentNode.removeChild(originalParent);
	}

	
	
	if (isEditable(newParent.nextSibling)
	&& siblingCriteria(newParent.nextSibling)) {
		
		
		
		
		
		if (!isInlineNode(newParent)
		&& isInlineNode(newParent.lastChild)
		&& isInlineNode(newParent.nextSibling.firstChild)
		&& !isHtmlElement(newParent.lastChild, "BR")) {
			newParent.appendChild(newParent.ownerDocument.createElement("br"));
		}

		
		
		while (newParent.nextSibling.hasChildNodes()) {
			movePreservingRanges(newParent.nextSibling.firstChild, newParent, -1);
		}

		
		newParent.parentNode.removeChild(newParent.nextSibling);
	}

	
	removeExtraneousLineBreaksFrom(newParent);

	
	return newParent;
}











var namesOfElementsWithInlineContents = ["a", "abbr", "b", "bdi", "bdo",
	"cite", "code", "dfn", "em", "h1", "h2", "h3", "h4", "h5", "h6", "i",
	"kbd", "mark", "p", "pre", "q", "rp", "rt", "ruby", "s", "samp", "small",
	"span", "strong", "sub", "sup", "u", "var", "acronym", "listing", "strike",
	"xmp", "big", "blink", "font", "marquee", "nobr", "tt"];



function isElementWithInlineContents(node) {
	return isHtmlElement(node, namesOfElementsWithInlineContents);
}

function isAllowedChild(child, parent_) {
	
	
	
	
	if ((["colgroup", "table", "tbody", "tfoot", "thead", "tr"].indexOf(parent_) != -1
	|| isHtmlElement(parent_, ["colgroup", "table", "tbody", "tfoot", "thead", "tr"]))
	&& typeof child == "object"
	&& child.nodeType == Node.TEXT_NODE
	&& !/^[ \t\n\f\r]*$/.test(child.data)) {
		return false;
	}

	
	
	
	if ((["script", "style", "plaintext", "xmp"].indexOf(parent_) != -1
	|| isHtmlElement(parent_, ["script", "style", "plaintext", "xmp"]))
	&& (typeof child != "object" || child.nodeType != Node.TEXT_NODE)) {
		return false;
	}

	
	
	if (typeof child == "object"
	&& (child.nodeType == Node.DOCUMENT_NODE
	|| child.nodeType == Node.DOCUMENT_FRAGMENT_NODE
	|| child.nodeType == Node.DOCUMENT_TYPE_NODE)) {
		return false;
	}

	
	if (isHtmlElement(child)) {
		child = child.tagName.toLowerCase();
	}

	
	if (typeof child != "string") {
		return true;
	}

	
	if (isHtmlElement(parent_)) {
		
		
		
		
		
		
		
		
		
		
		var ancestor = parent_;
		while (ancestor) {
			if (child == "a" && isHtmlElement(ancestor, "a")) {
				return false;
			}
			if (prohibitedParagraphChildNames.indexOf(child) != -1
			&& isElementWithInlineContents(ancestor)) {
				return false;
			}
			if (/^h[1-6]$/.test(child)
			&& isHtmlElement(ancestor)
			&& /^H[1-6]$/.test(ancestor.tagName)) {
				return false;
			}
			ancestor = ancestor.parentNode;
		}

		
		parent_ = parent_.tagName.toLowerCase();
	}

	
	if (typeof parent_ == "object"
	&& (parent_.nodeType == Node.ELEMENT_NODE
	|| parent_.nodeType == Node.DOCUMENT_FRAGMENT_NODE)) {
		return true;
	}

	
	if (typeof parent_ != "string") {
		return false;
	}

	
	
	
	switch (parent_) {
		case "colgroup":
			return child == "col";
		case "table":
			return ["caption", "col", "colgroup", "tbody", "td", "tfoot", "th", "thead", "tr"].indexOf(child) != -1;
		case "tbody":
		case "thead":
		case "tfoot":
			return ["td", "th", "tr"].indexOf(child) != -1;
		case "tr":
			return ["td", "th"].indexOf(child) != -1;
		case "dl":
			return ["dt", "dd"].indexOf(child) != -1;
		case "dir":
		case "ol":
		case "ul":
			return ["dir", "li", "ol", "ul"].indexOf(child) != -1;
		case "hgroup":
			return /^h[1-6]$/.test(child);
	}

	
	
	
	if (["body", "caption", "col", "colgroup", "frame", "frameset", "head",
	"html", "tbody", "td", "tfoot", "th", "thead", "tr"].indexOf(child) != -1) {
		return false;
	}

	
	if (["dd", "dt"].indexOf(child) != -1
	&& parent_ != "dl") {
		return false;
	}

	
	if (child == "li"
	&& parent_ != "ol"
	&& parent_ != "ul") {
		return false;
	}

	
	
	var table = [
		[["a"], ["a"]],
		[["dd", "dt"], ["dd", "dt"]],
		[["h1", "h2", "h3", "h4", "h5", "h6"], ["h1", "h2", "h3", "h4", "h5", "h6"]],
		[["li"], ["li"]],
		[["nobr"], ["nobr"]],
		[namesOfElementsWithInlineContents, prohibitedParagraphChildNames],
		[["td", "th"], ["caption", "col", "colgroup", "tbody", "td", "tfoot", "th", "thead", "tr"]],
	];
	for (var i = 0; i < table.length; i++) {
		if (table[i][0].indexOf(parent_) != -1
		&& table[i][1].indexOf(child) != -1) {
			return false;
		}
	}

	
	return true;
}













function isEffectivelyContained(node, range) {
	if (range.collapsed) {
		return false;
	}

	
	if (isContained(node, range)) {
		return true;
	}

	
	
	if (node == range.startContainer
	&& node.nodeType == Node.TEXT_NODE
	&& getNodeLength(node) != range.startOffset) {
		return true;
	}

	
	
	if (node == range.endContainer
	&& node.nodeType == Node.TEXT_NODE
	&& range.endOffset != 0) {
		return true;
	}

	
	
	
	
	
	if (node.hasChildNodes()
	&& [].every.call(node.childNodes, function(child) { return isEffectivelyContained(child, range) })
	&& (!isDescendant(range.startContainer, node)
	|| range.startContainer.nodeType != Node.TEXT_NODE
	|| range.startOffset == 0)
	&& (!isDescendant(range.endContainer, node)
	|| range.endContainer.nodeType != Node.TEXT_NODE
	|| range.endOffset == getNodeLength(range.endContainer))) {
		return true;
	}

	return false;
}


function getEffectivelyContainedNodes(range, condition) {
	if (typeof condition == "undefined") {
		condition = function() { return true };
	}
	var node = range.startContainer;
	while (isEffectivelyContained(node.parentNode, range)) {
		node = node.parentNode;
	}

	var stop = nextNodeDescendants(range.endContainer);

	var nodeList = [];
	while (isBefore(node, stop)) {
		if (isEffectivelyContained(node, range)
		&& condition(node)) {
			nodeList.push(node);
			node = nextNodeDescendants(node);
			continue;
		}
		node = nextNode(node);
	}
	return nodeList;
}

function getAllEffectivelyContainedNodes(range, condition) {
	if (typeof condition == "undefined") {
		condition = function() { return true };
	}
	var node = range.startContainer;
	while (isEffectivelyContained(node.parentNode, range)) {
		node = node.parentNode;
	}

	var stop = nextNodeDescendants(range.endContainer);

	var nodeList = [];
	while (isBefore(node, stop)) {
		if (isEffectivelyContained(node, range)
		&& condition(node)) {
			nodeList.push(node);
		}
		node = nextNode(node);
	}
	return nodeList;
}





function isModifiableElement(node) {
	if (!isHtmlElement(node)) {
		return false;
	}

	if (["B", "EM", "I", "S", "SPAN", "STRIKE", "STRONG", "SUB", "SUP", "U"].indexOf(node.tagName) != -1) {
		if (node.attributes.length == 0) {
			return true;
		}

		if (node.attributes.length == 1
		&& node.hasAttribute("style")) {
			return true;
		}
	}

	if (node.tagName == "FONT" || node.tagName == "A") {
		var numAttrs = node.attributes.length;

		if (node.hasAttribute("style")) {
			numAttrs--;
		}

		if (node.tagName == "FONT") {
			if (node.hasAttribute("color")) {
				numAttrs--;
			}

			if (node.hasAttribute("face")) {
				numAttrs--;
			}

			if (node.hasAttribute("size")) {
				numAttrs--;
			}
		}

		if (node.tagName == "A"
		&& node.hasAttribute("href")) {
			numAttrs--;
		}

		if (numAttrs == 0) {
			return true;
		}
	}

	return false;
}

function isSimpleModifiableElement(node) {
	
	
	if (!isHtmlElement(node)) {
		return false;
	}

	
	if (["A", "B", "EM", "FONT", "I", "S", "SPAN", "STRIKE", "STRONG", "SUB", "SUP", "U"].indexOf(node.tagName) == -1) {
		return false;
	}

	
	
	if (node.attributes.length == 0) {
		return true;
	}

	
	if (node.attributes.length > 1) {
		return false;
	}

	
	
	
	
	
	if (node.hasAttribute("style")
	&& node.style.length == 0) {
		return true;
	}

	
	if (node.tagName == "A"
	&& node.hasAttribute("href")) {
		return true;
	}

	
	
	if (node.tagName == "FONT"
	&& (node.hasAttribute("color")
	|| node.hasAttribute("face")
	|| node.hasAttribute("size")
	)) {
		return true;
	}

	
	
	
	if ((node.tagName == "B" || node.tagName == "STRONG")
	&& node.hasAttribute("style")
	&& node.style.length == 1
	&& node.style.fontWeight != "") {
		return true;
	}

	
	
	
	if ((node.tagName == "I" || node.tagName == "EM")
	&& node.hasAttribute("style")
	&& node.style.length == 1
	&& node.style.fontStyle != "") {
		return true;
	}

	
	
	
	
	if ((node.tagName == "A" || node.tagName == "FONT" || node.tagName == "SPAN")
	&& node.hasAttribute("style")
	&& node.style.length == 1
	&& node.style.textDecoration == "") {
		return true;
	}

	
	
	
	
	
	
	
	
	if (["A", "FONT", "S", "SPAN", "STRIKE", "U"].indexOf(node.tagName) != -1
	&& node.hasAttribute("style")
	&& (node.style.length == 1
	|| (node.style.length == 4
		&& "MozTextBlink" in node.style
		&& "textDecorationColor" in node.style
		&& "textDecorationLine" in node.style
		&& "textDecorationStyle" in node.style)
	)
	&& (node.style.textDecoration == "line-through"
	|| node.style.textDecoration == "underline"
	|| node.style.textDecoration == "overline"
	|| node.style.textDecoration == "none")) {
		return true;
	}

	return false;
}



function isFormattableNode(node) {
	return isEditable(node)
		&& isVisible(node)
		&& (node.nodeType == Node.TEXT_NODE
		|| isHtmlElement(node, ["img", "br"]));
}





function areEquivalentValues(command, val1, val2) {
	if (val1 === null && val2 === null) {
		return true;
	}

	if (typeof val1 == "string"
	&& typeof val2 == "string"
	&& val1 == val2
	&& !("equivalentValues" in commands[command])) {
		return true;
	}

	if (typeof val1 == "string"
	&& typeof val2 == "string"
	&& "equivalentValues" in commands[command]
	&& commands[command].equivalentValues(val1, val2)) {
		return true;
	}

	return false;
}







function areLooselyEquivalentValues(command, val1, val2) {
	if (areEquivalentValues(command, val1, val2)) {
		return true;
	}

	if (command != "fontsize"
	|| typeof val1 != "string"
	|| typeof val2 != "string") {
		return false;
	}

	
	var callee = areLooselyEquivalentValues;
	if (callee.sizeMap === undefined) {
		callee.sizeMap = {};
		var font = document.createElement("font");
		document.body.appendChild(font);
		["x-small", "small", "medium", "large", "x-large", "xx-large",
		"xxx-large"].forEach(function(keyword) {
			font.size = cssSizeToLegacy(keyword);
			callee.sizeMap[keyword] = getComputedStyle(font).fontSize;
		});
		document.body.removeChild(font);
	}

	return val1 === callee.sizeMap[val2]
		|| val2 === callee.sizeMap[val1];
}





function getEffectiveCommandValue(node, command) {
	
	if (node.nodeType != Node.ELEMENT_NODE
	&& (!node.parentNode || node.parentNode.nodeType != Node.ELEMENT_NODE)) {
		return null;
	}

	
	
	if (node.nodeType != Node.ELEMENT_NODE) {
		return getEffectiveCommandValue(node.parentNode, command);
	}

	
	if (command == "createlink" || command == "unlink") {
		
		
		while (node
		&& (!isHtmlElement(node)
		|| node.tagName != "A"
		|| !node.hasAttribute("href"))) {
			node = node.parentNode;
		}

		
		if (!node) {
			return null;
		}

		
		return node.getAttribute("href");
	}

	
	if (command == "backcolor"
	|| command == "hilitecolor") {
		
		
		
		
		
		while ((getComputedStyle(node).backgroundColor == "rgba(0, 0, 0, 0)"
		|| getComputedStyle(node).backgroundColor === ""
		|| getComputedStyle(node).backgroundColor == "transparent")
		&& node.parentNode
		&& node.parentNode.nodeType == Node.ELEMENT_NODE) {
			node = node.parentNode;
		}

		
		return getComputedStyle(node).backgroundColor;
	}

	
	if (command == "subscript" || command == "superscript") {
		
		
		var affectedBySubscript = false;
		var affectedBySuperscript = false;

		
		while (isInlineNode(node)) {
			var verticalAlign = getComputedStyle(node).verticalAlign;

			
			if (isHtmlElement(node, "sub")) {
				affectedBySubscript = true;
			
			
			} else if (isHtmlElement(node, "sup")) {
				affectedBySuperscript = true;
			}

			
			node = node.parentNode;
		}

		
		
		if (affectedBySubscript && affectedBySuperscript) {
			return "mixed";
		}

		
		if (affectedBySubscript) {
			return "subscript";
		}

		
		if (affectedBySuperscript) {
			return "superscript";
		}

		
		return null;
	}

	
	
	
	if (command == "strikethrough") {
		do {
			if (getComputedStyle(node).textDecoration.indexOf("line-through") != -1) {
				return "line-through";
			}
			node = node.parentNode;
		} while (node && node.nodeType == Node.ELEMENT_NODE);
		return null;
	}

	
	
	
	if (command == "underline") {
		do {
			if (getComputedStyle(node).textDecoration.indexOf("underline") != -1) {
				return "underline";
			}
			node = node.parentNode;
		} while (node && node.nodeType == Node.ELEMENT_NODE);
		return null;
	}

	if (!("relevantCssProperty" in commands[command])) {
		throw "Bug: no relevantCssProperty for " + command + " in getEffectiveCommandValue";
	}

	
	
	return getComputedStyle(node)[commands[command].relevantCssProperty];
}

function getSpecifiedCommandValue(element, command) {
	
	
	if ((command == "backcolor" || command == "hilitecolor")
	&& getComputedStyle(element).display != "inline") {
		return null;
	}

	
	if (command == "createlink" || command == "unlink") {
		
		
		if (isHtmlElement(element)
		&& element.tagName == "A"
		&& element.hasAttribute("href")) {
			return element.getAttribute("href");
		}

		
		return null;
	}

	
	if (command == "subscript" || command == "superscript") {
		
		if (isHtmlElement(element, "sup")) {
			return "superscript";
		}

		
		if (isHtmlElement(element, "sub")) {
			return "subscript";
		}

		
		return null;
	}

	
	
	if (command == "strikethrough"
	&& element.style.textDecoration != "") {
		
		
		if (element.style.textDecoration.indexOf("line-through") != -1) {
			return "line-through";
		}

		
		return null;
	}

	
	
	if (command == "strikethrough"
	&& isHtmlElement(element, ["S", "STRIKE"])) {
		return "line-through";
	}

	
	
	if (command == "underline"
	&& element.style.textDecoration != "") {
		
		
		if (element.style.textDecoration.indexOf("underline") != -1) {
			return "underline";
		}

		
		return null;
	}

	
	
	if (command == "underline"
	&& isHtmlElement(element, "U")) {
		return "underline";
	}

	
	var property = commands[command].relevantCssProperty;

	
	if (property === null) {
		return null;
	}

	
	
	if (element.style[property] != "") {
		return element.style[property];
	}

	
	
	
	
	if (isHtmlNamespace(element.namespaceURI)
	&& element.tagName == "FONT") {
		if (property == "color" && element.hasAttribute("color")) {
			return element.color;
		}
		if (property == "fontFamily" && element.hasAttribute("face")) {
			return element.face;
		}
		if (property == "fontSize" && element.hasAttribute("size")) {
			
			var size = parseInt(element.size);
			if (size < 1) {
				size = 1;
			}
			if (size > 7) {
				size = 7;
			}
			return {
				1: "x-small",
				2: "small",
				3: "medium",
				4: "large",
				5: "x-large",
				6: "xx-large",
				7: "xxx-large"
			}[size];
		}
	}

	
	
	
	
	if (property == "fontWeight"
	&& (element.tagName == "B" || element.tagName == "STRONG")) {
		return "bold";
	}
	if (property == "fontStyle"
	&& (element.tagName == "I" || element.tagName == "EM")) {
		return "italic";
	}

	
	return null;
}

function reorderModifiableDescendants(node, command, newValue) {
	
	var candidate = node;

	
	
	
	
	while (isModifiableElement(candidate)
	&& candidate.childNodes.length == 1
	&& isModifiableElement(candidate.firstChild)
	&& (!isSimpleModifiableElement(candidate)
	|| !areEquivalentValues(command, getSpecifiedCommandValue(candidate, command), newValue))) {
		candidate = candidate.firstChild;
	}

	
	
	
	
	if (candidate == node
	|| !isSimpleModifiableElement(candidate)
	|| !areEquivalentValues(command, getSpecifiedCommandValue(candidate, command), newValue)
	|| !areLooselyEquivalentValues(command, getEffectiveCommandValue(candidate, command), newValue)) {
		return;
	}

	
	
	while (candidate.hasChildNodes()) {
		movePreservingRanges(candidate.firstChild, candidate.parentNode, getNodeIndex(candidate));
	}

	
	node.parentNode.insertBefore(candidate, node.nextSibling);

	
	movePreservingRanges(node, candidate, -1);
}

function recordValues(nodeList) {
	
	
	var values = [];

	
	
	
	nodeList.forEach(function(node) {
		["subscript", "bold", "fontname", "fontsize", "forecolor",
		"hilitecolor", "italic", "strikethrough", "underline"].forEach(function(command) {
			
			var ancestor = node;

			
			if (ancestor.nodeType != Node.ELEMENT_NODE) {
				ancestor = ancestor.parentNode;
			}

			
			
			while (ancestor
			&& ancestor.nodeType == Node.ELEMENT_NODE
			&& getSpecifiedCommandValue(ancestor, command) === null) {
				ancestor = ancestor.parentNode;
			}

			
			
			
			if (ancestor && ancestor.nodeType == Node.ELEMENT_NODE) {
				values.push([node, command, getSpecifiedCommandValue(ancestor, command)]);
			} else {
				values.push([node, command, null]);
			}
		});
	});

	
	return values;
}

function restoreValues(values) {
	
	values.forEach(function(triple) {
		var node = triple[0];
		var command = triple[1];
		var value = triple[2];

		
		var ancestor = node;

		
		if (!ancestor || ancestor.nodeType != Node.ELEMENT_NODE) {
			ancestor = ancestor.parentNode;
		}

		
		
		while (ancestor
		&& ancestor.nodeType == Node.ELEMENT_NODE
		&& getSpecifiedCommandValue(ancestor, command) === null) {
			ancestor = ancestor.parentNode;
		}

		
		
		if (value === null
		&& ancestor
		&& ancestor.nodeType == Node.ELEMENT_NODE) {
			pushDownValues(node, command, null);

		
		
		
		
		} else if ((ancestor
		&& ancestor.nodeType == Node.ELEMENT_NODE
		&& !areEquivalentValues(command, getSpecifiedCommandValue(ancestor, command), value))
		|| ((!ancestor || ancestor.nodeType != Node.ELEMENT_NODE)
		&& value !== null)) {
			forceValue(node, command, value);
		}
	});
}






function clearValue(element, command) {
	
	if (!isEditable(element)) {
		return [];
	}

	
	
	if (getSpecifiedCommandValue(element, command) === null) {
		return [];
	}

	
	if (isSimpleModifiableElement(element)) {
		
		var children = Array.prototype.slice.call(element.childNodes);

		
		
		for (var i = 0; i < children.length; i++) {
			movePreservingRanges(children[i], element.parentNode, getNodeIndex(element));
		}

		
		element.parentNode.removeChild(element);

		
		return children;
	}

	
	
	
	if (command == "strikethrough"
	&& element.style.textDecoration.indexOf("line-through") != -1) {
		if (element.style.textDecoration == "line-through") {
			element.style.textDecoration = "";
		} else {
			element.style.textDecoration = element.style.textDecoration.replace("line-through", "");
		}
		if (element.getAttribute("style") == "") {
			element.removeAttribute("style");
		}
	}

	
	
	
	if (command == "underline"
	&& element.style.textDecoration.indexOf("underline") != -1) {
		if (element.style.textDecoration == "underline") {
			element.style.textDecoration = "";
		} else {
			element.style.textDecoration = element.style.textDecoration.replace("underline", "");
		}
		if (element.getAttribute("style") == "") {
			element.removeAttribute("style");
		}
	}

	
	
	if (commands[command].relevantCssProperty !== null) {
		element.style[commands[command].relevantCssProperty] = '';
		if (element.getAttribute("style") == "") {
			element.removeAttribute("style");
		}
	}

	
	if (isHtmlNamespace(element.namespaceURI) && element.tagName == "FONT") {
		
		if (command == "forecolor") {
			element.removeAttribute("color");
		}

		
		if (command == "fontname") {
			element.removeAttribute("face");
		}

		
		if (command == "fontsize") {
			element.removeAttribute("size");
		}
	}

	
	
	if (isHtmlElement(element, "A")
	&& (command == "createlink" || command == "unlink")) {
		element.removeAttribute("href");
	}

	
	
	if (getSpecifiedCommandValue(element, command) === null) {
		return [];
	}

	
	
	return [setTagName(element, "span")];
}






function pushDownValues(node, command, newValue) {
	
	if (!node.parentNode
	|| node.parentNode.nodeType != Node.ELEMENT_NODE) {
		return;
	}

	
	
	if (areLooselyEquivalentValues(command, getEffectiveCommandValue(node, command), newValue)) {
		return;
	}

	
	var currentAncestor = node.parentNode;

	
	var ancestorList = [];

	
	
	
	
	while (isEditable(currentAncestor)
	&& currentAncestor.nodeType == Node.ELEMENT_NODE
	&& !areLooselyEquivalentValues(command, getEffectiveCommandValue(currentAncestor, command), newValue)) {
		ancestorList.push(currentAncestor);
		currentAncestor = currentAncestor.parentNode;
	}

	
	if (!ancestorList.length) {
		return;
	}

	
	
	var propagatedValue = getSpecifiedCommandValue(ancestorList[ancestorList.length - 1], command);

	
	
	if (propagatedValue === null && propagatedValue != newValue) {
		return;
	}

	
	
	
	if (newValue !== null
	&& !areLooselyEquivalentValues(command, getEffectiveCommandValue(ancestorList[ancestorList.length - 1].parentNode, command), newValue)) {
		return;
	}

	
	while (ancestorList.length) {
		
		
		var currentAncestor = ancestorList.pop();

		
		
		if (getSpecifiedCommandValue(currentAncestor, command) !== null) {
			propagatedValue = getSpecifiedCommandValue(currentAncestor, command);
		}

		
		var children = Array.prototype.slice.call(currentAncestor.childNodes);

		
		
		if (getSpecifiedCommandValue(currentAncestor, command) !== null) {
			clearValue(currentAncestor, command);
		}

		
		for (var i = 0; i < children.length; i++) {
			var child = children[i];

			
			if (child == node) {
				continue;
			}

			
			
			
			if (child.nodeType == Node.ELEMENT_NODE
			&& getSpecifiedCommandValue(child, command) !== null
			&& !areEquivalentValues(command, propagatedValue, getSpecifiedCommandValue(child, command))) {
				continue;
			}

			
			
			if (child == ancestorList[ancestorList.length - 1]) {
				continue;
			}

			
			
			forceValue(child, command, propagatedValue);
		}
	}
}






function forceValue(node, command, newValue) {
	
	if (!node.parentNode) {
		return;
	}

	
	if (newValue === null) {
		return;
	}

	
	if (isAllowedChild(node, "span")) {
		
		reorderModifiableDescendants(node.previousSibling, command, newValue);

		
		reorderModifiableDescendants(node.nextSibling, command, newValue);

		
		
		
		
		
		wrap([node],
			function(node) {
				return isSimpleModifiableElement(node)
					&& areEquivalentValues(command, getSpecifiedCommandValue(node, command), newValue)
					&& areLooselyEquivalentValues(command, getEffectiveCommandValue(node, command), newValue);
			},
			function() { return null }
		);
	}

	
	if (isInvisible(node)) {
		return;
	}

	
	
	if (areLooselyEquivalentValues(command, getEffectiveCommandValue(node, command), newValue)) {
		return;
	}

	
	if (!isAllowedChild(node, "span")) {
		
		
		
		var children = [];
		for (var i = 0; i < node.childNodes.length; i++) {
			if (node.childNodes[i].nodeType == Node.ELEMENT_NODE) {
				var specifiedValue = getSpecifiedCommandValue(node.childNodes[i], command);

				if (specifiedValue !== null
				&& !areEquivalentValues(command, newValue, specifiedValue)) {
					continue;
				}
			}
			children.push(node.childNodes[i]);
		}

		
		
		for (var i = 0; i < children.length; i++) {
			forceValue(children[i], command, newValue);
		}

		
		return;
	}

	
	
	if (areLooselyEquivalentValues(command, getEffectiveCommandValue(node, command), newValue)) {
		return;
	}

	
	var newParent = null;

	
	if (!cssStylingFlag) {
		
		
		if (command == "bold" && (newValue == "bold" || newValue == "700")) {
			newParent = node.ownerDocument.createElement("b");
		}

		
		
		
		if (command == "italic" && newValue == "italic") {
			newParent = node.ownerDocument.createElement("i");
		}

		
		
		
		if (command == "strikethrough" && newValue == "line-through") {
			newParent = node.ownerDocument.createElement("s");
		}

		
		
		
		if (command == "underline" && newValue == "underline") {
			newParent = node.ownerDocument.createElement("u");
		}

		
		
		if (command == "forecolor" && parseSimpleColor(newValue)) {
			
			
			newParent = node.ownerDocument.createElement("font");

			
			
			
			newParent.setAttribute("color", parseSimpleColor(newValue));
		}

		
		
		
		if (command == "fontname") {
			newParent = node.ownerDocument.createElement("font");
			newParent.face = newValue;
		}
	}

	
	if (command == "createlink" || command == "unlink") {
		
		
		newParent = node.ownerDocument.createElement("a");

		
		newParent.setAttribute("href", newValue);

		
		var ancestor = node.parentNode;

		
		while (ancestor) {
			
			
			if (isHtmlElement(ancestor, "A")) {
				ancestor = setTagName(ancestor, "span");
			}

			
			ancestor = ancestor.parentNode;
		}
	}

	
	
	
	
	
	
	if (command == "fontsize"
	&& ["x-small", "small", "medium", "large", "x-large", "xx-large", "xxx-large"].indexOf(newValue) != -1
	&& (!cssStylingFlag || newValue == "xxx-large")) {
		newParent = node.ownerDocument.createElement("font");
		newParent.size = cssSizeToLegacy(newValue);
	}

	
	
	
	if ((command == "subscript" || command == "superscript")
	&& newValue == "subscript") {
		newParent = node.ownerDocument.createElement("sub");
	}

	
	
	
	if ((command == "subscript" || command == "superscript")
	&& newValue == "superscript") {
		newParent = node.ownerDocument.createElement("sup");
	}

	
	
	if (!newParent) {
		newParent = node.ownerDocument.createElement("span");
	}

	
	node.parentNode.insertBefore(newParent, node);

	
	
	
	
	var property = commands[command].relevantCssProperty;
	if (property !== null
	&& !areLooselyEquivalentValues(command, getEffectiveCommandValue(newParent, command), newValue)) {
		newParent.style[property] = newValue;
	}

	
	
	
	
	if (command == "strikethrough"
	&& newValue == "line-through"
	&& getEffectiveCommandValue(newParent, "strikethrough") != "line-through") {
		newParent.style.textDecoration = "line-through";
	}

	
	
	
	
	if (command == "underline"
	&& newValue == "underline"
	&& getEffectiveCommandValue(newParent, "underline") != "underline") {
		newParent.style.textDecoration = "underline";
	}

	
	movePreservingRanges(node, newParent, newParent.childNodes.length);

	
	
	if (node.nodeType == Node.ELEMENT_NODE
	&& !areEquivalentValues(command, getEffectiveCommandValue(node, command), newValue)) {
		
		
		movePreservingRanges(node, newParent.parentNode, getNodeIndex(newParent));

		
		newParent.parentNode.removeChild(newParent);

		
		
		
		var children = [];
		for (var i = 0; i < node.childNodes.length; i++) {
			if (node.childNodes[i].nodeType == Node.ELEMENT_NODE) {
				var specifiedValue = getSpecifiedCommandValue(node.childNodes[i], command);

				if (specifiedValue !== null
				&& !areEquivalentValues(command, newValue, specifiedValue)) {
					continue;
				}
			}
			children.push(node.childNodes[i]);
		}

		
		
		for (var i = 0; i < children.length; i++) {
			forceValue(children[i], command, newValue);
		}
	}
}






function setSelectionValue(command, newValue) {
	
	
	if (!getAllEffectivelyContainedNodes(getActiveRange())
	.some(isFormattableNode)) {
		
		
		if ("inlineCommandActivatedValues" in commands[command]) {
			setStateOverride(command, commands[command].inlineCommandActivatedValues
				.indexOf(newValue) != -1);
		}

		
		
		if (command == "subscript") {
			unsetStateOverride("superscript");
		}

		
		
		if (command == "superscript") {
			unsetStateOverride("subscript");
		}

		
		if (newValue === null) {
			unsetValueOverride(command);

		
		
		} else if (command == "createlink" || "value" in commands[command]) {
			setValueOverride(command, newValue);
		}

		
		return;
	}

	
	
	
	
	
	if (isEditable(getActiveRange().startContainer)
	&& getActiveRange().startContainer.nodeType == Node.TEXT_NODE
	&& getActiveRange().startOffset != 0
	&& getActiveRange().startOffset != getNodeLength(getActiveRange().startContainer)) {
		
		var newActiveRange = document.createRange();
		var newNode;
		if (getActiveRange().startContainer == getActiveRange().endContainer) {
			var newEndOffset = getActiveRange().endOffset - getActiveRange().startOffset;
			newNode = getActiveRange().startContainer.splitText(getActiveRange().startOffset);
			newActiveRange.setEnd(newNode, newEndOffset);
			getActiveRange().setEnd(newNode, newEndOffset);
		} else {
			newNode = getActiveRange().startContainer.splitText(getActiveRange().startOffset);
		}
		newActiveRange.setStart(newNode, 0);
		getSelection().removeAllRanges();
		getSelection().addRange(newActiveRange);

		getActiveRange().setStart(newNode, 0);
	}

	
	
	
	
	if (isEditable(getActiveRange().endContainer)
	&& getActiveRange().endContainer.nodeType == Node.TEXT_NODE
	&& getActiveRange().endOffset != 0
	&& getActiveRange().endOffset != getNodeLength(getActiveRange().endContainer)) {
		
		
		
		
		var activeRange = getActiveRange();
		var newStart = [activeRange.startContainer, activeRange.startOffset];
		var newEnd = [activeRange.endContainer, activeRange.endOffset];
		activeRange.endContainer.splitText(activeRange.endOffset);
		activeRange.setStart(newStart[0], newStart[1]);
		activeRange.setEnd(newEnd[0], newEnd[1]);

		getSelection().removeAllRanges();
		getSelection().addRange(activeRange);
	}

	
	
	
	
	getAllEffectivelyContainedNodes(getActiveRange(), function(node) {
		return isEditable(node) && node.nodeType == Node.ELEMENT_NODE;
	}).forEach(function(element) {
		clearValue(element, command);
	});

	
	
	
	
	getAllEffectivelyContainedNodes(getActiveRange(), isEditable).forEach(function(node) {
		
		pushDownValues(node, command, newValue);

		
		if (isAllowedChild(node, "span")) {
			forceValue(node, command, newValue);
		}
	});
}





commands.backcolor = {
	
	action: function(value) {
		

		
		
		
		
		
		
		if (/^([0-9a-fA-F]{3}){1,2}$/.test(value)) {
			value = "#" + value;
		}
		if (!/^(rgba?|hsla?)\(.*\)$/.test(value)
		&& !parseSimpleColor(value)
		&& value.toLowerCase() != "transparent") {
			return false;
		}

		
		setSelectionValue("backcolor", value);

		
		return true;
	}, standardInlineValueCommand: true, relevantCssProperty: "backgroundColor",
	equivalentValues: function(val1, val2) {
		
		
		
		return normalizeColor(val1) === normalizeColor(val2);
	},
};




commands.bold = {
	action: function() {
		
		
		
		if (myQueryCommandState("bold")) {
			setSelectionValue("bold", "normal");
		} else {
			setSelectionValue("bold", "bold");
		}
		return true;
	}, inlineCommandActivatedValues: ["bold", "600", "700", "800", "900"],
	relevantCssProperty: "fontWeight",
	equivalentValues: function(val1, val2) {
		
		
		return val1 == val2
			|| (val1 == "bold" && val2 == "700")
			|| (val1 == "700" && val2 == "bold")
			|| (val1 == "normal" && val2 == "400")
			|| (val1 == "400" && val2 == "normal");
	},
};




commands.createlink = {
	action: function(value) {
		
		if (value === "") {
			return false;
		}

		
		
		
		
		
		
		getAllEffectivelyContainedNodes(getActiveRange()).forEach(function(node) {
			getAncestors(node).forEach(function(ancestor) {
				if (isEditable(ancestor)
				&& isHtmlElement(ancestor, "a")
				&& ancestor.hasAttribute("href")) {
					ancestor.setAttribute("href", value);
				}
			});
		});

		
		setSelectionValue("createlink", value);

		
		return true;
	}
};




commands.fontname = {
	action: function(value) {
		
		setSelectionValue("fontname", value);
		return true;
	}, standardInlineValueCommand: true, relevantCssProperty: "fontFamily"
};








function normalizeFontSize(value) {
	
	
	
	value = value.trim();

	
	
	
	if (!/^[-+]?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?$/.test(value)) {
		return null;
	}

	var mode;

	
	
	if (value[0] == "+") {
		value = value.slice(1);
		mode = "relative-plus";
	
	
	} else if (value[0] == "-") {
		value = value.slice(1);
		mode = "relative-minus";
	
	} else {
		mode = "absolute";
	}

	
	
	
	
	var num = parseInt(value);

	
	if (mode == "relative-plus") {
		num += 3;
	}

	
	if (mode == "relative-minus") {
		num = 3 - num;
	}

	
	if (num < 1) {
		num = 1;
	}

	
	if (num > 7) {
		num = 7;
	}

	
	value = {
		1: "x-small",
		2: "small",
		3: "medium",
		4: "large",
		5: "x-large",
		6: "xx-large",
		7: "xxx-large"
	}[num];

	return value;
}

commands.fontsize = {
	action: function(value) {
		value = normalizeFontSize(value);
		if (value === null) {
			return false;
		}

		
		setSelectionValue("fontsize", value);

		
		return true;
	}, indeterm: function() {
		
		
		
		return getAllEffectivelyContainedNodes(getActiveRange(), isFormattableNode)
		.map(function(node) {
			return getEffectiveCommandValue(node, "fontsize");
		}).filter(function(value, i, arr) {
			return arr.slice(0, i).indexOf(value) == -1;
		}).length >= 2;
	}, value: function() {
		
		if (!getActiveRange()) {
			return "";
		}

		
		
		
		
		
		var node = getAllEffectivelyContainedNodes(getActiveRange(), isFormattableNode)[0];
		if (node === undefined) {
			node = getActiveRange().startContainer;
		}
		var pixelSize = getEffectiveCommandValue(node, "fontsize");

		
		return getLegacyFontSize(pixelSize);
	}, relevantCssProperty: "fontSize"
};

function getLegacyFontSize(size) {
	if (getLegacyFontSize.resultCache === undefined) {
		getLegacyFontSize.resultCache = {};
	}

	if (getLegacyFontSize.resultCache[size] !== undefined) {
		return getLegacyFontSize.resultCache[size];
	}

	
	
	
	if (normalizeFontSize(size) !== null) {
		return getLegacyFontSize.resultCache[size] = cssSizeToLegacy(normalizeFontSize(size));
	}

	if (["x-small", "x-small", "small", "medium", "large", "x-large", "xx-large", "xxx-large"].indexOf(size) == -1
	&& !/^[0-9]+(\.[0-9]+)?(cm|mm|in|pt|pc|px)$/.test(size)) {
		
		return getLegacyFontSize.resultCache[size] = null;
	}

	var font = document.createElement("font");
	document.body.appendChild(font);
	if (size == "xxx-large") {
		font.size = 7;
	} else {
		font.style.fontSize = size;
	}
	var pixelSize = parseInt(getComputedStyle(font).fontSize);
	document.body.removeChild(font);

	
	var returnedSize = 1;

	
	while (returnedSize < 7) {
		
		
		var font = document.createElement("font");
		font.size = returnedSize;
		document.body.appendChild(font);
		var lowerBound = parseInt(getComputedStyle(font).fontSize);

		
		
		
		font.size = 1 + returnedSize;
		var upperBound = parseInt(getComputedStyle(font).fontSize);
		document.body.removeChild(font);

		
		var average = (upperBound + lowerBound)/2;

		
		
		if (pixelSize < average) {
			return getLegacyFontSize.resultCache[size] = String(returnedSize);
		}

		
		returnedSize++;
	}

	
	return getLegacyFontSize.resultCache[size] = "7";
}




commands.forecolor = {
	action: function(value) {
		

		
		
		
		
		
		
		if (/^([0-9a-fA-F]{3}){1,2}$/.test(value)) {
			value = "#" + value;
		}
		if (!/^(rgba?|hsla?)\(.*\)$/.test(value)
		&& !parseSimpleColor(value)
		&& value.toLowerCase() != "transparent") {
			return false;
		}

		
		setSelectionValue("forecolor", value);

		
		return true;
	}, standardInlineValueCommand: true, relevantCssProperty: "color",
	equivalentValues: function(val1, val2) {
		
		
		
		return normalizeColor(val1) === normalizeColor(val2);
	},
};




commands.hilitecolor = {
	
	action: function(value) {
		

		
		
		
		
		
		
		if (/^([0-9a-fA-F]{3}){1,2}$/.test(value)) {
			value = "#" + value;
		}
		if (!/^(rgba?|hsla?)\(.*\)$/.test(value)
		&& !parseSimpleColor(value)
		&& value.toLowerCase() != "transparent") {
			return false;
		}

		
		setSelectionValue("hilitecolor", value);

		
		return true;
	}, indeterm: function() {
		
		
		
		return getAllEffectivelyContainedNodes(getActiveRange(), function(node) {
			return isEditable(node) && node.nodeType == Node.TEXT_NODE;
		}).map(function(node) {
			return getEffectiveCommandValue(node, "hilitecolor");
		}).filter(function(value, i, arr) {
			return arr.slice(0, i).indexOf(value) == -1;
		}).length >= 2;
	}, standardInlineValueCommand: true, relevantCssProperty: "backgroundColor",
	equivalentValues: function(val1, val2) {
		
		
		
		return normalizeColor(val1) === normalizeColor(val2);
	},
};




commands.italic = {
	action: function() {
		
		
		
		if (myQueryCommandState("italic")) {
			setSelectionValue("italic", "normal");
		} else {
			setSelectionValue("italic", "italic");
		}
		return true;
	}, inlineCommandActivatedValues: ["italic", "oblique"],
	relevantCssProperty: "fontStyle"
};




commands.removeformat = {
	action: function() {
		
		
		
		
		
		function isRemoveFormatCandidate(node) {
			return isEditable(node)
				&& isHtmlElement(node, ["abbr", "acronym", "b", "bdi", "bdo",
				"big", "blink", "cite", "code", "dfn", "em", "font", "i",
				"ins", "kbd", "mark", "nobr", "q", "s", "samp", "small",
				"span", "strike", "strong", "sub", "sup", "tt", "u", "var"]);
		}

		
		
		var elementsToRemove = getAllEffectivelyContainedNodes(getActiveRange(), isRemoveFormatCandidate);

		
		elementsToRemove.forEach(function(element) {
			
			
			
			while (element.hasChildNodes()) {
				movePreservingRanges(element.firstChild, element.parentNode, getNodeIndex(element));
			}

			
			element.parentNode.removeChild(element);
		});

		
		
		
		
		
		if (isEditable(getActiveRange().startContainer)
		&& getActiveRange().startContainer.nodeType == Node.TEXT_NODE
		&& getActiveRange().startOffset != 0
		&& getActiveRange().startOffset != getNodeLength(getActiveRange().startContainer)) {
			
			if (getActiveRange().startContainer == getActiveRange().endContainer) {
				var newEnd = getActiveRange().endOffset - getActiveRange().startOffset;
				var newNode = getActiveRange().startContainer.splitText(getActiveRange().startOffset);
				getActiveRange().setStart(newNode, 0);
				getActiveRange().setEnd(newNode, newEnd);
			} else {
				getActiveRange().setStart(getActiveRange().startContainer.splitText(getActiveRange().startOffset), 0);
			}
		}

		
		
		
		
		if (isEditable(getActiveRange().endContainer)
		&& getActiveRange().endContainer.nodeType == Node.TEXT_NODE
		&& getActiveRange().endOffset != 0
		&& getActiveRange().endOffset != getNodeLength(getActiveRange().endContainer)) {
			
			
			
			
			
			var newStart = [getActiveRange().startContainer, getActiveRange().startOffset];
			var newEnd = [getActiveRange().endContainer, getActiveRange().endOffset];
			getActiveRange().setEnd(document.documentElement, 0);
			newEnd[0].splitText(newEnd[1]);
			getActiveRange().setStart(newStart[0], newStart[1]);
			getActiveRange().setEnd(newEnd[0], newEnd[1]);
		}

		
		
		
		
		
		
		getAllEffectivelyContainedNodes(getActiveRange(), isEditable).forEach(function(node) {
			while (isRemoveFormatCandidate(node.parentNode)
			&& inSameEditingHost(node.parentNode, node)) {
				splitParent([node]);
			}
		});

		
		
		[
			"subscript",
			"bold",
			"fontname",
			"fontsize",
			"forecolor",
			"hilitecolor",
			"italic",
			"strikethrough",
			"underline",
		].forEach(function(command) {
			setSelectionValue(command, null);
		});

		
		return true;
	}
};




commands.strikethrough = {
	action: function() {
		
		
		
		if (myQueryCommandState("strikethrough")) {
			setSelectionValue("strikethrough", null);
		} else {
			setSelectionValue("strikethrough", "line-through");
		}
		return true;
	}, inlineCommandActivatedValues: ["line-through"]
};




commands.subscript = {
	action: function() {
		
		var state = myQueryCommandState("subscript");

		
		setSelectionValue("subscript", null);

		
		if (!state) {
			setSelectionValue("subscript", "subscript");
		}

		
		return true;
	}, indeterm: function() {
		
		
		
		
		
		
		var nodes = getAllEffectivelyContainedNodes(getActiveRange(), isFormattableNode);
		return (nodes.some(function(node) { return getEffectiveCommandValue(node, "subscript") == "subscript" })
			&& nodes.some(function(node) { return getEffectiveCommandValue(node, "subscript") != "subscript" }))
			|| nodes.some(function(node) { return getEffectiveCommandValue(node, "subscript") == "mixed" });
	}, inlineCommandActivatedValues: ["subscript"],
};




commands.superscript = {
	action: function() {
		
		
		var state = myQueryCommandState("superscript");

		
		setSelectionValue("superscript", null);

		
		if (!state) {
			setSelectionValue("superscript", "superscript");
		}

		
		return true;
	}, indeterm: function() {
		
		
		
		
		
		
		var nodes = getAllEffectivelyContainedNodes(getActiveRange(), isFormattableNode);
		return (nodes.some(function(node) { return getEffectiveCommandValue(node, "superscript") == "superscript" })
			&& nodes.some(function(node) { return getEffectiveCommandValue(node, "superscript") != "superscript" }))
			|| nodes.some(function(node) { return getEffectiveCommandValue(node, "superscript") == "mixed" });
	}, inlineCommandActivatedValues: ["superscript"],
};




commands.underline = {
	action: function() {
		
		
		
		if (myQueryCommandState("underline")) {
			setSelectionValue("underline", null);
		} else {
			setSelectionValue("underline", "underline");
		}
		return true;
	}, inlineCommandActivatedValues: ["underline"]
};




commands.unlink = {
	action: function() {
		
		
		
		
		
		
		var range = getActiveRange();
		var hyperlinks = [];
		for (
			var node = range.startContainer;
			node;
			node = node.parentNode
		) {
			if (isHtmlElement(node, "A")
			&& node.hasAttribute("href")) {
				hyperlinks.unshift(node);
			}
		}
		for (
			var node = range.startContainer;
			node != nextNodeDescendants(range.endContainer);
			node = nextNode(node)
		) {
			if (isHtmlElement(node, "A")
			&& node.hasAttribute("href")
			&& (isContained(node, range)
			|| isAncestor(node, range.endContainer)
			|| node == range.endContainer)) {
				hyperlinks.push(node);
			}
		}

		
		for (var i = 0; i < hyperlinks.length; i++) {
			clearValue(hyperlinks[i], "unlink");
		}

		
		return true;
	}
};












function isIndentationElement(node) {
	if (!isHtmlElement(node)) {
		return false;
	}

	if (node.tagName == "BLOCKQUOTE") {
		return true;
	}

	if (node.tagName != "DIV") {
		return false;
	}

	for (var i = 0; i < node.style.length; i++) {
		
		if (/^(-[a-z]+-)?margin/.test(node.style[i])) {
			return true;
		}
	}

	return false;
}







function isSimpleIndentationElement(node) {
	if (!isIndentationElement(node)) {
		return false;
	}

	for (var i = 0; i < node.attributes.length; i++) {
		if (!isHtmlNamespace(node.attributes[i].namespaceURI)
		|| ["style", "dir"].indexOf(node.attributes[i].name) == -1) {
			return false;
		}
	}

	for (var i = 0; i < node.style.length; i++) {
		
		if (!/^(-[a-z]+-)?(margin|border|padding)/.test(node.style[i])) {
			return false;
		}
	}

	return true;
}




function isNonListSingleLineContainer(node) {
	return isHtmlElement(node, ["address", "div", "h1", "h2", "h3", "h4", "h5",
		"h6", "listing", "p", "pre", "xmp"]);
}



function isSingleLineContainer(node) {
	return isNonListSingleLineContainer(node)
		|| isHtmlElement(node, ["li", "dt", "dd"]);
}

function getBlockNodeOf(node) {
	
	while (isInlineNode(node)) {
		node = node.parentNode;
	}

	
	return node;
}





function fixDisallowedAncestors(node) {
	
	if (!isEditable(node)) {
		return;
	}

	
	
	if (getAncestors(node).every(function(ancestor) {
		return !inSameEditingHost(node, ancestor)
			|| !isAllowedChild(node, ancestor)
	})) {
		
		
		
		
		
		if (isHtmlElement(node, ["dd", "dt"])) {
			wrap([node],
				function(sibling) { return isHtmlElement(sibling, "dl") && !sibling.attributes.length },
				function() { return document.createElement("dl") });
			return;
		}

		
		
		if (!isAllowedChild("p", getEditingHostOf(node))) {
			return;
		}

		
		if (!isProhibitedParagraphChild(node)) {
			return;
		}

		
		
		node = setTagName(node, defaultSingleLineContainerName);

		
		fixDisallowedAncestors(node);

		
		var children = [].slice.call(node.childNodes);

		
		
		children.filter(isProhibitedParagraphChild)
		.forEach(function(child) {
			
			
			var values = recordValues([child]);

			
			splitParent([child]);

			
			restoreValues(values);
		});

		
		return;
	}

	
	
	var values = recordValues([node]);

	
	
	while (!isAllowedChild(node, node.parentNode)) {
		splitParent([node]);
	}

	
	restoreValues(values);
}

function normalizeSublists(item) {
	
	
	if (!isHtmlElement(item, "LI")
	|| !isEditable(item)
	|| !isEditable(item.parentNode)) {
		return;
	}

	
	var newItem = null;

	
	while ([].some.call(item.childNodes, function (node) { return isHtmlElement(node, ["OL", "UL"]) })) {
		
		var child = item.lastChild;

		
		
		if (isHtmlElement(child, ["OL", "UL"])
		|| (!newItem && child.nodeType == Node.TEXT_NODE && /^[ \t\n\f\r]*$/.test(child.data))) {
			
			newItem = null;

			
			
			movePreservingRanges(child, item.parentNode, 1 + getNodeIndex(item));

		
		} else {
			
			
			
			if (!newItem) {
				newItem = item.ownerDocument.createElement("li");
				item.parentNode.insertBefore(newItem, item.nextSibling);
			}

			
			
			movePreservingRanges(child, newItem, 0);
		}
	}
}

function getSelectionListState() {
	
	if (!getActiveRange()) {
		return "none";
	}

	
	var newRange = blockExtend(getActiveRange());

	
	
	
	
	
	
	var nodeList = getContainedNodes(newRange, function(node) {
		return isEditable(node)
			&& !isIndentationElement(node)
			&& (isHtmlElement(node, ["ol", "ul"])
			|| isHtmlElement(node.parentNode, ["ol", "ul"])
			|| isAllowedChild(node, "li"));
	});

	
	if (!nodeList.length) {
		return "none";
	}

	
	
	
	if (nodeList.every(function(node) {
		return isHtmlElement(node, "ol")
			|| isHtmlElement(node.parentNode, "ol")
			|| (isHtmlElement(node.parentNode, "li") && isHtmlElement(node.parentNode.parentNode, "ol"));
	})
	&& !nodeList.some(function(node) { return isHtmlElement(node, "ul") || ("querySelector" in node && node.querySelector("ul")) })) {
		return "ol";
	}

	
	
	
	if (nodeList.every(function(node) {
		return isHtmlElement(node, "ul")
			|| isHtmlElement(node.parentNode, "ul")
			|| (isHtmlElement(node.parentNode, "li") && isHtmlElement(node.parentNode.parentNode, "ul"));
	})
	&& !nodeList.some(function(node) { return isHtmlElement(node, "ol") || ("querySelector" in node && node.querySelector("ol")) })) {
		return "ul";
	}

	var hasOl = nodeList.some(function(node) {
		return isHtmlElement(node, "ol")
			|| isHtmlElement(node.parentNode, "ol")
			|| ("querySelector" in node && node.querySelector("ol"))
			|| (isHtmlElement(node.parentNode, "li") && isHtmlElement(node.parentNode.parentNode, "ol"));
	});
	var hasUl = nodeList.some(function(node) {
		return isHtmlElement(node, "ul")
			|| isHtmlElement(node.parentNode, "ul")
			|| ("querySelector" in node && node.querySelector("ul"))
			|| (isHtmlElement(node.parentNode, "li") && isHtmlElement(node.parentNode.parentNode, "ul"));
	});
	
	
	
	
	if (hasOl && hasUl) {
		return "mixed";
	}

	
	
	if (hasOl) {
		return "mixed ol";
	}

	
	
	if (hasUl) {
		return "mixed ul";
	}

	
	return "none";
}

function getAlignmentValue(node) {
	
	
	
	while ((node && node.nodeType != Node.ELEMENT_NODE)
	|| (node.nodeType == Node.ELEMENT_NODE
	&& ["inline", "none"].indexOf(getComputedStyle(node).display) != -1)) {
		node = node.parentNode;
	}

	
	if (!node || node.nodeType != Node.ELEMENT_NODE) {
		return "left";
	}

	var resolvedValue = getComputedStyle(node).textAlign
		
		.replace(/^-(moz|webkit)-/, "")
		.replace(/^auto$/, "start");

	
	
	if (resolvedValue == "start") {
		return getDirectionality(node) == "ltr" ? "left" : "right";
	}

	
	
	if (resolvedValue == "end") {
		return getDirectionality(node) == "ltr" ? "right" : "left";
	}

	
	
	if (["center", "justify", "left", "right"].indexOf(resolvedValue) != -1) {
		return resolvedValue;
	}

	
	return "left";
}

function getNextEquivalentPoint(node, offset) {
	
	if (getNodeLength(node) == 0) {
		return null;
	}

	
	
	if (offset == getNodeLength(node)
	&& node.parentNode
	&& isInlineNode(node)) {
		return [node.parentNode, 1 + getNodeIndex(node)];
	}

	
	
	if (0 <= offset
	&& offset < node.childNodes.length
	&& getNodeLength(node.childNodes[offset]) != 0
	&& isInlineNode(node.childNodes[offset])) {
		return [node.childNodes[offset], 0];
	}

	
	return null;
}

function getPreviousEquivalentPoint(node, offset) {
	
	if (getNodeLength(node) == 0) {
		return null;
	}

	
	
	if (offset == 0
	&& node.parentNode
	&& isInlineNode(node)) {
		return [node.parentNode, getNodeIndex(node)];
	}

	
	
	
	if (0 <= offset - 1
	&& offset - 1 < node.childNodes.length
	&& getNodeLength(node.childNodes[offset - 1]) != 0
	&& isInlineNode(node.childNodes[offset - 1])) {
		return [node.childNodes[offset - 1], getNodeLength(node.childNodes[offset - 1])];
	}

	
	return null;
}

function getFirstEquivalentPoint(node, offset) {
	
	
	var prev;
	while (prev = getPreviousEquivalentPoint(node, offset)) {
		node = prev[0];
		offset = prev[1];
	}

	
	return [node, offset];
}

function getLastEquivalentPoint(node, offset) {
	
	
	var next;
	while (next = getNextEquivalentPoint(node, offset)) {
		node = next[0];
		offset = next[1];
	}

	
	return [node, offset];
}








function isBlockStartPoint(node, offset) {
	return (!node.parentNode && offset == 0)
		|| (0 <= offset - 1
		&& offset - 1 < node.childNodes.length
		&& isVisible(node.childNodes[offset - 1])
		&& (isBlockNode(node.childNodes[offset - 1])
		|| isHtmlElement(node.childNodes[offset - 1], "br")));
}




function isBlockEndPoint(node, offset) {
	return (!node.parentNode && offset == getNodeLength(node))
		|| (offset < node.childNodes.length
		&& isVisible(node.childNodes[offset])
		&& isBlockNode(node.childNodes[offset]));
}



function isBlockBoundaryPoint(node, offset) {
	return isBlockStartPoint(node, offset)
		|| isBlockEndPoint(node, offset);
}

function blockExtend(range) {
	
	
	var startNode = range.startContainer;
	var startOffset = range.startOffset;
	var endNode = range.endContainer;
	var endOffset = range.endOffset;

	
	
	
	var liAncestors = getAncestors(startNode).concat(startNode)
		.filter(function(ancestor) { return isHtmlElement(ancestor, "li") })
		.slice(-1);
	if (liAncestors.length) {
		startOffset = getNodeIndex(liAncestors[0]);
		startNode = liAncestors[0].parentNode;
	}

	
	
	if (!isBlockStartPoint(startNode, startOffset)) do {
		
		
		if (startOffset == 0) {
			startOffset = getNodeIndex(startNode);
			startNode = startNode.parentNode;

		
		} else {
			startOffset--;
		}

		
		
	} while (!isBlockBoundaryPoint(startNode, startOffset));

	
	
	while (startOffset == 0
	&& startNode.parentNode) {
		startOffset = getNodeIndex(startNode);
		startNode = startNode.parentNode;
	}

	
	
	
	var liAncestors = getAncestors(endNode).concat(endNode)
		.filter(function(ancestor) { return isHtmlElement(ancestor, "li") })
		.slice(-1);
	if (liAncestors.length) {
		endOffset = 1 + getNodeIndex(liAncestors[0]);
		endNode = liAncestors[0].parentNode;
	}

	
	
	if (!isBlockEndPoint(endNode, endOffset)) do {
		
		
		if (endOffset == getNodeLength(endNode)) {
			endOffset = 1 + getNodeIndex(endNode);
			endNode = endNode.parentNode;

		
		} else {
			endOffset++;
		}

		
		
	} while (!isBlockBoundaryPoint(endNode, endOffset));

	
	
	
	while (endOffset == getNodeLength(endNode)
	&& endNode.parentNode) {
		endOffset = 1 + getNodeIndex(endNode);
		endNode = endNode.parentNode;
	}

	
	
	var newRange = startNode.ownerDocument.createRange();
	newRange.setStart(startNode, startOffset);
	newRange.setEnd(endNode, endOffset);

	
	return newRange;
}

function followsLineBreak(node) {
	
	var offset = 0;

	
	while (!isBlockBoundaryPoint(node, offset)) {
		
		
		if (0 <= offset - 1
		&& offset - 1 < node.childNodes.length
		&& isVisible(node.childNodes[offset - 1])) {
			return false;
		}

		
		
		if (offset == 0
		|| !node.hasChildNodes()) {
			offset = getNodeIndex(node);
			node = node.parentNode;

		
		
		} else {
			node = node.childNodes[offset - 1];
			offset = getNodeLength(node);
		}
	}

	
	return true;
}

function precedesLineBreak(node) {
	
	var offset = getNodeLength(node);

	
	while (!isBlockBoundaryPoint(node, offset)) {
		
		if (offset < node.childNodes.length
		&& isVisible(node.childNodes[offset])) {
			return false;
		}

		
		
		if (offset == getNodeLength(node)
		|| !node.hasChildNodes()) {
			offset = 1 + getNodeIndex(node);
			node = node.parentNode;

		
		
		} else {
			node = node.childNodes[offset];
			offset = 0;
		}
	}

	
	return true;
}





function recordCurrentOverrides() {
	
	
	var overrides = [];

	
	
	if (getValueOverride("createlink") !== undefined) {
		overrides.push(["createlink", getValueOverride("createlink")]);
	}

	
	
	
	
	["bold", "italic", "strikethrough", "subscript", "superscript",
	"underline"].forEach(function(command) {
		if (getStateOverride(command) !== undefined) {
			overrides.push([command, getStateOverride(command)]);
		}
	});

	
	
	
	["fontname", "fontsize", "forecolor",
	"hilitecolor"].forEach(function(command) {
		if (getValueOverride(command) !== undefined) {
			overrides.push([command, getValueOverride(command)]);
		}
	});

	
	return overrides;
}

function recordCurrentStatesAndValues() {
	
	
	var overrides = [];

	
	
	var node = getAllEffectivelyContainedNodes(getActiveRange())
		.filter(isFormattableNode)[0];

	
	if (!node) {
		return overrides;
	}

	
	
	overrides.push(["createlink", getEffectiveCommandValue(node, "createlink")]);

	
	
	
	
	
	["bold", "italic", "strikethrough", "subscript", "superscript",
	"underline"].forEach(function(command) {
		if (commands[command].inlineCommandActivatedValues
		.indexOf(getEffectiveCommandValue(node, command)) != -1) {
			overrides.push([command, true]);
		} else {
			overrides.push([command, false]);
		}
	});

	
	
	["fontname", "fontsize", "forecolor", "hilitecolor"].forEach(function(command) {
		overrides.push([command, commands[command].value()]);
	});

	
	
	overrides.push(["fontsize", getEffectiveCommandValue(node, "fontsize")]);

	
	return overrides;
}

function restoreStatesAndValues(overrides) {
	
	
	var node = getAllEffectivelyContainedNodes(getActiveRange())
		.filter(isFormattableNode)[0];

	
	
	if (node) {
		for (var i = 0; i < overrides.length; i++) {
			var command = overrides[i][0];
			var override = overrides[i][1];

			
			
			
			if (typeof override == "boolean"
			&& myQueryCommandState(command) != override) {
				commands[command].action("");

			
			
			
			
			} else if (typeof override == "string"
			&& command != "createlink"
			&& command != "fontsize"
			&& !areEquivalentValues(command, myQueryCommandValue(command), override)) {
				commands[command].action(override);

			
			
			
			
			
			
			} else if (typeof override == "string"
			&& command == "createlink"
			&& (
				(
					getValueOverride("createlink") !== undefined
					&& getValueOverride("createlink") !== override
				) || (
					getValueOverride("createlink") === undefined
					&& getEffectiveCommandValue(node, "createlink") !== override
				)
			)) {
				commands.createlink.action(override);

			
			
			
			
			
			} else if (typeof override == "string"
			&& command == "fontsize"
			&& (
				(
					getValueOverride("fontsize") !== undefined
					&& getValueOverride("fontsize") !== override
				) || (
					getValueOverride("fontsize") === undefined
					&& !areLooselyEquivalentValues(command, getEffectiveCommandValue(node, "fontsize"), override)
				)
			)) {
				
				
				override = getLegacyFontSize(override);

				
				
				commands.fontsize.action(override);

			
			} else {
				continue;
			}

			
			
			node = getAllEffectivelyContainedNodes(getActiveRange())
				.filter(isFormattableNode)[0]
				|| node;
		}

	
	} else {
		for (var i = 0; i < overrides.length; i++) {
			var command = overrides[i][0];
			var override = overrides[i][1];

			
			
			if (typeof override == "boolean") {
				setStateOverride(command, override);
			}

			
			
			if (typeof override == "string") {
				setValueOverride(command, override);
			}
		}
	}
}







function deleteSelection(flags) {
	if (flags === undefined) {
		flags = {};
	}

	var blockMerging = "blockMerging" in flags ? Boolean(flags.blockMerging) : true;
	var stripWrappers = "stripWrappers" in flags ? Boolean(flags.stripWrappers) : true;
	var direction = "direction" in flags ? flags.direction : "forward";

	
	if (!getActiveRange()) {
		return;
	}

	
	canonicalizeWhitespace(getActiveRange().startContainer, getActiveRange().startOffset);

	
	canonicalizeWhitespace(getActiveRange().endContainer, getActiveRange().endOffset);

	
	
	var start = getLastEquivalentPoint(getActiveRange().startContainer, getActiveRange().startOffset);
	var startNode = start[0];
	var startOffset = start[1];

	
	
	var end = getFirstEquivalentPoint(getActiveRange().endContainer, getActiveRange().endOffset);
	var endNode = end[0];
	var endOffset = end[1];

	
	if (getPosition(endNode, endOffset, startNode, startOffset) !== "after") {
		
		
		
		
		
		
		
		
		
		
		if (direction == "forward") {
			if (getSelection().rangeCount) {
				getSelection().collapseToStart();
			}
			getActiveRange().collapse(true);

		
		} else {
			getSelection().collapseToEnd();
			getActiveRange().collapse(false);
		}

		
		return;
	}

	
	
	if (startNode.nodeType == Node.TEXT_NODE
	&& startOffset == 0) {
		startOffset = getNodeIndex(startNode);
		startNode = startNode.parentNode;
	}

	
	
	if (endNode.nodeType == Node.TEXT_NODE
	&& endOffset == getNodeLength(endNode)) {
		endOffset = 1 + getNodeIndex(endNode);
		endNode = endNode.parentNode;
	}

	
	
	getSelection().collapse(startNode, startOffset);
	getActiveRange().setStart(startNode, startOffset);

	
	getSelection().extend(endNode, endOffset);
	getActiveRange().setEnd(endNode, endOffset);

	
	var startBlock = getActiveRange().startContainer;

	
	
	while (inSameEditingHost(startBlock, startBlock.parentNode)
	&& isInlineNode(startBlock)) {
		startBlock = startBlock.parentNode;
	}

	
	
	
	if ((!isBlockNode(startBlock) && !isEditingHost(startBlock))
	|| !isAllowedChild("span", startBlock)
	|| isHtmlElement(startBlock, ["td", "th"])) {
		startBlock = null;
	}

	
	var endBlock = getActiveRange().endContainer;

	
	
	while (inSameEditingHost(endBlock, endBlock.parentNode)
	&& isInlineNode(endBlock)) {
		endBlock = endBlock.parentNode;
	}

	
	
	
	if ((!isBlockNode(endBlock) && !isEditingHost(endBlock))
	|| !isAllowedChild("span", endBlock)
	|| isHtmlElement(endBlock, ["td", "th"])) {
		endBlock = null;
	}

	
	var overrides = recordCurrentStatesAndValues();

	
	
	if (startNode == endNode
	&& isEditable(startNode)
	&& startNode.nodeType == Node.TEXT_NODE) {
		
		
		startNode.deleteData(startOffset, endOffset - startOffset);

		
		
		canonicalizeWhitespace(startNode, startOffset, false);

		
		
		if (direction == "forward") {
			if (getSelection().rangeCount) {
				getSelection().collapseToStart();
			}
			getActiveRange().collapse(true);

		
		} else {
			getSelection().collapseToEnd();
			getActiveRange().collapse(false);
		}

		
		restoreStatesAndValues(overrides);

		
		return;
	}

	
	
	
	if (isEditable(startNode)
	&& startNode.nodeType == Node.TEXT_NODE) {
		startNode.deleteData(startOffset, getNodeLength(startNode) - startOffset);
	}

	
	
	
	
	
	var nodeList = getContainedNodes(getActiveRange(),
		function(node) {
			return isEditable(node)
				&& !isHtmlElement(node, ["thead", "tbody", "tfoot", "tr", "th", "td"]);
		}
	);

	
	for (var i = 0; i < nodeList.length; i++) {
		var node = nodeList[i];

		
		var parent_ = node.parentNode;

		
		parent_.removeChild(node);

		
		
		
		if (![].some.call(getBlockNodeOf(parent_).childNodes, isVisible)
		&& (isEditable(parent_) || isEditingHost(parent_))) {
			parent_.appendChild(document.createElement("br"));
		}

		
		
		
		
		if (stripWrappers
		|| (!isAncestor(parent_, startNode) && parent_ != startNode)) {
			while (isEditable(parent_)
			&& isInlineNode(parent_)
			&& getNodeLength(parent_) == 0) {
				var grandparent = parent_.parentNode;
				grandparent.removeChild(parent_);
				parent_ = grandparent;
			}
		}
	}

	
	
	if (isEditable(endNode)
	&& endNode.nodeType == Node.TEXT_NODE) {
		endNode.deleteData(0, endOffset);
	}

	
	
	canonicalizeWhitespace(getActiveRange().startContainer, getActiveRange().startOffset, false);

	
	
	canonicalizeWhitespace(getActiveRange().endContainer, getActiveRange().endOffset, false);

	
	
	
	if (!blockMerging
	|| !startBlock
	|| !endBlock
	|| !inSameEditingHost(startBlock, endBlock)
	|| startBlock == endBlock) {
		
		
		if (direction == "forward") {
			if (getSelection().rangeCount) {
				getSelection().collapseToStart();
			}
			getActiveRange().collapse(true);

		
		} else {
			if (getSelection().rangeCount) {
				getSelection().collapseToEnd();
			}
			getActiveRange().collapse(false);
		}

		
		restoreStatesAndValues(overrides);

		
		return;
	}

	
	
	if (startBlock.children.length == 1
	&& isCollapsedBlockProp(startBlock.firstChild)) {
		startBlock.removeChild(startBlock.firstChild);
	}

	
	if (isAncestor(startBlock, endBlock)) {
		
		var referenceNode = endBlock;

		
		
		while (referenceNode.parentNode != startBlock) {
			referenceNode = referenceNode.parentNode;
		}

		
		
		
		getSelection().collapse(startBlock, getNodeIndex(referenceNode));
		getActiveRange().setStart(startBlock, getNodeIndex(referenceNode));
		getActiveRange().collapse(true);

		
		if (!endBlock.hasChildNodes()) {
			
			
			
			
			while (isEditable(endBlock)
			&& endBlock.parentNode.childNodes.length == 1
			&& endBlock.parentNode != startBlock) {
				var parent_ = endBlock;
				parent_.removeChild(endBlock);
				endBlock = parent_;
			}

			
			
			
			
			if (isEditable(endBlock)
			&& !isInlineNode(endBlock)
			&& isInlineNode(endBlock.previousSibling)
			&& isInlineNode(endBlock.nextSibling)) {
				endBlock.parentNode.insertBefore(document.createElement("br"), endBlock.nextSibling);
			}

			
			if (isEditable(endBlock)) {
				endBlock.parentNode.removeChild(endBlock);
			}

			
			restoreStatesAndValues(overrides);

			
			return;
		}

		
		
		if (!isInlineNode(endBlock.firstChild)) {
			restoreStatesAndValues(overrides);
			return;
		}

		
		var children = [];

		
		children.push(endBlock.firstChild);

		
		
		
		while (!isHtmlElement(children[children.length - 1], "br")
		&& isInlineNode(children[children.length - 1].nextSibling)) {
			children.push(children[children.length - 1].nextSibling);
		}

		
		var values = recordValues(children);

		
		
		while (children[0].parentNode != startBlock) {
			splitParent(children);
		}

		
		
		if (isEditable(children[0].previousSibling)
		&& isHtmlElement(children[0].previousSibling, "br")) {
			children[0].parentNode.removeChild(children[0].previousSibling);
		}

	
	} else if (isDescendant(startBlock, endBlock)) {
		
		
		getSelection().collapse(startBlock, getNodeLength(startBlock));
		getActiveRange().setStart(startBlock, getNodeLength(startBlock));
		getActiveRange().collapse(true);

		
		var referenceNode = startBlock;

		
		
		while (referenceNode.parentNode != endBlock) {
			referenceNode = referenceNode.parentNode;
		}

		
		
		if (isInlineNode(referenceNode.nextSibling)
		&& isHtmlElement(startBlock.lastChild, "br")) {
			startBlock.removeChild(startBlock.lastChild);
		}

		
		var nodesToMove = [];

		
		
		if (referenceNode.nextSibling
		&& !isBlockNode(referenceNode.nextSibling)) {
			nodesToMove.push(referenceNode.nextSibling);
		}

		
		
		
		if (nodesToMove.length
		&& !isHtmlElement(nodesToMove[nodesToMove.length - 1], "br")
		&& nodesToMove[nodesToMove.length - 1].nextSibling
		&& !isBlockNode(nodesToMove[nodesToMove.length - 1].nextSibling)) {
			nodesToMove.push(nodesToMove[nodesToMove.length - 1].nextSibling);
		}

		
		var values = recordValues(nodesToMove);

		
		
		nodesToMove.forEach(function(node) {
			movePreservingRanges(node, startBlock, -1);
		});

	
	} else {
		
		
		getSelection().collapse(startBlock, getNodeLength(startBlock));
		getActiveRange().setStart(startBlock, getNodeLength(startBlock));
		getActiveRange().collapse(true);

		
		
		if (isInlineNode(endBlock.firstChild)
		&& isHtmlElement(startBlock.lastChild, "br")) {
			startBlock.removeChild(startBlock.lastChild);
		}

		
		
		var values = recordValues([].slice.call(endBlock.childNodes));

		
		
		while (endBlock.hasChildNodes()) {
			movePreservingRanges(endBlock.firstChild, startBlock, -1);
		}

		
		
		
		while (!endBlock.hasChildNodes()) {
			var parent_ = endBlock.parentNode;
			parent_.removeChild(endBlock);
			endBlock = parent_;
		}
	}

	
	var ancestor = startBlock;

	
	
	
	
	while (getInclusiveAncestors(ancestor).some(function(node) {
		return inSameEditingHost(ancestor, node)
			&& (
				(isHtmlElement(node, "ol") && isHtmlElement(node.nextSibling, "ol"))
				|| (isHtmlElement(node, "ul") && isHtmlElement(node.nextSibling, "ul"))
			) && inSameEditingHost(ancestor, node.nextSibling);
	})) {
		
		
		
		while (!(
			isHtmlElement(ancestor, "ol")
			&& isHtmlElement(ancestor.nextSibling, "ol")
			&& inSameEditingHost(ancestor, ancestor.nextSibling)
		) && !(
			isHtmlElement(ancestor, "ul")
			&& isHtmlElement(ancestor.nextSibling, "ul")
			&& inSameEditingHost(ancestor, ancestor.nextSibling)
		)) {
			ancestor = ancestor.parentNode;
		}

		
		
		
		while (ancestor.nextSibling.hasChildNodes()) {
			movePreservingRanges(ancestor.nextSibling.firstChild, ancestor, -1);
		}

		
		ancestor.parentNode.removeChild(ancestor.nextSibling);
	}

	
	restoreValues(values);

	
	
	if (!startBlock.hasChildNodes()) {
		startBlock.appendChild(document.createElement("br"));
	}

	
	removeExtraneousLineBreaksAtTheEndOf(startBlock);

	
	restoreStatesAndValues(overrides);
}






function splitParent(nodeList) {
	
	var originalParent = nodeList[0].parentNode;

	
	
	if (!isEditable(originalParent)
	|| !originalParent.parentNode) {
		return;
	}

	
	
	if (nodeList.indexOf(originalParent.firstChild) != -1) {
		removeExtraneousLineBreaksBefore(originalParent);
	}

	
	
	
	var followsLineBreak_ = nodeList.indexOf(originalParent.firstChild) != -1
		&& followsLineBreak(originalParent);

	
	
	
	var precedesLineBreak_ = nodeList.indexOf(originalParent.lastChild) != -1
		&& precedesLineBreak(originalParent);

	
	
	if (nodeList.indexOf(originalParent.firstChild) == -1
	&& nodeList.indexOf(originalParent.lastChild) != -1) {
		
		
		
		for (var i = nodeList.length - 1; i >= 0; i--) {
			movePreservingRanges(nodeList[i], originalParent.parentNode, 1 + getNodeIndex(originalParent));
		}

		
		
		
		
		if (precedesLineBreak_
		&& !precedesLineBreak(nodeList[nodeList.length - 1])) {
			nodeList[nodeList.length - 1].parentNode.insertBefore(document.createElement("br"), nodeList[nodeList.length - 1].nextSibling);
		}

		
		removeExtraneousLineBreaksAtTheEndOf(originalParent);

		
		return;
	}

	
	if (nodeList.indexOf(originalParent.firstChild) == -1) {
		
		
		var clonedParent = originalParent.cloneNode(false);

		
		originalParent.removeAttribute("id");

		
		
		originalParent.parentNode.insertBefore(clonedParent, originalParent);

		
		
		
		while (nodeList[0].previousSibling) {
			movePreservingRanges(originalParent.firstChild, clonedParent, clonedParent.childNodes.length);
		}
	}

	
	
	for (var i = 0; i < nodeList.length; i++) {
		movePreservingRanges(nodeList[i], originalParent.parentNode, getNodeIndex(originalParent));
	}

	
	
	
	if (followsLineBreak_
	&& !followsLineBreak(nodeList[0])) {
		nodeList[0].parentNode.insertBefore(document.createElement("br"), nodeList[0]);
	}

	
	
	
	
	if (isInlineNode(nodeList[nodeList.length - 1])
	&& !isHtmlElement(nodeList[nodeList.length - 1], "br")
	&& isHtmlElement(originalParent.firstChild, "br")
	&& !isInlineNode(originalParent)) {
		originalParent.removeChild(originalParent.firstChild);
	}

	
	if (!originalParent.hasChildNodes()) {
		
		originalParent.parentNode.removeChild(originalParent);

		
		
		
		
		if (precedesLineBreak_
		&& !precedesLineBreak(nodeList[nodeList.length - 1])) {
			nodeList[nodeList.length - 1].parentNode.insertBefore(document.createElement("br"), nodeList[nodeList.length - 1].nextSibling);
		}

	
	} else {
		removeExtraneousLineBreaksBefore(originalParent);
	}

	
	
	
	if (!nodeList[nodeList.length - 1].nextSibling
	&& nodeList[nodeList.length - 1].parentNode) {
		removeExtraneousLineBreaksAtTheEndOf(nodeList[nodeList.length - 1].parentNode);
	}
}




function removePreservingDescendants(node) {
	if (node.hasChildNodes()) {
		splitParent([].slice.call(node.childNodes));
	} else {
		node.parentNode.removeChild(node);
	}
}






function canonicalSpaceSequence(n, nonBreakingStart, nonBreakingEnd) {
	
	if (n == 0) {
		return "";
	}

	
	
	if (n == 1 && !nonBreakingStart && !nonBreakingEnd) {
		return " ";
	}

	
	if (n == 1) {
		return "\xa0";
	}

	
	var buffer = "";

	
	
	var repeatedPair;
	if (nonBreakingStart) {
		repeatedPair = "\xa0 ";
	} else {
		repeatedPair = " \xa0";
	}

	
	
	while (n > 3) {
		buffer += repeatedPair;
		n -= 2;
	}

	
	
	if (n == 3) {
		buffer +=
			!nonBreakingStart && !nonBreakingEnd ? " \xa0 "
			: nonBreakingStart && !nonBreakingEnd ? "\xa0\xa0 "
			: !nonBreakingStart && nonBreakingEnd ? " \xa0\xa0"
			: nonBreakingStart && nonBreakingEnd ? "\xa0 \xa0"
			: "impossible";

	
	
	} else {
		buffer +=
			!nonBreakingStart && !nonBreakingEnd ? "\xa0 "
			: nonBreakingStart && !nonBreakingEnd ? "\xa0 "
			: !nonBreakingStart && nonBreakingEnd ? " \xa0"
			: nonBreakingStart && nonBreakingEnd ? "\xa0\xa0"
			: "impossible";
	}

	
	return buffer;
}

function canonicalizeWhitespace(node, offset, fixCollapsedSpace) {
	if (fixCollapsedSpace === undefined) {
		
		
		fixCollapsedSpace = true;
	}

	
	if (!isEditable(node) && !isEditingHost(node)) {
		return;
	}

	
	var startNode = node;
	var startOffset = offset;

	
	while (true) {
		
		
		
		if (0 <= startOffset - 1
		&& inSameEditingHost(startNode, startNode.childNodes[startOffset - 1])) {
			startNode = startNode.childNodes[startOffset - 1];
			startOffset = getNodeLength(startNode);

		
		
		
		
		} else if (startOffset == 0
		&& !followsLineBreak(startNode)
		&& inSameEditingHost(startNode, startNode.parentNode)) {
			startOffset = getNodeIndex(startNode);
			startNode = startNode.parentNode;

		
		
		
		
		
		} else if (startNode.nodeType == Node.TEXT_NODE
		&& ["pre", "pre-wrap"].indexOf(getComputedStyle(startNode.parentNode).whiteSpace) == -1
		&& startOffset != 0
		&& /[ \xa0]/.test(startNode.data[startOffset - 1])) {
			startOffset--;

		
		} else {
			break;
		}
	}

	
	var endNode = startNode;
	var endOffset = startOffset;

	
	var length = 0;

	
	
	var collapseSpaces = startOffset == 0 && followsLineBreak(startNode);

	
	while (true) {
		
		
		if (endOffset < endNode.childNodes.length
		&& inSameEditingHost(endNode, endNode.childNodes[endOffset])) {
			endNode = endNode.childNodes[endOffset];
			endOffset = 0;

		
		
		
		
		} else if (endOffset == getNodeLength(endNode)
		&& !precedesLineBreak(endNode)
		&& inSameEditingHost(endNode, endNode.parentNode)) {
			endOffset = 1 + getNodeIndex(endNode);
			endNode = endNode.parentNode;

		
		
		
		
		} else if (endNode.nodeType == Node.TEXT_NODE
		&& ["pre", "pre-wrap"].indexOf(getComputedStyle(endNode.parentNode).whiteSpace) == -1
		&& endOffset != getNodeLength(endNode)
		&& /[ \xa0]/.test(endNode.data[endOffset])) {
			
			
			
			
			if (fixCollapsedSpace
			&& collapseSpaces
			&& " " == endNode.data[endOffset]) {
				endNode.deleteData(endOffset, 1);
				continue;
			}

			
			
			collapseSpaces = " " == endNode.data[endOffset];

			
			endOffset++;

			
			length++;

		
		} else {
			break;
		}
	}

	
	
	if (fixCollapsedSpace) {
		while (getPosition(startNode, startOffset, endNode, endOffset) == "before") {
			
			
			
			if (0 <= endOffset - 1
			&& endOffset - 1 < endNode.childNodes.length
			&& inSameEditingHost(endNode, endNode.childNodes[endOffset - 1])) {
				endNode = endNode.childNodes[endOffset - 1];
				endOffset = getNodeLength(endNode);

			
			
			
			} else if (endOffset == 0
			&& inSameEditingHost(endNode, endNode.parentNode)) {
				endOffset = getNodeIndex(endNode);
				endNode = endNode.parentNode;

			
			
			
			
			} else if (endNode.nodeType == Node.TEXT_NODE
			&& ["pre", "pre-wrap"].indexOf(getComputedStyle(endNode.parentNode).whiteSpace) == -1
			&& endOffset == getNodeLength(endNode)
			&& endNode.data[endNode.data.length - 1] == " "
			&& precedesLineBreak(endNode)) {
				
				endOffset--;

				
				length--;

				
				endNode.deleteData(endOffset, 1);

			
			} else {
				break;
			}
		}
	}

	
	
	
	
	
	var replacementWhitespace = canonicalSpaceSequence(length,
		startOffset == 0 && followsLineBreak(startNode),
		endOffset == getNodeLength(endNode) && precedesLineBreak(endNode));

	
	while (getPosition(startNode, startOffset, endNode, endOffset) == "before") {
		
		
		if (startOffset < startNode.childNodes.length) {
			startNode = startNode.childNodes[startOffset];
			startOffset = 0;

		
		
		
		} else if (startNode.nodeType != Node.TEXT_NODE
		|| startOffset == getNodeLength(startNode)) {
			startOffset = 1 + getNodeIndex(startNode);
			startNode = startNode.parentNode;

		
		} else {
			
			
			var element = replacementWhitespace[0];
			replacementWhitespace = replacementWhitespace.slice(1);

			
			
			if (element != startNode.data[startOffset]) {
				
				startNode.insertData(startOffset, element);

				
				startNode.deleteData(startOffset + 1, 1);
			}

			
			startOffset++;
		}
	}
}






function indentNodes(nodeList) {
	
	if (!nodeList.length) {
		return;
	}

	
	var firstNode = nodeList[0];

	
	if (isHtmlElement(firstNode.parentNode, ["OL", "UL"])) {
		
		var tag = firstNode.parentNode.tagName;

		
		
		
		
		wrap(nodeList,
			function(node) { return isHtmlElement(node, tag) },
			function() { return firstNode.ownerDocument.createElement(tag) });

		
		return;
	}

	
	
	
	
	var newParent = wrap(nodeList,
		function(node) { return isSimpleIndentationElement(node) },
		function() { return firstNode.ownerDocument.createElement("blockquote") });

	
	fixDisallowedAncestors(newParent);
}

function outdentNode(node) {
	
	if (!isEditable(node)) {
		return;
	}

	
	
	if (isSimpleIndentationElement(node)) {
		removePreservingDescendants(node);
		return;
	}

	
	if (isIndentationElement(node)) {
		
		node.removeAttribute("dir");

		
		node.style.margin = "";
		node.style.padding = "";
		node.style.border = "";
		if (node.getAttribute("style") == ""
		
		|| node.getAttribute("style") == "border-width: initial; border-color: initial; ") {
			node.removeAttribute("style");
		}

		
		setTagName(node, "div");

		
		return;
	}

	
	var currentAncestor = node.parentNode;

	
	var ancestorList = [];

	
	
	
	while (isEditable(currentAncestor)
	&& currentAncestor.nodeType == Node.ELEMENT_NODE
	&& !isSimpleIndentationElement(currentAncestor)
	&& !isHtmlElement(currentAncestor, ["ol", "ul"])) {
		ancestorList.push(currentAncestor);
		currentAncestor = currentAncestor.parentNode;
	}

	
	if (!isEditable(currentAncestor)
	|| !isSimpleIndentationElement(currentAncestor)) {
		
		currentAncestor = node.parentNode;

		
		ancestorList = [];

		
		
		
		while (isEditable(currentAncestor)
		&& currentAncestor.nodeType == Node.ELEMENT_NODE
		&& !isIndentationElement(currentAncestor)
		&& !isHtmlElement(currentAncestor, ["ol", "ul"])) {
			ancestorList.push(currentAncestor);
			currentAncestor = currentAncestor.parentNode;
		}
	}

	
	
	if (isHtmlElement(node, ["OL", "UL"])
	&& (!isEditable(currentAncestor)
	|| !isIndentationElement(currentAncestor))) {
		
		
		node.removeAttribute("reversed");
		node.removeAttribute("start");
		node.removeAttribute("type");

		
		var children = [].slice.call(node.childNodes);

		
		
		if (node.attributes.length
		&& !isHtmlElement(node.parentNode, ["OL", "UL"])) {
			setTagName(node, "div");

		
		} else {
			
			
			var values = recordValues([].slice.call(node.childNodes));

			
			removePreservingDescendants(node);

			
			restoreValues(values);
		}

		
		for (var i = 0; i < children.length; i++) {
			fixDisallowedAncestors(children[i]);
		}

		
		return;
	}

	
	
	if (!isEditable(currentAncestor)
	|| !isIndentationElement(currentAncestor)) {
		return;
	}

	
	ancestorList.push(currentAncestor);

	
	var originalAncestor = currentAncestor;

	
	while (ancestorList.length) {
		
		
		
		currentAncestor = ancestorList.pop();

		
		
		var target = node.parentNode == currentAncestor
			? node
			: ancestorList[ancestorList.length - 1];

		
		
		if (isInlineNode(target)
		&& !isHtmlElement(target, "BR")
		&& isHtmlElement(target.nextSibling, "BR")) {
			target.parentNode.removeChild(target.nextSibling);
		}

		
		
		var precedingSiblings = [].slice.call(currentAncestor.childNodes, 0, getNodeIndex(target));
		var followingSiblings = [].slice.call(currentAncestor.childNodes, 1 + getNodeIndex(target));

		
		indentNodes(precedingSiblings);

		
		indentNodes(followingSiblings);
	}

	
	outdentNode(originalAncestor);
}






function toggleLists(tagName) {
	
	
	var mode = getSelectionListState() == tagName ? "disable" : "enable";

	var range = getActiveRange();
	tagName = tagName.toUpperCase();

	
	
	var otherTagName = tagName == "OL" ? "UL" : "OL";

	
	
	
	
	
	var items = [];
	(function(){
		for (
			var ancestorContainer = range.endContainer;
			ancestorContainer != range.commonAncestorContainer;
			ancestorContainer = ancestorContainer.parentNode
		) {
			if (isHtmlElement(ancestorContainer, "li")) {
				items.unshift(ancestorContainer);
			}
		}
		for (
			var ancestorContainer = range.startContainer;
			ancestorContainer;
			ancestorContainer = ancestorContainer.parentNode
		) {
			if (isHtmlElement(ancestorContainer, "li")) {
				items.unshift(ancestorContainer);
			}
		}
	})();

	
	items.forEach(normalizeSublists);

	
	var newRange = blockExtend(range);

	
	
	
	if (mode == "enable") {
		getAllContainedNodes(newRange, function(node) {
			return isEditable(node)
				&& isHtmlElement(node, otherTagName);
		}).forEach(function(list) {
			
			
			if ((isEditable(list.previousSibling) && isHtmlElement(list.previousSibling, tagName))
			|| (isEditable(list.nextSibling) && isHtmlElement(list.nextSibling, tagName))) {
				
				var children = [].slice.call(list.childNodes);

				
				
				var values = recordValues(children);

				
				splitParent(children);

				
				
				wrap(children, function(node) { return isHtmlElement(node, tagName) });

				
				restoreValues(values);

			
			} else {
				setTagName(list, tagName);
			}
		});
	}

	
	
	
	
	
	
	
	var nodeList = getContainedNodes(newRange, function(node) {
		return isEditable(node)
		&& !isIndentationElement(node)
		&& (isHtmlElement(node, ["OL", "UL"])
		|| isHtmlElement(node.parentNode, ["OL", "UL"])
		|| isAllowedChild(node, "li"));
	});

	
	
	if (mode == "enable") {
		nodeList = nodeList.filter(function(node) {
			return !isHtmlElement(node, ["ol", "ul"])
				|| isHtmlElement(node.parentNode, ["ol", "ul"]);
		});
	}

	
	if (mode == "disable") {
		while (nodeList.length) {
			
			var sublist = [];

			
			
			sublist.push(nodeList.shift());

			
			
			
			if (isHtmlElement(sublist[0], tagName)) {
				outdentNode(sublist[0]);
				continue;
			}

			
			
			
			
			while (nodeList.length
			&& nodeList[0] == sublist[sublist.length - 1].nextSibling
			&& !isHtmlElement(nodeList[0], tagName)) {
				sublist.push(nodeList.shift());
			}

			
			var values = recordValues(sublist);

			
			splitParent(sublist);

			
			for (var i = 0; i < sublist.length; i++) {
				fixDisallowedAncestors(sublist[i]);
			}

			
			restoreValues(values);
		}

	
	} else {
		while (nodeList.length) {
			
			var sublist = [];

			
			
			while (!sublist.length
			|| (nodeList.length
			&& nodeList[0] == sublist[sublist.length - 1].nextSibling)) {
				
				
				
				if (isHtmlElement(nodeList[0], ["p", "div"])) {
					sublist.push(setTagName(nodeList[0], "li"));
					nodeList.shift();

				
				
				} else if (isHtmlElement(nodeList[0], ["li", "ol", "ul"])) {
					sublist.push(nodeList.shift());

				
				} else {
					
					var nodesToWrap = [];

					
					
					
					
					
					
					while (!nodesToWrap.length
					|| (nodeList.length
					&& nodeList[0] == nodesToWrap[nodesToWrap.length - 1].nextSibling
					&& isInlineNode(nodeList[0])
					&& isInlineNode(nodesToWrap[nodesToWrap.length - 1])
					&& !isHtmlElement(nodesToWrap[nodesToWrap.length - 1], "br"))) {
						nodesToWrap.push(nodeList.shift());
					}

					
					
					
					sublist.push(wrap(nodesToWrap,
						undefined,
						function() { return document.createElement("li") }));
				}
			}

			
			
			
			if (isHtmlElement(sublist[0].parentNode, tagName)
			|| sublist.every(function(node) { return isHtmlElement(node, ["ol", "ul"]) })) {
				continue;
			}

			
			
			if (isHtmlElement(sublist[0].parentNode, otherTagName)) {
				
				
				var values = recordValues(sublist);

				
				splitParent(sublist);

				
				
				
				
				wrap(sublist,
					function(node) { return isHtmlElement(node, tagName) },
					function() { return document.createElement(tagName) });

				
				restoreValues(values);

				
				continue;
			}

			
			
			
			
			
			fixDisallowedAncestors(wrap(sublist,
				function(node) { return isHtmlElement(node, tagName) },
				function() {
					
					
					
					
					
					if (!isEditable(sublist[0].parentNode)
					|| !isSimpleIndentationElement(sublist[0].parentNode)
					|| !isEditable(sublist[0].parentNode.previousSibling)
					|| !isHtmlElement(sublist[0].parentNode.previousSibling, tagName)) {
						return document.createElement(tagName);
					}

					
					
					var list = sublist[0].parentNode.previousSibling;

					
					normalizeSublists(list.lastChild);

					
					
					
					
					if (!isEditable(list.lastChild)
					|| !isHtmlElement(list.lastChild, tagName)) {
						list.appendChild(document.createElement(tagName));
					}

					
					return list.lastChild;
				}
			));
		}
	}
}






function justifySelection(alignment) {
	
	var newRange = blockExtend(globalRange);

	
	
	
	
	var elementList = getAllContainedNodes(newRange, function(node) {
		return node.nodeType == Node.ELEMENT_NODE
			&& isEditable(node)
			
			&& (
				node.hasAttribute("align")
				|| node.style.textAlign != ""
				|| isHtmlElement(node, "center")
			);
	});

	
	for (var i = 0; i < elementList.length; i++) {
		var element = elementList[i];

		
		
		element.removeAttribute("align");

		
		
		element.style.textAlign = "";
		if (element.getAttribute("style") == "") {
			element.removeAttribute("style");
		}

		
		
		if (isHtmlElement(element, ["div", "span", "center"])
		&& !element.attributes.length) {
			removePreservingDescendants(element);
		}

		
		
		if (isHtmlElement(element, "center")
		&& element.attributes.length) {
			setTagName(element, "div");
		}
	}

	
	newRange = blockExtend(globalRange);

	
	var nodeList = [];

	
	
	
	
	nodeList = getContainedNodes(newRange, function(node) {
		return isEditable(node)
			&& isAllowedChild(node, "div")
			&& getAlignmentValue(node) != alignment;
	});

	
	while (nodeList.length) {
		
		var sublist = [];

		
		sublist.push(nodeList.shift());

		
		
		
		while (nodeList.length
		&& nodeList[0] == sublist[sublist.length - 1].nextSibling) {
			sublist.push(nodeList.shift());
		}

		
		
		
		
		
		
		
		
		
		
		
		
		
		wrap(sublist,
			function(node) {
				return isHtmlElement(node, "div")
					&& [].every.call(node.attributes, function(attr) {
						return (attr.name == "align" && attr.value.toLowerCase() == alignment)
							|| (attr.name == "style" && node.style.length == 1 && node.style.textAlign == alignment);
					});
			},
			function() {
				var newParent = document.createElement("div");
				newParent.setAttribute("style", "text-align: " + alignment);
				return newParent;
			}
		);
	}
}






var autolinkableUrlRegexp =
	
	
	
	
	
	"([a-zA-Z][a-zA-Z0-9+.-]*://|mailto:)"
	
	+ "[^ \t\n\f\r]*"
	
	+ "[^!\"'(),\\-.:;<>[\\]`{}]";











var validEmailRegexp =
	"[a-zA-Z0-9!#$%&'*+\\-/=?^_`{|}~.]+@[a-zA-Z0-9-]+(\.[a-zA-Z0-9-]+)*";

function autolink(node, endOffset) {
	
	
	while (getPreviousEquivalentPoint(node, endOffset)) {
		var prev = getPreviousEquivalentPoint(node, endOffset);
		node = prev[0];
		endOffset = prev[1];
	}

	
	
	if (node.nodeType != Node.TEXT_NODE
	|| getAncestors(node).some(function(ancestor) { return isHtmlElement(ancestor, "a") })) {
		return;
	}

	
	
	var search = /[^ \t\n\f\r]*$/.exec(node.substringData(0, endOffset))[0];

	
	if (new RegExp(autolinkableUrlRegexp).test(search)) {
		
		
		while (!(new RegExp(autolinkableUrlRegexp + "$").test(node.substringData(0, endOffset)))) {
			endOffset--;
		}

		
		
		var startOffset = new RegExp(autolinkableUrlRegexp + "$").exec(node.substringData(0, endOffset)).index;

		
		
		var href = node.substringData(startOffset, endOffset - startOffset);

	
	} else if (new RegExp(validEmailRegexp).test(search)) {
		
		
		while (!(new RegExp(validEmailRegexp + "$").test(node.substringData(0, endOffset)))) {
			endOffset--;
		}

		
		
		var startOffset = new RegExp(validEmailRegexp + "$").exec(node.substringData(0, endOffset)).index;

		
		
		var href = "mailto:" + node.substringData(startOffset, endOffset - startOffset);

	
	} else {
		return;
	}

	
	var originalRange = getActiveRange();

	
	
	var newRange = document.createRange();
	newRange.setStart(node, startOffset);
	newRange.setEnd(node, endOffset);
	getSelection().removeAllRanges();
	getSelection().addRange(newRange);
	globalRange = newRange;

	
	commands.createlink.action(href);

	
	getSelection().removeAllRanges();
	getSelection().addRange(originalRange);
	globalRange = originalRange;
}



commands["delete"] = {
	preservesOverrides: true,
	action: function() {
		
		
		if (!getActiveRange().collapsed) {
			deleteSelection();
			return true;
		}

		
		canonicalizeWhitespace(getActiveRange().startContainer, getActiveRange().startOffset);

		
		var node = getActiveRange().startContainer;
		var offset = getActiveRange().startOffset;

		
		while (true) {
			
			
			if (offset == 0
			&& isEditable(node.previousSibling)
			&& isInvisible(node.previousSibling)) {
				node.parentNode.removeChild(node.previousSibling);

			
			
			
			} else if (0 <= offset - 1
			&& offset - 1 < node.childNodes.length
			&& isEditable(node.childNodes[offset - 1])
			&& isInvisible(node.childNodes[offset - 1])) {
				node.removeChild(node.childNodes[offset - 1]);
				offset--;

			
			
			
			} else if ((offset == 0
			&& isInlineNode(node))
			|| isInvisible(node)) {
				offset = getNodeIndex(node);
				node = node.parentNode;

			
			
			
			} else if (0 <= offset - 1
			&& offset - 1 < node.childNodes.length
			&& isEditable(node.childNodes[offset - 1])
			&& isHtmlElement(node.childNodes[offset - 1], "a")) {
				removePreservingDescendants(node.childNodes[offset - 1]);
				return true;

			
			
			
			} else if (0 <= offset - 1
			&& offset - 1 < node.childNodes.length
			&& !isBlockNode(node.childNodes[offset - 1])
			&& !isHtmlElement(node.childNodes[offset - 1], ["br", "img"])) {
				node = node.childNodes[offset - 1];
				offset = getNodeLength(node);

			
			} else {
				break;
			}
		}

		
		
		
		if ((node.nodeType == Node.TEXT_NODE
		&& offset != 0)
		|| (isBlockNode(node)
		&& 0 <= offset - 1
		&& offset - 1 < node.childNodes.length
		&& isHtmlElement(node.childNodes[offset - 1], ["br", "hr", "img"]))) {
			
			getSelection().collapse(node, offset);
			getActiveRange().setEnd(node, offset);

			
			
			getSelection().extend(node, offset - 1);
			getActiveRange().setStart(node, offset - 1);

			
			deleteSelection();

			
			return true;
		}

		
		if (isInlineNode(node)) {
			return true;
		}

		
		
		if (isHtmlElement(node, ["li", "dt", "dd"])
		&& node == node.parentNode.firstChild
		&& offset == 0) {
			
			
			
			var items = [];
			for (var ancestor = node.parentNode; ancestor; ancestor = ancestor.parentNode) {
				if (isHtmlElement(ancestor, "li")) {
					items.unshift(ancestor);
				}
			}

			
			for (var i = 0; i < items.length; i++) {
				normalizeSublists(items[i]);
			}

			
			
			var values = recordValues([node]);

			
			splitParent([node]);

			
			restoreValues(values);

			
			
			
			
			if (isHtmlElement(node, ["dd", "dt"])
			&& getAncestors(node).every(function(ancestor) {
				return !inSameEditingHost(node, ancestor)
					|| !isAllowedChild(node, ancestor)
			})) {
				node = setTagName(node, defaultSingleLineContainerName);
			}

			
			fixDisallowedAncestors(node);

			
			return true;
		}

		
		var startNode = node;
		var startOffset = offset;

		
		while (true) {
			
			
			if (startOffset == 0) {
				startOffset = getNodeIndex(startNode);
				startNode = startNode.parentNode;

			
			
			
			} else if (0 <= startOffset - 1
			&& startOffset - 1 < startNode.childNodes.length
			&& isEditable(startNode.childNodes[startOffset - 1])
			&& isInvisible(startNode.childNodes[startOffset - 1])) {
				startNode.removeChild(startNode.childNodes[startOffset - 1]);
				startOffset--;

			
			} else {
				break;
			}
		}

		
		
		if (offset == 0
		&& getAncestors(node).concat(node).filter(function(ancestor) {
			return isEditable(ancestor)
				&& inSameEditingHost(ancestor, node)
				&& isIndentationElement(ancestor);
		}).length) {
			
			
			var newRange = document.createRange();
			newRange.setStart(node, 0);
			newRange = blockExtend(newRange);

			
			
			
			
			
			
			var nodeList = getContainedNodes(newRange, function(currentNode) {
				return isEditable(currentNode)
					&& !hasEditableDescendants(currentNode);
			});

			
			for (var i = 0; i < nodeList.length; i++) {
				outdentNode(nodeList[i]);
			}

			
			return true;
		}

		
		
		if (isHtmlElement(startNode.childNodes[startOffset], "table")) {
			return true;
		}

		
		
		if (0 <= startOffset - 1
		&& startOffset - 1 < startNode.childNodes.length
		&& isHtmlElement(startNode.childNodes[startOffset - 1], "table")) {
			
			
			getSelection().collapse(startNode, startOffset - 1);
			getActiveRange().setStart(startNode, startOffset - 1);

			
			
			getSelection().extend(startNode, startOffset);
			getActiveRange().setEnd(startNode, startOffset);

			
			return true;
		}

		
		
		
		if (offset == 0
		&& (isHtmlElement(startNode.childNodes[startOffset - 1], "hr")
			|| (
				isHtmlElement(startNode.childNodes[startOffset - 1], "br")
				&& (
					isHtmlElement(startNode.childNodes[startOffset - 1].previousSibling, "br")
					|| !isInlineNode(startNode.childNodes[startOffset - 1].previousSibling)
				)
			)
		)) {
			
			
			getSelection().collapse(startNode, startOffset - 1);
			getActiveRange().setStart(startNode, startOffset - 1);

			
			
			getSelection().extend(startNode, startOffset);
			getActiveRange().setEnd(startNode, startOffset);

			
			deleteSelection();

			
			getSelection().collapse(node, offset);
			getActiveRange().setStart(node, offset);
			getActiveRange().collapse(true);

			
			return true;
		}

		
		
		
		if (isHtmlElement(startNode.childNodes[startOffset], ["li", "dt", "dd"])
		&& isInlineNode(startNode.childNodes[startOffset].firstChild)
		&& startOffset != 0) {
			
			
			var previousItem = startNode.childNodes[startOffset - 1];

			
			
			
			if (isInlineNode(previousItem.lastChild)
			&& !isHtmlElement(previousItem.lastChild, "br")) {
				previousItem.appendChild(document.createElement("br"));
			}

			
			
			
			if (isInlineNode(previousItem.lastChild)) {
				previousItem.appendChild(document.createElement("br"));
			}
		}

		
		
		if (isHtmlElement(startNode.childNodes[startOffset], ["li", "dt", "dd"])
		&& isHtmlElement(startNode.childNodes[startOffset].previousSibling, ["li", "dt", "dd"])) {
			
			
			
			
			
			var originalRange = getActiveRange().cloneRange();
			extraRanges.push(originalRange);

			
			startNode = startNode.childNodes[startOffset - 1];

			
			startOffset = getNodeLength(startNode);

			
			node = startNode.nextSibling;

			
			
			getSelection().collapse(startNode, startOffset);
			getActiveRange().setStart(startNode, startOffset);

			
			getSelection().extend(node, 0);
			getActiveRange().setEnd(node, 0);

			
			deleteSelection();

			
			getSelection().removeAllRanges();

			
			
			getSelection().addRange(originalRange);
			getActiveRange().setStart(originalRange.startContainer, originalRange.startOffset);
			getActiveRange().setEnd(originalRange.endContainer, originalRange.endOffset);

			
			extraRanges.pop();
			return true;
		}

		
		while (0 <= startOffset - 1
		&& startOffset - 1 < startNode.childNodes.length) {
			
			
			
			if (isEditable(startNode.childNodes[startOffset - 1])
			&& isInvisible(startNode.childNodes[startOffset - 1])) {
				startNode.removeChild(startNode.childNodes[startOffset - 1]);
				startOffset--;

			
			
			} else {
				startNode = startNode.childNodes[startOffset - 1];
				startOffset = getNodeLength(startNode);
			}
		}

		
		
		getSelection().collapse(startNode, startOffset);
		getActiveRange().setStart(startNode, startOffset);

		
		getSelection().extend(node, offset);
		getActiveRange().setEnd(node, offset);

		
		deleteSelection({direction: "backward"});

		
		return true;
	}
};






var formattableBlockNames = ["address", "dd", "div", "dt", "h1", "h2", "h3",
	"h4", "h5", "h6", "p", "pre"];

commands.formatblock = {
	preservesOverrides: true,
	action: function(value) {
		
		
		if (/^<.*>$/.test(value)) {
			value = value.slice(1, -1);
		}

		
		value = value.toLowerCase();

		
		if (formattableBlockNames.indexOf(value) == -1) {
			return false;
		}

		
		var newRange = blockExtend(getActiveRange());

		
		
		
		
		
		
		
		var nodeList = getContainedNodes(newRange, function(node) {
			return isEditable(node)
				&& (isNonListSingleLineContainer(node)
				|| isAllowedChild(node, "p")
				|| isHtmlElement(node, ["dd", "dt"]))
				&& !getDescendants(node).some(isProhibitedParagraphChild);
		});

		
		var values = recordValues(nodeList);

		
		
		
		
		
		for (var i = 0; i < nodeList.length; i++) {
			var node = nodeList[i];
			while (getAncestors(node).some(function(ancestor) {
				return isEditable(ancestor)
					&& inSameEditingHost(ancestor, node)
					&& isHtmlElement(ancestor, formattableBlockNames)
					&& !getDescendants(ancestor).some(isProhibitedParagraphChild);
			})) {
				splitParent([node]);
			}
		}

		
		restoreValues(values);

		
		while (nodeList.length) {
			var sublist;

			
			
			if (isSingleLineContainer(nodeList[0])) {
				
				
				sublist = [].slice.call(nodeList[0].childNodes);

				
				
				var values = recordValues(sublist);

				
				
				removePreservingDescendants(nodeList[0]);

				
				restoreValues(values);

				
				nodeList.shift();

			
			} else {
				
				sublist = [];

				
				
				sublist.push(nodeList.shift());

				
				
				
				
				
				
				while (nodeList.length
				&& nodeList[0] == sublist[sublist.length - 1].nextSibling
				&& !isSingleLineContainer(nodeList[0])
				&& !isHtmlElement(sublist[sublist.length - 1], "BR")) {
					sublist.push(nodeList.shift());
				}
			}

			
			
			
			
			
			
			fixDisallowedAncestors(wrap(sublist,
				["div", "p"].indexOf(value) == - 1
					? function(node) { return isHtmlElement(node, value) && !node.attributes.length }
					: function() { return false },
				function() { return document.createElement(value) }));
		}

		
		return true;
	}, indeterm: function() {
		
		if (!getActiveRange()) {
			return false;
		}

		
		var newRange = blockExtend(getActiveRange());

		
		
		var nodeList = getAllContainedNodes(newRange, function(node) {
			return isVisible(node)
				&& isEditable(node)
				&& !node.hasChildNodes();
		});

		
		if (!nodeList.length) {
			return false;
		}

		
		var type = null;

		
		for (var i = 0; i < nodeList.length; i++) {
			var node = nodeList[i];

			
			
			
			while (isEditable(node.parentNode)
			&& inSameEditingHost(node, node.parentNode)
			&& !isHtmlElement(node, formattableBlockNames)) {
				node = node.parentNode;
			}

			
			var currentType = "";

			
			
			
			
			if (isEditable(node)
			&& isHtmlElement(node, formattableBlockNames)
			&& !getDescendants(node).some(isProhibitedParagraphChild)) {
				currentType = node.tagName;
			}

			
			if (type === null) {
				type = currentType;

			
			} else if (type != currentType) {
				return true;
			}
		}

		
		return false;
	}, value: function() {
		
		if (!getActiveRange()) {
			return "";
		}

		
		var newRange = blockExtend(getActiveRange());

		
		
		
		var nodes = getAllContainedNodes(newRange, function(node) {
			return isVisible(node)
				&& isEditable(node)
				&& !node.hasChildNodes();
		});
		if (!nodes.length) {
			return "";
		}
		var node = nodes[0];

		
		
		
		while (isEditable(node.parentNode)
		&& inSameEditingHost(node, node.parentNode)
		&& !isHtmlElement(node, formattableBlockNames)) {
			node = node.parentNode;
		}

		
		
		
		
		if (isEditable(node)
		&& isHtmlElement(node, formattableBlockNames)
		&& !getDescendants(node).some(isProhibitedParagraphChild)) {
			return node.tagName.toLowerCase();
		}

		
		return "";
	}
};




commands.forwarddelete = {
	preservesOverrides: true,
	action: function() {
		
		
		if (!getActiveRange().collapsed) {
			deleteSelection();
			return true;
		}

		
		canonicalizeWhitespace(getActiveRange().startContainer, getActiveRange().startOffset);

		
		var node = getActiveRange().startContainer;
		var offset = getActiveRange().startOffset;

		
		while (true) {
			
			
			
			if (offset == getNodeLength(node)
			&& isEditable(node.nextSibling)
			&& isInvisible(node.nextSibling)) {
				node.parentNode.removeChild(node.nextSibling);

			
			
			} else if (offset < node.childNodes.length
			&& isEditable(node.childNodes[offset])
			&& isInvisible(node.childNodes[offset])) {
				node.removeChild(node.childNodes[offset]);

			
			
			
			} else if ((offset == getNodeLength(node)
			&& isInlineNode(node))
			|| isInvisible(node)) {
				offset = 1 + getNodeIndex(node);
				node = node.parentNode;

			
			
			
			} else if (offset < node.childNodes.length
			&& !isBlockNode(node.childNodes[offset])
			&& !isHtmlElement(node.childNodes[offset], ["br", "img"])
			&& !isCollapsedBlockProp(node.childNodes[offset])) {
				node = node.childNodes[offset];
				offset = 0;

			
			} else {
				break;
			}
		}

		
		if (node.nodeType == Node.TEXT_NODE
		&& offset != getNodeLength(node)) {
			
			var endOffset = offset + 1;

			
			
			
			
			
			
			
			
			while (endOffset != node.length
			&& /^[\u0300-\u036f\u0591-\u05bd\u05c1\u05c2]$/.test(node.data[endOffset])) {
				endOffset++;
			}

			
			getSelection().collapse(node, offset);
			getActiveRange().setStart(node, offset);

			
			
			getSelection().extend(node, endOffset);
			getActiveRange().setEnd(node, endOffset);

			
			deleteSelection();

			
			return true;
		}

		
		if (isInlineNode(node)) {
			return true;
		}

		
		
		if (offset < node.childNodes.length
		&& isHtmlElement(node.childNodes[offset], ["br", "hr", "img"])
		&& !isCollapsedBlockProp(node.childNodes[offset])) {
			
			getSelection().collapse(node, offset);
			getActiveRange().setStart(node, offset);

			
			
			getSelection().extend(node, offset + 1);
			getActiveRange().setEnd(node, offset + 1);

			
			deleteSelection();

			
			return true;
		}

		
		var endNode = node;
		var endOffset = offset;

		
		
		if (endOffset < endNode.childNodes.length
		&& isCollapsedBlockProp(endNode.childNodes[endOffset])) {
			endOffset++;
		}

		
		while (true) {
			
			
			if (endOffset == getNodeLength(endNode)) {
				endOffset = 1 + getNodeIndex(endNode);
				endNode = endNode.parentNode;

			
			
			} else if (endOffset < endNode.childNodes.length
			&& isEditable(endNode.childNodes[endOffset])
			&& isInvisible(endNode.childNodes[endOffset])) {
				endNode.removeChild(endNode.childNodes[endOffset]);

			
			} else {
				break;
			}
		}

		
		
		if (isHtmlElement(endNode.childNodes[endOffset - 1], "table")) {
			return true;
		}

		
		if (isHtmlElement(endNode.childNodes[endOffset], "table")) {
			
			
			getSelection().collapse(endNode, endOffset);
			getActiveRange().setStart(endNode, endOffset);

			
			
			getSelection().extend(endNode, endOffset + 1);
			getActiveRange().setEnd(endNode, endOffset + 1);

			
			return true;
		}

		
		
		if (offset == getNodeLength(node)
		&& isHtmlElement(endNode.childNodes[endOffset], ["br", "hr"])) {
			
			
			getSelection().collapse(endNode, endOffset);
			getActiveRange().setStart(endNode, endOffset);

			
			
			getSelection().extend(endNode, endOffset + 1);
			getActiveRange().setEnd(endNode, endOffset + 1);

			
			deleteSelection();

			
			getSelection().collapse(node, offset);
			getActiveRange().setStart(node, offset);
			getActiveRange().collapse(true);

			
			return true;
		}

		
		while (endOffset < endNode.childNodes.length) {
			
			
			if (isEditable(endNode.childNodes[endOffset])
			&& isInvisible(endNode.childNodes[endOffset])) {
				endNode.removeChild(endNode.childNodes[endOffset]);

			
			
			} else {
				endNode = endNode.childNodes[endOffset];
				endOffset = 0;
			}
		}

		
		getSelection().collapse(node, offset);
		getActiveRange().setStart(node, offset);

		
		
		getSelection().extend(endNode, endOffset);
		getActiveRange().setEnd(endNode, endOffset);

		
		deleteSelection();

		
		return true;
	}
};




commands.indent = {
	preservesOverrides: true,
	action: function() {
		
		
		
		
		var items = [];
		for (var node = getActiveRange().endContainer; node != getActiveRange().commonAncestorContainer; node = node.parentNode) {
			if (isHtmlElement(node, "LI")) {
				items.unshift(node);
			}
		}
		for (var node = getActiveRange().startContainer; node != getActiveRange().commonAncestorContainer; node = node.parentNode) {
			if (isHtmlElement(node, "LI")) {
				items.unshift(node);
			}
		}
		for (var node = getActiveRange().commonAncestorContainer; node; node = node.parentNode) {
			if (isHtmlElement(node, "LI")) {
				items.unshift(node);
			}
		}

		
		for (var i = 0; i < items.length; i++) {
			normalizeSublists(items[i]);
		}

		
		var newRange = blockExtend(getActiveRange());

		
		var nodeList = [];

		
		
		
		nodeList = getContainedNodes(newRange, function(node) {
			return isEditable(node)
				&& (isAllowedChild(node, "div")
				|| isAllowedChild(node, "ol"));
		});

		
		
		if (isHtmlElement(nodeList.filter(isVisible)[0], "li")
		&& isHtmlElement(nodeList.filter(isVisible)[0].parentNode, ["ol", "ul"])) {
			
			
			var sibling = nodeList.filter(isVisible)[0].previousSibling;

			
			
			while (isInvisible(sibling)) {
				sibling = sibling.previousSibling;
			}

			
			if (isHtmlElement(sibling, "li")) {
				normalizeSublists(sibling);
			}
		}

		
		while (nodeList.length) {
			
			var sublist = [];

			
			sublist.push(nodeList.shift());

			
			
			
			while (nodeList.length
			&& nodeList[0] == sublist[sublist.length - 1].nextSibling) {
				sublist.push(nodeList.shift());
			}

			
			indentNodes(sublist);
		}

		
		return true;
	}
};




commands.inserthorizontalrule = {
	preservesOverrides: true,
	action: function() {
		
		
		var startNode = getActiveRange().startContainer;
		var startOffset = getActiveRange().startOffset;
		var endNode = getActiveRange().endContainer;
		var endOffset = getActiveRange().endOffset;

		
		
		
		while (startOffset == 0
		&& startNode.parentNode) {
			startOffset = getNodeIndex(startNode);
			startNode = startNode.parentNode;
		}

		
		
		
		while (endOffset == getNodeLength(endNode)
		&& endNode.parentNode) {
			endOffset = 1 + getNodeIndex(endNode);
			endNode = endNode.parentNode;
		}

		
		
		getSelection().collapse(startNode, startOffset);
		getActiveRange().setStart(startNode, startOffset);

		
		
		getSelection().extend(endNode, endOffset);
		getActiveRange().setEnd(endNode, endOffset);

		
		deleteSelection({blockMerging: false});

		
		
		if (!isEditable(getActiveRange().startContainer)
		&& !isEditingHost(getActiveRange().startContainer)) {
			return true;
		}

		
		
		
		
		if (getActiveRange().startContainer.nodeType == Node.TEXT_NODE
		&& getActiveRange().startOffset == 0) {
			var newNode = getActiveRange().startContainer.parentNode;
			var newOffset = getNodeIndex(getActiveRange().startContainer);
			getSelection().collapse(newNode, newOffset);
			getActiveRange().setStart(newNode, newOffset);
			getActiveRange().collapse(true);
		}

		
		
		
		
		
		if (getActiveRange().startContainer.nodeType == Node.TEXT_NODE
		&& getActiveRange().startOffset == getNodeLength(getActiveRange().startContainer)) {
			var newNode = getActiveRange().startContainer.parentNode;
			var newOffset = 1 + getNodeIndex(getActiveRange().startContainer);
			getSelection().collapse(newNode, newOffset);
			getActiveRange().setStart(newNode, newOffset);
			getActiveRange().collapse(true);
		}

		
		
		var hr = document.createElement("hr");

		
		getActiveRange().insertNode(hr);

		
		fixDisallowedAncestors(hr);

		
		
		
		getSelection().collapse(hr.parentNode, 1 + getNodeIndex(hr));
		getActiveRange().setStart(hr.parentNode, 1 + getNodeIndex(hr));
		getActiveRange().collapse(true);

		
		return true;
	}
};




commands.inserthtml = {
	preservesOverrides: true,
	action: function(value) {
		
		deleteSelection();

		
		
		if (!isEditable(getActiveRange().startContainer)
		&& !isEditingHost(getActiveRange().startContainer)) {
			return true;
		}

		
		
		var frag = getActiveRange().createContextualFragment(value);

		
		var lastChild = frag.lastChild;

		
		if (!lastChild) {
			return true;
		}

		
		var descendants = getDescendants(frag);

		
		if (isBlockNode(getActiveRange().startContainer)) {
			
			
			
			
			
			
			[].filter.call(getActiveRange().startContainer.childNodes, function(node) {
				return isEditable(node)
					&& isCollapsedBlockProp(node)
					&& getNodeIndex(node) >= getActiveRange().startOffset;
			}).forEach(function(node) {
				node.parentNode.removeChild(node);
			});
		}

		
		getActiveRange().insertNode(frag);

		
		
		
		if (isBlockNode(getActiveRange().startContainer)
		&& ![].some.call(getActiveRange().startContainer.childNodes, isVisible)) {
			getActiveRange().startContainer.appendChild(document.createElement("br"));
		}

		
		
		
		getActiveRange().setStart(lastChild.parentNode, 1 + getNodeIndex(lastChild));
		getActiveRange().setEnd(lastChild.parentNode, 1 + getNodeIndex(lastChild));

		
		for (var i = 0; i < descendants.length; i++) {
			fixDisallowedAncestors(descendants[i]);
		}

		
		return true;
	}
};




commands.insertimage = {
	preservesOverrides: true,
	action: function(value) {
		
		if (value === "") {
			return false;
		}

		
		deleteSelection({stripWrappers: false});

		
		var range = getActiveRange();

		
		
		if (!isEditable(getActiveRange().startContainer)
		&& !isEditingHost(getActiveRange().startContainer)) {
			return true;
		}

		
		
		if (isBlockNode(range.startContainer)
		&& range.startContainer.childNodes.length == 1
		&& isHtmlElement(range.startContainer.firstChild, "br")
		&& range.startOffset == 0) {
			range.startContainer.removeChild(range.startContainer.firstChild);
		}

		
		
		var img = document.createElement("img");

		
		img.setAttribute("src", value);

		
		range.insertNode(img);

		
		
		
		
		
		
		
		range.setStart(img.parentNode, 1 + getNodeIndex(img));
		range.setEnd(img.parentNode, 1 + getNodeIndex(img));
		getSelection().removeAllRanges();
		getSelection().addRange(range);

		
		
		img.removeAttribute("width");
		img.removeAttribute("height");

		
		return true;
	}
};




commands.insertlinebreak = {
	preservesOverrides: true,
	action: function(value) {
		
		deleteSelection({stripWrappers: false});

		
		
		if (!isEditable(getActiveRange().startContainer)
		&& !isEditingHost(getActiveRange().startContainer)) {
			return true;
		}

		
		
		if (getActiveRange().startContainer.nodeType == Node.ELEMENT_NODE
		&& !isAllowedChild("br", getActiveRange().startContainer)) {
			return true;
		}

		
		
		
		if (getActiveRange().startContainer.nodeType != Node.ELEMENT_NODE
		&& !isAllowedChild("br", getActiveRange().startContainer.parentNode)) {
			return true;
		}

		
		
		
		
		if (getActiveRange().startContainer.nodeType == Node.TEXT_NODE
		&& getActiveRange().startOffset == 0) {
			var newNode = getActiveRange().startContainer.parentNode;
			var newOffset = getNodeIndex(getActiveRange().startContainer);
			getSelection().collapse(newNode, newOffset);
			getActiveRange().setStart(newNode, newOffset);
			getActiveRange().setEnd(newNode, newOffset);
		}

		
		
		
		
		
		if (getActiveRange().startContainer.nodeType == Node.TEXT_NODE
		&& getActiveRange().startOffset == getNodeLength(getActiveRange().startContainer)) {
			var newNode = getActiveRange().startContainer.parentNode;
			var newOffset = 1 + getNodeIndex(getActiveRange().startContainer);
			getSelection().collapse(newNode, newOffset);
			getActiveRange().setStart(newNode, newOffset);
			getActiveRange().setEnd(newNode, newOffset);
		}

		
		
		var br = document.createElement("br");

		
		getActiveRange().insertNode(br);

		
		
		
		getSelection().collapse(br.parentNode, 1 + getNodeIndex(br));
		getActiveRange().setStart(br.parentNode, 1 + getNodeIndex(br));
		getActiveRange().setEnd(br.parentNode, 1 + getNodeIndex(br));

		
		
		
		if (isCollapsedLineBreak(br)) {
			getActiveRange().insertNode(document.createElement("br"));

			
			getSelection().collapse(br.parentNode, 1 + getNodeIndex(br));
			getActiveRange().setStart(br.parentNode, 1 + getNodeIndex(br));
			getActiveRange().setEnd(br.parentNode, 1 + getNodeIndex(br));
		}

		
		return true;
	}
};




commands.insertorderedlist = {
	preservesOverrides: true,
	
	action: function() { toggleLists("ol"); return true },
	
	
	indeterm: function() { return /^mixed( ol)?$/.test(getSelectionListState()) },
	
	state: function() { return getSelectionListState() == "ol" },
};




commands.insertparagraph = {
	preservesOverrides: true,
	action: function() {
		
		deleteSelection();

		
		
		if (!isEditable(getActiveRange().startContainer)
		&& !isEditingHost(getActiveRange().startContainer)) {
			return true;
		}

		
		var node = getActiveRange().startContainer;
		var offset = getActiveRange().startOffset;

		
		
		if (node.nodeType == Node.TEXT_NODE
		&& offset != 0
		&& offset != getNodeLength(node)) {
			node.splitText(offset);
		}

		
		
		if (node.nodeType == Node.TEXT_NODE
		&& offset == getNodeLength(node)) {
			offset = 1 + getNodeIndex(node);
			node = node.parentNode;
		}

		
		
		if (node.nodeType == Node.TEXT_NODE
		|| node.nodeType == Node.COMMENT_NODE) {
			offset = getNodeIndex(node);
			node = node.parentNode;
		}

		
		getSelection().collapse(node, offset);
		getActiveRange().setStart(node, offset);
		getActiveRange().setEnd(node, offset);

		
		var container = node;

		
		
		
		while (!isSingleLineContainer(container)
		&& isEditable(container.parentNode)
		&& inSameEditingHost(node, container.parentNode)) {
			container = container.parentNode;
		}

		
		
		if (isEditable(container)
		&& isSingleLineContainer(container)
		&& inSameEditingHost(node, container.parentNode)
		&& (container.tagName == "P" || container.tagName == "DIV")) {
			
			var outerContainer = container;

			
			
			
			while (!isHtmlElement(outerContainer, ["dd", "dt", "li"])
			&& isEditable(outerContainer.parentNode)) {
				outerContainer = outerContainer.parentNode;
			}

			
			
			if (isHtmlElement(outerContainer, ["dd", "dt", "li"])) {
				container = outerContainer;
			}
		}

		
		
		if (!isEditable(container)
		|| !inSameEditingHost(container, node)
		|| !isSingleLineContainer(container)) {
			
			var tag = defaultSingleLineContainerName;

			
			
			var newRange = blockExtend(getActiveRange());

			
			
			
			
			var nodeList = getContainedNodes(newRange, function(node) { return isAllowedChild(node, "p") })
				.slice(0, 1);

			
			if (!nodeList.length) {
				
				
				if (!isAllowedChild(tag, getActiveRange().startContainer)) {
					return true;
				}

				
				
				container = document.createElement(tag);

				
				getActiveRange().insertNode(container);

				
				
				container.appendChild(document.createElement("br"));

				
				
				getSelection().collapse(container, 0);
				getActiveRange().setStart(container, 0);
				getActiveRange().setEnd(container, 0);

				
				return true;
			}

			
			
			while (nodeList[nodeList.length - 1].nextSibling
			&& isAllowedChild(nodeList[nodeList.length - 1].nextSibling, "p")) {
				nodeList.push(nodeList[nodeList.length - 1].nextSibling);
			}

			
			
			
			
			container = wrap(nodeList,
				function() { return false },
				function() { return document.createElement(tag) }
			);
		}

		
		if (container.tagName == "ADDRESS"
		|| container.tagName == "LISTING"
		|| container.tagName == "PRE") {
			
			
			var br = document.createElement("br");

			
			getActiveRange().insertNode(br);

			
			
			getSelection().collapse(node, offset + 1);
			getActiveRange().setStart(node, offset + 1);
			getActiveRange().setEnd(node, offset + 1);

			
			
			
			
			
			
			if (!isDescendant(nextNode(br), container)) {
				getActiveRange().insertNode(document.createElement("br"));
				getSelection().collapse(node, offset + 1);
				getActiveRange().setEnd(node, offset + 1);
			}

			
			return true;
		}

		
		
		if (["LI", "DT", "DD"].indexOf(container.tagName) != -1
		&& (!container.hasChildNodes()
		|| (container.childNodes.length == 1
		&& isHtmlElement(container.firstChild, "br")))) {
			
			splitParent([container]);

			
			
			
			if (!container.hasChildNodes()) {
				container.appendChild(document.createElement("br"));
			}

			
			
			
			
			if (isHtmlElement(container, ["dd", "dt"])
			&& getAncestors(container).every(function(ancestor) {
				return !inSameEditingHost(container, ancestor)
					|| !isAllowedChild(container, ancestor)
			})) {
				container = setTagName(container, defaultSingleLineContainerName);
			}

			
			fixDisallowedAncestors(container);

			
			return true;
		}

		
		
		
		var newLineRange = document.createRange();
		newLineRange.setStart(getActiveRange().startContainer, getActiveRange().startOffset);
		newLineRange.setEnd(container, getNodeLength(container));

		
		
		
		while (newLineRange.startOffset == 0
		&& !isProhibitedParagraphChild(newLineRange.startContainer)) {
			newLineRange.setStart(newLineRange.startContainer.parentNode, getNodeIndex(newLineRange.startContainer));
		}

		
		
		
		while (newLineRange.startOffset == getNodeLength(newLineRange.startContainer)
		&& !isProhibitedParagraphChild(newLineRange.startContainer)) {
			newLineRange.setStart(newLineRange.startContainer.parentNode, 1 + getNodeIndex(newLineRange.startContainer));
		}

		
		
		var containedInNewLineRange = getContainedNodes(newLineRange);
		var endOfLine = !containedInNewLineRange.length
			|| (containedInNewLineRange.length == 1
			&& isHtmlElement(containedInNewLineRange[0], "br"));

		
		
		
		var newContainerName;
		if (/^H[1-6]$/.test(container.tagName)
		&& endOfLine) {
			newContainerName = defaultSingleLineContainerName;

		
		
		} else if (container.tagName == "DT"
		&& endOfLine) {
			newContainerName = "dd";

		
		
		} else if (container.tagName == "DD"
		&& endOfLine) {
			newContainerName = "dt";

		
		} else {
			newContainerName = container.tagName.toLowerCase();
		}

		
		
		var newContainer = document.createElement(newContainerName);

		
		for (var i = 0; i < container.attributes.length; i++) {
			newContainer.setAttributeNS(container.attributes[i].namespaceURI, container.attributes[i].name, container.attributes[i].value);
		}

		
		newContainer.removeAttribute("id");

		
		
		container.parentNode.insertBefore(newContainer, container.nextSibling);

		
		var containedNodes = getAllContainedNodes(newLineRange);

		
		
		var frag = newLineRange.extractContents();

		
		
		var descendants = getDescendants(frag);
		for (var i = 0; i < descendants.length; i++) {
			if (descendants[i].nodeType == Node.ELEMENT_NODE
			&& containedNodes.indexOf(descendants[i]) == -1) {
				descendants[i].removeAttribute("id");
			}
		}

		
		newContainer.appendChild(frag);

		
		
		while (isProhibitedParagraphChild(container.lastChild)) {
			container = container.lastChild;
		}

		
		
		while (isProhibitedParagraphChild(newContainer.lastChild)) {
			newContainer = newContainer.lastChild;
		}

		
		
		
		if (![].some.call(container.childNodes, isVisible)) {
			container.appendChild(document.createElement("br"));
		}

		
		
		
		if (![].some.call(newContainer.childNodes, isVisible)) {
			newContainer.appendChild(document.createElement("br"));
		}

		
		getSelection().collapse(newContainer, 0);
		getActiveRange().setStart(newContainer, 0);
		getActiveRange().setEnd(newContainer, 0);

		
		return true;
	}
};




commands.inserttext = {
	action: function(value) {
		
		deleteSelection({stripWrappers: false});

		
		
		if (!isEditable(getActiveRange().startContainer)
		&& !isEditingHost(getActiveRange().startContainer)) {
			return true;
		}

		
		if (value.length > 1) {
			
			
			for (var i = 0; i < value.length; i++) {
				commands.inserttext.action(value[i]);
			}

			
			return true;
		}

		
		if (value == "") {
			return true;
		}

		
		
		if (value == "\n") {
			commands.insertparagraph.action();
			return true;
		}

		
		var node = getActiveRange().startContainer;
		var offset = getActiveRange().startOffset;

		
		
		
		if (0 <= offset - 1
		&& offset - 1 < node.childNodes.length
		&& node.childNodes[offset - 1].nodeType == Node.TEXT_NODE) {
			node = node.childNodes[offset - 1];
			offset = getNodeLength(node);
		}

		
		
		if (0 <= offset
		&& offset < node.childNodes.length
		&& node.childNodes[offset].nodeType == Node.TEXT_NODE) {
			node = node.childNodes[offset];
			offset = 0;
		}

		
		var overrides = recordCurrentOverrides();

		
		getSelection().collapse(node, offset);
		getActiveRange().setStart(node, offset);
		getActiveRange().setEnd(node, offset);

		
		canonicalizeWhitespace(node, offset);

		
		node = getActiveRange().startContainer;
		offset = getActiveRange().startOffset;

		
		if (node.nodeType == Node.TEXT_NODE) {
			
			node.insertData(offset, value);

			
			getSelection().collapse(node, offset);
			getActiveRange().setStart(node, offset);

			
			
			
			
			
			try { getSelection().extend(node, offset + 1); } catch(e) {}
			getActiveRange().setEnd(node, offset + 1);

		
		} else {
			
			
			
			
			
			if (node.childNodes.length == 1
			&& isCollapsedLineBreak(node.firstChild)) {
				node.removeChild(node.firstChild);
			}

			
			
			var text = document.createTextNode(value);

			
			getActiveRange().insertNode(text);

			
			getSelection().collapse(text, 0);
			getActiveRange().setStart(text, 0);

			
			getSelection().extend(text, 1);
			getActiveRange().setEnd(text, 1);
		}

		
		restoreStatesAndValues(overrides);

		
		
		canonicalizeWhitespace(getActiveRange().startContainer, getActiveRange().startOffset, false);

		
		
		canonicalizeWhitespace(getActiveRange().endContainer, getActiveRange().endOffset, false);

		
		if (/^[ \t\n\f\r]$/.test(value)) {
			autolink(getActiveRange().startContainer, getActiveRange().startOffset);
		}

		
		
		
		
		try { getSelection().collapseToEnd(); } catch(e) {}
		getActiveRange().collapse(false);

		
		return true;
	}
};




commands.insertunorderedlist = {
	preservesOverrides: true,
	
	action: function() { toggleLists("ul"); return true },
	
	
	indeterm: function() { return /^mixed( ul)?$/.test(getSelectionListState()) },
	
	state: function() { return getSelectionListState() == "ul" },
};




commands.justifycenter = {
	preservesOverrides: true,
	
	action: function() { justifySelection("center"); return true },
	indeterm: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.some(function(node) { return getAlignmentValue(node) == "center" })
			&& nodes.some(function(node) { return getAlignmentValue(node) != "center" });
	}, state: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.length
			&& nodes.every(function(node) { return getAlignmentValue(node) == "center" });
	}, value: function() {
		
		
		
		
		if (!getActiveRange()) {
			return "";
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		if (nodes.length) {
			return getAlignmentValue(nodes[0]);
		} else {
			return "left";
		}
	},
};




commands.justifyfull = {
	preservesOverrides: true,
	
	action: function() { justifySelection("justify"); return true },
	indeterm: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.some(function(node) { return getAlignmentValue(node) == "justify" })
			&& nodes.some(function(node) { return getAlignmentValue(node) != "justify" });
	}, state: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.length
			&& nodes.every(function(node) { return getAlignmentValue(node) == "justify" });
	}, value: function() {
		
		
		
		
		if (!getActiveRange()) {
			return "";
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		if (nodes.length) {
			return getAlignmentValue(nodes[0]);
		} else {
			return "left";
		}
	},
};




commands.justifyleft = {
	preservesOverrides: true,
	
	action: function() { justifySelection("left"); return true },
	indeterm: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.some(function(node) { return getAlignmentValue(node) == "left" })
			&& nodes.some(function(node) { return getAlignmentValue(node) != "left" });
	}, state: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.length
			&& nodes.every(function(node) { return getAlignmentValue(node) == "left" });
	}, value: function() {
		
		
		
		
		if (!getActiveRange()) {
			return "";
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		if (nodes.length) {
			return getAlignmentValue(nodes[0]);
		} else {
			return "left";
		}
	},
};




commands.justifyright = {
	preservesOverrides: true,
	
	action: function() { justifySelection("right"); return true },
	indeterm: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.some(function(node) { return getAlignmentValue(node) == "right" })
			&& nodes.some(function(node) { return getAlignmentValue(node) != "right" });
	}, state: function() {
		
		
		
		
		
		if (!getActiveRange()) {
			return false;
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		return nodes.length
			&& nodes.every(function(node) { return getAlignmentValue(node) == "right" });
	}, value: function() {
		
		
		
		
		if (!getActiveRange()) {
			return "";
		}
		var nodes = getAllContainedNodes(blockExtend(getActiveRange()), function(node) {
			return isEditable(node) && isVisible(node) && !node.hasChildNodes();
		});
		if (nodes.length) {
			return getAlignmentValue(nodes[0]);
		} else {
			return "left";
		}
	},
};




commands.outdent = {
	preservesOverrides: true,
	action: function() {
		
		
		
		
		
		
		var items = [];
		(function(){
			for (
				var ancestorContainer = getActiveRange().endContainer;
				ancestorContainer != getActiveRange().commonAncestorContainer;
				ancestorContainer = ancestorContainer.parentNode
			) {
				if (isHtmlElement(ancestorContainer, "li")) {
					items.unshift(ancestorContainer);
				}
			}
			for (
				var ancestorContainer = getActiveRange().startContainer;
				ancestorContainer;
				ancestorContainer = ancestorContainer.parentNode
			) {
				if (isHtmlElement(ancestorContainer, "li")) {
					items.unshift(ancestorContainer);
				}
			}
		})();

		
		items.forEach(normalizeSublists);

		
		var newRange = blockExtend(getActiveRange());

		
		
		
		
		
		
		var nodeList = getContainedNodes(newRange, function(node) {
			return isEditable(node)
				&& (!getDescendants(node).some(isEditable)
				|| isHtmlElement(node, ["ol", "ul"])
				|| (isHtmlElement(node, "li") && isHtmlElement(node.parentNode, ["ol", "ul"])));
		});

		
		while (nodeList.length) {
			
			
			
			while (nodeList.length
			&& (isHtmlElement(nodeList[0], ["OL", "UL"])
			|| !isHtmlElement(nodeList[0].parentNode, ["OL", "UL"]))) {
				outdentNode(nodeList.shift());
			}

			
			if (!nodeList.length) {
				break;
			}

			
			var sublist = [];

			
			sublist.push(nodeList.shift());

			
			
			
			
			while (nodeList.length
			&& nodeList[0] == sublist[sublist.length - 1].nextSibling
			&& !isHtmlElement(nodeList[0], ["OL", "UL"])) {
				sublist.push(nodeList.shift());
			}

			
			var values = recordValues(sublist);

			
			splitParent(sublist);

			
			sublist.forEach(fixDisallowedAncestors);

			
			restoreValues(values);
		}

		
		return true;
	}
};









commands.defaultparagraphseparator = {
	action: function(value) {
		
		
		
		value = value.toLowerCase();
		if (value == "p" || value == "div") {
			defaultSingleLineContainerName = value;
			return true;
		}
		return false;
	}, value: function() {
		
		return defaultSingleLineContainerName;
	},
};




commands.selectall = {
	
	
	action: function() {
		
		var target = document.body;

		
		
		if (!target) {
			target = document.documentElement;
		}

		
		
		if (!target) {
			getSelection().removeAllRanges();

		
		
		} else {
			getSelection().selectAllChildren(target);
		}

		
		return true;
	}
};




commands.stylewithcss = {
	action: function(value) {
		
		
		
		cssStylingFlag = String(value).toLowerCase() != "false";
		return true;
	}, state: function() { return cssStylingFlag }
};




commands.usecss = {
	action: function(value) {
		
		
		
		cssStylingFlag = String(value).toLowerCase() == "false";
		return true;
	}
};




(function() {



var commandNames = [];
for (var command in commands) {
	commandNames.push(command);
}
commandNames.forEach(function(command) {
	
	
	if (!("relevantCssProperty" in commands[command])) {
		commands[command].relevantCssProperty = null;
	}

	
	
	
	
	
	
	if ("inlineCommandActivatedValues" in commands[command]
	&& !("indeterm" in commands[command])) {
		commands[command].indeterm = function() {
			if (!getActiveRange()) {
				return false;
			}

			var values = getAllEffectivelyContainedNodes(getActiveRange(), isFormattableNode)
				.map(function(node) { return getEffectiveCommandValue(node, command) });

			var matchingValues = values.filter(function(value) {
				return commands[command].inlineCommandActivatedValues.indexOf(value) != -1;
			});

			return matchingValues.length >= 1
				&& values.length - matchingValues.length >= 1;
		};
	}

	
	
	
	
	
	
	if ("inlineCommandActivatedValues" in commands[command]) {
		commands[command].state = function() {
			if (!getActiveRange()) {
				return false;
			}

			var nodes = getAllEffectivelyContainedNodes(getActiveRange(), isFormattableNode);

			if (nodes.length == 0) {
				return commands[command].inlineCommandActivatedValues
					.indexOf(getEffectiveCommandValue(getActiveRange().startContainer, command)) != -1;
			} else {
				return nodes.every(function(node) {
					return commands[command].inlineCommandActivatedValues
						.indexOf(getEffectiveCommandValue(node, command)) != -1;
				});
			}
		};
	}

	
	
	
	
	
	
	
	if ("standardInlineValueCommand" in commands[command]) {
		commands[command].indeterm = function() {
			if (!getActiveRange()) {
				return false;
			}

			var values = getAllEffectivelyContainedNodes(getActiveRange())
				.filter(isFormattableNode)
				.map(function(node) { return getEffectiveCommandValue(node, command) });
			for (var i = 1; i < values.length; i++) {
				if (values[i] != values[i - 1]) {
					return true;
				}
			}
			return false;
		};

		commands[command].value = function() {
			if (!getActiveRange()) {
				return "";
			}

			var refNode = getAllEffectivelyContainedNodes(getActiveRange(), isFormattableNode)[0];

			if (typeof refNode == "undefined") {
				refNode = getActiveRange().startContainer;
			}

			var ret = getEffectiveCommandValue(refNode, command);
			if (ret === null) {
				return "";
			}
			return ret;
		};
	}

	
	
	
	
	if ("preservesOverrides" in commands[command]) {
		var oldAction = commands[command].action;

		commands[command].action = function(value) {
			var overrides = recordCurrentOverrides();
			var ret = oldAction(value);
			if (getActiveRange().collapsed) {
				restoreStatesAndValues(overrides);
			}
			return ret;
		};
	}
});
})();



