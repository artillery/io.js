// Copyright 2015 Artillery Games, Inc. All rights reserved.
//
// This code, and all derivative work, is the exclusive property of Artillery
// Games, Inc. and may not be used without Artillery Games, Inc.'s authorization.
//
// Author: Mark Logan

// C-linkage functions for interacting with the v8 api.
//
// The naming convention is:
//
//   V8_Wrap_<class>_<method> for simple methods.
//   V8_Wrap_<class>_<method>_<arguments> for overloaded methods.
//
// If `class` is a template, then it is named as `<template>_<paramter>`, e.g.
// Local_Object for Local<Object>

#ifndef _V8_WRAP_EXTERN
#define _V8_WRAP_EXTERN

#include <stdint.h>
#include <stddef.h>

namespace v8 {
  class Value;
  template <typename T>
  class FunctionCallbackInfo;
}

//typedef void (*WrappedFunction)(const FunctionCallbackInfo<Value>&);
namespace v8wrap {
  typedef void (*WrappedFunction)(v8hidden::Isolate*, void*, const v8hidden::FunctionCallbackInfo_Value*);
  typedef void (*WrappedWeakCallback_FunctionTemplate)
               (v8hidden::Isolate*, v8hidden::Local_FunctionTemplate*, void*);
  typedef void (*WrappedWeakCallback_Object)
               (v8hidden::Isolate*, v8hidden::Local_Object*, void*);
}



#define V8_WRAP_LOCAL_DECLS(sym) \
v8hidden::Local_##sym* V8_Wrap_Local_##sym##_Copy(v8hidden::Local_##sym* other); \
V8_EXPORT void V8_Wrap_Local_##sym##_Assign(v8hidden::Local_##sym* a, v8hidden::Local_##sym* b); \
V8_EXPORT void V8_Wrap_Local_##sym##_Delete(v8hidden::Local_##sym* n);

#define V8_WRAP_LOCAL_CONVERSION_DECLS(sym) \
V8_EXPORT v8hidden::Local_Int32* V8_Wrap_Local_##sym##_ToInt32(v8hidden::Local_##sym* l, v8hidden::Isolate* i); \
V8_EXPORT v8hidden::Local_Boolean* V8_Wrap_Local_##sym##_ToBoolean(v8hidden::Local_##sym* l, v8hidden::Isolate* i); \
V8_EXPORT v8hidden::Local_Object* V8_Wrap_Local_##sym##_ToObject(v8hidden::Local_##sym* l, v8hidden::Isolate* i); \

#define V8_WRAP_LOCAL_CAST_DECLS(sym) \
V8_EXPORT v8hidden::Local_ArrayBuffer* V8_Wrap_Local_##sym##_Cast_ArrayBuffer(v8hidden::Local_##sym* l); \
V8_EXPORT v8hidden::Local_TypedArray* V8_Wrap_Local_##sym##_Cast_TypedArray(v8hidden::Local_##sym* l); \

#define V8_WRAP_LOCAL_PREDICATE_DECLS(sym) \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsInt32(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsUint32(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsBoolean(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsObject(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsNumber(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsString(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsFunction(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsArrayBuffer(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_IsTypedArray(v8hidden::Local_##sym* l); \

#define V8_WRAP_LOCAL_VALUE_DECLS(sym) \
V8_EXPORT int32_t V8_Wrap_Local_##sym##_Int32Value(v8hidden::Local_##sym* l); \
V8_EXPORT uint32_t V8_Wrap_Local_##sym##_Uint32Value(v8hidden::Local_##sym* l); \
V8_EXPORT bool V8_Wrap_Local_##sym##_BooleanValue(v8hidden::Local_##sym* l); \
V8_EXPORT double V8_Wrap_Local_##sym##_NumberValue(v8hidden::Local_##sym* l);

extern "C" {

// Exceptions
V8_EXPORT void V8_Wrap_ThrowException_Error(v8hidden::Isolate* i, const char* s);

V8_EXPORT void V8_Wrap_ThrowException_Type(v8hidden::Isolate* i, const char* s);

// Isolate
V8_EXPORT v8hidden::Isolate* V8_Wrap_Isolate_Get_Current();

// Local<Int32>
V8_WRAP_LOCAL_DECLS(Int32);

V8_EXPORT int32_t V8_Wrap_Local_Int32_Value(v8hidden::Local_Int32* v);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_Int32_To_Local_Value(v8hidden::Local_Int32*);

// Local<Uint32>
V8_WRAP_LOCAL_DECLS(Int32);

V8_WRAP_LOCAL_DECLS(Uint32);

V8_EXPORT uint32_t V8_Wrap_Local_Uint32_Value(v8hidden::Local_Uint32* v);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_Uint32_To_Local_Value(v8hidden::Local_Uint32*);

// Local<String>
V8_WRAP_LOCAL_DECLS(String);

V8_EXPORT v8hidden::Local_String* V8_Wrap_Local_String_NewFromUtf8(v8hidden::Isolate* i, const char *s);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_String_To_Local_Value(v8hidden::Local_String*);

V8_EXPORT char* V8_Wrap_Local_String_Utf8Value_C_Str(v8hidden::Local_Value* s);

// Local<Value>
V8_WRAP_LOCAL_DECLS(Value);
V8_WRAP_LOCAL_CONVERSION_DECLS(Value);
V8_WRAP_LOCAL_CAST_DECLS(Value);
V8_WRAP_LOCAL_PREDICATE_DECLS(Value);
V8_WRAP_LOCAL_VALUE_DECLS(Value);

// Local<Object>
V8_WRAP_LOCAL_DECLS(Object);

v8hidden::Local_Object*
V8_Wrap_Local_Object_New_Persistent(v8hidden::Isolate*, v8hidden::Persistent_Object*);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_Object_GetInternalField(v8hidden::Local_Object* o, int idx);

V8_EXPORT void V8_Wrap_Local_Object_SetInternalField(v8hidden::Local_Object* o, int idx, v8hidden::Local_Value* v);

V8_EXPORT void V8_Wrap_Local_Object_SetInternalField_Boolean(v8hidden::Local_Object* o, int idx, v8hidden::Local_Boolean* v);

V8_EXPORT int V8_Wrap_Local_Object_InternalFieldCount(v8hidden::Local_Object* o);

V8_EXPORT v8hidden::Local_Object* V8_Wrap_Local_Object_New(v8hidden::Isolate* i, double val);

v8hidden::Local_Boolean* V8_Local_Object_To_Boolean(v8hidden::Local_Object* num);

V8_EXPORT bool V8_Wrap_Local_Object_IsEmpty(v8hidden::Local_Object* l);

v8hidden::Local_Value*
V8_Wrap_Local_Object_Get_String(v8hidden::Local_Object*, v8hidden::Local_String*);

V8_EXPORT void V8_Wrap_Local_Object_SetPrototype(v8hidden::Local_Object*, v8hidden::Local_Value*);

V8_EXPORT void V8_Wrap_Local_Object_Set_String_Function(
  v8hidden::Local_Object* o, v8hidden::Local_String* key, v8hidden::Local_Function* f);

V8_EXPORT void V8_Wrap_Local_Object_Set_String_Integer(
  v8hidden::Local_Object* o, v8hidden::Local_String* key, v8hidden::Local_Integer* f);

V8_EXPORT void V8_Wrap_Local_Object_Set_String_Object(
  v8hidden::Local_Object* o, v8hidden::Local_String* key, v8hidden::Local_Object* d);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_Object_To_Local_Value(v8hidden::Local_Object* o);

// Local<Function>
V8_WRAP_LOCAL_DECLS(Function);

v8hidden::Local_Value*
V8_Wrap_Local_Function_Get_String(v8hidden::Local_Function*, v8hidden::Local_String*);

// Number
V8_EXPORT v8hidden::Local_Number* V8_Wrap_Number_New(v8hidden::Isolate* i, double val);

// Local<Number>
V8_EXPORT v8hidden::Local_Number* V8_Wrap_Local_Number_Copy(v8hidden::Local_Number* other);

V8_EXPORT void V8_Wrap_Local_Number_Assign(v8hidden::Local_Number* a, v8hidden::Local_Number* b);

V8_EXPORT void V8_Wrap_Local_Number_Delete(v8hidden::Local_Number* n);

v8hidden::Local_Boolean* V8_Local_Number_To_Boolean(v8hidden::Local_Number* num);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_Number_To_Local_Value(v8hidden::Local_Number* n);

// Integer
V8_EXPORT v8hidden::Local_Integer* V8_Wrap_Integer_New(v8hidden::Isolate* i, int32_t val);

// Local<Integer>
V8_WRAP_LOCAL_DECLS(Integer);

V8_EXPORT int64_t V8_Wrap_Local_Integer_Value(v8hidden::Local_Integer*);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_Integer_To_Local_Value(v8hidden::Local_Integer*);

V8_EXPORT v8hidden::Local_Int32* V8_Wrap_Local_Integer_ToInt32(v8hidden::Local_Integer* d, v8hidden::Isolate* i);

V8_EXPORT v8hidden::Local_Uint32* V8_Wrap_Local_Integer_ToUint32(v8hidden::Local_Integer* d, v8hidden::Isolate* i);

// Local<Boolean>
V8_WRAP_LOCAL_DECLS(Boolean);

V8_EXPORT v8hidden::Local_Boolean* V8_Wrap_Boolean_New(v8hidden::Isolate* i, bool val);

V8_EXPORT bool V8_Wrap_Local_Boolean_Value(v8hidden::Local_Boolean* v);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_Boolean_To_Local_Value(v8hidden::Local_Boolean*);

// Local<External>
V8_WRAP_LOCAL_DECLS(External);

V8_EXPORT v8hidden::Local_Value* V8_Wrap_Local_External_To_Local_Value(v8hidden::Local_External*);

// Function
V8_EXPORT v8hidden::Local_Value* V8_Wrap_Function_Call(
  v8hidden::Function*, v8hidden::Local_Object* ctx, int argc, v8hidden::Local_Value* argv[]);

V8_EXPORT v8hidden::Function* V8_Wrap_Function_Cast(v8hidden::Local_Value* f);

V8_EXPORT v8hidden::HandleScope* V8_Wrap_Create_Handle_Scope(v8hidden::Isolate* i);

V8_EXPORT void V8_Wrap_Delete_Handle_Scope(v8hidden::HandleScope* h);

void *V8_Wrap_External_Cast_Value(v8hidden::Local_Value* v);

V8_EXPORT v8hidden::Local_External* V8_Wrap_Local_External_New(v8hidden::Isolate* i, void* v);

// FunctionTemplate
V8_EXPORT v8hidden::Local_FunctionTemplate* V8_Wrap_FunctionTemplate_New(
  v8hidden::Isolate* i, void* userFn, v8wrap::WrappedFunction fn);

// Local<FunctionTemplate>
V8_WRAP_LOCAL_DECLS(FunctionTemplate);

V8_EXPORT v8hidden::Local_FunctionTemplate* V8_Wrap_Local_FunctionTemplate_New_Persistent(
  v8hidden::Isolate* i, v8hidden::Persistent_FunctionTemplate*);

V8_EXPORT v8hidden::Local_ObjectTemplate* V8_Wrap_Local_FunctionTemplate_PrototypeTemplate(
  v8hidden::Local_FunctionTemplate* f);

V8_EXPORT v8hidden::Local_ObjectTemplate* V8_Wrap_Local_FunctionTemplate_InstanceTemplate(
  v8hidden::Local_FunctionTemplate* f);

V8_EXPORT void V8_Wrap_Local_FunctionTemplate_Inherit(
  v8hidden::Local_FunctionTemplate* _val, v8hidden::Local_FunctionTemplate* f);

V8_EXPORT v8hidden::Local_Function* V8_Wrap_Local_FunctionTemplate_GetFunction(
  v8hidden::Local_FunctionTemplate* f);

// Local<ObjectTemplate>
V8_WRAP_LOCAL_DECLS(ObjectTemplate);

V8_EXPORT v8hidden::Local_ObjectTemplate* V8_Wrap_Local_ObjectTemplate_New(v8hidden::Isolate*);

V8_EXPORT void V8_Wrap_Local_ObjectTemplate_Set_Func(
  v8hidden::Local_ObjectTemplate* o,
  v8hidden::Local_String* key,
  v8hidden::Local_Function* function);

V8_EXPORT void V8_Wrap_Local_ObjectTemplate_SetInternalFieldCount(v8hidden::Local_ObjectTemplate*, int);

V8_EXPORT v8hidden::Local_Object* V8_Wrap_Local_ObjectTemplate_NewInstance(v8hidden::Local_ObjectTemplate* o);

// Local<ArrayBuffer>
V8_WRAP_LOCAL_DECLS(ArrayBuffer);
V8_EXPORT void V8_Wrap_Local_ArrayBuffer_GetContents(
  v8hidden::Local_ArrayBuffer* b, void** data /* OUT */, size_t* length /* OUT */);

// Local<TypedArray>
V8_WRAP_LOCAL_DECLS(TypedArray);
V8_EXPORT size_t V8_Wrap_Local_TypedArray_ByteOffset(v8hidden::Local_TypedArray* a);
V8_EXPORT size_t V8_Wrap_Local_TypedArray_ByteLength(v8hidden::Local_TypedArray* a);
V8_EXPORT v8hidden::Local_ArrayBuffer* V8_Wrap_Local_TypedArray_Buffer(v8hidden::Local_TypedArray* a);

// Local<Float32Array>
V8_WRAP_LOCAL_DECLS(Float32Array);

// Local<Uint8Array>
V8_WRAP_LOCAL_DECLS(Uint8Array);

// FunctionCallbackInfo<Value>
v8hidden::Local_Value*
V8_Wrap_FunctionCallbackInfo_Value_subscript(const v8hidden::FunctionCallbackInfo_Value* v, int i);

V8_EXPORT int V8_Wrap_FunctionCallbackInfo_Value_Length(const v8hidden::FunctionCallbackInfo_Value* v);

v8hidden::Local_Object*
V8_Wrap_FunctionCallbackInfo_Value_This(const v8hidden::FunctionCallbackInfo_Value* v);

v8hidden::ReturnValue_Value*
V8_Wrap_FunctionCallbackInfo_Value_GetReturnValue(const v8hidden::FunctionCallbackInfo_Value* v);

V8_EXPORT bool V8_Wrap_FunctionCallbackInfo_Value_IsConstructCall(const v8hidden::FunctionCallbackInfo_Value* v);

// ReturnValue<Value>
V8_EXPORT v8hidden::ReturnValue_Value* V8_Wrap_ReturnValue_Copy(v8hidden::ReturnValue_Value*);

V8_EXPORT void V8_Wrap_ReturnValue_Assign(v8hidden::ReturnValue_Value*, v8hidden::ReturnValue_Value*);

V8_EXPORT void V8_Wrap_ReturnValue_Delete(v8hidden::ReturnValue_Value*);

V8_EXPORT void V8_Wrap_ReturnValue_Set_bool(v8hidden::ReturnValue_Value* v, bool a);

V8_EXPORT void V8_Wrap_ReturnValue_Set_int32_t(v8hidden::ReturnValue_Value* v, int32_t a);

V8_EXPORT void V8_Wrap_ReturnValue_Set_uint32_t(v8hidden::ReturnValue_Value* v, uint32_t a);

V8_EXPORT void V8_Wrap_ReturnValue_Set_double(v8hidden::ReturnValue_Value* v, double a);

V8_EXPORT void V8_Wrap_ReturnValue_Set_Local_Object(v8hidden::ReturnValue_Value* v, v8hidden::Local_Object* o);

V8_EXPORT void V8_Wrap_ReturnValue_Set_Local_Boolean(v8hidden::ReturnValue_Value* v, v8hidden::Local_Boolean* o);

V8_EXPORT void V8_Wrap_ReturnValue_Set_Local_String(v8hidden::ReturnValue_Value* v, v8hidden::Local_String* a);

V8_EXPORT void V8_Wrap_ReturnValue_Set_Local_Value(v8hidden::ReturnValue_Value* v, v8hidden::Local_Value* a);

V8_EXPORT void V8_Wrap_ReturnValue_Set_Local_Float32Array(v8hidden::ReturnValue_Value* v, v8hidden::Local_Float32Array* a);

V8_EXPORT void V8_Wrap_ReturnValue_Set_Local_Uint8Array(v8hidden::ReturnValue_Value* v, v8hidden::Local_Uint8Array* a);

// Persistent<FunctionTemplate>
V8_EXPORT v8hidden::Persistent_FunctionTemplate* V8_Wrap_Create_Persistent_FunctionTemplate(
  v8hidden::Isolate* i, v8hidden::Local_FunctionTemplate* f);

V8_EXPORT void V8_Wrap_Delete_Persistent_FunctionTemplate(v8hidden::Persistent_FunctionTemplate* f);

V8_EXPORT void V8_Wrap_Persistant_FunctionTemplate_SetWeak(
  v8hidden::Persistent_FunctionTemplate* o,
  void* data,
  void(*cb)(v8hidden::Isolate*, v8hidden::Local_FunctionTemplate*, void*),
  uint8_t buf[16]
);

// Persistent<Object>
V8_EXPORT v8hidden::Persistent_Object* V8_Wrap_Persistent_Object_Create(
  v8hidden::Isolate* i, v8hidden::Local_Object* f);

V8_EXPORT void V8_Wrap_Persistent_Object_Delete(v8hidden::Persistent_Object* p);

V8_EXPORT void V8_Wrap_Persistent_Object_Reset(v8hidden::Persistent_Object* p);

V8_EXPORT void V8_Wrap_Persistent_Object_ClearWeak(v8hidden::Persistent_Object* p);

V8_EXPORT void V8_Wrap_Persistant_Object_SetWeak(
  v8hidden::Persistent_Object* o,
  void* data,
  void(*cb)(v8hidden::Isolate*, v8hidden::Local_Object*, void*),
  uint8_t buf[16]
);

// ArrayBuffer
V8_EXPORT v8hidden::Local_ArrayBuffer* V8_Wrap_ArrayBuffer_New(v8hidden::Isolate*, void*, size_t);

// Float32Array
V8_EXPORT v8hidden::Local_Float32Array* V8_Wrap_Float32Array_New(v8hidden::Local_ArrayBuffer*, size_t, size_t);

// Uint8Array
V8_EXPORT v8hidden::Local_Uint8Array* V8_Wrap_Uint8Array_New(v8hidden::Local_ArrayBuffer*, size_t, size_t);


}

#endif // _V8_WRAP_EXTERN
