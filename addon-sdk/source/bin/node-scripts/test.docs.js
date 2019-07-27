


"use strict";

var createHash = require('crypto').createHash;
var fs = require("fs");
var fsExtra = require("fs-extra")
var path = require("path");
var Promise = require("promise");
var chai = require("chai");
var expect = chai.expect;
var teacher = require("teacher");

var rootURI = path.join(__dirname, "..", "..");


var NEW_WORDS = fs.readFileSync(path.join(__dirname, "words.txt")).toString().trim().split("\n");

var CACHE_PATH = path.join(__dirname, "..", "..", "cache", "spellchecks.json");

var CACHE = {};

try {
  CACHE = JSON.parse(fs.readFileSync(CACHE_PATH).toString());
}
catch (e) {}

function md5(str) {
  return createHash("md5").update(str).digest("utf8");
}

function addCacheHash(hash) {
  CACHE[hash] = true;
  fsExtra.ensureFileSync(CACHE_PATH);
  fsExtra.writeJSONSync(CACHE_PATH, CACHE);
}

describe("Spell Checking", function () {
  it("Spellcheck CONTRIBUTING.md", function (done) {
   var readme = path.join(rootURI, "CONTRIBUTING.md");

    fs.readFile(readme, function (err, data) {
      if (err) {
        throw err;
      }
      var text = data.toString();
      var hash = md5(text);

      
      
      if (CACHE[hash]) {
        expect(CACHE[hash]).to.be.equal(true);
        return done();
      }

      teacher.check(text, function(err, data) {
        expect(err).to.be.equal(null);

        var results = data || [];
        results = results.filter(function(result) {
          if (NEW_WORDS.indexOf(result.string.toLowerCase()) != -1) {
            return false;
          }

          
          if (result.string[0] == "-") {
            return false;
          }

          if (!(new RegExp(result.string)).test(text)) {
            return false;
          }

          return true;
        })

        if (results.length > 0) {
          console.log(results);
        }
        else {
          addCacheHash(hash);
        }

        expect(results.length).to.be.equal(0);

        setTimeout(done, 500);
      });
    });
  });

  it("Spellcheck README.md", function (done) {
   var readme = path.join(rootURI, "README.md");

    fs.readFile(readme, function (err, data) {
      if (err) {
        throw err;
      }
      var text = data.toString();
      var hash = md5(text);

      
      
      if (CACHE[hash]) {
        expect(CACHE[hash]).to.be.equal(true);
        return done();
      }

      teacher.check(text, function(err, data) {
        expect(err).to.be.equal(null);

        var results = data || [];
        results = results.filter(function(result) {
          if (NEW_WORDS.indexOf(result.string.toLowerCase()) != -1) {
            return false;
          }

          
          if (result.string[0] == "-") {
            return false;
          }

          
          
          if (!(new RegExp(result.string)).test(text)) {
            return false;
          }

          return true;
        })

        if (results.length > 0) {
          console.log(results);
        }
        else {
          addCacheHash(hash);
        }

        expect(results.length).to.be.equal(0);

        done();
      });
    });
  });
});
