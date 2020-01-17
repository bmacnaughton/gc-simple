#include <nan.h>
#include <math.h>

using namespace v8;

struct HeapData {
	uint64_t   gcStartTime;
	uint64_t   gcEndTime;
	int        gctype;
};

class GCResponseResource : public Nan::AsyncResource {
 public:
	GCResponseResource(Local<Function> callback_)
		: Nan::AsyncResource("nan:test.DelayRequest") {
			callback.Reset(callback_);
		}

	~GCResponseResource() {
		callback.Reset();
	}

	Nan::Persistent<Function> callback;
};

static GCResponseResource* asyncResource;

static HeapStatistics beforeGCStats;
uint64_t gcStartTime;

static NAN_GC_CALLBACK(recordBeforeGC) {
	gcStartTime = uv_hrtime();
}

static void closeCB(uv_handle_t *handle) {
	delete handle;
}

static void asyncCB(uv_async_t *handle) {
	Nan::HandleScope scope;

	HeapData* data = static_cast<HeapData*>(handle->data);

	Local<Object> obj = Nan::New<Object>();
	Local<Object> beforeGCStats = Nan::New<Object>();
	Local<Object> afterGCStats = Nan::New<Object>();

	formatStats(beforeGCStats, data->before);
	formatStats(afterGCStats, data->after);

	Local<Object> diffStats = Nan::New<Object>();
	formatStatDiff(diffStats, data->before, data->after);

	Nan::Set(obj, Nan::New("startTime").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>(data->gcStartTime)));
	Nan::Set(obj, Nan::New("endTime").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>(data->gcEndTime)));
	Nan::Set(obj, Nan::New("pause").ToLocalChecked(),
		Nan::New<Number>(static_cast<double>(data->gcEndTime - data->gcStartTime)));
	Nan::Set(obj, Nan::New("pauseMS").ToLocalChecked(),
		Nan::New<Number>(round((data->gcEndTime - data->gcStartTime) / 1000000.0)));
	Nan::Set(obj, Nan::New("gctype").ToLocalChecked(), Nan::New<Number>(data->gctype));

	Local<Value> arguments[] = {obj};

	Local<Function> callback = Nan::New(asyncResource->callback);
	v8::Local<v8::Object> target = Nan::New<v8::Object>();
	asyncResource->runInAsyncScope(target, callback, 1, arguments);

	delete data->before;
	delete data->after;
	delete data;

	uv_close((uv_handle_t*) handle, closeCB);
}

NAN_GC_CALLBACK(afterGC) {
	uv_async_t *async = new uv_async_t;

	HeapData* data = new HeapData;
	data->before = new HeapInfo;
	data->after = new HeapInfo;
	data->gctype = type;

	HeapStatistics stats;

	Nan::GetHeapStatistics(&stats);

	data->gcEndTime = uv_hrtime();
	data->gcStartTime = gcStartTime;

	copyHeapStats(&beforeGCStats, data->before);
	copyHeapStats(&stats, data->after);

	async->data = data;

	uv_async_init(uv_default_loop(), async, asyncCB);
	uv_async_send(async);
}

static NAN_METHOD(AfterGC) {
	if(info.Length() != 1 || !info[0]->IsFunction()) {
		return Nan::ThrowError("Callback is required");
	}

	Local<Function> cb = Nan::To<Function>(info[0]).ToLocalChecked();
	asnycResource = new GCResponseResource(cb);

	Nan::AddGCEpilogueCallback(afterGC);
}

NAN_MODULE_INIT(init) {
	Nan::HandleScope scope;
	Nan::AddGCPrologueCallback(recordBeforeGC);

	Nan::Set(target,
		Nan::New("afterGC").ToLocalChecked(),
		Nan::GetFunction(
			Nan::New<FunctionTemplate>(AfterGC)).ToLocalChecked());
}

NODE_MODULE(gcstats, init)
