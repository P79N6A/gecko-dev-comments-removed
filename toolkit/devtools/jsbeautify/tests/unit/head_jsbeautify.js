



"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

const { devtools } = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});
const { require } = devtools;

var beautify = require("devtools/jsbeautify");
var SanityTest = require('devtools/toolkit/jsbeautify/sanitytest');
var Urlencoded = require('devtools/toolkit/jsbeautify/urlencode_unpacker');
var {run_beautifier_tests} = require('devtools/toolkit/jsbeautify/beautify-tests');
