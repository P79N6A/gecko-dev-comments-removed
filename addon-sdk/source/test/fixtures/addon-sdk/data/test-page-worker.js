





self.postMessage(["equal", document.title, "Page Worker test",
            "Correct page title accessed directly"]);


let p = document.getElementById("paragraph");
self.postMessage(["ok", !!p, "<p> can be accessed directly"]);
self.postMessage(["equal", p.firstChild.nodeValue,
            "Lorem ipsum dolor sit amet.",
            "Correct text node expected"]);


let div = document.createElement("div");
div.setAttribute("id", "block");
div.appendChild(document.createTextNode("Test text created"));
document.body.appendChild(div);


div = document.getElementById("block");
self.postMessage(["ok", !!div, "<div> can be accessed directly"]);
self.postMessage(["equal", div.firstChild.nodeValue,
            "Test text created", "Correct text node expected"]);
self.postMessage(["done"]);

