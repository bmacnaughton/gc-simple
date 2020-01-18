'use strict';

/* eslint-disable no-console */
//const gcstats = require('bindings')('gcstats');
const gcstats = require('./build/gcstats/v1.4.0/Release/node-v72-linux-x64/gcstats.node');

const throwError = process.argv.indexOf('error') > 1;
const callbacks = throwError || process.argv.indexOf('callbacks') > 1;

let invocationCount = 0;
gcstats.afterGC(function (stats) {
  //console.log(stats);
  invocationCount += 1;
  console.log('call: ', invocationCount);
})

createObjects(5, each);

//
// helpers
//

function each () {
  console.log('createObjects cycle ended, invocations: ', invocationCount);
}

function createObjects (iterations, fn = function () {}, max = 1000000) {
  const a = [];
  let iterationCount = 0;

  const loop = () => {
    if (iterationCount++ >= iterations) {
      return;
    }
    const execute = () => {
      return new Promise(resolve => {
        setTimeout(function () {
          a.length = 0;
          for (let i = 0; i < max; i++) {
            a[i] = {i: i * i};
          }
          fn();
          resolve();
        }, 5);
      });
    }
    // count on tail recursion optimization to loop without growing the stack.
    execute().then(loop);
  };

  // kick it off
  loop();
}
