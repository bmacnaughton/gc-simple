'use strict';

// stdlib
const nvm = process.env.NVM_DIR;
const taploc = nvm ? `${nvm}/versions/node/${process.version}/lib/node_modules/tap` : 'tap';
const tap = require(taploc);
const test = tap.test;

const watcher = require('..');

test('works with no callback', function (t) {
  t.plan(2);

  t.equal(watcher.start(), 1, 'enable should succeed');
  t.equal(watcher.stop(), 1, 'disable should succeed');
});

test('works with a callback', function (t) {
  t.plan(2);

  function after () {}

  t.equal(watcher.start(after), 2, 'enable should succeed');
  t.equal(watcher.stop(), 1, 'disable should succeed');
});

test('throws with non-function argument', function (t) {
  t.plan(1);

  t.throws(
    function () {watcher.start(42)},
    new TypeError('invalid signature'),
    'invalid argument should throw a TypeError'
  );
});

test('cannot start twice', function (t) {
  t.plan(3);

  t.equal(watcher.start(), 1, 'start should succeed');
  t.throws(
    function () {watcher.start()},
    new Error('already started'),
    'the second start must fail'
  );
  t.equal(watcher.stop(), 1, 'stop should succeed');
});

test('stop works even when not started', function (t) {
  t.plan(1);

  t.equal(watcher.stop(), 0, 'stop should succeed and return 0');
});

test('callback gets called', function (t) {
  let count = 0;
  function cb (stats) {
    count += 1;
  }

  const output = [];

  const status = watcher.start(cb);
  t.equal(status, 2, 'start should succeed');

  createObjects(5, {each, final: check});

  function each () {
    output.push(Object.assign({type: 'cumulative'}, watcher.getCumulative()));
  }
  function check () {
    t.notEqual(count, 0, 'callback must be called');
    t.equal(watcher.stop(), 1, 'stop should return that it was enabled');
    t.end();
  }
});

test('callbacks and cumulative get the same data', function (t) {
  let count = 0;
  let gcTime = 0;
  let lastStats;

  function cb (stats) {
    count += 1;
    t.equal(count, stats.gcCount, `callbacks (${count}) should equal gcCount (${stats.gcCount})`);
    // gcTime is a delta value so accumulate total by adding each callback's value. all other
    // values are cumulative.
    gcTime += stats.gcTime;
    lastStats = stats;
  }

  const status = watcher.start(cb);
  t.equal(status, 2, 'start should succeed');

  createObjects(5, {final: check});

  function check () {
    const cumStats = watcher.getCumulative();
    t.equal(cumStats.gcCount, count,
      `cumulative callbacks (${cumStats.gcCount}) should equal callbacks (${count})`);
    t.equal(cumStats.gcTime, gcTime, 'cumulative time should equal sum of callback times');
    delete lastStats.gcTime;
    delete cumStats.gcTime;
    t.strictSame(lastStats, cumStats, 'last callback stats should equal cumulative stats');
    watcher.stop();
    t.end();
  }

});


//
// helper to allocate memory. it uses promises so the
// callback can be delivered.
//
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

