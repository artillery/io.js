#include "worker.h"
#include "bson.h"

#include "node.h"
#include "node_internals.h"
#include "uv.h"
#include "env.h"
#include "env-inl.h"
#include "v8.h"
#include "libplatform/libplatform.h"
#include "v8-debug.h"
#include "util.h"
#include "util-inl.h"
#include "producer-consumer-queue.h"

#include <new>

namespace node {

using v8::Object;
using v8::FunctionCallbackInfo;
using v8::Value;
using v8::String;
using v8::Boolean;
using v8::FunctionTemplate;
using v8::Local;
using v8::Context;
using v8::HandleScope;
using v8::SealHandleScope;
using v8::Integer;
using v8::Isolate;
using v8::Function;
using v8::Persistent;
using v8::Locker;
using v8::TryCatch;

static Isolate::CreateParams makeCreateParams(ArrayBufferAllocator* allocator) {
  Isolate::CreateParams ret;
  ret.array_buffer_allocator = allocator;
  return ret;
}

WorkerContext::WorkerContext(Environment* owner_env,
                             Local<Object> owner_wrapper,
                             uv_loop_t* event_loop,
                             int argc,
                             const char** argv,
                             int exec_argc,
                             const char** exec_argv,
                             bool keep_alive,
                             char* user_data,
                             bool eval,
                             LoadExtensionsCb loadExtensions)
    : owner_env_(owner_env),
      array_buffer_allocator_(new ArrayBufferAllocator()),
      worker_isolate_(Isolate::New(makeCreateParams(array_buffer_allocator_))),
      owner_wrapper_(owner_env->isolate(), owner_wrapper),
      keep_alive_(keep_alive),
      user_data_(user_data),
      eval_(eval),
      load_extensions_(loadExtensions),
      event_loop_(event_loop),
      owner_notifications_(owner_event_loop(),
                           OwnerNotificationCallback,
                           this),
      worker_notifications_(worker_event_loop(),
                            WorkerNotificationCallback,
                            this),
      worker_bson_(nullptr),
      owner_bson_(nullptr),
      argc_(argc),
      argv_(argv),
      exec_argc_(exec_argc),
      exec_argv_(exec_argv) {
  CHECK_NE(event_loop, nullptr);
  CHECK_NE(event_loop, uv_default_loop());
  CHECK_CALLED_FROM_OWNER(this);
  node::Wrap(owner_wrapper, this);

  owner_env->AddSubWorkerContext(this);

  CHECK_EQ(uv_mutex_init(termination_mutex()), 0);
  CHECK_EQ(uv_mutex_init(api_mutex()), 0);
  CHECK_EQ(uv_mutex_init(initialization_mutex()), 0);
  CHECK_EQ(uv_mutex_init(to_owner_messages_mutex()), 0);
  CHECK_EQ(uv_mutex_init(to_worker_messages_mutex()), 0);

  to_owner_messages_temp_.reserve(kPrimaryQueueSize);
  to_worker_messages_temp_.reserve(kPrimaryQueueSize);

  HandleScope scope(owner_env->isolate());
  owner_bson_ = new BSON(owner_env->isolate());
}

void WorkerContext::New(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args);
  HandleScope scope(env->isolate());
  CHECK(args[0]->IsString());
  CHECK(args[1]->IsObject());

  Local<String> entry_module_path = args[0].As<String>();
  Local<Object> options = args[1].As<Object>();

#define GET_OPTION(var_name, option_name, Type)                               \
  Local<Type> var_name;                                                       \
  {                                                                           \
    Local<Value> val = options->Get(FIXED_ONE_BYTE_STRING(env->isolate(),     \
                                                          #option_name));     \
    var_name = val.As<Type>();                                                \
  }                                                                           \

  GET_OPTION(keep_alive, keepAlive, Value)
  GET_OPTION(user_data, userData, String)
  GET_OPTION(eval, eval, Value)
#undef GET_OPTION



  uv_loop_t* worker_event_loop = static_cast<uv_loop_t*>(
      malloc(sizeof(uv_loop_t)));
  int err = uv_loop_init(worker_event_loop);
  if (err != 0) {
    free(worker_event_loop);
    return env->ThrowUVException(err, "uv_loop_init");
  }

  // Heap allocated cause this stack frame can be gone by the time the worker
  // needs them.
  int argc = 2;
  char** argv = new char*[argc];
  // TODO(petkaantonov) should use correct executable name here
  argv[0] = const_cast<char*>("node");
  argv[1] = ToUtf8Value(env->isolate(), entry_module_path);
  // TODO(petkaantonov) should use the process's real exec args
  int exec_argc = 0;
  char** exec_argv = new char*[exec_argc];

  WorkerContext* context =
      new WorkerContext(env,
                        args.This(),
                        worker_event_loop,
                        argc,
                        const_cast<const char**>(argv),
                        exec_argc,
                        const_cast<const char**>(exec_argv),
                        keep_alive->BooleanValue(),
                        ToUtf8Value(env->isolate(), user_data),
                        eval->BooleanValue(),
                        env->load_extensions());

  err = uv_thread_create(context->worker_thread(), RunWorkerThread, context);

  if (err != 0) {
    context->worker_notifications()->Dispose();
    context->Dispose();
    CHECK_EQ(uv_loop_close(worker_event_loop), 0);
    free(worker_event_loop);
    return env->ThrowUVException(err, "uv_thread_create");
  }
}

void WorkerContext::PostMessageToOwner(WorkerMessage* message) {
  // The consumer tries very hard to keep the primary queue empty, so its reasonable to spin
  // here if the queue is full.
  while (!to_owner_messages_.PushBack(message));
  owner_notifications()->Notify();
}

static std::vector<Local<ArrayBuffer>> populateTransferables(Local<Value> transferablesArg) {
  std::vector<Local<ArrayBuffer>> ret;

  Local<Array> array = Local<Array>::Cast(transferablesArg->ToObject());

  ret.reserve(array->Length());
  for (unsigned int j = 0; j < array->Length(); j++) {
    Local<Value> element = array->Get(j);
    // TODO: Validate this earlier.
    CHECK(element->IsArrayBuffer());
    ret.push_back(Local<ArrayBuffer>::Cast(element));
  }
  return ret;
}

WorkerMessage* WorkerContext::SerializePostMessage(BSON* bson, Isolate* i, const FunctionCallbackInfo<Value>& args) {
  CHECK(args[1]->IsArray());
  CHECK(args[2]->IsInt32());

  std::vector<Local<ArrayBuffer>> transferables = populateTransferables(args[1]);

  size_t object_size = 0;
  Local<Object> object = Object::New(i);

  object->Set(0, args[0]);

  BSONSerializer<CountStream> counter(i, bson, false, false, &transferables);
  counter.SerializeDocument(object);
  object_size = counter.GetSerializeSize();

  char* buffer = static_cast<char*>(malloc(object_size));

  BSONSerializer<DataStream> data(i, bson, false, false, buffer, &transferables);
  data.SerializeDocument(object);

  WorkerMessageType message_type =
      static_cast<WorkerMessageType>(args[2]->Int32Value());
  return new WorkerMessage(buffer, object_size, message_type);
}

void WorkerContext::PostMessageToOwner(
    const FunctionCallbackInfo<Value>& args) {
  CHECK_EQ(args.Length(), 3);
  WorkerContext* context = node::Unwrap<WorkerContext>(args.Holder());
  if (context == nullptr)
    return;
  CHECK_CALLED_FROM_WORKER(context);

  ScopedLock::Mutex term_lock(context->termination_mutex());

  if (context->termination_kind() != NONE) {
    // We can't serialize this message, but that's fine - the owner can't expect to receive messages
    // sent after the call to terminate.
    return;
  }

  HandleScope scope(context->worker_isolate());
  auto serialized = context->SerializePostMessage(context->worker_bson_, context->worker_isolate(), args);
  term_lock.unlock(); // release mutex before PostMessageToOwner, which can spin.

  context->PostMessageToOwner(serialized);
}

void WorkerContext::PostMessageToWorker(WorkerMessage* message) {
  {
  ScopedLock::Mutex term_lock(termination_mutex());
  if (termination_kind() != NONE) {
    delete message;
    return;
  }
  } // Don't need term_lock now, we aren't making any v8 api calls here.

  // The consumer tries very hard to keep the primary queue empty, so its reasonable to spin
  // here if the queue is full.
  while (!to_worker_messages_.PushBack(message));
  worker_notifications()->Notify();
}

void WorkerContext::PostMessageToWorker(
    const FunctionCallbackInfo<Value>& args) {
  WorkerContext* context = node::Unwrap<WorkerContext>(args.Holder());
  if (context == nullptr)
    return;
  CHECK_CALLED_FROM_OWNER(context);

  HandleScope scope(context->owner_env()->isolate());
  context->PostMessageToWorker(context->SerializePostMessage(context->owner_bson_, context->owner_env()->isolate(), args));
}

void WorkerContext::WrapperNew(const FunctionCallbackInfo<Value>& args) {
  Environment* env = Environment::GetCurrent(args.GetIsolate());
  if (!args.IsConstructCall())
    return env->ThrowError("Illegal invocation");
  if (args.Length() < 1)
    return env->ThrowError("entryModulePath is required");

  Local<Object> recv = args.Holder();
  Local<Function> init = recv->Get(env->worker_init_symbol()).As<Function>();
  CHECK(init->IsFunction());
  Local<Value>* argv = new Local<Value>[args.Length()];
  for (int i = 0; i < args.Length(); ++i)
    argv[i] = args[i];
  init->Call(recv, args.Length(), argv);
  delete[] argv;
}

void WorkerContext::Initialize(Local<Object> target,
                        Local<Value> unused,
                        Local<Context> context) {
  Environment* env = Environment::GetCurrent(context);
  if (!node::experimental_workers)
    return env->ThrowError("Experimental workers are not enabled");

  Local<FunctionTemplate> t = env->NewFunctionTemplate(WorkerContext::New);
  t->InstanceTemplate()->SetInternalFieldCount(1);
  t->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "WorkerBinding"));

  env->SetProtoMethod(t, "terminate", Terminate);
  env->SetProtoMethod(t, "postMessage", PostMessageToWorker);
  env->SetProtoMethod(t, "ref", Ref);
  env->SetProtoMethod(t, "unref", Unref);

  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "WorkerContext"),
              t->GetFunction());

#define EXPORT_MESSAGE_TYPE(type)                                             \
  do {                                                                        \
  target->ForceSet(FIXED_ONE_BYTE_STRING(env->isolate(), #type),              \
                   Integer::New(env->isolate(),                               \
                                static_cast<int>(WorkerMessageType::type)),   \
                   v8::ReadOnly);                                             \
  } while (0);
  WORKER_MESSAGE_TYPES(EXPORT_MESSAGE_TYPE)
#undef EXPORT_MESSAGE_TYPE

  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "initSymbol"),
              env->worker_init_symbol());

  Local<FunctionTemplate> ctor_t =
      FunctionTemplate::New(env->isolate(), WorkerContext::WrapperNew);
  ctor_t->SetClassName(FIXED_ONE_BYTE_STRING(env->isolate(), "Worker"));
  ctor_t->SetLength(1);
  ctor_t->GetFunction()->SetName(
      FIXED_ONE_BYTE_STRING(env->isolate(), "Worker"));

  target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "JsConstructor"),
              ctor_t->GetFunction());


  if (env->is_worker_thread()) {
    Local<FunctionTemplate> t = FunctionTemplate::New(env->isolate());
    t->InstanceTemplate()->SetInternalFieldCount(1);
    Local<Object> worker_wrapper = t->GetFunction()->NewInstance();

    node::Wrap<WorkerContext>(worker_wrapper, env->worker_context());
    env->worker_context()->set_worker_wrapper(worker_wrapper);

    env->SetMethod(worker_wrapper,
                   "_postMessage",
                   PostMessageToOwner);

    target->Set(FIXED_ONE_BYTE_STRING(env->isolate(), "workerWrapper"),
                worker_wrapper);
  }
}

bool WorkerContext::LoopWouldEnd() {
  CHECK_CALLED_FROM_WORKER(this);
  bool is_not_terminated;
  {
    ScopedLock::Mutex lock(termination_mutex());
    is_not_terminated = termination_kind() == NONE;
  }

  if (is_not_terminated) {
    CHECK_NE(worker_env(), nullptr);
    node::EmitBeforeExit(worker_env());
    {
      ScopedLock::Mutex lock(termination_mutex());
      if (termination_kind() == NONE) {
        lock.unlock();
        bool ret = uv_loop_alive(worker_event_loop());
          if (uv_run(worker_event_loop(), UV_RUN_NOWAIT))
            ret = termination_kind() == NONE;
        return ret;
      }
    }
  }
  return false;
}

void WorkerContext::LoopEnded() {
  CHECK_CALLED_FROM_WORKER(this);
  ScopedLock::Mutex lock(termination_mutex());

  if (termination_kind() == NONE) {
    CHECK_NE(worker_env(), nullptr);
    set_termination_kind(NORMAL);
    lock.unlock();
    int exit_code = node::EmitExit(worker_env_);
    set_exit_code(exit_code);
    DisposeWorker(NORMAL);
  } else if (termination_kind() == ABRUPT) {
    lock.unlock();
    DisposeWorker(ABRUPT);
  }
}

void WorkerContext::Ref(const FunctionCallbackInfo<Value>& args) {
  WorkerContext* context = node::Unwrap<WorkerContext>(args.Holder());
  if (context == nullptr)
    return;
  CHECK_CALLED_FROM_OWNER(context);
  context->owner_notifications()->Ref();
}

void WorkerContext::Unref(const FunctionCallbackInfo<Value>& args) {
  WorkerContext* context = node::Unwrap<WorkerContext>(args.Holder());
  if (context == nullptr)
    return;
  CHECK_CALLED_FROM_OWNER(context);
  context->owner_notifications()->Unref();
}

void WorkerContext::Exit(int exit_code) {
  CHECK_CALLED_FROM_WORKER(this);
  CHECK(is_initialized());

  set_exit_code(exit_code);
  ScopedLock::Mutex lock(termination_mutex());

  if (termination_kind() != NONE)
    return;

  set_termination_kind(ABRUPT);
  worker_notifications()->Unref();
  uv_stop(worker_event_loop());
  worker_isolate()->TerminateExecution();
}

void WorkerContext::Terminate(const FunctionCallbackInfo<Value>& args) {
  WorkerContext* context = node::Unwrap<WorkerContext>(args.Holder());
  if (context == nullptr)
    return;
  context->Terminate();
}

void WorkerContext::Terminate(bool should_emit_exit_event) {
  CHECK_CALLED_FROM_OWNER(this);
  ScopedLock::Mutex init_lock(initialization_mutex());
  ScopedLock::Mutex api_lock(api_mutex());
  ScopedLock::Mutex term_lock(termination_mutex());

  if (termination_kind() != NONE)
    return;

  set_termination_kind(ABRUPT);

  if (is_initialized()) {
    should_emit_exit_event_ = should_emit_exit_event;
    worker_isolate()->TerminateExecution();
    worker_notifications()->Notify();
  }
}

static Local<Value> DeserializeMessage(BSON* bson, Isolate* isolate, WorkerMessage* message) {
  BSONDeserializer deserializer(isolate, bson, message->payload(), message->size());
  Local<Value> val = deserializer.DeserializeDocument(true);
  CHECK(val->IsObject());
  Local<Object> object = val->ToObject();
  return object->Get(0);
}

bool WorkerContext::ProcessMessageToWorker(Isolate* isolate,
                                           std::unique_ptr<WorkerMessage> message) {
  Local<Value> argv[] = {
    DeserializeMessage(worker_bson_, isolate, message.get()),
    Integer::New(isolate, message->type())
  };
  message.reset(nullptr);
  MakeCallback(isolate,
               worker_wrapper(),
               "_onmessage",
               ARRAY_SIZE(argv),
               argv);
  return worker_env()->CanCallIntoJs();
}

void WorkerContext::ProcessMessagesToWorker() {
  CHECK_CALLED_FROM_WORKER(this);

  Isolate* isolate = worker_env()->isolate();
  HandleScope scope(isolate);
  Context::Scope context_scope(worker_env()->context());

  to_worker_messages_temp_.clear();
  size_t messageIdx = 0;
  // Pop messages off of the lock free queue quickly into temporary storage, and process them
  // one at a time before checking the queue again. This means the
  // producer will only block if it is filling up the primary queue faster than the worker can
  // process a single message, which corresponds to an imbalance of kPrimaryQueueSize:1 which
  // is totally untenable in any case.
  do {
    uint32_t numPopped = 0;
    while (WorkerMessage* message = to_worker_messages_.PopFront()) {
      to_worker_messages_temp_.push_back(std::unique_ptr<WorkerMessage>(message));
      numPopped++;
      if (numPopped >= kPrimaryQueueSize) { break; }
    }

    if (messageIdx < to_worker_messages_temp_.size()) {
      std::unique_ptr<WorkerMessage> message = std::move(to_worker_messages_temp_[messageIdx]);
      messageIdx++;
      if (!ProcessMessageToWorker(isolate, std::move(message))) {
        return;
      }
    } else {
      break;
    }
  } while (true);
}

bool WorkerContext::ProcessMessageToOwner(Isolate* isolate,
                                          std::unique_ptr<WorkerMessage> message) {
  Local<Value> argv[] = {
    DeserializeMessage(owner_bson_, isolate, message.get()),
    Integer::New(isolate, message->type())
  };
  message.reset(nullptr);
  MakeCallback(isolate,
               owner_wrapper(),
               "_onmessage",
               ARRAY_SIZE(argv),
               argv);
  return owner_env()->CanCallIntoJs();
}

void WorkerContext::ProcessMessagesToOwner() {
  CHECK_CALLED_FROM_OWNER(this);
  Isolate* isolate = owner_env()->isolate();
  HandleScope scope(isolate);
  Context::Scope context_scope(owner_env()->context());

  to_owner_messages_temp_.clear();
  size_t messageIdx = 0;
  // Pop messages off of the lock free queue quickly into temporary storage, and process them
  // one at a time before checking the queue again. This means the
  // producer will only block if it is filling up the primary queue faster than the worker can
  // process a single message, which corresponds to an imbalance of kPrimaryQueueSize:1 which
  // is totally untenable in any case.
  do {
    uint32_t numPopped = 0;
    while (WorkerMessage* message = to_owner_messages_.PopFront()) {
      to_owner_messages_temp_.push_back(std::unique_ptr<WorkerMessage>(message));
      numPopped++;
      if (numPopped >= kPrimaryQueueSize) { break; }
    }

    if (messageIdx < to_owner_messages_temp_.size()) {
      std::unique_ptr<WorkerMessage> message = std::move(to_owner_messages_temp_[messageIdx]);
      messageIdx++;
      if (!ProcessMessageToOwner(isolate, std::move(message))) {
        return;
      }
    } else {
      break;
    }
  } while (true);
}

void WorkerContext::Dispose() {
  CHECK_CALLED_FROM_OWNER(this);

  owner_notifications()->Dispose();

  node::ClearWrap(owner_wrapper());
  owner_wrapper_.Reset();

  uv_mutex_destroy(termination_mutex());
  uv_mutex_destroy(api_mutex());
  uv_mutex_destroy(initialization_mutex());
  uv_mutex_destroy(to_owner_messages_mutex());
  uv_mutex_destroy(to_worker_messages_mutex());

  if (owner_bson_ != nullptr) { delete owner_bson_; }
  owner_bson_ = nullptr;

  delete[] argv_;
  delete[] exec_argv_;

  owner_env()->RemoveSubWorkerContext(this);
  // Deleting WorkerContexts in response to their notification signals
  // will cause use-after-free inside libuv. So the final `delete this`
  // call must be made somewhere else.
  node::QueueWorkerContextCleanup(this);
}

void WorkerContext::DisposeWorker(TerminationKind termination_kind) {
  CHECK_CALLED_FROM_WORKER(this);
  CHECK(termination_kind != NONE);

  delete array_buffer_allocator_;

  if (worker_env() != nullptr) {
    worker_env()->TerminateSubWorkers();
    worker_env()->CleanupHandles();
    if (!worker_wrapper_.IsEmpty()) {
      node::ClearWrap(worker_wrapper());
      worker_wrapper_.Reset();
    } else {
      CHECK(termination_kind == ABRUPT);
    }
  }

  worker_notifications()->Dispose();

  if (worker_env() != nullptr) {
    worker_env()->Dispose();
    worker_env_ = nullptr;
  }
  if (worker_bson_ != nullptr) { delete worker_bson_; }
  worker_bson_ = nullptr;
  pending_owner_cleanup_ = true;
  owner_notifications()->Notify();
}

void WorkerContext::WorkerNotificationCallback(void* arg) {
  WorkerContext* context = static_cast<WorkerContext*>(arg);
  CHECK_CALLED_FROM_WORKER(context);
  if (context->termination_kind() == NONE)
      context->ProcessMessagesToWorker();
  else
      uv_stop(context->worker_event_loop());
}

void WorkerContext::OwnerNotificationCallback(void* arg) {
  WorkerContext* context = static_cast<WorkerContext*>(arg);
  CHECK_CALLED_FROM_OWNER(context);
  if (context->owner_env()->CanCallIntoJs())
    context->ProcessMessagesToOwner();
  context->DisposeIfNecessary();
}

void WorkerContext::DisposeIfNecessary() {
  CHECK_CALLED_FROM_OWNER(this);
  if (pending_owner_cleanup_) {
    pending_owner_cleanup_ = false;
    uv_thread_join(worker_thread());
    int loop_was_not_empty = uv_loop_close(worker_event_loop());
    // TODO(petkaantonov) replace this with a message that also reports
    // what is still on the loop if https://github.com/libuv/libuv/pull/291
    // is accepted.
    CHECK(!loop_was_not_empty);
    free(worker_event_loop());
    event_loop_ = nullptr;

    if (should_emit_exit_event_) {
      Environment* env = owner_env();
      HandleScope scope(env->isolate());
      Context::Scope context_scope(env->context());
      Local<Value> argv[] = {
        Integer::New(env->isolate(), exit_code())
      };
      MakeCallback(env->isolate(),
                   owner_wrapper(),
                   "_onexit",
                   ARRAY_SIZE(argv),
                   argv);
    }
    Dispose();
  }
}

void WorkerContext::Run() {
  CHECK_CALLED_FROM_WORKER(this);

  if (!keep_alive_)
    worker_notifications()->Unref();

  {
    ScopedLock::Mutex lock(initialization_mutex());
    if (!JsExecutionAllowed()) {
      DisposeWorker(ABRUPT);
    } else {
      Isolate::Scope isolate_scope(worker_isolate());
      Locker locker(worker_isolate());

      // Based on the lowest common denominator, OS X, where
      // pthreads get by default 512kb of stack space.
      // On Linux it is 2MB and on Windows 1MB.
      static const size_t kStackSize = 512 * 1024;
      // Currently a multiplier of 2 is enough but I left some room to grow.
      static const size_t kStackAlreadyUsedGuess = 8 * 1024;
      uint32_t stack_pointer_source;
      uintptr_t stack_limit = reinterpret_cast<uintptr_t>(
          &stack_pointer_source - ((kStackSize - kStackAlreadyUsedGuess) /
                                  sizeof(stack_pointer_source)));
      worker_isolate()->SetStackLimit(stack_limit);

      HandleScope handle_scope(worker_isolate());
      Local<Context> context = Context::New(worker_isolate());
      Context::Scope context_scope(context);
      Environment* env = node::CreateEnvironment(worker_isolate(),
                                                 context,
                                                 this);
      array_buffer_allocator_->set_env(env);

      // To enable debugging uncomment StartDebug and EnableDebug.
      // TODO: Control debugging of worker threads via a commandline flag.
      //StartDebug(env, true);
      node::LoadEnvironment(env, load_extensions_);

      //EnableDebug(env);

      Local<Value> user_data =
          v8::JSON::Parse(String::NewFromUtf8(worker_isolate(), user_data_));
      free(user_data_);
      user_data_ = nullptr;
      env->process_object()->ForceSet(FIXED_ONE_BYTE_STRING(worker_isolate(),
                                                            "workerData"),
                                      user_data,
                                      v8::ReadOnly);

      if (eval_) {
        Local<String> eval_string = String::NewFromUtf8(worker_isolate(),
                                                        argv_[1]);
        env->process_object()->ForceSet(FIXED_ONE_BYTE_STRING(worker_isolate(),
                                                            "_eval"),
                                        eval_string,
                                        v8::ReadOnly);
      }

      worker_bson_ = new BSON(worker_isolate());

      Local<Function> entry =
          env->process_object()->Get(
              FIXED_ONE_BYTE_STRING(env->isolate(), "_runMain")).As<Function>();
      set_initialized();
      // Must not hold any locks when calling into JS
      lock.unlock();
      entry->Call(env->process_object(), 0, nullptr);
      SealHandleScope seal(worker_isolate());
      bool more;
      {
        do {
          v8::platform::PumpMessageLoop(platform(), worker_isolate());
          env->ProcessNotifications();
          more = uv_run(worker_event_loop(), UV_RUN_ONCE);
          if (!more) {
            v8::platform::PumpMessageLoop(platform(), worker_isolate());
            more = LoopWouldEnd();
          }
        } while (more && JsExecutionAllowed());
      }
      env->set_trace_sync_io(false);
      LoopEnded();
    }
  }
  worker_isolate()->Dispose();
}

// Entry point for worker_context instance threads
void WorkerContext::RunWorkerThread(void* arg) {
  static_cast<WorkerContext*>(arg)->Run();
}

void WorkerContext::set_worker_wrapper(Local<Object> worker_wrapper) {
  CHECK_CALLED_FROM_WORKER(this);
  new(&worker_wrapper_) Persistent<Object>(worker_env()->isolate(),
                                            worker_wrapper);
}

Local<Object> WorkerContext::owner_wrapper() {
  CHECK_CALLED_FROM_OWNER(this);
  return node::PersistentToLocal(owner_env()->isolate(), owner_wrapper_);
}

Local<Object> WorkerContext::worker_wrapper() {
  CHECK_CALLED_FROM_WORKER(this);
  return node::PersistentToLocal(worker_env()->isolate(), worker_wrapper_);
}

uv_loop_t* WorkerContext::owner_event_loop() const {
  return owner_env()->event_loop();
}

uv_thread_t* WorkerContext::owner_thread() {
  if (owner_env()->is_worker_thread())
    return owner_env()->worker_context()->worker_thread();
  else
    return main_thread();
}

v8::Platform* WorkerContext::platform() {
  return node::default_platform;
}

}  // namespace node

NODE_MODULE_CONTEXT_AWARE_BUILTIN(worker_context,
                                  node::WorkerContext::Initialize)
