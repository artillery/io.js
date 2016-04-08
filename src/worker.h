#ifndef SRC_WORKER_H_
#define SRC_WORKER_H_

#include "util.h"
#include "util-inl.h"
#include "notification-channel.h"
#include "producer-consumer-queue.h"

#include "uv.h"
#include "v8.h"

#include <memory>
#include <vector>

#ifndef NODE_OS_MACOSX
  #define CHECK_CALLED_FROM_OWNER(worker_context)                              \
    do {                                                                       \
      uv_thread_t self = uv_thread_self();                                     \
      CHECK(uv_thread_equal(&self, worker_context->owner_thread()));           \
    } while (0);                                                               \

  #define CHECK_CALLED_FROM_WORKER(worker_context)                             \
    do {                                                                       \
      uv_thread_t self = uv_thread_self();                                     \
      CHECK(uv_thread_equal(&self, worker_context->worker_thread()));          \
    } while (0);                                                               \

#else
  #define CHECK_CALLED_FROM_OWNER(worker_context)
  #define CHECK_CALLED_FROM_WORKER(worker_context)
#endif  // NODE_OS_MACOSX

#define WORKER_MESSAGE_TYPES(V)                                               \
  V(USER)                                                                     \
  V(INTERNAL)                                                                 \
  V(EXCEPTION)                                                                \

class BSON;

namespace node {
class Environment;
class WorkerContext;

enum WorkerMessageType {
#define DECLARE_ENUM_ITEM(type) type,
  WORKER_MESSAGE_TYPES(DECLARE_ENUM_ITEM)
#undef DECLARE_ENUM_ITEM
};

typedef void(*LoadExtensionsCb)(v8::Isolate*, v8::Local<v8::Object> global);

class WorkerMessage {
  public:
    WorkerMessage(char* payload, size_t size,
                  WorkerMessageType message_type = WorkerMessageType::USER)
        : payload_(payload), size_(size), message_type_(message_type) {
    }

    ~WorkerMessage() {
      free(payload_);
    }

    char* payload() const {
      return payload_;
    }

    size_t size() const {
      return size_;
    }

    WorkerMessageType type() const {
      return message_type_;
    }

    ListNode<WorkerMessage> member;
  private:
    char* const payload_;
    const size_t size_;
    WorkerMessageType message_type_;

    friend class WorkerContext;
};

// Terms:
// 'owner' the environment of this worker's owner. Can run in main thread but
// a worker can also own other workers.
// 'worker' the environment of this worker. Cannot run in main thread.
class WorkerContext {
  public:
    WorkerContext(Environment* env,
           v8::Handle<v8::Object> owner_wrapper,
           uv_loop_t* event_loop,
           int argc,
           const char** argv,
           int exec_argc,
           const char** exec_argv,
           bool keep_alive,
           char* user_data,
           bool eval,
           LoadExtensionsCb loadExtensions);
    static void Initialize(v8::Handle<v8::Object> target,
                           v8::Handle<v8::Value> unused,
                           v8::Handle<v8::Context> context);
    static void RunWorkerThread(void* arg);

    bool IsTerminated() {
      return termination_kind() != NONE;
    }

    uv_loop_t* worker_event_loop() const {
      return event_loop_;
    }

    int argc() const {
      return argc_;
    }

    const char** argv() const {
      return argv_;
    }

    int exec_argc() const {
      return exec_argc_;
    }

    const char** exec_argv() const {
      return exec_argv_;
    }

    uv_mutex_t* api_mutex() {
      return &api_mutex_;
    }

    ListNode<WorkerContext> subworker_list_member_;
    ListNode<WorkerContext> cleanup_queue_member_;
  private:
    enum TerminationKind { NONE, NORMAL, ABRUPT };
    bool JsExecutionAllowed() const {
      return termination_kind() != ABRUPT;
    }

    void NotifyWorker();
    void NotifyOwner();
    void Run();
    void LoopEnded();
    bool LoopWouldEnd();
    WorkerMessage* SerializePostMessage(
        BSON* bson, v8::Isolate* i, const v8::FunctionCallbackInfo<v8::Value>& args);
    void ProcessMessagesToOwner();
    bool ProcessMessageToOwner(v8::Isolate* isolate, std::unique_ptr<WorkerMessage> message);
    void ProcessMessagesToWorker();
    bool ProcessMessageToWorker(v8::Isolate* isolate, std::unique_ptr<WorkerMessage> message);
    void PostMessageToOwner(WorkerMessage* message);
    void PostMessageToWorker(WorkerMessage* message);
    void Exit(int exit_code);
    void Terminate(bool should_emit_exit_event = true);
    void DisposeIfNecessary();
    void DidTerminateAbruptly();
    void DisposeWorker(TerminationKind kind);
    void Dispose();
    static void WorkerNotificationCallback(void* arg);
    static void OwnerNotificationCallback(void* arg);
    static WorkerContext* Unwrap(
        const v8::FunctionCallbackInfo<v8::Value>& args);
    static void New(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void WrapperNew(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void PostMessageToOwner(
        const v8::FunctionCallbackInfo<v8::Value>& args);
    static void PostMessageToWorker(
        const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Terminate(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void NoKeepAlive(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Ref(const v8::FunctionCallbackInfo<v8::Value>& args);
    static void Unref(const v8::FunctionCallbackInfo<v8::Value>& args);

    v8::Platform* platform();

    uv_mutex_t* termination_mutex() {
      return &termination_mutex_;
    }

    uv_mutex_t* initialization_mutex() {
      return &initialization_mutex_;
    }

    uv_thread_t* worker_thread() {
      return &thread_;
    }

    uv_thread_t* owner_thread();

    uv_mutex_t* to_owner_messages_mutex() {
      return &to_owner_messages_mutex_;
    }

    uv_mutex_t* to_worker_messages_mutex() {
      return &to_worker_messages_mutex_;
    }

    NotificationChannel* owner_notifications() {
      return &owner_notifications_;
    }

    NotificationChannel* worker_notifications() {
      return &worker_notifications_;
    }

    int exit_code() const {
      return exit_code_;
    }

    void set_exit_code(int exit_code) {
      exit_code_ = exit_code;
    }

    void set_initialized() {
      is_initialized_ = true;
    }

    TerminationKind termination_kind() const {
      return termination_kind_;
    }

    void set_termination_kind(TerminationKind kind) {
      termination_kind_ = kind;
    }

    void set_worker_wrapper(v8::Local<v8::Object> worker_wrapper);

    uv_loop_t* owner_event_loop() const;

    Environment* worker_env() const {
      return worker_env_;
    }

    Environment* owner_env() const {
      return owner_env_;
    }

    void set_worker_env(Environment* worker_env) {
      CHECK_NE(worker_env, nullptr);
      CHECK_EQ(worker_env_, nullptr);
      worker_env_ = worker_env;
    }

    bool is_initialized() const {
      return is_initialized_;
    }

    v8::Isolate* worker_isolate() const {
      return worker_isolate_;
    }

    // The JS object in the owner thread that wraps this WorkerContext.
    v8::Local<v8::Object> owner_wrapper();
    // The JS object in the worker thread that wraps this WorkerContext.
    v8::Local<v8::Object> worker_wrapper();

    static const uint32_t kPrimaryQueueSize = 2048;
    static const uint32_t kMaxPrimaryQueueMessages = kPrimaryQueueSize - 1;

    Environment* const owner_env_;
    Environment* worker_env_ = nullptr;
    v8::Isolate* worker_isolate_;
    v8::Persistent<v8::Object> owner_wrapper_;
    v8::Persistent<v8::Object> worker_wrapper_;

    bool const keep_alive_ = true;
    char* user_data_ = nullptr;
    bool const eval_ = false;

    LoadExtensionsCb load_extensions_ = nullptr;

    uv_loop_t* event_loop_;
    uv_thread_t thread_;
    uv_mutex_t termination_mutex_;
    uv_mutex_t api_mutex_;
    uv_mutex_t initialization_mutex_;
    uv_mutex_t to_owner_messages_mutex_;
    uv_mutex_t to_worker_messages_mutex_;
    NotificationChannel owner_notifications_;
    NotificationChannel worker_notifications_;

    BSON* worker_bson_;
    BSON* owner_bson_;

    bool is_initialized_ = false;
    bool pending_owner_cleanup_ = false;
    bool should_emit_exit_event_ = true;
    TerminationKind termination_kind_ = NONE;
    int exit_code_ = 0;
    int const argc_;
    const char** const argv_;
    int const exec_argc_;
    const char** const exec_argv_;

    ProducerConsumerQueue<kPrimaryQueueSize,
                          WorkerMessage> to_owner_messages_;
    ProducerConsumerQueue<kPrimaryQueueSize,
                          WorkerMessage> to_worker_messages_;

    std::vector<std::unique_ptr<WorkerMessage>> to_owner_messages_temp_;
    std::vector<std::unique_ptr<WorkerMessage>> to_worker_messages_temp_;

    friend class Environment;
};

}  // namespace node

#endif  // SRC_WORKER_H_
