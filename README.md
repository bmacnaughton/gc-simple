# gc-minimal

Make minimal v8 garbage collection stats available via callback or polling.

# Usage

To enable polling invoke `start()`.

```js
const gcminimal = require('gc-minimal');

gcminimal.start();

// when you want gc data:

const stats = gcminimal.getCumulative();
```

will print information similar to:

```
{ gcCount: 2, gcTime: 2138709, gcTypeCounts: { '1': 2 } }

```

To enable callbacks when garbage collection has completed invoke `start(callback)`.

```js
const gcminimal = require('gc-minimal');

gcminimal.start(function (stats) {
  console.log(stats);
});
```

will print information similar to:

```
{ gcCount: 1, gcTime: 820028, gcTypeCounts: { '1': 1 } },
{ gcCount: 2, gcTime: 989847, gcTypeCounts: { '1': 2 } },
{ gcCount: 3, gcTime: 2807363, gcTypeCounts: { '1': 3 } },
{ gcCount: 4, gcTime: 2930841, gcTypeCounts: { '1': 4 } },
{ gcCount: 5, gcTime: 6149228, gcTypeCounts: { '1': 5 } },
{ gcCount: 6, gcTime: 4569092, gcTypeCounts: { '1': 6 } },
{ gcCount: 7, gcTime: 14181468, gcTypeCounts: { '1': 7 } },
{ gcCount: 8, gcTime: 13296259, gcTypeCounts: { '1': 8 } },
{ gcCount: 9, gcTime: 22442218, gcTypeCounts: { '1': 9 } },
{ gcCount: 10, gcTime: 20099, gcTypeCounts: { '1': 9, '4': 1 } },
{ gcCount: 11, gcTime: 7511308, gcTypeCounts: { '1': 9, '2': 1, '4': 1 } },

```

## Explanation of stats
* gcCount - number of times a garbage collection was performed (cumulative)
* gcTime - nanoseconds taken by this garbage collection
* gcTypeCounts - counts for each value of the type argument (cumulative)

#### gc types
  * 1: Scavenge (minor GC)
  * 2: Mark/Sweep/Compact (major GC)
  * 4: Incremental marking
  * 8: Weak/Phantom callback processing
  * 15: All

See [v8 source](https://github.com/nodejs/node/blob/554fa24916c5c6d052b51c5cee9556b76489b3f7/deps/v8/include/v8.h#L6137-L6144) for details.


# Installation
    npm install --save gc-minimal

# Node version support
gc-minimal depends on a C++ extension that is compiled during installation. We maintain support
only for node versions 8 and higher.

# Limitations
gc-minimal can only be invoked once per process so it's not suitable to be embedded in most libraries.
it will throw an exception if it `start()` is called more than once. the appoptics-apm agent for node
uses this package so if your application might use appoptics-apm then it should not use this. removing
this limitation is planned but not scheduled.

cumulative in `getCumulative()` is a bit of a misnomer. While the `gcTime` property is cumulative
`gcCount` and the `gcTypeCounts` are incremental since the last call to `getCumulative()`. the plan
is to add an option to keep all counts cumulative.
