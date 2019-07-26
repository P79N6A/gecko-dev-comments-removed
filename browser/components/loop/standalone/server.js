



var express = require('express');
var app = express();


app.use(express.static(__dirname + '/../'));

app.use(express.static(__dirname + '/'));

app.listen(3000);
console.log("Serving repository root over HTTP at http://localhost:3000/");
console.log("Static contents are available at http://localhost:3000/content/");
console.log("Tests are viewable at http://localhost:3000/test/");
console.log("Use this for development only.");
