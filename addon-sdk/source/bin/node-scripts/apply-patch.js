


"use strict";

var path = require("path");
var cp = require("child_process");
var fs = require("fs");
var Promise = require("promise");
var patcher = require("patch-editor");
var readParam = require("./utils").readParam;

var isKeeper = /\/addon-sdk\/source/;

function apply(options) {
  return clean(options).then(function() {
    return new Promise(function(resolve) {
      var patch = path.resolve(readParam("patch"));
      var proc = cp.spawn("git", ["apply", patch]);
      proc.stdout.pipe(process.stdout);
      proc.stderr.pipe(process.stderr);
      proc.on("close", resolve);
    });
  });
}
exports.apply = apply;

function clean(options) {
  return new Promise(function(resolve) {
    var patch = path.resolve(readParam("patch"));
    if (!patch) {
      throw new Error("no --patch was provided.");
    }
    console.log("Cleaning patch " + patch);

    patcher.getChunks({ patch: patch }).then(function(chunks) {
      var keepers = [];

      for (var i = chunks.length - 1; i >= 0; i--) {
        var chunk = chunks[i];
        var files = chunk.getFilesChanged();

        
        var keepIt = files.map(function(file) {
          return (isKeeper.test(file));
        }).reduce(function(prev, curr) {
          return prev || curr;
        }, false);

        if (keepIt) {
          keepers.push(chunk);
        }
      }

      var contents = "\n" + keepers.join("\n") + "\n";
      contents = contents.replace(/\/addon-sdk\/source/g, "");

      fs.writeFileSync(patch, contents, { encoding: "utf8" });

      console.log("Done cleaning patch.");
    }).then(resolve).catch(console.error);
  });
}
exports.clean = clean;
