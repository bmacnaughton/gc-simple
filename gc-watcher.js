'use strict';

/* eslint-disable no-console */
//const gcstats = require('bindings')('gcstats');
const gcstats = require('bindings')('gcstats');

const throwError = process.argv.indexOf('error') > 1;
const callbacks = throwError || process.argv.indexOf('callbacks') > 1;

const output = [];

let invocationCount = 0;
gcstats.afterGC(function (stats) {
  invocationCount += 1;
  //console.log(stats, ',');
  output.push(stats);
})

createObjects(5, each);

//
// helpers
//
function each () {
  //console.log('createObjects cycle ended, cum: ', gcstats.getCumulative());
  output.push(Object.assign({type: 'cumulative'}, gcstats.getCumulative()));
}

function createObjects (iterations, fn = function () {}, max = 1000000) {
  const a = [];
  let iterationCount = 0;

  const loop = () => {
    if (iterationCount++ >= iterations) {
      console.log(output);
      return;
    }
    const execute = () => {
      return new Promise(resolve => {
        setTimeout(function () {
          a.length = 0;
          for (let i = 0; i < max; i++) {
            a[i] = {i: i * i};
          }
          // let gc callbacks occur before calling fn and resolving. they are scheduled
          // on another thread and need to let the event loop run before they materialize.
          // if no setTimeout() before fn() and resolve() then the cumulative numbers appear
          // before the individual gc callbacks.
          setTimeout(function () {
            fn();
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
