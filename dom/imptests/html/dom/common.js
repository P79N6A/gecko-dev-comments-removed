"use strict";






var selection;
var testDiv, paras, detachedDiv, detachedPara1, detachedPara2,
	foreignDoc, foreignPara1, foreignPara2, xmlDoc, xmlElement,
	detachedXmlElement, detachedTextNode, foreignTextNode,
	detachedForeignTextNode, xmlTextNode, detachedXmlTextNode,
	processingInstruction, detachedProcessingInstruction, comment,
	detachedComment, foreignComment, detachedForeignComment, xmlComment,
	detachedXmlComment, docfrag, foreignDocfrag, xmlDocfrag, doctype,
	foreignDoctype, xmlDoctype;
var testRangesShort, testRanges, testPoints, testNodesShort, testNodes;

function setupRangeTests() {
	selection = getSelection();
	testDiv = document.querySelector("#test");
	if (testDiv) {
		testDiv.parentNode.removeChild(testDiv);
	}
	testDiv = document.createElement("div");
	testDiv.id = "test";
	document.body.insertBefore(testDiv, document.body.firstChild);

	paras = [];
	paras.push(document.createElement("p"));
	paras[0].setAttribute("id", "a");
	
	
	paras[0].textContent = "A\u0308b\u0308c\u0308d\u0308e\u0308f\u0308g\u0308h\u0308\n";
	testDiv.appendChild(paras[0]);

	paras.push(document.createElement("p"));
	paras[1].setAttribute("id", "b");
	paras[1].setAttribute("style", "display:none");
	paras[1].textContent = "Ijklmnop\n";
	testDiv.appendChild(paras[1]);

	paras.push(document.createElement("p"));
	paras[2].setAttribute("id", "c");
	paras[2].textContent = "Qrstuvwx";
	testDiv.appendChild(paras[2]);

	paras.push(document.createElement("p"));
	paras[3].setAttribute("id", "d");
	paras[3].setAttribute("style", "display:none");
	paras[3].textContent = "Yzabcdef";
	testDiv.appendChild(paras[3]);

	paras.push(document.createElement("p"));
	paras[4].setAttribute("id", "e");
	paras[4].setAttribute("style", "display:none");
	paras[4].textContent = "Ghijklmn";
	testDiv.appendChild(paras[4]);

	detachedDiv = document.createElement("div");
	detachedPara1 = document.createElement("p");
	detachedPara1.appendChild(document.createTextNode("Opqrstuv"));
	detachedPara2 = document.createElement("p");
	detachedPara2.appendChild(document.createTextNode("Wxyzabcd"));
	detachedDiv.appendChild(detachedPara1);
	detachedDiv.appendChild(detachedPara2);

	
	
	
	
	
	foreignDoc = document.implementation.createHTMLDocument("");
	foreignPara1 = foreignDoc.createElement("p");
	foreignPara1.appendChild(foreignDoc.createTextNode("Efghijkl"));
	foreignPara2 = foreignDoc.createElement("p");
	foreignPara2.appendChild(foreignDoc.createTextNode("Mnopqrst"));
	foreignDoc.body.appendChild(foreignPara1);
	foreignDoc.body.appendChild(foreignPara2);

	
	
	
	xmlDoctype = document.implementation.createDocumentType("qorflesnorf", "abcde", "x\"'y");
	xmlDoc = document.implementation.createDocument(null, null, xmlDoctype);
	detachedXmlElement = xmlDoc.createElement("everyone-hates-hyphenated-element-names");
	detachedTextNode = document.createTextNode("Uvwxyzab");
	detachedForeignTextNode = foreignDoc.createTextNode("Cdefghij");
	detachedXmlTextNode = xmlDoc.createTextNode("Klmnopqr");
	
	
	detachedProcessingInstruction = xmlDoc.createProcessingInstruction("whippoorwill", "chirp chirp chirp");
	detachedComment = document.createComment("Stuvwxyz");
	
	detachedForeignComment = foreignDoc.createComment("אריה יהודה");
	detachedXmlComment = xmlDoc.createComment("בן חיים אליעזר");

	
	
	docfrag = document.createDocumentFragment();
	foreignDocfrag = foreignDoc.createDocumentFragment();
	xmlDocfrag = xmlDoc.createDocumentFragment();

	xmlElement = xmlDoc.createElement("igiveuponcreativenames");
	xmlTextNode = xmlDoc.createTextNode("do re mi fa so la ti");
	xmlElement.appendChild(xmlTextNode);
	processingInstruction = xmlDoc.createProcessingInstruction("somePI", 'Did you know that ":syn sync fromstart" is very useful when using vim to edit large amounts of JavaScript embedded in HTML?');
	xmlDoc.appendChild(xmlElement);
	xmlDoc.appendChild(processingInstruction);
	xmlComment = xmlDoc.createComment("I maliciously created a comment that will break incautious XML serializers, but Firefox threw an exception, so all I got was this lousy T-shirt");
	xmlDoc.appendChild(xmlComment);

	comment = document.createComment("Alphabet soup?");
	testDiv.appendChild(comment);

	foreignComment = foreignDoc.createComment('"Commenter" and "commentator" mean different things.  I\'ve seen non-native speakers trip up on this.');
	foreignDoc.appendChild(foreignComment);
	foreignTextNode = foreignDoc.createTextNode("I admit that I harbor doubts about whether we really need so many things to test, but it's too late to stop now.");
	foreignDoc.body.appendChild(foreignTextNode);

	doctype = document.doctype;
	foreignDoctype = foreignDoc.doctype;

	testRangesShort = [
		
		
		"[paras[0].firstChild, 0, paras[0].firstChild, 0]",
		"[paras[0].firstChild, 0, paras[0].firstChild, 1]",
		"[paras[0].firstChild, 2, paras[0].firstChild, 8]",
		"[paras[0].firstChild, 2, paras[0].firstChild, 9]",
		"[paras[1].firstChild, 0, paras[1].firstChild, 0]",
		"[paras[1].firstChild, 2, paras[1].firstChild, 9]",
		"[detachedPara1.firstChild, 0, detachedPara1.firstChild, 0]",
		"[detachedPara1.firstChild, 2, detachedPara1.firstChild, 8]",
		"[foreignPara1.firstChild, 0, foreignPara1.firstChild, 0]",
		"[foreignPara1.firstChild, 2, foreignPara1.firstChild, 8]",
		
		"[document.documentElement, 0, document.documentElement, 1]",
		"[document.documentElement, 0, document.documentElement, 2]",
		"[document.documentElement, 1, document.documentElement, 2]",
		"[document.head, 1, document.head, 1]",
		"[document.body, 4, document.body, 5]",
		"[foreignDoc.documentElement, 0, foreignDoc.documentElement, 1]",
		"[paras[0], 0, paras[0], 1]",
		"[detachedPara1, 0, detachedPara1, 1]",
		
		"[paras[0].firstChild, 0, paras[1].firstChild, 0]",
		"[paras[0].firstChild, 0, paras[1].firstChild, 8]",
		"[paras[0].firstChild, 3, paras[3], 1]",
		
		"[paras[0], 0, paras[0].firstChild, 7]",
		"[testDiv, 2, paras[4], 1]",
		
		"[document, 0, document, 1]",
		"[document, 0, document, 2]",
		"[comment, 2, comment, 3]",
		"[testDiv, 0, comment, 5]",
		"[foreignDoc, 1, foreignComment, 2]",
		"[foreignDoc.body, 0, foreignTextNode, 36]",
		"[xmlDoc, 1, xmlComment, 0]",
		"[detachedTextNode, 0, detachedTextNode, 8]",
		"[detachedForeignTextNode, 0, detachedForeignTextNode, 8]",
		"[detachedXmlTextNode, 0, detachedXmlTextNode, 8]",
		"[detachedComment, 3, detachedComment, 4]",
		"[detachedForeignComment, 0, detachedForeignComment, 1]",
		"[detachedXmlComment, 2, detachedXmlComment, 6]",
		"[docfrag, 0, docfrag, 0]",
	];

	testRanges = testRangesShort.concat([
		"[paras[1].firstChild, 0, paras[1].firstChild, 1]",
		"[paras[1].firstChild, 2, paras[1].firstChild, 8]",
		"[detachedPara1.firstChild, 0, detachedPara1.firstChild, 1]",
		"[foreignPara1.firstChild, 0, foreignPara1.firstChild, 1]",
		"[foreignDoc.head, 1, foreignDoc.head, 1]",
		"[foreignDoc.body, 0, foreignDoc.body, 0]",
		"[paras[0], 0, paras[0], 0]",
		"[detachedPara1, 0, detachedPara1, 0]",
		"[testDiv, 1, paras[2].firstChild, 5]",
		"[document.documentElement, 1, document.body, 0]",
		"[foreignDoc.documentElement, 1, foreignDoc.body, 0]",
		"[document, 1, document, 2]",
		"[paras[2].firstChild, 4, comment, 2]",
		"[paras[3], 1, comment, 8]",
		"[foreignDoc, 0, foreignDoc, 0]",
		"[xmlDoc, 0, xmlDoc, 0]",
		"[detachedForeignTextNode, 7, detachedForeignTextNode, 7]",
		"[detachedXmlTextNode, 7, detachedXmlTextNode, 7]",
		"[detachedComment, 5, detachedComment, 5]",
		"[detachedForeignComment, 4, detachedForeignComment, 4]",
		"[foreignDocfrag, 0, foreignDocfrag, 0]",
		"[xmlDocfrag, 0, xmlDocfrag, 0]",
	]);

	testPoints = [
		
		
		"[paras[0].firstChild, -1]",
		"[paras[0].firstChild, 0]",
		"[paras[0].firstChild, 1]",
		"[paras[0].firstChild, 2]",
		"[paras[0].firstChild, 8]",
		"[paras[0].firstChild, 9]",
		"[paras[0].firstChild, 10]",
		"[paras[0].firstChild, 65535]",
		"[paras[1].firstChild, -1]",
		"[paras[1].firstChild, 0]",
		"[paras[1].firstChild, 1]",
		"[paras[1].firstChild, 2]",
		"[paras[1].firstChild, 8]",
		"[paras[1].firstChild, 9]",
		"[paras[1].firstChild, 10]",
		"[paras[1].firstChild, 65535]",
		"[detachedPara1.firstChild, 0]",
		"[detachedPara1.firstChild, 1]",
		"[detachedPara1.firstChild, 8]",
		"[detachedPara1.firstChild, 9]",
		"[foreignPara1.firstChild, 0]",
		"[foreignPara1.firstChild, 1]",
		"[foreignPara1.firstChild, 8]",
		"[foreignPara1.firstChild, 9]",
		
		"[document.documentElement, -1]",
		"[document.documentElement, 0]",
		"[document.documentElement, 1]",
		"[document.documentElement, 2]",
		"[document.documentElement, 7]",
		"[document.head, 1]",
		"[document.body, 3]",
		"[foreignDoc.documentElement, 0]",
		"[foreignDoc.documentElement, 1]",
		"[foreignDoc.head, 0]",
		"[foreignDoc.body, 1]",
		"[paras[0], 0]",
		"[paras[0], 1]",
		"[paras[0], 2]",
		"[paras[1], 0]",
		"[paras[1], 1]",
		"[paras[1], 2]",
		"[detachedPara1, 0]",
		"[detachedPara1, 1]",
		"[testDiv, 0]",
		"[testDiv, 3]",
		
		"[document, -1]",
		"[document, 0]",
		"[document, 1]",
		"[document, 2]",
		"[document, 3]",
		"[comment, -1]",
		"[comment, 0]",
		"[comment, 4]",
		"[comment, 96]",
		"[foreignDoc, 0]",
		"[foreignDoc, 1]",
		"[foreignComment, 2]",
		"[foreignTextNode, 0]",
		"[foreignTextNode, 36]",
		"[xmlDoc, -1]",
		"[xmlDoc, 0]",
		"[xmlDoc, 1]",
		"[xmlDoc, 5]",
		"[xmlComment, 0]",
		"[xmlComment, 4]",
		"[processingInstruction, 0]",
		"[processingInstruction, 5]",
		"[processingInstruction, 9]",
		"[detachedTextNode, 0]",
		"[detachedTextNode, 8]",
		"[detachedForeignTextNode, 0]",
		"[detachedForeignTextNode, 8]",
		"[detachedXmlTextNode, 0]",
		"[detachedXmlTextNode, 8]",
		"[detachedProcessingInstruction, 12]",
		"[detachedComment, 3]",
		"[detachedComment, 5]",
		"[detachedForeignComment, 0]",
		"[detachedForeignComment, 4]",
		"[detachedXmlComment, 2]",
		"[docfrag, 0]",
		"[foreignDocfrag, 0]",
		"[xmlDocfrag, 0]",
		"[doctype, 0]",
		"[doctype, -17]",
		"[doctype, 1]",
		"[foreignDoctype, 0]",
		"[xmlDoctype, 0]",
	];

	testNodesShort = [
		"paras[0]",
		"paras[0].firstChild",
		"paras[1].firstChild",
		"foreignPara1",
		"foreignPara1.firstChild",
		"detachedPara1",
		"detachedPara1.firstChild",
		"document",
		"detachedDiv",
		"foreignDoc",
		"foreignPara2",
		"xmlDoc",
		"xmlElement",
		"detachedTextNode",
		"foreignTextNode",
		"processingInstruction",
		"detachedProcessingInstruction",
		"comment",
		"detachedComment",
		"docfrag",
		"doctype",
		"foreignDoctype",
	];

	testNodes = testNodesShort.concat([
		"paras[1]",
		"detachedPara2",
		"detachedPara2.firstChild",
		"testDiv",
		"detachedXmlElement",
		"detachedForeignTextNode",
		"xmlTextNode",
		"detachedXmlTextNode",
		"xmlComment",
		"foreignComment",
		"detachedForeignComment",
		"detachedXmlComment",
		"foreignDocfrag",
		"xmlDocfrag",
		"xmlDoctype",
	]);
}
if ("setup" in window) {
	setup(setupRangeTests);
} else {
	
	setupRangeTests();
}




function nodeLength(node) {
	
	
	
	
	if (node.nodeType == Node.DOCUMENT_TYPE_NODE) {
		return 0;
	}
	
	
	
	
	
	
	
	if (node.nodeType == Node.TEXT_NODE || node.nodeType == Node.PROCESSING_INSTRUCTION_NODE || node.nodeType == Node.COMMENT_NODE) {
		return node.data.length;
	}
	
	
	return node.childNodes.length;
}




function furthestAncestor(node) {
	var root = node;
	while (root.parentNode != null) {
		root = root.parentNode;
	}
	return root;
}







function isAncestorContainer(node1, node2) {
	return node1 == node2 ||
		(node2.compareDocumentPosition(node1) & Node.DOCUMENT_POSITION_CONTAINS);
}





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
	return node.parentNode;
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





function ownerDocument(node) {
	return node.nodeType == Node.DOCUMENT_NODE
		? node
		: node.ownerDocument;
}




function isAncestor(ancestor, descendant) {
	if (!ancestor || !descendant) {
		return false;
	}
	while (descendant && descendant != ancestor) {
		descendant = descendant.parentNode;
	}
	return descendant == ancestor;
}





function isInclusiveAncestor(ancestor, descendant) {
	return ancestor === descendant || isAncestor(ancestor, descendant);
}




function isDescendant(descendant, ancestor) {
	return isAncestor(ancestor, descendant);
}





function isInclusiveDescendant(descendant, ancestor) {
	return descendant === ancestor || isDescendant(descendant, ancestor);
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

		
		if (indexOf(child) < offsetA) {
			return "after";
		}
	}

	
	return "before";
}






function isContained(node, range) {
	var pos1 = getPosition(node, 0, range.startContainer, range.startOffset);
	var pos2 = getPosition(node, nodeLength(node), range.endContainer, range.endOffset);

	return furthestAncestor(node) == furthestAncestor(range.startContainer)
		&& pos1 == "after"
		&& pos2 == "before";
}






function isPartiallyContained(node, range) {
	var cond1 = isAncestorContainer(node, range.startContainer);
	var cond2 = isAncestorContainer(node, range.endContainer);
	return (cond1 && !cond2) || (cond2 && !cond1);
}




function indexOf(node) {
	if (!node.parentNode) {
		
		return 0;
	}
	var i = 0;
	while (node != node.parentNode.childNodes[i]) {
		i++;
	}
	return i;
}








function myExtractContents(range) {
	
	
	try {
		range.collapsed;
	} catch (e) {
		return "INVALID_STATE_ERR";
	}

	
	
	var ownerDoc = range.startContainer.nodeType == Node.DOCUMENT_NODE
		? range.startContainer
		: range.startContainer.ownerDocument;
	var frag = ownerDoc.createDocumentFragment();

	
	
	if (range.startContainer == range.endContainer
	&& range.startOffset == range.endOffset) {
		return frag;
	}

	
	
	
	var originalStartNode = range.startContainer;
	var originalStartOffset = range.startOffset;
	var originalEndNode = range.endContainer;
	var originalEndOffset = range.endOffset;

	
	
	if (range.startContainer == range.endContainer
	&& (range.startContainer.nodeType == Node.TEXT_NODE
	|| range.startContainer.nodeType == Node.COMMENT_NODE)) {
		
		
		var clone = originalStartNode.cloneNode(false);

		
		
		
		clone.data = originalStartNode.substringData(originalStartOffset,
			originalEndOffset - originalStartOffset);

		
		frag.appendChild(clone);

		
		
		originalStartNode.deleteData(originalStartOffset,
			originalEndOffset - originalStartOffset);

		
		return frag;
	}

	
	var commonAncestor = originalStartNode;

	
	
	while (!isAncestorContainer(commonAncestor, originalEndNode)) {
		commonAncestor = commonAncestor.parentNode;
	}

	
	
	var firstPartiallyContainedChild;
	if (isAncestorContainer(originalStartNode, originalEndNode)) {
		firstPartiallyContainedChild = null;
	
	
	} else {
		for (var i = 0; i < commonAncestor.childNodes.length; i++) {
			if (isPartiallyContained(commonAncestor.childNodes[i], range)) {
				firstPartiallyContainedChild = commonAncestor.childNodes[i];
				break;
			}
		}
		if (!firstPartiallyContainedChild) {
			throw "Spec bug: no first partially contained child!";
		}
	}

	
	
	var lastPartiallyContainedChild;
	if (isAncestorContainer(originalEndNode, originalStartNode)) {
		lastPartiallyContainedChild = null;
	
	
	} else {
		for (var i = commonAncestor.childNodes.length - 1; i >= 0; i--) {
			if (isPartiallyContained(commonAncestor.childNodes[i], range)) {
				lastPartiallyContainedChild = commonAncestor.childNodes[i];
				break;
			}
		}
		if (!lastPartiallyContainedChild) {
			throw "Spec bug: no last partially contained child!";
		}
	}

	
	
	
	
	
	var containedChildren = [];
	for (var i = 0; i < commonAncestor.childNodes.length; i++) {
		if (isContained(commonAncestor.childNodes[i], range)) {
			if (commonAncestor.childNodes[i].nodeType
			== Node.DOCUMENT_TYPE_NODE) {
				return "HIERARCHY_REQUEST_ERR";
			}
			containedChildren.push(commonAncestor.childNodes[i]);
		}
	}

	
	
	
	var newNode, newOffset;
	if (isAncestorContainer(originalStartNode, originalEndNode)) {
		newNode = originalStartNode;
		newOffset = originalStartOffset;
	
	} else {
		
		var referenceNode = originalStartNode;

		
		
		while (referenceNode.parentNode
		&& !isAncestorContainer(referenceNode.parentNode, originalEndNode)) {
			referenceNode = referenceNode.parentNode;
		}

		
		
		newNode = referenceNode.parentNode;
		newOffset = 1 + indexOf(referenceNode);
	}

	
	if (firstPartiallyContainedChild
	&& (firstPartiallyContainedChild.nodeType == Node.TEXT_NODE
	|| firstPartiallyContainedChild.nodeType == Node.COMMENT_NODE)) {
		
		
		var clone = originalStartNode.cloneNode(false);

		
		
		
		
		clone.data = originalStartNode.substringData(originalStartOffset,
			nodeLength(originalStartNode) - originalStartOffset);

		
		frag.appendChild(clone);

		
		
		
		originalStartNode.deleteData(originalStartOffset,
			nodeLength(originalStartNode) - originalStartOffset);
	
	} else if (firstPartiallyContainedChild) {
		
		
		var clone = firstPartiallyContainedChild.cloneNode(false);

		
		frag.appendChild(clone);

		
		
		
		var subrange = ownerDoc.createRange();
		subrange.setStart(originalStartNode, originalStartOffset);
		subrange.setEnd(firstPartiallyContainedChild,
			nodeLength(firstPartiallyContainedChild));

		
		
		var subfrag = myExtractContents(subrange);

		
		
		for (var i = 0; i < subfrag.childNodes.length; i++) {
			clone.appendChild(subfrag.childNodes[i]);
		}
	}

	
	
	for (var i = 0; i < containedChildren.length; i++) {
		frag.appendChild(containedChildren[i]);
	}

	
	if (lastPartiallyContainedChild
	&& (lastPartiallyContainedChild.nodeType == Node.TEXT_NODE
	|| lastPartiallyContainedChild.nodeType == Node.COMMENT_NODE)) {
		
		
		var clone = originalEndNode.cloneNode(false);

		
		
		clone.data = originalEndNode.substringData(0, originalEndOffset);

		
		frag.appendChild(clone);

		
		originalEndNode.deleteData(0, originalEndOffset);
	
	} else if (lastPartiallyContainedChild) {
		
		
		var clone = lastPartiallyContainedChild.cloneNode(false);

		
		frag.appendChild(clone);

		
		
		
		var subrange = ownerDoc.createRange();
		subrange.setStart(lastPartiallyContainedChild, 0);
		subrange.setEnd(originalEndNode, originalEndOffset);

		
		
		var subfrag = myExtractContents(subrange);

		
		
		for (var i = 0; i < subfrag.childNodes.length; i++) {
			clone.appendChild(subfrag.childNodes[i]);
		}
	}

	
	range.setStart(newNode, newOffset);
	range.setEnd(newNode, newOffset);

	
	return frag;
}








function myInsertNode(range, node) {
	
	
	
	
	try {
		range.collapsed;
	} catch (e) {
		return "INVALID_STATE_ERR";
	}

	
	
	if (range.startContainer.nodeType == Node.COMMENT_NODE
	|| (range.startContainer.nodeType == Node.TEXT_NODE
	&& !range.startContainer.parentNode)) {
		return "HIERARCHY_REQUEST_ERR";
	}

	
	
	var referenceNode;
	if (range.startContainer.nodeType == Node.TEXT_NODE) {
		
		
		
		var start = [range.startContainer, range.startOffset];
		var end = [range.endContainer, range.endOffset];

		referenceNode = range.startContainer.splitText(range.startOffset);

		if (start[0] == end[0]
		&& end[1] > start[1]) {
			end[0] = referenceNode;
			end[1] -= start[1];
		} else if (end[0] == start[0].parentNode
		&& end[1] > indexOf(referenceNode)) {
			end[1]++;
		}
		range.setStart(start[0], start[1]);
		range.setEnd(end[0], end[1]);

	
	
	} else {
		referenceNode = range.startContainer.childNodes[range.startOffset];
		if (typeof referenceNode == "undefined") {
			referenceNode = null;
		}
	}

	
	var parent_;
	if (!referenceNode) {
		parent_ = range.startContainer;

	
	} else {
		parent_ = referenceNode.parentNode;
	}

	
	
	var newOffset = referenceNode ? indexOf(referenceNode) : nodeLength(parent_);

	
	
	newOffset += node.nodeType == Node.DOCUMENT_FRAGMENT_NODE
		? nodeLength(node)
		: 1;

	
	try {
		parent_.insertBefore(node, referenceNode);
	} catch (e) {
		return getDomExceptionName(e);
	}

	
	if (range.collapsed) {
		range.setEnd(parent_, newOffset);
	}
}






function assertNodesEqual(actual, expected, msg) {
	if (!actual.isEqualNode(expected)) {
		msg = "Actual and expected mismatch for " + msg + ".  ";

		while (actual && expected) {
			assert_true(actual.nodeType === expected.nodeType
				&& actual.nodeName === expected.nodeName
				&& actual.nodeValue === expected.nodeValue
				&& actual.childNodes.length === expected.childNodes.length,
				"First differing node: expected " + format_value(expected)
				+ ", got " + format_value(actual));
			actual = nextNode(actual);
			expected = nextNode(expected);
		}

		assert_unreached("DOMs were not equal but we couldn't figure out why");
	}
}




function getDomExceptionName(e) {
	var ret = null;
	for (var prop in e) {
		if (/^[A-Z_]+_ERR$/.test(prop) && e[prop] == e.code) {
			return prop;
		}
	}

	throw "Exception seems to not be a DOMException?  " + e;
}





function rangeFromEndpoints(endpoints) {
	
	
	
	var range = ownerDocument(endpoints[0]).createRange();
	range.setStart(endpoints[0], endpoints[1]);
	range.setEnd(endpoints[2], endpoints[3]);
	return range;
}
