//===========================================================================

#ifndef BSON_H_
#define BSON_H_

//===========================================================================

#ifdef __arm__
#define USE_MISALIGNED_MEMORY_ACCESS 0
#else
#define USE_MISALIGNED_MEMORY_ACCESS 1
#endif

#include <vector>

#include <v8.h>
#include <node.h>
#include <node_buffer.h>

using namespace v8;
using namespace node;

template <typename T>
Local<T> NewFromPersistent(Isolate* isolate, const Persistent<T>& p) {
  return Local<T>::New(isolate, p);
}

void ThrowAllocatedStringException(size_t allocationSize, const char* format, ...);

//===========================================================================

enum BsonType
{
  BSON_TYPE_NUMBER		= 1,
  BSON_TYPE_STRING		= 2,
  BSON_TYPE_OBJECT		= 3,
  BSON_TYPE_ARRAY			= 4,
  BSON_TYPE_BINARY		= 5,
  BSON_TYPE_UNDEFINED		= 6,
  BSON_TYPE_OID			= 7,
  BSON_TYPE_BOOLEAN		= 8,
  BSON_TYPE_DATE			= 9,
  BSON_TYPE_NULL			= 10,
  BSON_TYPE_REGEXP		= 11,
  BSON_TYPE_CODE			= 13,
  BSON_TYPE_SYMBOL		= 14,
  BSON_TYPE_CODE_W_SCOPE	= 15,
  BSON_TYPE_INT			= 16,
  BSON_TYPE_TIMESTAMP		= 17,
  BSON_TYPE_LONG			= 18,
  BSON_TYPE_TYPED_ARRAY		= 19,
  BSON_TYPE_TYPED_ARRAY_TRANSFERABLE	= 20,
  BSON_TYPE_MAX_KEY		= 0x7f,
  BSON_TYPE_MIN_KEY		= 0xff
};

enum TypedArrayType
{
  TYPED_ARRAY_INT8 = 1,
  TYPED_ARRAY_UINT8 = 2,
  TYPED_ARRAY_UINT8_CLAMPED = 3,

  TYPED_ARRAY_INT16 = 4,
  TYPED_ARRAY_UINT16 = 5,

  TYPED_ARRAY_INT32 = 6,
  TYPED_ARRAY_UINT32 = 7,

  TYPED_ARRAY_FLOAT32 = 8,
  TYPED_ARRAY_FLOAT64 = 9,
};

//===========================================================================

template<typename T> class BSONSerializer;

class BSON {
public:
  BSON(Isolate* i);
  ~BSON() {}

  // Constructor used for creating new BSON objects from C++
  static Persistent<FunctionTemplate> constructor_template;

private:
  static Handle<Value> deserialize(BSON *bson, char *data, uint32_t dataLength, uint32_t startIndex, bool is_array_item);

  // BSON type instantiate functions
  Persistent<Function> longConstructor;
  Persistent<Function> objectIDConstructor;
  Persistent<Function> binaryConstructor;
  Persistent<Function> codeConstructor;
  Persistent<Function> dbrefConstructor;
  Persistent<Function> symbolConstructor;
  Persistent<Function> doubleConstructor;
  Persistent<Function> timestampConstructor;
  Persistent<Function> minKeyConstructor;
  Persistent<Function> maxKeyConstructor;

  // Equality Objects
  Persistent<String> longString;
  Persistent<String> objectIDString;
  Persistent<String> binaryString;
  Persistent<String> codeString;
  Persistent<String> dbrefString;
  Persistent<String> symbolString;
  Persistent<String> doubleString;
  Persistent<String> timestampString;
  Persistent<String> minKeyString;
  Persistent<String> maxKeyString;

  // Equality speed up comparison objects
  Persistent<String> _bsontypeString;
  Persistent<String> _longLowString;
  Persistent<String> _longHighString;
  Persistent<String> _objectIDidString;
  Persistent<String> _binaryPositionString;
  Persistent<String> _binarySubTypeString;
  Persistent<String> _binaryBufferString;
  Persistent<String> _doubleValueString;
  Persistent<String> _symbolValueString;

  Persistent<String> _dbRefRefString;
  Persistent<String> _dbRefIdRefString;
  Persistent<String> _dbRefDbRefString;
  Persistent<String> _dbRefNamespaceString;
  Persistent<String> _dbRefDbString;
  Persistent<String> _dbRefOidString;

  Persistent<String> _codeCodeString;
  Persistent<String> _codeScopeString;
  Persistent<String> _toBSONString;

public:	Local<Object> GetSerializeObject(Isolate* iso, const Handle<Value>& object);

  template<typename T> friend class BSONSerializer;
  friend class BSONDeserializer;
};

//===========================================================================

class CountStream
{
public:
  CountStream() : count(0) { }

  void	WriteByte(int value)									{ ++count; }
  void	WriteByte(const Handle<Object>&, const Handle<String>&)	{ ++count; }
  void	WriteBool(const Handle<Value>& value)					{ ++count; }
  void	WriteInt32(int32_t value)								{ count += 4; }
  void	WriteInt32(const Handle<Value>& value)					{ count += 4; }
  void	WriteInt32(const Handle<Object>& object, const Handle<String>& key) { count += 4; }
  void	WriteInt64(int64_t value)								{ count += 8; }
  void	WriteInt64(const Handle<Value>& value)					{ count += 8; }
  void	WriteDouble(double value)								{ count += 8; }
  void	WriteDouble(const Handle<Value>& value)					{ count += 8; }
  void	WriteDouble(const Handle<Object>&, const Handle<String>&) { count += 8; }
  void	WriteUInt32String(uint32_t name)						{ char buffer[32]; count += sprintf(buffer, "%u", name) + 1; }
  void	WriteLengthPrefixedString(const Local<String>& value)	{ count += value->Utf8Length()+5; }
  void	WriteObjectId(Isolate* iso, const Handle<Object>& object, const Handle<String>& key)				{ count += 12; }
  void	WriteString(const Local<String>& value)					{ count += value->Utf8Length() + 1; }	// This returns the number of bytes exclusive of the NULL terminator
  void	WriteData(const char* data, size_t length)				{ count += length; }
  void	WritePointer(void*)										{ count += sizeof(void*); }

  void*	BeginWriteType()										{ ++count; return NULL; }
  void	CommitType(void*, BsonType)								{ }
  void*	BeginWriteSize()										{ count += 4; return NULL; }
  void	CommitSize(void*)										{ }

  size_t GetSerializeSize() const									{ return count; }

  // Do nothing. CheckKey is implemented for DataStream
  void	CheckKey(const Local<String>&)							{ }

private:
  size_t	count;
};

class DataStream
{
public:
  DataStream(char* aDestinationBuffer) : destinationBuffer(aDestinationBuffer), p(aDestinationBuffer) { }

  void	WriteByte(int value)									{ *p++ = value; }
  void	WriteByte(const Handle<Object>& object, const Handle<String>& key)	{ *p++ = object->Get(key)->Int32Value(); }
#if USE_MISALIGNED_MEMORY_ACCESS
  void	WriteInt32(int32_t value)								{ *reinterpret_cast<int32_t*>(p) = value; p += 4; }
  void	WriteInt64(int64_t value)								{ *reinterpret_cast<int64_t*>(p) = value; p += 8; }
  void	WriteDouble(double value)								{ *reinterpret_cast<double*>(p) = value; p += 8; }
  void	WritePointer(void* value)								{ *reinterpret_cast<void**>(p) = value; p += sizeof(void*); }
#else
  void	WriteInt32(int32_t value)								{ memcpy(p, &value, 4); p += 4; }
  void	WriteInt64(int64_t value)								{ memcpy(p, &value, 8); p += 8; }
  void	WriteDouble(double value)								{ memcpy(p, &value, 8); p += 8; }
    void	WritePointer(void* value)								{ memcpy(p, &value, sizeof(void*)); p += sizeof(void*); }
#endif
  void	WriteBool(const Handle<Value>& value)					{ WriteByte(value->BooleanValue() ? 1 : 0); }
  void	WriteInt32(const Handle<Value>& value)					{ WriteInt32(value->Int32Value());			}
  void	WriteInt32(const Handle<Object>& object, const Handle<String>& key) { WriteInt32(object->Get(key)); }
  void	WriteInt64(const Handle<Value>& value)					{ WriteInt64(value->IntegerValue());		}
  void	WriteDouble(const Handle<Value>& value)					{ WriteDouble(value->NumberValue());		}
  void	WriteDouble(const Handle<Object>& object, const Handle<String>& key) { WriteDouble(object->Get(key)); }
  void	WriteUInt32String(uint32_t name)						{ p += sprintf(p, "%u", name) + 1;			}
  void	WriteLengthPrefixedString(const Local<String>& value)	{ WriteInt32(value->Utf8Length()+1); WriteString(value); }
  void	WriteObjectId(Isolate* iso, const Handle<Object>& object, const Handle<String>& key);
  void	WriteString(const Local<String>& value)					{ p += value->WriteUtf8(p); }		// This returns the number of bytes inclusive of the NULL terminator.
  void	WriteData(const char* data, size_t length)				{ memcpy(p, data, length); p += length; }

  void*	BeginWriteType()										{ void* returnValue = p; p++; return returnValue; }
  void	CommitType(void* beginPoint, BsonType value)			{ *reinterpret_cast<unsigned char*>(beginPoint) = value; }
  void*	BeginWriteSize()										{ void* returnValue = p; p += 4; return returnValue; }

#if USE_MISALIGNED_MEMORY_ACCESS
  void	CommitSize(void* beginPoint)							{ *reinterpret_cast<int32_t*>(beginPoint) = (int32_t) (p - (char*) beginPoint); }
#else
  void	CommitSize(void* beginPoint)							{ int32_t value = (int32_t) (p - (char*) beginPoint); memcpy(beginPoint, &value, 4); }
#endif

  size_t GetSerializeSize() const									{ return p - destinationBuffer; }

  void	CheckKey(const Local<String>& keyName);

protected:
  char *const	destinationBuffer;		// base, never changes
  char*		p;						// cursor into buffer
};

template<typename T> class BSONSerializer : public T
{
private:
  typedef T Inherited;

public:
  BSONSerializer(Isolate* i_, BSON* aBson, bool aCheckKeys, bool aSerializeFunctions, const std::vector<Local<ArrayBuffer> >* aTransferables)
  : Inherited(), iso(i_), checkKeys(aCheckKeys), serializeFunctions(aSerializeFunctions), bson(aBson), transferables(aTransferables)
  {
    if (transferables != NULL) { transferableContents.resize(transferables->size(), NULL); }
  }
  BSONSerializer(Isolate* i_, BSON* aBson, bool aCheckKeys, bool aSerializeFunctions, char* parentParam, const std::vector<Local<ArrayBuffer> >* aTransferables)
  : Inherited(parentParam), iso(i_), checkKeys(aCheckKeys), serializeFunctions(aSerializeFunctions), bson(aBson), transferables(aTransferables)
  {
    if (transferables != NULL) { transferableContents.resize(transferables->size(), NULL); }
  }

  void SerializeDocument(const Handle<Value>& value);
  void SerializeArray(const Handle<Value>& value);
  void SerializeValue(void* typeLocation, const Handle<Value>& value);

private:
  int TransferableIndex(Local<ArrayBuffer> buffer);

  Isolate* iso;
  bool		checkKeys;
  bool		serializeFunctions;
  BSON*		bson;
  const std::vector<Local<ArrayBuffer> >* transferables;

  // Parallels transferables. Each entry stores the pointer returned by ArrayBuffer::Externalize
  // (if that buffer has already been externalized during serialization) or a NULL pointer if the
  // corresponding buffer has not yet been externalized.
  std::vector<unsigned char*> transferableContents;
};

//===========================================================================

class BSONDeserializer
{
public:
  BSONDeserializer(Isolate* i_, BSON* aBson, char* data, size_t length);
  BSONDeserializer(BSONDeserializer& parentSerializer, size_t length);

  Handle<Value> DeserializeDocument(bool promoteLongs);

  bool			HasMoreData() const { return p < pEnd; }
  MaybeLocal<String>	ReadCString();
  uint32_t		ReadIntegerString();
  int32_t			ReadRegexOptions();
  Local<String>	ReadString();
  Local<String>	ReadObjectId();

  unsigned char	ReadByte()			{ return *reinterpret_cast<unsigned char*>(p++); }
#if USE_MISALIGNED_MEMORY_ACCESS
  int32_t			ReadInt32()			{ int32_t returnValue = *reinterpret_cast<int32_t*>(p); p += 4; return returnValue; }
  uint32_t		ReadUInt32()		{ uint32_t returnValue = *reinterpret_cast<uint32_t*>(p); p += 4; return returnValue; }
  int64_t			ReadInt64()			{ int64_t returnValue = *reinterpret_cast<int64_t*>(p); p += 8; return returnValue; }
  double			ReadDouble()		{ double returnValue = *reinterpret_cast<double*>(p); p += 8; return returnValue; }
  void*				ReadPointer()		{ void* returnValue = *reinterpret_cast<void**>(p); p += sizeof(void*); return returnValue; }
#else
  int32_t			ReadInt32()			{ int32_t returnValue; memcpy(&returnValue, p, 4); p += 4; return returnValue; }
  uint32_t		ReadUInt32()		{ uint32_t returnValue; memcpy(&returnValue, p, 4); p += 4; return returnValue; }
  int64_t			ReadInt64()			{ int64_t returnValue; memcpy(&returnValue, p, 8); p += 8; return returnValue; }
  double			ReadDouble()		{ double returnValue; memcpy(&returnValue, p, 8); p += 8; return returnValue; }
  void*				ReadPointer()		{ void* returnValue; memcpy(&returnValue, p, sizeof(void*)); p += sizeof(void*); return returnValue; }
#endif

  size_t			GetSerializeSize() const { return p - pStart; }

private:
  Handle<Value> DeserializeArray(bool promoteLongs);
  Handle<Value> DeserializeValue(BsonType type, bool promoteLongs);
  Handle<Value> DeserializeDocumentInternal(bool promoteLongs);
  Handle<Value> DeserializeArrayInternal(bool promoteLongs);

  Isolate* iso;
  Local<Context> context;
  BSON*		bson;
  char* const pStart;
  char*		p;
  char* const	pEnd;
};

//===========================================================================


template<typename T> void BSONSerializer<T>::SerializeDocument(const Handle<Value>& value)
{
  void* documentSize = this->BeginWriteSize();

  Local<Object> object = bson->GetSerializeObject(iso, value);

  // Get the object property names
  Local<Array> propertyNames = object->GetPropertyNames();

  // Length of the property
  int propertyLength = propertyNames->Length();
  for(int i = 0;  i < propertyLength; ++i)
  {
    const Local<String>& propertyName = propertyNames->Get(i)->ToString(iso);
    if(checkKeys) this->CheckKey(propertyName);

    const Local<Value>& propertyValue = object->Get(propertyName);

    if(serializeFunctions || !propertyValue->IsFunction())
    {
      void* typeLocation = this->BeginWriteType();
      this->WriteString(propertyName);
      SerializeValue(typeLocation, propertyValue);
    }
  }

  this->WriteByte(0);
  this->CommitSize(documentSize);
}

template<typename T> void BSONSerializer<T>::SerializeArray(const Handle<Value>& value)
{
  void* documentSize = this->BeginWriteSize();

  Local<Array> array = Local<Array>::Cast(value->ToObject(iso));
  uint32_t arrayLength = array->Length();

  for(uint32_t i = 0;  i < arrayLength; ++i)
  {
    void* typeLocation = this->BeginWriteType();
    this->WriteUInt32String(i);
    SerializeValue(typeLocation, array->Get(i));
  }

  this->WriteByte(0);
  this->CommitSize(documentSize);
}

static inline TypedArrayType getTypedArrayType(Local<Value> value) {
  if (value->IsUint8Array()) { return TYPED_ARRAY_UINT8; }
  else if (value->IsUint8ClampedArray()) { return TYPED_ARRAY_UINT8_CLAMPED; }
  else if (value->IsInt8Array()) { return TYPED_ARRAY_INT8; }
  else if (value->IsUint16Array()) { return TYPED_ARRAY_UINT16; }
  else if (value->IsInt16Array()) { return TYPED_ARRAY_INT16; }
  else if (value->IsUint32Array()) { return TYPED_ARRAY_UINT32; }
  else if (value->IsInt32Array()) { return TYPED_ARRAY_INT32; }
  else if (value->IsFloat32Array()) { return TYPED_ARRAY_FLOAT32; }
  else if (value->IsFloat64Array()) { return TYPED_ARRAY_FLOAT64; }
  else {
    ThrowAllocatedStringException(64, "Unknown typed array sub-type");
  }
  return static_cast<TypedArrayType>(0); // unreachable.
}

template <typename T>
int BSONSerializer<T>::TransferableIndex(Local<ArrayBuffer> buffer) {
  if (transferables != NULL)
  {
    for (size_t i = 0; i < transferables->size(); i++)
    {
      if (buffer->StrictEquals((*transferables)[i])) { return i; }
    }
  }
  return -1;
}

template <typename T>
struct Externalizer;

template <>
struct Externalizer<CountStream> {
  static unsigned char* externalize(Local<ArrayBuffer> buffer, int bufferLength)
  {
    // We don't actually externalize the buffers until the DataStream step runs.
    return NULL;
  }
};

template <>
struct Externalizer<DataStream> {
  static unsigned char* externalize(Local<ArrayBuffer> buffer, int bufferLength)
  {
    ArrayBuffer::Contents contents = buffer->Externalize();
    buffer->Neuter();
    return static_cast<unsigned char*>(contents.Data());
  }
};


// This is templated so that we can use this function to both count the number of bytes, and to serialize those bytes.
// The template approach eliminates almost all of the inspection of values unless they're required (eg. string lengths)
// and ensures that there is always consistency between bytes counted and bytes written by design.
template<typename T> void BSONSerializer<T>::SerializeValue(void* typeLocation, const Handle<Value>& value)
{
  if(value->IsNumber())
  {
    double doubleValue = value->NumberValue();
    int intValue = (int) doubleValue;
    if(intValue == doubleValue)
    {
      this->CommitType(typeLocation, BSON_TYPE_INT);
      this->WriteInt32(intValue);
    }
    else
    {
      this->CommitType(typeLocation, BSON_TYPE_NUMBER);
      this->WriteDouble(doubleValue);
    }
  }
  else if(value->IsString())
  {
    this->CommitType(typeLocation, BSON_TYPE_STRING);
    this->WriteLengthPrefixedString(value->ToString(iso));
  }
  else if(value->IsBoolean())
  {
    this->CommitType(typeLocation, BSON_TYPE_BOOLEAN);
    this->WriteBool(value);
  }
  else if(value->IsArray())
  {
    this->CommitType(typeLocation, BSON_TYPE_ARRAY);
    SerializeArray(value);
  }
  else if(value->IsDate())
  {
    this->CommitType(typeLocation, BSON_TYPE_DATE);
    this->WriteInt64(value);
  }
  else if(value->IsRegExp())
  {
    this->CommitType(typeLocation, BSON_TYPE_REGEXP);
    const Handle<RegExp>& regExp = Handle<RegExp>::Cast(value);

    this->WriteString(regExp->GetSource());

    int flags = regExp->GetFlags();
    if(flags & RegExp::kGlobal) this->WriteByte('s');
    if(flags & RegExp::kIgnoreCase) this->WriteByte('i');
    if(flags & RegExp::kMultiline) this->WriteByte('m');
    this->WriteByte(0);
  }
  else if(value->IsFunction())
  {
    this->CommitType(typeLocation, BSON_TYPE_CODE);
    this->WriteLengthPrefixedString(value->ToString(iso));
  }
  else if(value->IsObject())
  {
    const Local<Object>& object = value->ToObject(iso);
    if(object->Has(NewFromPersistent(iso, bson->_bsontypeString)))
    {
      const Local<String>& constructorString = object->GetConstructorName();
      if(NewFromPersistent(iso, bson->longString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_LONG);
        this->WriteInt32(object, NewFromPersistent(iso, bson->_longLowString));
        this->WriteInt32(object, NewFromPersistent(iso, bson->_longHighString));
      }
      else if(NewFromPersistent(iso, bson->timestampString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_TIMESTAMP);
        this->WriteInt32(object, NewFromPersistent(iso, bson->_longLowString));
        this->WriteInt32(object, NewFromPersistent(iso, bson->_longHighString));
      }
      else if(NewFromPersistent(iso, bson->objectIDString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_OID);
        this->WriteObjectId(iso, object, NewFromPersistent(iso, bson->_objectIDidString));
      }
      else if(NewFromPersistent(iso, bson->binaryString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_BINARY);

        uint32_t length = object->Get(NewFromPersistent(iso, bson->_binaryPositionString))->Uint32Value();
        Local<Object> bufferObj = object->Get(NewFromPersistent(iso, bson->_binaryBufferString))->ToObject(iso);

        this->WriteInt32(length);
        this->WriteByte(object, NewFromPersistent(iso, bson->_binarySubTypeString));  // write subtype
        // If type 0x02 write the array length aswell
        if(object->Get(NewFromPersistent(iso, bson->_binarySubTypeString))->Int32Value() == 0x02) {
          this->WriteInt32(length);
        }
        // Write the actual data
        this->WriteData(Buffer::Data(bufferObj), length);
      }
      else if(NewFromPersistent(iso, bson->doubleString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_NUMBER);
        this->WriteDouble(object, NewFromPersistent(iso, bson->_doubleValueString));
      }
      else if(NewFromPersistent(iso, bson->symbolString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_SYMBOL);
        this->WriteLengthPrefixedString(object->Get(NewFromPersistent(iso, bson->_symbolValueString))->ToString(iso));
      }
      else if(NewFromPersistent(iso, bson->codeString)->StrictEquals(constructorString))
      {
        const Local<String>& function = object->Get(NewFromPersistent(iso, bson->_codeCodeString))->ToString(iso);
        const Local<Object>& scope = object->Get(NewFromPersistent(iso, bson->_codeScopeString))->ToObject(iso);

        // For Node < 0.6.X use the GetPropertyNames
        #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 6
          uint32_t propertyNameLength = scope->GetPropertyNames()->Length();
        #else
          uint32_t propertyNameLength = scope->GetOwnPropertyNames()->Length();
        #endif

        if(propertyNameLength > 0)
        {
          this->CommitType(typeLocation, BSON_TYPE_CODE_W_SCOPE);
          void* codeWidthScopeSize = this->BeginWriteSize();
          this->WriteLengthPrefixedString(function->ToString(iso));
          SerializeDocument(scope);
          this->CommitSize(codeWidthScopeSize);
        }
        else
        {
          this->CommitType(typeLocation, BSON_TYPE_CODE);
          this->WriteLengthPrefixedString(function->ToString(iso));
        }
      }
      else if(NewFromPersistent(iso, bson->dbrefString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_OBJECT);

        void* dbRefSize = this->BeginWriteSize();

        void* refType = this->BeginWriteType();
        this->WriteData("$ref", 5);
        SerializeValue(refType, object->Get(NewFromPersistent(iso, bson->_dbRefNamespaceString)));

        void* idType = this->BeginWriteType();
        this->WriteData("$id", 4);
        SerializeValue(idType, object->Get(NewFromPersistent(iso, bson->_dbRefOidString)));

        const Local<Value>& refDbValue = object->Get(NewFromPersistent(iso, bson->_dbRefDbString));
        if(!refDbValue->IsUndefined())
        {
          void* dbType = this->BeginWriteType();
          this->WriteData("$db", 4);
          SerializeValue(dbType, refDbValue);
        }

        this->WriteByte(0);
        this->CommitSize(dbRefSize);
      }
      else if(NewFromPersistent(iso, bson->minKeyString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_MIN_KEY);
      }
      else if(NewFromPersistent(iso, bson->maxKeyString)->StrictEquals(constructorString))
      {
        this->CommitType(typeLocation, BSON_TYPE_MAX_KEY);
      }
    }
    else if (value->IsArrayBufferView()) {
      if (!value->IsTypedArray()) {
        ThrowAllocatedStringException(128, "ArrayBufferViews other than Typed Arrays not supported");
      }
      TypedArrayType arrayType = getTypedArrayType(value);

      // When an ArrayBuffer has not been specified as a transferable, we serialize the entire ArrayBuffer,
      // even if the array is just a small view into a larger buffer. This is how webworkers in chrome behave.
      ArrayBufferView* bufferView = ArrayBufferView::Cast(*value);
      Local<ArrayBuffer> buffer = bufferView->Buffer();

      size_t viewLength = bufferView->ByteLength();
      size_t viewOffset = bufferView->ByteOffset();
      size_t bufferLength = buffer->ByteLength();

      int transferableIdx = TransferableIndex(buffer);
      bool transferable = transferableIdx >= 0;
      unsigned char* p = NULL;
      if (transferable)
      {
        if (buffer->IsExternal())
        {
          p = static_cast<unsigned char*>(buffer->GetContents().Data());
        }
        else
        {
          p = Externalizer<T>::externalize(buffer, bufferLength);
        }
        transferableContents[transferableIdx] = p;
      }
      if (p == NULL)
      {
        p = static_cast<unsigned char*>(buffer->GetContents().Data());
      }

      if (p == NULL) { return; }

      this->CommitType(typeLocation, BSON_TYPE_TYPED_ARRAY);
      // If you change the order of writes here, be sure to update the order of the corresponding
      // reads in DeserializeValue.
      this->WriteByte(arrayType);
      this->WriteInt64(viewLength);
      this->WriteInt64(viewOffset);
      this->WriteInt64(bufferLength);
      if (transferable)
      {
        this->WritePointer(p);
      }
      else
      {
        // The deserializer uses NULL as the signal to read in the ArrayBuffer contents from
        // the data stream.
        this->WritePointer(NULL);
        for (size_t i = 0; i < bufferLength; i++) { this->WriteByte(p[i]); }
      }
    }
    else if(Buffer::HasInstance(value))
    { 
      this->CommitType(typeLocation, BSON_TYPE_BINARY);

      #if NODE_MAJOR_VERSION == 0 && NODE_MINOR_VERSION < 3
       Local<Object> buffer = ObjectWrap::Unwrap<Buffer>(value->ToObject(iso));
       uint32_t length = object->length();
      #else
       uint32_t length = Buffer::Length(value->ToObject(iso));
      #endif

      this->WriteInt32(length);
      this->WriteByte(0);
      this->WriteData(Buffer::Data(value->ToObject(iso)), length);
    }
    else
    {
      this->CommitType(typeLocation, BSON_TYPE_OBJECT);
      SerializeDocument(value);
    }
  }
  else if(value->IsNull() || value->IsUndefined())
  {
    this->CommitType(typeLocation, BSON_TYPE_NULL);
  }
}

#endif  // BSON_H_

//===========================================================================
