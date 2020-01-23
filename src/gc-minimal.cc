#include <nan.h>
#include <math.h>
#include <iostream>

using namespace v8;

/* https://v8docs.nodesource.com/node-8.16/d4/da0/v8_8h_source.html#l06365
 enum GCType {
   kGCTypeScavenge = 1 << 0,
   kGCTypeMarkSweepCompact = 1 << 1,
   kGCTypeIncrementalMarking = 1 << 2,
   kGCTypeProcessWeakCallbacks = 1 << 3,
   kGCTypeAll = kGCTypeScavenge | kGCTypeMarkSweepCompact |
                kGCTypeIncrementalMarking | kGCTypeProcessWeakCallbacks
 };

 enum GCCallbackFlags {
   kNoGCCallbackFlags = 0,
   kGCCallbackFlagConstructRetainedObjectInfos = 1 << 1,
   kGCCallbackFlagForced = 1 << 2,
   kGCCallbackFlagSynchronousPhantomCallbackProcessing = 1 << 3,
   kGCCallbackFlagCollectAllAvailableGarbage = 1 << 4,
   kGCCallbackFlagCollectAllExternalMemory = 1 << 5,
   kGCCallbackScheduleIdleGarbageCollection = 1 << 6,
 };
 */

const int maxTypeCount = kGCTypeAll + 1;
typedef struct GCData {
	uint64_t gcTime;
    uint64_t gcCount;
    uint64_t typeCounts[maxTypeCount];
} GCData_t;

GCData_t raw;
GCData_t cumulative_base;
GCData_t cumulative;

bool doCallbacks = false;
bool enabled = false;

class GCResponseResource : public Nan::AsyncResource {
 public:
	GCResponseResource(Local<Function> callback_)
		: Nan::AsyncResource("nan:gcminimal.DeferredCallback")
    {
        callback.Reset(callback_);
    }
	~GCResponseResource() {
		callback.Reset();
	}

	Nan::Persistent<Function> callback;
};

static GCResponseResource* asyncResource;

uint64_t gcStartTime;

static NAN_GC_CALLBACK(recordBeforeGC) {
	gcStartTime = uv_hrtime();
}

static void closeCB(uv_handle_t *handle) {
	delete handle;
}

static void asyncCB(uv_async_t *handle) {
	Nan::HandleScope scope;

	GCData_t* data = static_cast<GCData_t*>(handle->data);

    Local<Object> obj = Nan::New<Object>();

    Nan::Set(obj, Nan::New("gcCount").ToLocalChecked(),
        Nan::New<Number>(data->gcCount));
    Nan::Set(obj, Nan::New("gcTime").ToLocalChecked(),
        Nan::New<Number>(data->gcTime));

    Local<Object> counts = Nan::New<v8::Object>();
    for (int i = 0; i < maxTypeCount; i++) {
        if (data->typeCounts[i] != 0) {
        Nan::Set(counts, i, Nan::New<Number>(data->typeCounts[i]));
        }
    }
    Nan::Set(obj, Nan::New("gcTypeCounts").ToLocalChecked(), counts);

    Local<Value> arguments[] = {obj};

	Local<Function> callback = Nan::New(asyncResource->callback);
	v8::Local<v8::Object> target = Nan::New<v8::Object>();
	asyncResource->runInAsyncScope(target, callback, 1, arguments);

	delete data;

	uv_close((uv_handle_t*) handle, closeCB);
}

NAN_GC_CALLBACK(afterGC) {
    uint64_t et = uv_hrtime() - gcStartTime;

    // keep raw up-to-date
    raw.gcCount += 1;
    raw.gcTime += et;

    const int index = type & kGCTypeAll;
    raw.typeCounts[index] += 1;
    cumulative.typeCounts[index] += 1;

    // keep cumulative up-to-date
    cumulative.gcCount += 1;
    cumulative.gcTime += et;

    if (doCallbacks) {
        uv_async_t* async = new uv_async_t;
        GCData_t* data = new GCData_t;

        // update this gc cycle's data. because count is always 1 provide
        // the raw count as it provides additional information.
        *data = raw;
        data->gcTime = et;

        async->data = data;

        uv_async_init(uv_default_loop(), async, asyncCB);
        uv_async_send(async);
    }
}

static NAN_METHOD(start) {
    if (enabled) {
        Nan::ThrowError("already started");
        return;
    }
    doCallbacks = false;
    // cumulative base is data at last fetch of cumulative information so
    // the first fetch is everything before.
    memset(&cumulative_base, 0, sizeof(cumulative_base));
    memset(&cumulative, 0, sizeof(cumulative));

    // don't require a callback. the caller might just want to fetch
    // cumulative stats from time-to-time as it is far less overhead.
	if (info.Length() == 1) {
        if (!info[0]->IsFunction()) {
            Nan::ThrowTypeError("invalid signature");
            return;
        }
        doCallbacks = true;
        Local<Function> cb = Nan::To<Function>(info[0]).ToLocalChecked();
        asyncResource = new GCResponseResource(cb);
	}

    enabled = true;
    Nan::AddGCPrologueCallback(recordBeforeGC);
	Nan::AddGCEpilogueCallback(afterGC);

    info.GetReturnValue().Set(Nan::New<Number>(doCallbacks ? 2 : 1));
}

static NAN_METHOD(stop) {
    int status = enabled ? 1 : 0;

    if (enabled) {
        enabled = false;
        memset(&raw, 0, sizeof(raw));
        memset(&cumulative_base, 0, sizeof(cumulative_base));
        memset(&cumulative, 0, sizeof(cumulative));
        Nan::RemoveGCPrologueCallback(recordBeforeGC);
        Nan::RemoveGCEpilogueCallback(afterGC);
    }

    info.GetReturnValue().Set(Nan::New<Number>(status));
}


void getCumulative(const Nan::FunctionCallbackInfo<v8::Value>& info) {
    v8::Local<v8::Context> context = info.GetIsolate()->GetCurrentContext();

    v8::Local<v8::Object> obj = Nan::New<v8::Object>();

    uint64_t count = raw.gcCount - cumulative_base.gcCount;
    v8::Maybe<bool> b = obj->Set(context, Nan::New("gcCount").ToLocalChecked(),
        Nan::New<Number>(count));

    uint64_t time = raw.gcTime - cumulative_base.gcTime;
    b = obj->Set(context, Nan::New("gcTime").ToLocalChecked(),
        Nan::New<Number>(time));

    Local<Object> counts = Nan::New<v8::Object>();
    for (int i = 0; i < maxTypeCount; i++) {
      uint64_t count = raw.typeCounts[i] - cumulative_base.typeCounts[i];
      if (count != 0) {
        Nan::Set(counts, i, Nan::New<Number>(count));
      }
      // reset the base
      cumulative_base.typeCounts[i] = raw.typeCounts[i];
    }
    Nan::Set(obj, Nan::New("gcTypeCounts").ToLocalChecked(), counts);

    cumulative_base.gcCount = raw.gcCount;
    cumulative_base.gcTime = raw.gcTime;

    info.GetReturnValue().Set(obj);
}

NAN_MODULE_INIT(init) {
	Nan::HandleScope scope;

	Nan::Set(target, Nan::New("start").ToLocalChecked(),
      Nan::GetFunction(Nan::New<FunctionTemplate>(start)).ToLocalChecked());
	Nan::Set(target, Nan::New("stop").ToLocalChecked(),
      Nan::GetFunction(Nan::New<FunctionTemplate>(stop)).ToLocalChecked());

    Nan::Set(target, Nan::New("getCumulative").ToLocalChecked(),
      Nan::GetFunction(Nan::New<FunctionTemplate>(getCumulative)).ToLocalChecked());
}

NODE_MODULE(gcminimal, init)
