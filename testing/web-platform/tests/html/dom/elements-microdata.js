



for (var element in elements) {
	elements[element].itemScope = "boolean";
	elements[element].itemType = "settable tokenlist";
	elements[element].itemId = "url";
	elements[element].itemRef = "settable tokenlist";
	elements[element].itemProp = "settable tokenlist";
}
extraTests.push(function() {
	
	
	
	var reflectItemValue = function(data, localName, attribute) {
		var element = document.createElement(localName);
		element.setAttribute("itemprop", "");
		ReflectionTests.reflects(data, "itemValue", element, attribute);
	}
	reflectItemValue("string", "meta", "content");
	reflectItemValue("url", "audio", "src");
	reflectItemValue("url", "embed", "src");
	reflectItemValue("url", "iframe", "src");
	reflectItemValue("url", "img", "src");
	reflectItemValue("url", "source", "src");
	reflectItemValue("url", "track", "src");
	reflectItemValue("url", "video", "src");
	reflectItemValue("url", "a", "href");
	reflectItemValue("url", "area", "href");
	reflectItemValue("url", "link", "href");
	reflectItemValue("url", "object", "data");
	reflectItemValue("string", "data", "value");
	
});
