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

static Local<Int32> NewInt(Isolate* i, int32_t d) { return Integer::New(i, d)->ToInt32(); }
static Local<Uint32> NewInt(Isolate* i, uint32_t d) { return Integer::NewFromUnsigned(i, d)->ToUint32(); }
static Local<Number> NewNum(Isolate* i, double d) { return Number::New(i, d); }
static Local<Number> NewNum(Isolate* i, int64_t d) { return Number::New(i, d); }
static Local<String> NewString(Isolate* i, const char* s) { return String::NewFromUtf8(i, s); }

static void AssignPersistent(Isolate* i, Persistent<String>& p, Local<String> s) {
  p.Reset(i, s);
}

//===========================================================================

void DataStream::WriteObjectId(const Handle<Object>& object, const Handle<String>& key)
{
  uint16_t buffer[12];
  object->Get(key)->ToString()->Write(buffer, 0, 12);
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
: i(i_),
  bson(aBson),
  pStart(data),
  p(data),
  pEnd(data + length - 1)
{
  if(*pEnd != '\0') ThrowAllocatedStringException(64, "Missing end of document marker '\\0'");
}

BSONDeserializer::BSONDeserializer(BSONDeserializer& parentSerializer, size_t length)
: i(parentSerializer.i),
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
    return Null(i);
  }
  return String::NewFromUtf8(i, start, String::kNormalString, p - start - 1);
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
  return String::NewFromUtf8(i, start, String::kNormalString, length - 1);
}

Local<String> BSONDeserializer::ReadObjectId()
{
  uint16_t objectId[12];
  for(size_t i = 0; i < 12; ++i)
  {
    objectId[i] = *reinterpret_cast<unsigned char*>(p++);
  }
  return String::NewFromTwoByte(i, objectId, String::kNormalString, 12);
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
  Local<Object> returnObject = Object::New(i);

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
  if(returnObject->Has(NewFromPersistent(i, bson->_dbRefIdRefString)))
  {
    Local<Value> argv[] = { returnObject->Get(NewFromPersistent(i, bson->_dbRefRefString)), returnObject->Get(NewFromPersistent(i, bson->_dbRefIdRefString)), returnObject->Get(NewFromPersistent(i, bson->_dbRefDbRefString)) };
    return NewFromPersistent(i, bson->dbrefConstructor)->NewInstance(3, argv);
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
  Local<Array> returnArray = Array::New(i);

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
    return NewInt(i, ReadInt32());

  case BSON_TYPE_NUMBER:
    return NewNum(i, ReadDouble());

  case BSON_TYPE_NULL:
    return Null(i);

  case BSON_TYPE_UNDEFINED:
    return Undefined(i);

  case BSON_TYPE_TIMESTAMP:
    {
      int32_t lowBits = ReadInt32();
      int32_t highBits = ReadInt32();
      Local<Value> argv[] = { NewInt(i, lowBits), NewInt(i, highBits) };
      return NewFromPersistent(i, bson->timestampConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_BOOLEAN:
    return (ReadByte() != 0) ? True(i) : False(i);

  case BSON_TYPE_REGEXP:
    {
      const Handle<Value>& regex = ReadCString();
      if(regex->IsNull()) ThrowAllocatedStringException(64, "Bad BSON Document: illegal CString");
      int32_t options = ReadRegexOptions();
      return RegExp::New(regex->ToString(), (RegExp::Flags) options);
    }

  case BSON_TYPE_CODE:
    {
      const Local<Value>& code = ReadString();
      const Local<Value>& scope = Object::New(i);
      Local<Value> argv[] = { code, scope };
      return NewFromPersistent(i, bson->codeConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_CODE_W_SCOPE:
    {
      ReadUInt32();
      const Local<Value>& code = ReadString();
      const Handle<Value>& scope = DeserializeDocument(promoteLongs);
      Local<Value> argv[] = { code, scope->ToObject() };
      return NewFromPersistent(i, bson->codeConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_OID:
    {
      Local<Value> argv[] = { ReadObjectId() };
      return NewFromPersistent(i, bson->objectIDConstructor)->NewInstance(1, argv);
    }

  case BSON_TYPE_BINARY:
    {
      uint32_t length = ReadUInt32();
      uint32_t subType = ReadByte();
      if(subType == 0x02) {
        length = ReadInt32();
      }

      Local<Object> buffer = Buffer::New(i, p, length);
      p += length;

      Handle<Value> argv[] = { buffer, NewInt(i, subType) };
      return NewFromPersistent(i, bson->binaryConstructor)->NewInstance(2, argv);
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
          return NewNum(i, finalValue);
        }
      }

      // Decode the Long value
      Local<Value> argv[] = { NewInt(i, lowBits), NewInt(i, highBits) };
      return NewFromPersistent(i, bson->longConstructor)->NewInstance(2, argv);
    }

  case BSON_TYPE_DATE:
    return Date::New(i, ReadInt64());

  case BSON_TYPE_ARRAY:
    return DeserializeArray(promoteLongs);

  case BSON_TYPE_OBJECT:
    return DeserializeDocument(promoteLongs);

  case BSON_TYPE_SYMBOL:
    {
      const Local<String>& string = ReadString();
      Local<Value> argv[] = { string };
      return NewFromPersistent(i, bson->symbolConstructor)->NewInstance(1, argv);
    }

  case BSON_TYPE_MIN_KEY:
    return NewFromPersistent(i, bson->minKeyConstructor)->NewInstance();

  case BSON_TYPE_MAX_KEY:
    return NewFromPersistent(i, bson->maxKeyConstructor)->NewInstance();

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
        buffer = ArrayBuffer::New(i, bufferLength);
        unsigned char* p = static_cast<unsigned char*>(buffer->GetContents().Data());
        if (p == NULL) { return Null(i); }
        for (size_t i = 0; i < bufferLength; i++) { p[i] = ReadByte(); }
      }
      else
      {
        // Create buffer from existing memory. The array buffer has been externalized and neutered
        // when the array was transferred here.
        buffer = ArrayBuffer::NewNonExternal(i, pointer, bufferLength);
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
      return Null(i);
    }

  default:
    ThrowAllocatedStringException(64, "Unhandled BSON Type: %d", type);
  }

  return Null(i);
}

Persistent<FunctionTemplate> BSON::constructor_template;

BSON::BSON(Isolate* i)
{
  // Setup pre-allocated comparision objects
  AssignPersistent(i, _bsontypeString, NewString(i, "_bsontype"));
  AssignPersistent(i, _longLowString, NewString(i, "low_"));
  AssignPersistent(i, _longHighString, NewString(i, "high_"));
  AssignPersistent(i, _objectIDidString, NewString(i, "id"));
  AssignPersistent(i, _binaryPositionString, NewString(i, "position"));
  AssignPersistent(i, _binarySubTypeString, NewString(i, "sub_type"));
  AssignPersistent(i, _binaryBufferString, NewString(i, "buffer"));
  AssignPersistent(i, _doubleValueString, NewString(i, "value"));
  AssignPersistent(i, _symbolValueString, NewString(i, "value"));
  AssignPersistent(i, _dbRefRefString, NewString(i, "$ref"));
  AssignPersistent(i, _dbRefIdRefString, NewString(i, "$id"));
  AssignPersistent(i, _dbRefDbRefString, NewString(i, "$db"));
  AssignPersistent(i, _dbRefNamespaceString, NewString(i, "namespace"));
  AssignPersistent(i, _dbRefDbString, NewString(i, "db"));
  AssignPersistent(i, _dbRefOidString, NewString(i, "oid"));
  AssignPersistent(i, _codeCodeString, NewString(i, "code"));
  AssignPersistent(i, _codeScopeString, NewString(i, "scope"));
  AssignPersistent(i, _toBSONString, NewString(i, "toBSON"));

  AssignPersistent(i, longString, NewString(i, "Long"));
  AssignPersistent(i, objectIDString, NewString(i, "ObjectID"));
  AssignPersistent(i, binaryString, NewString(i, "Binary"));
  AssignPersistent(i, codeString, NewString(i, "Code"));
  AssignPersistent(i, dbrefString, NewString(i, "DBRef"));
  AssignPersistent(i, symbolString, NewString(i, "Symbol"));
  AssignPersistent(i, doubleString, NewString(i, "Double"));
  AssignPersistent(i, timestampString, NewString(i, "Timestamp"));
  AssignPersistent(i, minKeyString, NewString(i, "MinKey"));
  AssignPersistent(i, maxKeyString, NewString(i, "MaxKey"));
}

Local<Object> BSON::GetSerializeObject(Isolate* i, const Handle<Value>& argValue)
{
  Local<Object> object = argValue->ToObject();
  if(object->Has(NewFromPersistent(i, _toBSONString)))
  {
    const Local<Value>& toBSON = object->Get(NewFromPersistent(i, _toBSONString));
    if(!toBSON->IsFunction()) ThrowAllocatedStringException(64, "toBSON is not a function");

    Local<Value> result = Local<Function>::Cast(toBSON)->Call(object, 0, NULL);
    if(!result->IsObject()) ThrowAllocatedStringException(64, "toBSON function did not return an object");
    return result->ToObject();
  }
  else
  {
    return object;
  }
}
