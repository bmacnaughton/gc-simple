# GCStats

Exposes stats about V8 GC after it has been executed.

# Usage

Create a new instance of the module and subscribe to `stats`-events from that:

```js
gcstats = require('gc-stats');

gcstats.afterGC(function (stats) {
  console.log(stats);
})

// This will print information like this whenever a GC happened:

{ gcCount: 4, gcTime: 2756555 },
{ gcCount: 5, gcTime: 5717422 },
{ gcCount: 6, gcTime: 5081609 },
{ gcCount: 7, gcTime: 15052260 },
{ gcCount: 8, gcTime: 13403539 },
{ gcCount: 9, gcTime: 20765839 },
{ gcCount: 10, gcTime: 27269 },
{ gcCount: 11, gcTime: 7959755 },
```



## Property insights
* gcCount - number of times a garbage collection was performed (cumulative)
* gcTime - nanoseconds taken by this garbage collection

* gctype can have the following values([v8 source](https://github.com/nodejs/node/blob/554fa24916c5c6d052b51c5cee9556b76489b3f7/deps/v8/include/v8.h#L6137-L6144)):
  * 1: Scavenge (minor GC)
  * 2: Mark/Sweep/Compact (major GC)
  * 4: Incremental marking
  * 8: Weak/Phantom callback processing
  * 15: All

# Installation

    npm install gc-stats

# Node version support
node-gcstats depends on C++ extensions which are compiled when the *gc-stats* module is installed. Compatibility information can be inspected via the [Travis-CI build jobs](https://travis-ci.org/dainis/node-gcstats/).
