// Flags: --experimental-workers
'use strict';

var assert = require('assert');
var Worker = require('worker');

var gotMessage = false;
var a = new Worker('require("worker").postMessage(null);', {
  keepAlive: false,
  eval: true
});

a.on('message', function(data) {
  assert.strictEqual(data, null);
  gotMessage = true;
});

process.on('beforeExit', function() {
  assert(gotMessage);
});
