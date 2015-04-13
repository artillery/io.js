// Flags: --experimental-workers
'use strict';

var assert = require('assert');
var Worker = require('worker');
var checks = 0;

if (process.isMainThread) {
  var aWorker = new Worker(__filename);
  aWorker.on('exit', function() {
    if (checks === 2)
      checks++;
  });
  aWorker.on('message', function() {
    checks++;
    setTimeout(function() {
      checks++;
      aWorker.terminate();
    }, 5);
  });
  process.on('beforeExit', function() {
    assert.equal(3, checks);
  });
} else {
  Worker.postMessage();
}
