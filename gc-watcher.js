'use strict';

/* eslint-disable no-console */
const gcminimal = require('.');
//const gcminimal = require('bindings')('gc-minimal');

const throwError = process.argv.indexOf('error') > 1;
const callbacks = throwError || process.argv.indexOf('callbacks') > 1;

const output = [];

//
// gcTypeCounts
//
// 1 Scavenge
// 2 MarkSweepCompact
// 4 IncrementalMarking
// 8 ProcessWeakCallbacks
// 15 all
let status;
if (callbacks) {
  status = gcminimal.start(function (stats) {
    output.push(stats);
  });
} else {
  status = gcminimal.start();
}

console.log(`status = ${status}`);

function final () {
  console.log(output);
}
createObjects(5, {each, final});

//
// helpers
//
function each () {
  output.push(Object.assign({type: 'cumulative'}, gcminimal.getCumulative()));
}

function createObjects (iterations, opts = {}) {
  const final = opts.final || function () {};
  const each = opts.each || function () {};
  const max = opts.max || 1000000;
  const a = [];
  let iterationCount = 0;

  const loop = () => {
    if (iterationCount++ >= iterations) {
      final();
      return;
    }
    const execute = () => {
      return new Promise(resolve => {
        setTimeout(function () {
          a.length = 0;
          for (let i = 0; i < max; i++) {
            a[i] = {i: i * i};
          }
          // let gc callbacks occur before calling each and resolving. they are scheduled
          // on another thread so the event loop must run before they materialize here.
          // if no setTimeout() before fn() and resolve() then the cumulative numbers appear
          // before the individual gc callbacks.
          setTimeout(function () {
            each();
            resolve();
          }, 1);
        }, 5);
      });
    }
    // count on tail recursion optimization to loop without growing the stack.
    execute().then(loop);
  };

  // kick it off
  loop();
}
