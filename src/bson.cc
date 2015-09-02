//===========================================================================

#include <stdarg.h>
#include <cstdlib>
#include <cstring>
#include <string.h>
#include <stdlib.h>

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include <v8.h>

// this and the above block must be around the v8.h header otherwise
// v8 is not happy
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include <node.h>
#include <node_version.h>
#include <node_buffer.h>

#include <cmath>
#include <iostream>
#include <limits>
#include <vector>

#if defined(__sun) || defined(_AIX)
  #include <alloca.h>
#endif

#include "bson.h"

using namespace v8;
using namespace node;

static Local<Int32> NewInt(Isolate* iso, int32_t d) { return Integer::New(iso, d)->ToInt32(iso); }
static Local<Uint32> NewInt(Isolate* iso, uint32_t d) { return Integer::NewFromUnsigned(iso, d)->ToUint32(iso); }
static Local<Number> NewNum(Isolate* iso, double d) { return Number::New(iso, d); }
static Local<Number> NewNum(Isolate* iso, int64_t d) { return Number::New(iso, d); }
static Local<String> NewString(Isolate* iso, const char* s) { return String::NewFromUtf8(iso, s); }

static void AssignPersistent(Isolate* iso, Persistent<String>& p, Local<String> s) {
  p.Reset(iso, s);
}

//===========================================================================

void DataStream::WriteObjectId(Isolate* iso, const Handle<Object>& object, const Handle<String>& key)
{
  uint16_t buffer[12];
  object->Get(key)->ToString(iso)->Write(buffer, 0, 12);
  for(uint32_t i = 0; i < 12; ++i)
  {
    *p++ = (char) buffer[i];
  }
}

void ThrowAllocatedStringException(size_t allocationSize, const char* format, ...)
{
  va_list args;
  va_start(args, format);
  char* string = (char*) malloc(allocationSize);
  vsprintf(string, format, args);
  va_end(args);

  assert(false); // exceptions disabled in node
}

void DataStream::CheckKey(const Local<String>& keyName)
{
  size_t keyLength = keyName->Utf8Length();
  if(keyLength == 0) return;

  // Allocate space for the key, do not need to zero terminate as WriteUtf8 does it
  char* keyStringBuffer = (char*) alloca(keyLength + 1);
  // Write the key to the allocated buffer
  keyName->WriteUtf8(keyStringBuffer);
  // Check for the zero terminator
  char* terminator = strchr(keyStringBuffer, 0x00);

  // If the location is not at the end of the string we've got an illegal 0x00 byte somewhere
  if(terminator != &keyStringBuffer[keyLength]) {
    ThrowAllocatedStringException(64+keyLength, "key %s must not contain null bytes", keyStringBuffer);
  }

  if(keyStringBuffer[0] == '$')
  {
    ThrowAllocatedStringException(64+keyLength, "key %s must not start with '$'", keyStringBuffer);
  }

  if(strchr(keyStringBuffer, '.') != NULL)
  {
    ThrowAllocatedStringException(64+keyLength, "key %s must not contain '.'", keyStringBuffer);
  }
}

// Data points to start of element list, length is length of entire document including '\0' but excluding initial size
BSONDeserializer::BSONDeserializer(Isolate* i_, BSON* aBson, char* data, size_t length)
: iso(i_),
  bson(aBson),
  pStart(data),
  p(data),
  pEnd(data + length - 1)
{
  if(*pEnd != '\0') ThrowAllocatedStringException(64, "Missing end of document marker '\\0'");
}

BSONDeserializer::BSONDeserializer(BSONDeserializer& parentSerializer, size_t length)
: iso(parentSerializer.iso),
  bson(parentSerializer.bson),
  pStart(parentSerializer.p),
  p(parentSerializer.p),
  pEnd(parentSerializer.p + length - 1)
{
  parentSerializer.p += length;
  if(pEnd > parentSerializer.pEnd) ThrowAllocatedStringException(64, "Child document exceeds parent's bounds");
  if(*pEnd != '\0') ThrowAllocatedStringException(64, "Missing end of document marker '\\0'");
}

Handle<Value> BSONDeserializer::ReadCString()
{
  char* start = p;
  while(*p++ && (p < pEnd)) { }
  if(p > pEnd) {
    return Null(iso);
  }
  return String::NewFromUtf8(iso, start, String::kNormalString, p - start - 1);
}

int32_t BSONDeserializer::ReadRegexOptions()
{
  int32_t options = 0;
  for(;;)
  {
    switch(*p++)
    {
    case '\0': return options;
    case 's': options |= RegExp::kGlobal; break;
    case 'i': options |= RegExp::kIgnoreCase; break;
    case 'm': options |= RegExp::kMultiline; break;
    }
  }
}

uint32_t BSONDeserializer::ReadIntegerString()
{
  uint32_t value = 0;
  while(*p)
  {
    if(*p < '0' || *p > '9') ThrowAllocatedStringException(64, "Invalid key for array");
    value = value * 10 + *p++ - '0';
  }
  ++p;
  return value;
}

Local<String> BSONDeserializer::ReadString()
{
  uint32_t length = ReadUInt32();
  char* start = p;
  p += length;
  return String::NewFromUtf8(iso, start, String::kNormalString, length - 1);
}

Local<String> BSONDeserializer::ReadObjectId()
{
  uint16_t objectId[12];
  for(size_t i = 0; i < 12; ++i)
  {
    objectId[i] = *reinterpret_cast<unsigned char*>(p++);
  }
  return String::NewFromTwoByte(iso, objectId, String::kNormalString, 12);
}

Handle<Value> BSONDeserializer::DeserializeDocument(bool promoteLongs)
{
  uint32_t length = ReadUInt32();
  if(length < 5) ThrowAllocatedStringException(64, "Bad BSON: Document is less than 5 bytes");

  BSONDeserializer documentDeserializer(*this, length-4);
  return documentDeserializer.DeserializeDocumentInternal(promoteLongs);
}

Handle<Value> BSONDeserializer::DeserializeDocumentInternal(bool promoteLongs)
{
  Local<Object> returnObject = Object::New(iso);

  while(HasMoreData())
  {
    BsonType type = (BsonType) ReadByte();
    const Handle<Value>& name = ReadCString();
    if(name->IsNull()) ThrowAllocatedStringException(64, "Bad BSON Document: illegal CString");
    // name->Is
    const Handle<Value>& value = DeserializeValue(type, promoteLongs);
    returnObject->ForceSet(name, value);
  }
  if(p != pEnd) ThrowAllocatedStringException(64, "Bad BSON Document: Serialize consumed unexpected number of bytes");

  // From JavaScript:
  // if(object['$id'] != null) object = new DBRef(object['$ref'], object['$id'], object['$db']);
  if(returnObject->Has(NewFromPersistent(iso, bson->_dbRefIdRefString)))
  {
    Local<Value> argv[] = {
      returnObject->Get(NewFromPersistent(iso, bson->_dbRefRefString)),
      returnObject->Get(NewFromPersistent(iso, bson->_dbRefIdRefString)),
      returnObject->Get(NewFromPersistent(iso, bson->_dbRefDbRefString))
    };
    return NewFromPersistent(iso, bson->dbrefConstructor)->NewInstance(3, argv);
  }
  else
  {
    return returnObject;
  }
}

Handle<Value> BSONDeserializer::DeserializeArray(bool promoteLongs)
{
  uint32_t length = ReadUInt32();
  if(length < 5) ThrowAllocatedStringException(64, "Bad BSON: Array Document is less than 5 bytes");

  BSONDeserializer documentDeserializer(*this, length-4);
  return documentDeserializer.DeserializeArrayInternal(promoteLongs);
}

Handle<Value> BSONDeserializer::DeserializeArrayInternal(bool promoteLongs)
{
  Local<Array> returnArray = Array::New(iso);

  while(HasMoreData())
  {
    BsonType type = (BsonType) ReadByte();
    uint32_t index = ReadIntegerString();
    const Handle<Value>& value = DeserializeValue(type, promoteLongs);
    returnArray->Set(index, value);
  }
  if(p != pEnd) ThrowAllocatedStringException(64, "Bad BSON Array: Serialize consumed unexpected number of bytes");

  return returnArray;
}

static int getTypedArrayElementBytesize(TypedArrayType type)
{
  switch (type) {
    case TYPED_ARRAY_INT8:
    case TYPED_ARRAY_UINT8:
    case TYPED_ARRAY_UINT8_CLAMPED:
      return 1;

    case TYPED_ARRAY_INT16:
    case TYPED_ARRAY_UINT16:
      return 2;

    case TYPED_ARRAY_INT32:
    case TYPED_ARRAY_UINT32:
    case TYPED_ARRAY_FLOAT32:
      return 4;

    case TYPED_ARRAY_FLOAT64:
      return 8;
  }
  ThrowAllocatedStringException(64, "Unhandled type array type %d", type);
}

Handle<Value> BSONDeserializer::DeserializeValue(BsonType type, bool promoteLongs)
{
  switch(type)
  {
  case BSON_TYPE_STRING:
    return ReadString();

  case BSON_TYPE_INT:
    return NewInt(iso, ReadInt32());

  case BSON_TYPE_NUMBER:
    return NewNum(iso, ReadDouble());

  case BSON_TYPE_NULL:
    return Null(iso);

  case BSON_TYPE_UNDEFINED:
    return Undefined(iso);

  case BSON_TYPE_TIMESTAMP:
    {
      int32_t lowBits = ReadInt32();
      int32_t highBits = ReadInt32();
      Local<Value> argv[] = { NewInt(iso, lowBits), NewInt(iso, highBits) };
      return NewFromPersistent(iso, bson->timestampConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_BOOLEAN:
    return (ReadByte() != 0) ? True(iso) : False(iso);

  case BSON_TYPE_REGEXP:
    {
      const Handle<Value>& regex = ReadCString();
      if(regex->IsNull()) ThrowAllocatedStringException(64, "Bad BSON Document: illegal CString");
      int32_t options = ReadRegexOptions();
      return RegExp::New(regex->ToString(iso), (RegExp::Flags) options);
    }

  case BSON_TYPE_CODE:
    {
      const Local<Value>& code = ReadString();
      const Local<Value>& scope = Object::New(iso);
      Local<Value> argv[] = { code, scope };
      return NewFromPersistent(iso, bson->codeConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_CODE_W_SCOPE:
    {
      ReadUInt32();
      const Local<Value>& code = ReadString();
      const Handle<Value>& scope = DeserializeDocument(promoteLongs);
      Local<Value> argv[] = { code, scope->ToObject(iso) };
      return NewFromPersistent(iso, bson->codeConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_OID:
    {
      Local<Value> argv[] = { ReadObjectId() };
      return NewFromPersistent(iso, bson->objectIDConstructor)->NewInstance(1, argv);
    }

  case BSON_TYPE_BINARY:
    {
      uint32_t length = ReadUInt32();
      uint32_t subType = ReadByte();
      if(subType == 0x02) {
        length = ReadInt32();
      }

      Local<Object> buffer = Buffer::New(iso, p, length).ToLocalChecked();
      p += length;

      Handle<Value> argv[] = { buffer, NewInt(iso, subType) };
      return NewFromPersistent(iso, bson->binaryConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_LONG:
    {
      // Read 32 bit integers
      int32_t lowBits = (int32_t) ReadInt32();
      int32_t highBits = (int32_t) ReadInt32();

      // Promote long is enabled
      if(promoteLongs) {
        // If value is < 2^53 and >-2^53
        if((highBits < 0x200000 || (highBits == 0x200000 && lowBits == 0)) && highBits >= -0x200000) {
          // Adjust the pointer and read as 64 bit value
          p -= 8;
          // Read the 64 bit value
          int64_t finalValue = (int64_t) ReadInt64();
          return NewNum(iso, finalValue);
        }
      }

      // Decode the Long value
      Local<Value> argv[] = { NewInt(iso, lowBits), NewInt(iso, highBits) };
      return NewFromPersistent(iso, bson->longConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_DATE:
    return Date::New(iso, ReadInt64());

  case BSON_TYPE_ARRAY:
    return DeserializeArray(promoteLongs);

  case BSON_TYPE_OBJECT:
    return DeserializeDocument(promoteLongs);

  case BSON_TYPE_SYMBOL:
    {
      const Local<String>& string = ReadString();
      Local<Value> argv[] = { string };
      return NewFromPersistent(iso, bson->symbolConstructor)->NewInstance(1, argv);
    }

  case BSON_TYPE_MIN_KEY:
    return NewFromPersistent(iso, bson->minKeyConstructor)->NewInstance();

  case BSON_TYPE_MAX_KEY:
    return NewFromPersistent(iso, bson->maxKeyConstructor)->NewInstance();

  case BSON_TYPE_TYPED_ARRAY:
    {
      TypedArrayType arrayType = static_cast<TypedArrayType>(ReadByte());
      size_t viewLength = ReadInt64();
      size_t viewOffset = ReadInt64();
      size_t bufferLength = ReadInt64();
      unsigned char* pointer = static_cast<unsigned char*>(ReadPointer());

      int elementCount = viewLength / getTypedArrayElementBytesize(arrayType);

      Local<ArrayBuffer> buffer;
      if (pointer == NULL)
      {
        // ArrayBuffer was not transferred, must allocate new one.
        buffer = ArrayBuffer::New(iso, bufferLength);
        unsigned char* p = static_cast<unsigned char*>(buffer->GetContents().Data());
        if (p == NULL) { return Null(iso); }
        for (size_t i = 0; i < bufferLength; i++) { p[i] = ReadByte(); }
      }
      else
      {
        // Create buffer from existing memory. The array buffer has been externalized and neutered
        // when the array was transferred here.
        buffer = ArrayBuffer::New(iso, pointer, bufferLength,
                                  v8::ArrayBufferCreationMode::kInternalized);
      }

      switch (arrayType) {
        case TYPED_ARRAY_INT8:          return Int8Array::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_UINT8:         return Uint8Array::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_UINT8_CLAMPED: return Uint8ClampedArray::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_INT16:         return Int16Array::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_UINT16:        return Uint16Array::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_INT32:         return Int32Array::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_UINT32:        return Uint32Array::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_FLOAT32:       return Float32Array::New(buffer, viewOffset, elementCount);
        case TYPED_ARRAY_FLOAT64:       return Float64Array::New(buffer, viewOffset, elementCount);
      }
      ThrowAllocatedStringException(64, "Unhandled TypedArrayType arrayType: %d", arrayType);
      return Null(iso);
    }

  default:
    ThrowAllocatedStringException(64, "Unhandled BSON Type: %d", type);
  }

  return Null(iso);
}

Persistent<FunctionTemplate> BSON::constructor_template;

BSON::BSON(Isolate* iso)
{
  // Setup pre-allocated comparision objects
  AssignPersistent(iso, _bsontypeString, NewString(iso, "_bsontype"));
  AssignPersistent(iso, _longLowString, NewString(iso, "low_"));
  AssignPersistent(iso, _longHighString, NewString(iso, "high_"));
  AssignPersistent(iso, _objectIDidString, NewString(iso, "id"));
  AssignPersistent(iso, _binaryPositionString, NewString(iso, "position"));
  AssignPersistent(iso, _binarySubTypeString, NewString(iso, "sub_type"));
  AssignPersistent(iso, _binaryBufferString, NewString(iso, "buffer"));
  AssignPersistent(iso, _doubleValueString, NewString(iso, "value"));
  AssignPersistent(iso, _symbolValueString, NewString(iso, "value"));
  AssignPersistent(iso, _dbRefRefString, NewString(iso, "$ref"));
  AssignPersistent(iso, _dbRefIdRefString, NewString(iso, "$id"));
  AssignPersistent(iso, _dbRefDbRefString, NewString(iso, "$db"));
  AssignPersistent(iso, _dbRefNamespaceString, NewString(iso, "namespace"));
  AssignPersistent(iso, _dbRefDbString, NewString(iso, "db"));
  AssignPersistent(iso, _dbRefOidString, NewString(iso, "oid"));
  AssignPersistent(iso, _codeCodeString, NewString(iso, "code"));
  AssignPersistent(iso, _codeScopeString, NewString(iso, "scope"));
  AssignPersistent(iso, _toBSONString, NewString(iso, "toBSON"));

  AssignPersistent(iso, longString, NewString(iso, "Long"));
  AssignPersistent(iso, objectIDString, NewString(iso, "ObjectID"));
  AssignPersistent(iso, binaryString, NewString(iso, "Binary"));
  AssignPersistent(iso, codeString, NewString(iso, "Code"));
  AssignPersistent(iso, dbrefString, NewString(iso, "DBRef"));
  AssignPersistent(iso, symbolString, NewString(iso, "Symbol"));
  AssignPersistent(iso, doubleString, NewString(iso, "Double"));
  AssignPersistent(iso, timestampString, NewString(iso, "Timestamp"));
  AssignPersistent(iso, minKeyString, NewString(iso, "MinKey"));
  AssignPersistent(iso, maxKeyString, NewString(iso, "MaxKey"));
}

Local<Object> BSON::GetSerializeObject(Isolate* iso, const Handle<Value>& argValue)
{
  Local<Object> object = argValue->ToObject(iso);
  if(object->Has(NewFromPersistent(iso, _toBSONString)))
  {
    const Local<Value>& toBSON = object->Get(NewFromPersistent(iso, _toBSONString));
    if(!toBSON->IsFunction()) ThrowAllocatedStringException(64, "toBSON is not a function");

    Local<Value> result = Local<Function>::Cast(toBSON)->Call(object, 0, NULL);
    if(!result->IsObject()) ThrowAllocatedStringException(64, "toBSON function did not return an object");
    return result->ToObject(iso);
  }
  else
  {
    return object;
  }
}
