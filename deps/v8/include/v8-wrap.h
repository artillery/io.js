// Copyright 2015 Artillery Games, Inc. All rights reserved.
//
// This code, and all derivative work, is the exclusive property of Artillery
// Games, Inc. and may not be used without Artillery Games, Inc.'s authorization.
//
// Author: Mark Logan

#ifndef _V8_WRAP
#define _V8_WRAP

namespace v8hidden {
  class HandleScope;
  class Isolate;
  class Local_Number;
  class Local_External;
  class Local_Int32;
  class Local_Uint32;
  class Local_Integer;
  class Local_Value;
  class Local_Function;
  class Local_String;
  class Local_Object;
  class Local_Boolean;
  class Local_FunctionTemplate;
  class Local_ObjectTemplate;
  class Local_ArrayBuffer;
  class Local_Float32Array;
  class Local_Uint8Array;
  class Persistent_FunctionTemplate;
  class Persistent_Object;
  class FunctionCallbackInfo_Value;
  class ReturnValue_Value;
  class Function;
}

#include "v8-wrap-extern.h"

#include <utility>

namespace v8wrap {

class Isolate {
 public:
  static v8hidden::Isolate* GetCurrent() {
    return V8_Wrap_Isolate_Get_Current();
  }

  static void ThrowException_Error(v8hidden::Isolate* i, const char* s) {
    V8_Wrap_ThrowException_Error(i, s);
  }
  static void ThrowException_Type(v8hidden::Isolate* i, const char* s) {
    V8_Wrap_ThrowException_Type(i, s);
  }
};

class HandleScope {
 public:
  HandleScope(v8hidden::Isolate* i) {
    _impl = V8_Wrap_Create_Handle_Scope(i);
  }
  ~HandleScope() {
    if (_impl != nullptr) {
      V8_Wrap_Delete_Handle_Scope(_impl);
    }
    _impl = nullptr;
  }

 private:
  HandleScope(const HandleScope&) = delete;
  HandleScope& operator=(const HandleScope&) = delete;

  HandleScope(HandleScope&&) = delete;
  HandleScope& operator=(HandleScope&&) = delete;

  v8hidden::HandleScope* _impl;
};

class Value;
class Function;
class Number;
class Object;
class Boolean;
class Int32;
class Uint32;
class Integer;
class String;
class External;
class FunctionTemplate;
class ObjectTemplate;
class ArrayBuffer;
class Float32Array;
class Uint8Array;

template <typename T>
class LocalTraits;

template <typename T>
class Persistent;

template <typename T>
class Local;

#define V8_WRAP_LOCAL_TRAITS_BOILERPLATE(sym) \
typedef v8hidden::Local_##sym Type; \
static Type* Copy(Type* other) { return V8_Wrap_Local_##sym##_Copy(other); } \
static void Assign(Type* a, Type* b) { V8_Wrap_Local_##sym##_Assign(a, b); } \
static void Destroy(Type* t) { V8_Wrap_Local_##sym##_Delete(t); }


template <> class LocalTraits<Int32> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Int32)

  static Local<Value> To_Local_Value(Local<Int32> o);

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}
    int32_t Value() { return V8_Wrap_Local_Int32_Value(_val); }

   private:
    Type* _val;
  };
};

template <> class LocalTraits<Uint32> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Uint32)

  static Local<Value> To_Local_Value(Local<Uint32> o);

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}
    uint32_t Value() { return V8_Wrap_Local_Uint32_Value(_val); }

   private:
    Type* _val;
  };
};

template <> class LocalTraits<Integer> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Integer)

  static Local<Value> To_Local_Value(Local<Integer> o);

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}
    int32_t Value() { return V8_Wrap_Local_Integer_Value(_val); }

    Local<Uint32> ToUint32(v8hidden::Isolate* i);
    Local<Int32> ToInt32(v8hidden::Isolate* i);

   private:
    Type* _val;
  };
};


template <> class LocalTraits<Function> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Function)

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}

    Local<Value> Get(Local<String> key);

   private:
    Type* _val;
  };
};

template <> class LocalTraits<String> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(String)

  static Local<Value> To_Local_Value(Local<String> o);

  class Wrap {
   public:
    explicit Wrap(Type* val) {}
  };
};


template <> class LocalTraits<Value> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Value)

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}
    Local<Int32> ToInt32(v8hidden::Isolate* i);
    Local<Boolean> ToBoolean(v8hidden::Isolate* i);
    Local<Object> ToObject(v8hidden::Isolate* i) const;
    Type* getHidden() { return _val; }
    Type* operator*() { return _val; }

    bool IsObject() { return V8_Wrap_Local_Value_IsObject(_val); }
    bool IsBoolean() { return V8_Wrap_Local_Value_IsBoolean(_val); }
    bool IsInt32() { return V8_Wrap_Local_Value_IsInt32(_val); }
    bool IsUint32() { return V8_Wrap_Local_Value_IsUint32(_val); }
    bool IsNumber() { return V8_Wrap_Local_Value_IsNumber(_val); }
    bool IsString() { return V8_Wrap_Local_Value_IsString(_val); }
    bool IsFunction() { return V8_Wrap_Local_Value_IsFunction(_val); }

    int32_t Int32Value() const { return V8_Wrap_Local_Value_Int32Value(_val); }
    uint32_t Uint32Value() const { return V8_Wrap_Local_Value_Uint32Value(_val); }
    bool BooleanValue() const { return V8_Wrap_Local_Value_BooleanValue(_val); }
    double NumberValue() const { return V8_Wrap_Local_Value_NumberValue(_val); }

   private:
    Type* _val;
  };
};

template <> class LocalTraits<Object> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Object);

  static Local<Object>
  NewFromPersistent(v8hidden::Isolate* i, const Persistent<Object>& p);

  static bool IsEmpty(Type* l) { return V8_Wrap_Local_Object_IsEmpty(l); }

  static Local<Value> To_Local_Value(Local<Object> o);

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}
    int InternalFieldCount() { return V8_Wrap_Local_Object_InternalFieldCount(_val); }
    Local<Value> GetInternalField(int i);
    void SetInternalField(int i, Local<Value> v);

    Local<Value> Get(Local<String> key);
    void SetPrototype(Local<Value> p);

    void Set(Local<String> key, Local<Function> f);
    void Set(Local<String> key, Local<Integer> d);
    void Set(Local<String> key, Local<Object> o);

   private:
    Type* _val;
  };
};

template <> class LocalTraits<Number> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Number);

  static Local<Value> To_Local_Value(Local<Number> o);

  class Wrap {
   public:
    explicit Wrap(Type* val) {}
  };
};

template <> class LocalTraits<Boolean> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Boolean);

  static Local<Value> To_Local_Value(Local<Boolean> b);

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}
    int32_t Value() { return V8_Wrap_Local_Boolean_Value(_val); }

   private:
    Type* _val;
  };
};

template <> class LocalTraits<External> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(External);

  static Local<Value> To_Local_Value(Local<External> o);

  class Wrap {
   public:
    explicit Wrap(Type* val) {}
  };
};

template <> class LocalTraits<FunctionTemplate> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(FunctionTemplate);

  static Local<FunctionTemplate>
  NewFromPersistent(v8hidden::Isolate* i, const Persistent<FunctionTemplate>& p);

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}

    Local<ObjectTemplate> PrototypeTemplate();

    Local<Function> GetFunction();

    Local<ObjectTemplate> InstanceTemplate();

    void Inherit(Local<FunctionTemplate> f);

   private:
    Type* _val;
  };
};

template <> class LocalTraits<ObjectTemplate> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(ObjectTemplate);

  class Wrap {
   public:
    explicit Wrap(Type* val) : _val(val) {}

    void Set(Local<String> key, Local<Function> fn);

    void SetInternalFieldCount(int count) {
      V8_Wrap_Local_ObjectTemplate_SetInternalFieldCount(_val, count);
    }

    Local<Object> NewInstance();

   private:
    Type* _val;
  };
};

template <> class LocalTraits<ArrayBuffer> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(ArrayBuffer);

  class Wrap {
   public:
    explicit Wrap(Type* val) {}
  };
};

template <> class LocalTraits<Float32Array> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Float32Array);

  class Wrap {
   public:
    explicit Wrap(Type* val) {}
  };
};

template <> class LocalTraits<Uint8Array> {
 public:
  V8_WRAP_LOCAL_TRAITS_BOILERPLATE(Uint8Array);

  class Wrap {
   public:
    explicit Wrap(Type* val) {}
  };
};

template <typename T>
class Local {
 public:
  explicit Local(typename LocalTraits<T>::Type* l) : _l(l), _wrap(_l) {}

  Local(const Local<T>& other) : _l(LocalTraits<T>::Copy(other._l)), _wrap(_l) {}

  Local<T>& operator=(const Local<T>& other) {
    LocalTraits<T>::Assign(_l, other._l);
    return *this;
  }

  ~Local() {
    LocalTraits<T>::Destroy(_l);
  }

  operator Local<Value>();

  typename LocalTraits<T>::Wrap* operator->() { return &_wrap; }
  const typename LocalTraits<T>::Wrap* operator->() const { return &_wrap; }
  typename LocalTraits<T>::Wrap* operator*() { return &_wrap; }
  typename LocalTraits<T>::Type* getHidden() { return _l; }

  static Local<T> New(v8hidden::Isolate* i, const Persistent<T>& p);

  bool IsEmpty() { return LocalTraits<T>::IsEmpty(_l); }

 private:
  typename LocalTraits<T>::Type* const _l;

  typename LocalTraits<T>::Wrap _wrap;
};

template <typename T>
Local<T>::operator Local<Value>() {
  return LocalTraits<T>::To_Local_Value(*this);
}

template <typename T>
class ReturnValue {
 public:
  typedef v8hidden::ReturnValue_Value Type;

  explicit ReturnValue(Type* v) : _v(v) {}

  ReturnValue(const ReturnValue<T>& other)
    : _v(V8_Wrap_ReturnValue_Copy(other._v)) {}

  ReturnValue<T>& operator=(const ReturnValue<T>& other) {
    V8_Wrap_ReturnValue_Assign(_v, other._v);
    return *this;
  }

  ~ReturnValue() {
    if (_v != nullptr) {
      V8_Wrap_ReturnValue_Delete(_v);
      _v = nullptr;
    }
  }

  void Set(bool a) { V8_Wrap_ReturnValue_Set_bool(_v, a); }
  void Set(int32_t a) { V8_Wrap_ReturnValue_Set_int32_t(_v, a); }
  void Set(uint32_t a) { V8_Wrap_ReturnValue_Set_uint32_t(_v, a); }
  void Set(double a) { V8_Wrap_ReturnValue_Set_double(_v, a); }

  void Set(Local<String> a) { V8_Wrap_ReturnValue_Set_Local_String(_v, a.getHidden()); }
  void Set(Local<Value> a) { V8_Wrap_ReturnValue_Set_Local_Value(_v, a.getHidden()); }
  void Set(Local<Float32Array> a) { V8_Wrap_ReturnValue_Set_Local_Float32Array(_v, a.getHidden()); }
  void Set(Local<Uint8Array> a) { V8_Wrap_ReturnValue_Set_Local_Uint8Array(_v, a.getHidden()); }
  void Set(Local<Object> o) { V8_Wrap_ReturnValue_Set_Local_Object(_v, o.getHidden()); }
  void Set(Local<Boolean> b) { V8_Wrap_ReturnValue_Set_Local_Boolean(_v, b.getHidden()); }

 private:
  Type* _v;
};

template <typename T, typename P>
class WeakCallbackData {
 public:
  typedef void(*Callback)(const WeakCallbackData&);

  WeakCallbackData(v8hidden::Isolate* i, Local<T> h, P* p) : _param(p), _handle(h), _i(i) {}

  P* GetParameter() const { return _param; }
  Local<T> GetValue() const {
    return _handle;
  }
  v8hidden::Isolate* GetIsolate() const { return _i; }

 private:
  P* _param;
  Local<T> _handle;
  v8hidden::Isolate* _i;
};

template <typename T>
class PersistentTraits;

template <>
class PersistentTraits<FunctionTemplate> {
 public:
  typedef v8hidden::Persistent_FunctionTemplate Type;

  static Type* Create(v8hidden::Isolate* i, v8hidden::Local_FunctionTemplate* f) {
    return V8_Wrap_Create_Persistent_FunctionTemplate(i, f);
  }
  static void Destroy(Type* f) {
    V8_Wrap_Delete_Persistent_FunctionTemplate(f);
  }

  static void SetWeak(Type* val, void* data,
                      void(*cb)(v8hidden::Isolate*, LocalTraits<FunctionTemplate>::Type*, void*),
                      uint8_t buf[16]) {
    V8_Wrap_Persistant_FunctionTemplate_SetWeak(val, data, cb, buf);
  }
};

template <>
class PersistentTraits<Object> {
 public:
  typedef v8hidden::Persistent_Object Type;

  static Type* Create(v8hidden::Isolate* i, v8hidden::Local_Object* f) {
    return V8_Wrap_Persistent_Object_Create(i, f);
  }
  static void Destroy(Type* f) {
    V8_Wrap_Persistent_Object_Delete(f);
  }

  static void Reset(Type* p) {
    V8_Wrap_Persistent_Object_Reset(p);
  }

  static void ClearWeak(Type* p) {
    V8_Wrap_Persistent_Object_ClearWeak(p);
  }

  static void SetWeak(Type* val, void* data,
                      void(*cb)(v8hidden::Isolate*, LocalTraits<Object>::Type*, void*),
                      uint8_t buf[16]) {
    V8_Wrap_Persistant_Object_SetWeak(val, data, cb, buf);
  }
};

template <typename T>
class PersistentWrap {
 public:
  explicit PersistentWrap(typename PersistentTraits<T>::Type* val) : _val(val) {}
  PersistentWrap(const PersistentWrap&) = delete;
  PersistentWrap& operator=(const PersistentWrap&) = delete;

  PersistentWrap(PersistentWrap&& other) : _val(other._val) { other._val = nullptr; }

  template <typename P>
  struct CallbackData {
    P* parameter;
    typename WeakCallbackData<T, P>::Callback fn;
  };

  template <typename P>
  static void WeakCallback(v8hidden::Isolate* i, typename LocalTraits<T>::Type* handlePtr,
                           void* param) {
    PersistentWrap* _this = reinterpret_cast<PersistentWrap*>(param);
    auto data = reinterpret_cast<CallbackData<P>*>(_this->_cbData);
    CHECK(data != nullptr, "Possible double-call of WeakCallback");

    Local<T> handle(handlePtr);
    WeakCallbackData<T, P> cbData(i, handle, data->parameter);
    data->fn(cbData);
    delete data;
    _this->_cbData = nullptr;
  }

  template <typename P>
  void SetWeak(P* parameter, typename WeakCallbackData<T, P>::Callback cb) {
    if (_cbData != nullptr) {
      auto cbData = reinterpret_cast<CallbackData<P>*>(_cbData);
      delete cbData;
      _cbData = nullptr;
    }
    _cbData = new CallbackData<P> { parameter, cb };
    PersistentTraits<T>::SetWeak(_val, this, WeakCallback<P>, _buf);
  }

 private:
  typename PersistentTraits<T>::Type* _val;
  void* _cbData = nullptr;
  uint8_t _buf[16];
};

template <typename T>
class Persistent {
 public:
  explicit Persistent(v8hidden::Isolate* i, Local<T> handle)
    : _p(PersistentTraits<T>::Create(i, handle.getHidden())), _wrap(_p) {}
  ~Persistent() {
    if (_p != nullptr) {
      PersistentTraits<T>::Destroy(_p);
      _p = nullptr;
    }
  }

  Persistent(const Persistent<T>&) = delete;
  Persistent<T>& operator=(const Persistent<T>&) = delete;

  Persistent(Persistent<T>&& other) : _p(other._p), _wrap(std::move(other._wrap)) {
    other._p = nullptr;
  }

  template <typename P>
  void SetWeak(P* param, typename WeakCallbackData<T, P>::Callback cb) {
    _wrap.SetWeak(param, cb);
  }

  void Reset() { PersistentTraits<T>::Reset(_p); }
  void ClearWeak() { PersistentTraits<T>::ClearWeak(_p); }

  typename PersistentTraits<T>::Type* getHidden() const { return _p; }

 private:
  typename PersistentTraits<T>::Type* _p;
  PersistentWrap<T> _wrap;
  //typename PersistentTraits<T>::Wrap _wrap;
};

template <typename T>
Local<T> Local<T>::New(v8hidden::Isolate* i, const Persistent<T>& p) {
  return LocalTraits<T>::NewFromPersistent(i, p);
}

// LocalTraits<Object>
Local<Object> LocalTraits<Object>::NewFromPersistent(
  v8hidden::Isolate* i, const Persistent<Object>& o) {
  return Local<Object>(V8_Wrap_Local_Object_New_Persistent(i, o.getHidden()));
}

Local<Value> LocalTraits<Object>::Wrap::GetInternalField(int i) {
  return Local<Value>(V8_Wrap_Local_Object_GetInternalField(_val, i));
}
void LocalTraits<Object>::Wrap::SetInternalField(int i, Local<Value> v) {
  V8_Wrap_Local_Object_SetInternalField(_val, i, v.getHidden());
}
Local<Value> LocalTraits<Object>::Wrap::Get(Local<String> key) {
  return Local<Value>(V8_Wrap_Local_Object_Get_String(_val, key.getHidden()));
}
void LocalTraits<Object>::Wrap::SetPrototype(Local<Value> p) {
  V8_Wrap_Local_Object_SetPrototype(_val, p.getHidden());
}
void LocalTraits<Object>::Wrap::Set(Local<String> key, Local<Function> p) {
  V8_Wrap_Local_Object_Set_String_Function(_val, key.getHidden(), p.getHidden());
}
void LocalTraits<Object>::Wrap::Set(Local<String> key, Local<Integer> d) {
  V8_Wrap_Local_Object_Set_String_Integer(_val, key.getHidden(), d.getHidden());
}
void LocalTraits<Object>::Wrap::Set(Local<String> key, Local<Object> o) {
  V8_Wrap_Local_Object_Set_String_Object(_val, key.getHidden(), o.getHidden());
}
Local<Value> LocalTraits<Object>::To_Local_Value(Local<Object> o) {
  return Local<Value>(V8_Wrap_Local_Object_To_Local_Value(o.getHidden()));
}

// LocalTraits<String>
Local<Value> LocalTraits<String>::To_Local_Value(Local<String> s) {
  return Local<Value>(V8_Wrap_Local_String_To_Local_Value(s.getHidden()));
}

// LocalTraits<Uint32>
Local<Value> LocalTraits<Uint32>::To_Local_Value(Local<Uint32> o) {
  return Local<Value>(V8_Wrap_Local_Uint32_To_Local_Value(o.getHidden()));
}

// LocalTraits<Int32>
Local<Value> LocalTraits<Int32>::To_Local_Value(Local<Int32> o) {
  return Local<Value>(V8_Wrap_Local_Int32_To_Local_Value(o.getHidden()));
}

// LocalTraits<Boolean>
Local<Value> LocalTraits<Boolean>::To_Local_Value(Local<Boolean> b) {
  return Local<Value>(V8_Wrap_Local_Boolean_To_Local_Value(b.getHidden()));
}

// LocalTraits<Function>
Local<Value> LocalTraits<Function>::Wrap::Get(Local<String> key) {
  return Local<Value>(V8_Wrap_Local_Function_Get_String(_val, key.getHidden()));
}

// LocalTraits<Value>
Local<Int32> LocalTraits<Value>::Wrap::ToInt32(v8hidden::Isolate* i) {
  return Local<Int32>(V8_Wrap_Local_Value_ToInt32(_val, i));
}
Local<Object> LocalTraits<Value>::Wrap::ToObject(v8hidden::Isolate* i) const {
  return Local<Object>(V8_Wrap_Local_Value_ToObject(_val, i));
}

// LocalTraits<Number>
Local<Value> LocalTraits<Number>::To_Local_Value(Local<Number> n) {
  return Local<Value>(V8_Wrap_Local_Number_To_Local_Value(n.getHidden()));
}

// LocalTraits<Integer>
Local<Uint32> LocalTraits<Integer>::Wrap::ToUint32(v8hidden::Isolate* i) {
  return Local<Uint32>(V8_Wrap_Local_Integer_ToUint32(_val, i));
}
Local<Int32> LocalTraits<Integer>::Wrap::ToInt32(v8hidden::Isolate* i) {
  return Local<Int32>(V8_Wrap_Local_Integer_ToInt32(_val, i));
}
Local<Value> LocalTraits<Integer>::To_Local_Value(Local<Integer> o) {
  return Local<Value>(V8_Wrap_Local_Integer_To_Local_Value(o.getHidden()));
}

// LocalTraits<FunctionTemplate>
Local<FunctionTemplate> LocalTraits<FunctionTemplate>::NewFromPersistent(
  v8hidden::Isolate* i, const Persistent<FunctionTemplate>& p) {
  return Local<FunctionTemplate>(V8_Wrap_Local_FunctionTemplate_New_Persistent(i, p.getHidden()));
}
Local<ObjectTemplate> LocalTraits<FunctionTemplate>::Wrap::PrototypeTemplate() {
  return Local<ObjectTemplate>(V8_Wrap_Local_FunctionTemplate_PrototypeTemplate(_val));
}
Local<ObjectTemplate> LocalTraits<FunctionTemplate>::Wrap::InstanceTemplate() {
  return Local<ObjectTemplate>(V8_Wrap_Local_FunctionTemplate_InstanceTemplate(_val));
}
void LocalTraits<FunctionTemplate>::Wrap::Inherit(Local<FunctionTemplate> f) {
  V8_Wrap_Local_FunctionTemplate_Inherit(_val, f.getHidden());
}

void LocalTraits<ObjectTemplate>::Wrap::Set(Local<String> key, Local<Function> fn) {
  V8_Wrap_Local_ObjectTemplate_Set_Func(_val, key.getHidden(), fn.getHidden());
}

Local<Object> LocalTraits<ObjectTemplate>::Wrap::NewInstance() {
  return Local<Object>(V8_Wrap_Local_ObjectTemplate_NewInstance(_val));
}

Local<Function> LocalTraits<FunctionTemplate>::Wrap::GetFunction() {
  return Local<Function>(V8_Wrap_Local_FunctionTemplate_GetFunction(_val));
}

// LocalTraits<External>
Local<Value> LocalTraits<External>::To_Local_Value(Local<External> o) {
  return Local<Value>(V8_Wrap_Local_External_To_Local_Value(o.getHidden()));
}

class Number {
 public:
  static Local<Number> New(v8hidden::Isolate* i, double value) {
    return Local<Number>(V8_Wrap_Number_New(i, value));
  }

 private:
  Number() = default;

};

class Integer {
 public:
  static Local<Integer> New(v8hidden::Isolate* i, int32_t val) {
    return Local<Integer>(V8_Wrap_Integer_New(i, val));
  }
 private:
  Integer() = default;
};

class Boolean {
 public:
  static Local<Boolean> New(v8hidden::Isolate* i, bool val) {
    return Local<Boolean>(V8_Wrap_Boolean_New(i, val));
  }
 private:
  Boolean() = default;
};

class String {
 public:
  static Local<String> NewFromUtf8(v8hidden::Isolate* i, const char* s) {
    return Local<String>(V8_Wrap_Local_String_NewFromUtf8(i, s));
  }

  class Utf8Value {
   public:
    Utf8Value(Local<Value> val) : _s(V8_Wrap_Local_String_Utf8Value_C_Str(val.getHidden())) {}
    ~Utf8Value() { delete[] _s; }
    Utf8Value(const Utf8Value&) = delete;
    Utf8Value& operator=(const Utf8Value&) = delete;

    const char* operator*() const { return _s; }
   private:
    char* _s;
  };
};

class Function {
 public:
  class FunctionWrap {
   public:
    explicit FunctionWrap(v8hidden::Function* fn) : _fn(fn) {}

    Local<Value> Call(Local<Object> ctx, int argc, Local<Value> argv[]) {
      v8hidden::Local_Value* hiddenArgv[argc + 1];
      for (int i = 0; i < argc; i++) {
        hiddenArgv[i] = argv[i].getHidden();
      }
      return Local<Value>(V8_Wrap_Function_Call(_fn, ctx.getHidden(), argc, hiddenArgv));
    }
   private:
    v8hidden::Function* _fn;
  };

  FunctionWrap* operator->() { return &_wrap; }

  static Function Cast(LocalTraits<Value>::Wrap* v) {
    return Function(V8_Wrap_Function_Cast(v->getHidden()));
  }

 private:
  Function(v8hidden::Function* fn) : _wrap(fn) {}
  FunctionWrap _wrap;
};

class ArrayBuffer {
 public:
  static Local<ArrayBuffer> New(v8hidden::Isolate* isolate, void* data, size_t byteLength) {
    return Local<ArrayBuffer>(V8_Wrap_ArrayBuffer_New(isolate, data, byteLength));
  }
};

class Float32Array {
 public:
  static Local<Float32Array> New(Local<ArrayBuffer> buffer, size_t byteOffset, size_t length) {
    return Local<Float32Array>(V8_Wrap_Float32Array_New(buffer.getHidden(), byteOffset, length));
  }
};

class Uint8Array {
 public:
  static Local<Uint8Array> New(Local<ArrayBuffer> buffer, size_t byteOffset, size_t length) {
    return Local<Uint8Array>(V8_Wrap_Uint8Array_New(buffer.getHidden(), byteOffset, length));
  }
};


class External {
 public:
  static Local<External> New(v8hidden::Isolate* i, void* value);
  static External Cast(LocalTraits<Value>::Wrap* val);

  void* Value() { return _val; }

  External* operator->() { return this; }

 private:
  explicit External(void* val) : _val(val) {}

  void* _val;
};

typedef LocalTraits<Value>::Wrap ValueWrap;
Local<External> External::New(v8hidden::Isolate* i, void* value) {
  return Local<External>(V8_Wrap_Local_External_New(i, value));
}
External External::Cast(ValueWrap* val) {
  return External(V8_Wrap_External_Cast_Value(**val));
}

template <typename T>
class FunctionCallbackInfo;

template <>
class FunctionCallbackInfo<Value> {
 public:
  FunctionCallbackInfo(v8hidden::Isolate* i, const v8hidden::FunctionCallbackInfo_Value* v)
    : _i(i), _v(v) {}

  bool IsConstructCall() const {
    return V8_Wrap_FunctionCallbackInfo_Value_IsConstructCall(_v);
  }

  v8hidden::Isolate* GetIsolate() const { return _i; }

  int Length() const { return V8_Wrap_FunctionCallbackInfo_Value_Length(_v); }
  Local<Value> operator[](int i) const {
    return Local<Value>(V8_Wrap_FunctionCallbackInfo_Value_subscript(_v, i));
  }

  Local<Object> This() const {
    return Local<Object>(V8_Wrap_FunctionCallbackInfo_Value_This(_v));
  }

  ReturnValue<Value> GetReturnValue() const {
    return ReturnValue<Value>(V8_Wrap_FunctionCallbackInfo_Value_GetReturnValue(_v));
  }

 private:
  v8hidden::Isolate* _i;
  const v8hidden::FunctionCallbackInfo_Value* _v;
};

typedef void(*FunctionCallback)(const FunctionCallbackInfo<Value>& args);

class FunctionTemplate {
 public:
  static Local<FunctionTemplate> New(v8hidden::Isolate* i, FunctionCallback fn) {
    return Local<FunctionTemplate>(V8_Wrap_FunctionTemplate_New(
      i,
      reinterpret_cast<void*>(fn),
      [](v8hidden::Isolate* i, void* userFnVoid, const v8hidden::FunctionCallbackInfo_Value* args) {
        FunctionCallbackInfo<Value> wrappedArgs(i, args);
        auto userFn = reinterpret_cast<FunctionCallback>(userFnVoid);
        userFn(wrappedArgs);
      })
    );
  }
};

class ObjectTemplate {
 public:
  static Local<ObjectTemplate> New(v8hidden::Isolate* i) {
    return Local<ObjectTemplate>(V8_Wrap_Local_ObjectTemplate_New(i));
  }
};

};

#endif // _V8_WRAP
