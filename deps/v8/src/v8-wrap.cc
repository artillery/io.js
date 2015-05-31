// Copyright 2015 Artillery Games, Inc. All rights reserved.
//
// This code, and all derivative work, is the exclusive property of Artillery
// Games, Inc. and may not be used without Artillery Games, Inc.'s authorization.
//
// Author: Mark Logan

#include "include/v8.h"
#include "base/logging.h"

#include <cstring>

using v8::Local;
using v8::FunctionTemplate;
using v8::ObjectTemplate;
using v8::FunctionCallbackInfo;
using v8::Isolate;
using v8::Value;
using v8::Boolean;
using v8::External;
using v8::Object;
using v8::String;
using v8::WeakCallbackData;
using v8::HandleScope;
using v8::Int32;
using v8::Uint32;
using v8::Integer;
using v8::Function;
using v8::Number;
using v8::ArrayBuffer;
using v8::Float32Array;
using v8::Uint8Array;
using v8::ReturnValue;
using v8::Persistent;

namespace v8hidden {

  //typedef v8::HandleScope HandleScope;
  typedef v8::Isolate Isolate;
  typedef v8::Local<v8::Number> Local_Number;
  typedef v8::Local<v8::External> Local_External;
  typedef v8::Local<v8::Int32> Local_Int32;
  typedef v8::Local<v8::Uint32> Local_Uint32;
  typedef v8::Local<v8::Integer> Local_Integer;
  typedef v8::Local<v8::Value> Local_Value;
  typedef v8::Local<v8::Function> Local_Function;
  typedef v8::Local<v8::String> Local_String;
  typedef v8::Local<v8::Object> Local_Object;
  typedef v8::Local<v8::Boolean> Local_Boolean;
  typedef v8::Local<v8::FunctionTemplate> Local_FunctionTemplate;
  typedef v8::Local<v8::ObjectTemplate> Local_ObjectTemplate;
  typedef v8::Local<v8::ArrayBuffer> Local_ArrayBuffer;
  typedef v8::Local<v8::Float32Array> Local_Float32Array;
  typedef v8::Local<v8::Uint8Array> Local_Uint8Array;
  typedef v8::Persistent<v8::FunctionTemplate> Persistent_FunctionTemplate;
  typedef v8::Persistent<v8::Object> Persistent_Object;
  typedef v8::FunctionCallbackInfo<v8::Value> FunctionCallbackInfo_Value;
  typedef v8::ReturnValue<v8::Value> ReturnValue_Value;
  typedef v8::Function Function;

  // v8::HandleScope has a private operator new, so we wrap it in a struct.
  // (Because we're really, really sure that we want to do this.)
  struct HandleScope {
    HandleScope(Isolate* i) : s(i) {}
    v8::HandleScope s;
  };
}

#include "include/v8-wrap-extern.h"


v8hidden::Isolate* V8_Wrap_Isolate_Get_Current() {
  return v8::Isolate::GetCurrent();
}

v8hidden::HandleScope* V8_Wrap_Create_Handle_Scope(v8hidden::Isolate* i) {
  return new v8hidden::HandleScope(i);
}
void V8_Wrap_Delete_Handle_Scope(v8hidden::HandleScope* h) {
  delete h;
}


void V8_Wrap_ThrowException_Error(v8hidden::Isolate* i, const char* s) {
  i->ThrowException(v8::Exception::Error(v8::String::NewFromUtf8(i, s)));
}

void V8_Wrap_ThrowException_Type(v8hidden::Isolate* i, const char* s) {
  i->ThrowException(v8::Exception::TypeError(v8::String::NewFromUtf8(i, s)));
}

#define V8_WRAP_LOCAL_DEFS(sym) \
v8hidden::Local_##sym* V8_Wrap_Local_##sym##_Copy(v8hidden::Local_##sym* other) { \
  return new v8::Local<sym>(*other); \
} \
void V8_Wrap_Local_##sym##_Assign(v8hidden::Local_##sym* a, v8hidden::Local_##sym* b) { *a = *b; } \
void V8_Wrap_Local_##sym##_Delete(v8hidden::Local_##sym* l) { delete l; }


#define V8_WRAP_LOCAL_CONVERSION_DEFS(sym) \
v8hidden::Local_Int32* V8_Wrap_Local_##sym##_ToInt32(v8hidden::Local_##sym* l, v8hidden::Isolate* i) { \
  return new v8::Local<Int32>((*l)->ToInt32(i)); \
} \
v8hidden::Local_Boolean* V8_Wrap_Local_##sym##_ToBoolean(v8hidden::Local_##sym* l, v8hidden::Isolate* i) { \
  return new v8::Local<Boolean>((*l)->ToBoolean(i)); \
} \
v8hidden::Local_Object* V8_Wrap_Local_##sym##_ToObject(v8hidden::Local_##sym* l, v8hidden::Isolate* i) { \
  return new v8::Local<Object>((*l)->ToObject(i)); \
} \

#define V8_WRAP_LOCAL_CASTS(sym) \
v8hidden::Local_ArrayBuffer* V8_Wrap_Local_##sym##_Cast_ArrayBuffer(v8hidden::Local_##sym* l) { \
  return new v8::Local<ArrayBuffer>(v8::Local<ArrayBuffer>::Cast(*l)); \
} \

#define V8_WRAP_LOCAL_PREDICATE_DEFS(sym) \
bool V8_Wrap_Local_##sym##_IsInt32(v8hidden::Local_##sym* l) { \
  return (*l)->IsInt32(); \
} \
bool V8_Wrap_Local_##sym##_IsUint32(v8hidden::Local_##sym* l) { \
  return (*l)->IsUint32(); \
} \
bool V8_Wrap_Local_##sym##_IsBoolean(v8hidden::Local_##sym* l) { \
  return (*l)->IsBoolean(); \
} \
bool V8_Wrap_Local_##sym##_IsObject(v8hidden::Local_##sym* l) { \
  return (*l)->IsObject(); \
} \
bool V8_Wrap_Local_##sym##_IsNumber(v8hidden::Local_##sym* l) { \
  return (*l)->IsNumber(); \
} \
bool V8_Wrap_Local_##sym##_IsString(v8hidden::Local_##sym* l) { \
  return (*l)->IsString(); \
} \
bool V8_Wrap_Local_##sym##_IsFunction(v8hidden::Local_##sym* l) { \
  return (*l)->IsFunction(); \
} \
bool V8_Wrap_Local_##sym##_IsArrayBuffer(v8hidden::Local_##sym* l) { \
  return (*l)->IsArrayBuffer(); \
} \

#define V8_WRAP_LOCAL_VALUE_DEFS(sym) \
int32_t V8_Wrap_Local_##sym##_Int32Value(v8hidden::Local_##sym* l) { \
  return (*l)->Int32Value(); \
} \
uint32_t V8_Wrap_Local_##sym##_Uint32Value(v8hidden::Local_##sym* l) { \
  return (*l)->Uint32Value(); \
} \
bool V8_Wrap_Local_##sym##_BooleanValue(v8hidden::Local_##sym* l) { \
  return (*l)->BooleanValue(); \
} \
double V8_Wrap_Local_##sym##_NumberValue(v8hidden::Local_##sym* l) { \
  return (*l)->NumberValue(); \
}


// Local<Int32>
V8_WRAP_LOCAL_DEFS(Int32)
int32_t V8_Wrap_Local_Int32_Value(v8hidden::Local_Int32* v) {
  return (*v)->Value();
}

// Local<Uint32>
V8_WRAP_LOCAL_DEFS(Uint32)
uint32_t V8_Wrap_Local_Uint32_Value(v8hidden::Local_Uint32* v) {
  return (*v)->Value();
}

// Local<Value>
V8_WRAP_LOCAL_DEFS(Value)
V8_WRAP_LOCAL_CONVERSION_DEFS(Value)
V8_WRAP_LOCAL_PREDICATE_DEFS(Value);
V8_WRAP_LOCAL_VALUE_DEFS(Value);

// Local<Object>

v8hidden::Local_Object*
V8_Wrap_Local_Object_New_Persistent(v8hidden::Isolate* i, v8hidden::Persistent_Object* o) {
  return new Local<Object>(Local<Object>::New(i, *o));
}

int V8_Wrap_Local_Object_InternalFieldCount(v8hidden::Local_Object* o) {
  return (*o)->InternalFieldCount();
}

void V8_Wrap_Local_Object_SetPrototype(v8hidden::Local_Object* f, v8hidden::Local_Value* v) {
  (*f)->SetPrototype(*v);
}

v8hidden::Local_Value* V8_Wrap_Local_Object_GetInternalField(v8hidden::Local_Object* o, int idx) {
  return new v8::Local<Value>((*o)->GetInternalField(idx));
}

void V8_Wrap_Local_Object_SetInternalField(v8hidden::Local_Object* o, int idx, v8hidden::Local_Value* v) {
  (*o)->SetInternalField(idx, *v);
}

v8hidden::Local_Object* V8_Wrap_Local_Object_Copy(v8hidden::Local_Object* other) {
  return new v8::Local<Object>(*other);
}

void V8_Wrap_Local_Object_Assign(v8hidden::Local_Object* a, v8hidden::Local_Object* b) {
  *a = *b;
}

void V8_Wrap_Local_Object_Delete(v8hidden::Local_Object* l) {
  delete l;
}

v8hidden::Local_Boolean* V8_Local_Object_To_Boolean(v8hidden::Local_Object* num) {
  return new v8::Local<Boolean>((*num)->ToBoolean());
}

bool V8_Wrap_Local_Object_IsEmpty(v8hidden::Local_Object* l) {
  return l->IsEmpty();
}

v8hidden::Local_Value* V8_Wrap_Local_Object_Get_String(v8hidden::Local_Object* o,
                                                       v8hidden::Local_String* key) {
  return new Local<Value>((*o)->Get(*key));
}

void V8_Wrap_Local_Object_Set_String_Function(v8hidden::Local_Object* o,
                                              v8hidden::Local_String* key,
                                              v8hidden::Local_Function* f) {
  (*o)->Set(*key, *f);
}

void V8_Wrap_Local_Object_Set_String_Integer(v8hidden::Local_Object* o,
                                              v8hidden::Local_String* key,
                                              v8hidden::Local_Integer* d) {
  (*o)->Set(*key, *d);
}

void V8_Wrap_Local_Object_Set_String_Object(v8hidden::Local_Object* o,
                                            v8hidden::Local_String* key,
                                            v8hidden::Local_Object* d) {
  (*o)->Set(*key, *d);
}


#define V8_WRAP_CAST_TO_LOCAL_VALUE(sym) \
v8hidden::Local_Value* V8_Wrap_Local_##sym##_To_Local_Value(v8hidden::Local_##sym* o) { \
  return new Local<Value>(*o); \
}

V8_WRAP_CAST_TO_LOCAL_VALUE(Object);
V8_WRAP_CAST_TO_LOCAL_VALUE(Boolean);
V8_WRAP_CAST_TO_LOCAL_VALUE(External);
V8_WRAP_CAST_TO_LOCAL_VALUE(Int32);
V8_WRAP_CAST_TO_LOCAL_VALUE(Uint32);
V8_WRAP_CAST_TO_LOCAL_VALUE(Integer);
V8_WRAP_CAST_TO_LOCAL_VALUE(Number);
V8_WRAP_CAST_TO_LOCAL_VALUE(String);


// Local<Function>
V8_WRAP_LOCAL_DEFS(Function);

v8hidden::Local_Value* V8_Wrap_Local_Function_Get_String(v8hidden::Local_Function* f,
                                                         v8hidden::Local_String* key) {
  return new Local<Value>((*f)->Get(*key));
}


// Local<String>
V8_WRAP_LOCAL_DEFS(String);
v8hidden::Local_String* V8_Wrap_Local_String_NewFromUtf8(v8hidden::Isolate* i, const char *s) {
  return new v8::Local<String>(String::NewFromUtf8(i, s));
}

char* V8_Wrap_Local_String_Utf8Value_C_Str(v8hidden::Local_Value* s) {
  String::Utf8Value str(*s);
  char* ret = new char[str.length() + 1];
  strncpy(ret, *str, str.length());
  ret[str.length()] = '\0';
  return ret;
}

// Local<Number>
v8hidden::Local_Number* V8_Wrap_Number_New(v8hidden::Isolate* i, double val) {
  return new v8::Local<Number>(Number::New(i, val));
}

v8hidden::Local_Number* V8_Wrap_Local_Number_Copy(v8hidden::Local_Number* other) {
  return new v8::Local<Number>(*other);
}

void V8_Wrap_Local_Number_Assign(v8hidden::Local_Number* a, v8hidden::Local_Number* b) {
  *a = *b;
}

void V8_Wrap_Local_Number_Delete(v8hidden::Local_Number* l) {
  delete l;
}

// Integer
v8hidden::Local_Integer* V8_Wrap_Integer_New(v8hidden::Isolate* i, int32_t val) {
  return new Local<Integer>(Integer::New(i, val));
}
// Local<Integer>
V8_WRAP_LOCAL_DEFS(Integer);
int64_t V8_Wrap_Local_Integer_Value(v8hidden::Local_Integer* integer) {
  return (*integer)->Value();
}

v8hidden::Local_Int32* V8_Wrap_Local_Integer_ToInt32(v8hidden::Local_Integer* d, v8hidden::Isolate* i) {
  return new Local<Int32>((*d)->ToInt32(i));
}
v8hidden::Local_Uint32* V8_Wrap_Local_Integer_ToUint32(v8hidden::Local_Integer* d, v8hidden::Isolate* i) {
  return new Local<Uint32>((*d)->ToUint32(i));
}

// Boolean
v8hidden::Local_Boolean* V8_Wrap_Boolean_New(v8hidden::Isolate* i, bool val) {
  return new v8::Local<Boolean>(Boolean::New(i, val));
}

// Local<Boolean>
V8_WRAP_LOCAL_DEFS(Boolean);
bool V8_Wrap_Local_Boolean_Value(v8hidden::Local_Boolean* b) {
  return (*b)->Value();
}

// Function
v8hidden::Local_Value* V8_Wrap_Function_Call(v8hidden::Function* f, v8hidden::Local_Object* ctx,
                                             int argc, v8hidden::Local_Value* hiddenArgv[]) {

  // Hacky workaround for lack of variable length non-POD arrays.
  alignas(alignof(Local<Value>)) uint8_t argv_buf[argc * sizeof(Local<Value>)];
  Local<Value>* argv = reinterpret_cast<Local<Value>*>(argv_buf);

  for (int i = 0; i < argc; i++) {
    new (&argv[i]) Local<Value>(*(hiddenArgv[i]));
  }
  v8hidden::Local_Value* ret = new Local<Value>(f->Call(*ctx, argc, argv));
  for (int i = 0; i < argc; i++) {
    argv[i].~Local();
  }
  return ret;
}

v8hidden::Function* V8_Wrap_Function_Cast(v8hidden::Local_Value* f) {
  return Function::Cast(**f);
}

// ArrayBuffer
v8hidden::Local_ArrayBuffer* V8_Wrap_ArrayBuffer_New(v8hidden::Isolate* isolate, void* data,
                                                     size_t byteLength) {
  return new Local<ArrayBuffer>(ArrayBuffer::New(isolate, data, byteLength));
}
void V8_Wrap_Local_ArrayBuffer_GetContents(
  v8hidden::Local_ArrayBuffer* b, void** data /* OUT */, size_t* length /* OUT */) {
  auto contents = (*b)->GetContents();
  *data = contents.Data();
  *length = contents.ByteLength();
}

// Float32Array
v8hidden::Local_Float32Array* V8_Wrap_Float32Array_New(v8hidden::Local_ArrayBuffer* buffer,
                                                       size_t byteOffset, size_t length) {
  return new Local<Float32Array>(Float32Array::New(*buffer, byteOffset, length));
}

// Uint8Array
v8hidden::Local_Uint8Array* V8_Wrap_Uint8Array_New(v8hidden::Local_ArrayBuffer* buffer,
                                                       size_t byteOffset, size_t length) {
  return new Local<Uint8Array>(Uint8Array::New(*buffer, byteOffset, length));
}

// External

V8_WRAP_LOCAL_DEFS(External);

v8hidden::Local_External* V8_Wrap_Local_External_New(v8hidden::Isolate* i, void* v) {
  return new v8::Local<External>(v8::External::New(i, v));
}

void* V8_Wrap_External_Cast_Value(v8hidden::Local_Value* v) {
  return External::Cast(**v)->Value();
}

// FunctionTemplate
v8hidden::Local_FunctionTemplate* V8_Wrap_FunctionTemplate_New(
  v8hidden::Isolate* i, void* userFn, v8wrap::WrappedFunction wrapperFn) {

  static v8wrap::WrappedFunction wrapper = nullptr;
  if (wrapper == nullptr) {
    wrapper = wrapperFn;
  } else {
    CHECK(wrapper == wrapperFn);
  }

  v8::Local<External> callback = External::New(i, userFn);

  return new v8::Local<v8::FunctionTemplate>(
    v8::FunctionTemplate::New(
      i,
      [](const v8::FunctionCallbackInfo<Value>& args) {
        void* fn = External::Cast(*(args.Data()))->Value();
        wrapper(args.GetIsolate(), fn, &args);
      },
      callback
    )
  );
}

// Local<FunctionTemplate>
V8_WRAP_LOCAL_DEFS(FunctionTemplate);

v8hidden::Local_FunctionTemplate* V8_Wrap_Local_FunctionTemplate_New_Persistent(
  v8hidden::Isolate* i, v8hidden::Persistent_FunctionTemplate* t) {
  return new Local<FunctionTemplate>(Local<FunctionTemplate>::New(i, *t));
}

v8hidden::Local_ObjectTemplate* V8_Wrap_Local_FunctionTemplate_PrototypeTemplate(
  v8hidden::Local_FunctionTemplate* f) {
  return new Local<ObjectTemplate>((*f)->PrototypeTemplate());
}

v8hidden::Local_ObjectTemplate* V8_Wrap_Local_FunctionTemplate_InstanceTemplate(
  v8hidden::Local_FunctionTemplate* f) {
  return new Local<ObjectTemplate>((*f)->InstanceTemplate());
}
void V8_Wrap_Local_FunctionTemplate_Inherit(
  v8hidden::Local_FunctionTemplate* child, v8hidden::Local_FunctionTemplate* parent) {
  (*child)->Inherit(*parent);
}

// Local<ObjectTemplate>
V8_WRAP_LOCAL_DEFS(ObjectTemplate);
void V8_Wrap_Local_ObjectTemplate_Set_Func(
  v8hidden::Local_ObjectTemplate* o,
  v8hidden::Local_String* key,
  v8hidden::Local_Function* function) {
  (*o)->Set(*key, *function);
}

v8hidden::Local_ObjectTemplate* V8_Wrap_Local_ObjectTemplate_New(v8hidden::Isolate* i) {
  return new Local<ObjectTemplate>(ObjectTemplate::New(i));
}
void V8_Wrap_Local_ObjectTemplate_SetInternalFieldCount(v8hidden::Local_ObjectTemplate* o, int count) {
  (*o)->SetInternalFieldCount(count);
}

v8hidden::Local_Object*
V8_Wrap_Local_ObjectTemplate_NewInstance(v8hidden::Local_ObjectTemplate* o) {
  return new Local<Object>((*o)->NewInstance());
}

// Local<ArrayBuffer>
V8_WRAP_LOCAL_DEFS(ArrayBuffer);
// Local<Float32Array>
V8_WRAP_LOCAL_DEFS(Float32Array);
// Local<Uint8Array>
V8_WRAP_LOCAL_DEFS(Uint8Array);

// FunctionCallbackInfo<Value>
v8hidden::Local_Value*
V8_Wrap_FunctionCallbackInfo_Value_subscript(const v8hidden::FunctionCallbackInfo_Value* v, int i) {
  return new Local<Value>((*v)[i]);
}
int V8_Wrap_FunctionCallbackInfo_Value_Length(const v8hidden::FunctionCallbackInfo_Value* v) {
  return v->Length();
}
v8hidden::Local_Object*
V8_Wrap_FunctionCallbackInfo_Value_This(const v8hidden::FunctionCallbackInfo_Value* v) {
  return new Local<Object>(v->This());
}

// ReturnValue<Value>
v8hidden::ReturnValue_Value*
V8_Wrap_FunctionCallbackInfo_Value_GetReturnValue(const v8hidden::FunctionCallbackInfo_Value* v) {
  return new ReturnValue<Value>(v->GetReturnValue());
}

bool V8_Wrap_FunctionCallbackInfo_Value_IsConstructCall(const v8hidden::FunctionCallbackInfo_Value* v) {
  return v->IsConstructCall();
}

v8hidden::ReturnValue_Value* V8_Wrap_ReturnValue_Copy(v8hidden::ReturnValue_Value* v) {
  return new ReturnValue<Value>(*v);
}

void V8_Wrap_ReturnValue_Assign(v8hidden::ReturnValue_Value* a, v8hidden::ReturnValue_Value* b) {
  *a = *b;
}

void V8_Wrap_ReturnValue_Delete(v8hidden::ReturnValue_Value* v) {
  delete v;
}

// Persistent<FunctionTemplate>
v8hidden::Persistent_FunctionTemplate* V8_Wrap_Create_Persistent_FunctionTemplate(
  v8hidden::Isolate* i, v8hidden::Local_FunctionTemplate* f) {
  return new Persistent<FunctionTemplate>(i, *f);
}
void V8_Wrap_Delete_Persistent_FunctionTemplate(v8hidden::Persistent_FunctionTemplate* f) {
  delete f;
}

v8hidden::Local_Function* V8_Wrap_Local_FunctionTemplate_GetFunction(
  v8hidden::Local_FunctionTemplate* f) {
  return new Local<Function>((*f)->GetFunction());
}

template <typename T>
struct SetWeakTraits;

template <>
struct SetWeakTraits<FunctionTemplate> {
  typedef v8hidden::Persistent_FunctionTemplate Type;
  typedef v8wrap::WrappedWeakCallback_FunctionTemplate Callback;
  struct CallbackData {
    void* data;
    Callback cb;
  };
};

template <>
struct SetWeakTraits<Object> {
  typedef v8hidden::Persistent_Object Type;
  typedef v8wrap::WrappedWeakCallback_Object Callback;
  struct CallbackData {
    void* data;
    Callback cb;
  };
};

template <typename T>
void SetWeakImpl(typename SetWeakTraits<T>::Type* f,
                 void* data,
                 typename SetWeakTraits<T>::Callback cb,
                 uint8_t buf[16]) {

  typedef typename SetWeakTraits<T>::CallbackData CallbackData;

  static_assert(sizeof(CallbackData) <= 16, "buf[16] is not large enough");

  auto cbData = new (buf) CallbackData { data, cb };

  f->SetWeak(cbData,
    [](const WeakCallbackData<T, CallbackData>& data) {
      Isolate* isolate = data.GetIsolate();
      CallbackData* cbData = data.GetParameter();

      auto handle = new Local<T>(data.GetValue());

      cbData->cb(isolate, handle, cbData->data);
    }
  );
};

void V8_Wrap_Persistant_FunctionTemplate_SetWeak(
  v8hidden::Persistent_FunctionTemplate* f,
  void* data,
  v8wrap::WrappedWeakCallback_FunctionTemplate cb,
  uint8_t buf[16]) {
  SetWeakImpl<FunctionTemplate>(f, data, cb, buf);
}

// Persistent<Object>
v8hidden::Persistent_Object* V8_Wrap_Persistent_Object_Create(
  v8hidden::Isolate* i, v8hidden::Local_Object* f) {
  return new Persistent<Object>(i, *f);
}
void V8_Wrap_Persistent_Object_Delete(v8hidden::Persistent_Object* f) {
  delete f;
}

void V8_Wrap_Persistent_Object_Reset(v8hidden::Persistent_Object* p) {
  p->Reset();
}

void V8_Wrap_Persistent_Object_ClearWeak(v8hidden::Persistent_Object* p) {
  p->ClearWeak();
}

void V8_Wrap_Persistant_Object_SetWeak(
  v8hidden::Persistent_Object* f,
  void* data,
  v8wrap::WrappedWeakCallback_Object cb,
  uint8_t buf[16]) {
  SetWeakImpl<Object>(f, data, cb, buf);
}

void V8_Wrap_ReturnValue_Set_bool(v8hidden::ReturnValue_Value* v, bool a) { v->Set(a); }
void V8_Wrap_ReturnValue_Set_int32_t(v8hidden::ReturnValue_Value* v, int32_t a) { v->Set(a); }
void V8_Wrap_ReturnValue_Set_uint32_t(v8hidden::ReturnValue_Value* v, uint32_t a) { v->Set(a); }
void V8_Wrap_ReturnValue_Set_double(v8hidden::ReturnValue_Value* v, double a) { v->Set(a); }

void V8_Wrap_ReturnValue_Set_Local_Object(v8hidden::ReturnValue_Value* v,
                                          v8hidden::Local_Object* o) {
  v->Set(*o);
}
void V8_Wrap_ReturnValue_Set_Local_Boolean(v8hidden::ReturnValue_Value* v,
                                          v8hidden::Local_Boolean* b) {
  v->Set(*b);
}
void V8_Wrap_ReturnValue_Set_Local_String(v8hidden::ReturnValue_Value* v, v8hidden::Local_String* a) {
  v->Set(*a);
}
void V8_Wrap_ReturnValue_Set_Local_Value(v8hidden::ReturnValue_Value* v, v8hidden::Local_Value* a) {
  v->Set(*a);
}
void V8_Wrap_ReturnValue_Set_Local_Float32Array(v8hidden::ReturnValue_Value* v,
                                                v8hidden::Local_Float32Array* a) {
  v->Set(*a);
}
void V8_Wrap_ReturnValue_Set_Local_Uint8Array(v8hidden::ReturnValue_Value* v,
                                              v8hidden::Local_Uint8Array* a) {
  v->Set(*a);
}
