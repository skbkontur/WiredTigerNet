#include "NativeTiger.h"
#include "WiredTigerNet.h"
#include "msclr\marshal_cppstd.h"
#include "msclr\marshal.h"
#include <string>
#include <cstring>

using namespace WiredTigerNet;

#define INVOKE_NATIVE(f) \
	try { \
		f; \
	} \
	catch (const NativeWiredTigerApiException& e) { \
		throw gcnew WiredTigerApiException(e.ErrorCode(), gcnew System::String(e.ApiName().c_str())); \
	} \

#define RANGE_UNWRAP() \
	pin_ptr<Byte> leftPtr; \
	int leftSize; \
	if (range.Left.HasValue && range.Left.Value.Bytes->Length > 0) { \
		array<Byte>^ bytes = range.Left.Value.Bytes; \
		leftSize = bytes->Length; \
		leftPtr = &bytes[0]; \
				} \
								else { \
		leftSize = 0; \
		leftPtr = nullptr; \
	} \
	pin_ptr<Byte> rightPtr; \
	int rightSize; \
	if (range.Right.HasValue && range.Right.Value.Bytes->Length) { \
		array<Byte>^ bytes = range.Right.Value.Bytes; \
		rightSize = bytes->Length; \
		rightPtr = &bytes[0]; \
				} \
								else { \
		rightSize = 0; \
		rightPtr = nullptr; \
	} \

// *************
// WiredTigerException
// *************

WiredTigerException::WiredTigerException(System::String^ message)
	: System::Exception(message) {
}

// *************
// WiredTigerApiException
// *************

static System::String^ FormatMessage(int errorCode, System::String^ apiName) {
	const char* err = wiredtiger_strerror(errorCode);
	return System::String::Format("error code [{0}], error message [{1}], api name [{2}]",
		errorCode, gcnew System::String(err), apiName);
}

WiredTigerApiException::WiredTigerApiException(int errorCode, System::String^ apiName)
	: errorCode_(errorCode), apiName_(apiName), WiredTigerException(FormatMessage(errorCode, apiName)) {
}

// *************
// WiredTigerComponent
// *************

WiredTigerComponent::WiredTigerComponent() :disposed_(false) {
}

WiredTigerComponent::~WiredTigerComponent() {
	if (disposed_)
		return;
	try {
		this->Close();
	}
	finally {
		disposed_ = true;
		System::GC::SuppressFinalize(this);
	}
}

WiredTigerComponent::!WiredTigerComponent() {
	if (disposed_)
		return;
	try {
		this->Close();
	}
	finally {
		disposed_ = true;
	}
}


// *************
// Boundary/Range
// *************

Boundary::Boundary(array<Byte>^ value, bool inclusive) :bytes_(value), inclusive_(inclusive) {
}

Range::Range(System::Nullable<Boundary> left, System::Nullable<Boundary> right) : left_(left), right_(right) {
}

Range Range::Segment(array<Byte>^ left, array<Byte>^ right) {
	return Range(Boundary(left, true), Boundary(right, true));
}

Range Range::PositiveRay(array<Byte>^ left) {
	return Range(Boundary(left, true), System::Nullable<Boundary>());
}

Range Range::NegativeRay(array<Byte>^ right) {
	return Range(System::Nullable<Boundary>(), Boundary(right, true));
}

Range Range::NegativeOpenRay(array<Byte>^ right) {
	return Range(System::Nullable<Boundary>(), Boundary(right, false));
}

Range Range::Line() {
	return line;
}

Range Range::Empty() {
	return emptyRange;
}

Range Range::PositiveOpenRay(array<Byte>^ left) {
	return Range(Boundary(left, false), System::Nullable<Boundary>());
}

Range Range::LeftOpenSegment(array<Byte>^ left, array<Byte>^ right) {
	return Range(Boundary(left, false), Boundary(right, true));
}

Range Range::RightOpenSegment(array<Byte>^ left, array<Byte>^ right) {
	return Range(Boundary(left, true), Boundary(right, false));
}

Range Range::Interval(array<Byte>^ left, array<Byte>^ right) {
	return Range(Boundary(left, false), Boundary(right, false));
}

Range Range::Prefix(array<Byte>^ prefix) {
	return Prefix(prefix, prefix);
}

Range Range::Prefix(array<Byte>^ left, array<Byte>^ right) {
	array<Byte>^ rightBoundary = IncrementBytes(right);
	return Range(Boundary(left, true),
		rightBoundary == nullptr ? System::Nullable<Boundary>() : Boundary(rightBoundary, false));
}

array<Byte>^ Range::IncrementBytes(array<Byte>^ source) {
	array<Byte>^ result = gcnew array<Byte>(source->Length);
	System::Array::Copy(source, result, source->Length);
	for (int i = result->Length - 1; i >= 0; i--)
		if (result[i] == System::Byte::MaxValue)
			result[i] = 0;
		else
		{
			result[i]++;
			return result;
		}
	return nullptr;
}

array<Byte>^ Range::DecrementBytes(array<Byte>^ source) {
	array<Byte>^ result = gcnew array<Byte>(source->Length);
	System::Array::Copy(source, result, source->Length);
	for (int i = result->Length - 1; i >= 0; i--)
		if (result[i] == 0)
			result[i] = System::Byte::MaxValue;
		else
		{
			result[i]--;
			return result;
		}
	return nullptr;
}

Range RangeOperators::Inclusive(Range range) {
	return Range(Inclusive(range.Left), Inclusive(range.Right));
}

Range RangeOperators::Prepend(Range range, array<Byte>^ prefix) {
	return Range(Prepend(range.Left, prefix), Prepend(range.Right, prefix));
}

Range RangeOperators::IntersectWith(Range a, Range b) {
	return Range(MaxLeft(a.Left, b.Left), MinRight(a.Right, b.Right));
}

static int CompareBytes(array<Byte>^ a, array<Byte>^ b) {
	if (a->Length == 0)
		return b->Length == 0 ? 0 : -1;
	if (b->Length == 0)
		return 1;
	pin_ptr<Byte> aPtr = &a[0];
	pin_ptr<Byte> bPtr = &b[0];
	if (a->Length == b->Length)
		return memcmp(aPtr, bPtr, a->Length);
	if (a->Length < b->Length) {
		int result = memcmp(aPtr, bPtr, a->Length);
		return result != 0 ? result : -1;
	}
	int result = memcmp(aPtr, bPtr, b->Length);
	return result != 0 ? result : 1;
}

bool RangeOperators::IsEmpty(Range range) {
	if (!range.Left.HasValue || !range.Right.HasValue)
		return false;
	int compareResult = CompareBytes(range.Left.Value.Bytes, range.Right.Value.Bytes);
	if (compareResult != 0)
		return compareResult > 0;
	return !range.Left.Value.Inclusive || !range.Right.Value.Inclusive;
}

System::Nullable<Boundary> RangeOperators::MaxLeft(System::Nullable<Boundary> a, System::Nullable<Boundary> b) {
	return CompareLeft(a, b) > 0 ? a : b;
}

System::Nullable<Boundary> RangeOperators::MinRight(System::Nullable<Boundary> a, System::Nullable<Boundary> b) {
	return CompareRight(a, b) < 0 ? a : b;
}

int RangeOperators::CompareLeft(System::Nullable<Boundary> a, System::Nullable<Boundary> b) {
	if (!a.HasValue && !b.HasValue)
		return 0;
	if (!a.HasValue)
		return -1;
	if (!b.HasValue)
		return 1;
	int result = CompareBytes(a.Value.Bytes, b.Value.Bytes);
	if (result != 0)
		return result;
	if (a.Value.Inclusive == b.Value.Inclusive)
		return 0;
	if (a.Value.Inclusive)
		return -1;
	return 1;
}

int RangeOperators::CompareRight(System::Nullable<Boundary> a, System::Nullable<Boundary> b) {
	if (!a.HasValue && !b.HasValue)
		return 0;
	if (!a.HasValue)
		return 1;
	if (!b.HasValue)
		return -1;
	int result = CompareBytes(a.Value.Bytes, b.Value.Bytes);
	if (result != 0)
		return result;
	if (a.Value.Inclusive == b.Value.Inclusive)
		return 0;
	if (a.Value.Inclusive)
		return 1;
	return -1;
}

static array<Byte>^ ConcatBytes(array<Byte>^ a, array<Byte>^ b) {
	if (b->Length == 0)
		return a;
	if (a->Length == 0)
		return b;
	array<Byte>^ result = gcnew array<Byte>(a->Length + b->Length);
	System::Array::Copy(a, result, a->Length);
	System::Array::Copy(b, 0, result, a->Length, b->Length);
	return result;
}

System::Nullable<Boundary> RangeOperators::Prepend(System::Nullable<Boundary> boundary, array<Byte>^ prefix) {
	return !boundary.HasValue ? System::Nullable<Boundary>() : Boundary(ConcatBytes(prefix, boundary.Value.Bytes), boundary.Value.Inclusive);
}

System::Nullable<Boundary> RangeOperators::Inclusive(System::Nullable<Boundary> boundary) {
	return !boundary.HasValue || boundary.Value.Inclusive ? boundary : Boundary(boundary.Value.Bytes, true);
}

// *************
// Cursor
// *************

static bool is_raw_bytes(const char* format) {
	return strcmp(format, "u") == 0 || strcmp(format, "U") == 0;
}

Cursor::Cursor(NativeCursor* cursor) : cursor_(cursor) {
	if (is_raw_bytes(cursor->KeyFormat()) && is_raw_bytes(cursor->ValueFormat()))
		schemaType_ = CursorSchemaType::KeyAndValue;
	else if (is_raw_bytes(cursor->KeyFormat()) && strcmp(cursor->ValueFormat(), "") == 0)
		schemaType_ = CursorSchemaType::KeyOnly;
	else {
		char* messageFormat = "unsupported cursor schema (key_format->value_format) = ({0}->{1}), expected (u->u) or (u->)";
		throw gcnew WiredTigerException(System::String::Format(gcnew System::String(messageFormat),
			gcnew System::String(cursor->KeyFormat()), gcnew System::String(cursor->ValueFormat())));
	}
}

void Cursor::Close() {
	if (cursor_ != nullptr) {
		delete cursor_;
		cursor_ = nullptr;
	}
}

void Cursor::Insert(array<Byte>^ key, array<Byte>^ value) {
	if (schemaType_ == CursorSchemaType::KeyOnly)
		throw gcnew WiredTigerException("invalid Insert overload, current schema is [CursorSchemaType.KeyOnly] so use Insert(byte[]) instead");
	pin_ptr<Byte> keyPtr = &key[0];
	pin_ptr<Byte> valuePtr = &value[0];
	INVOKE_NATIVE(cursor_->Insert(keyPtr, key->Length, valuePtr, value->Length))
}

void Cursor::Insert(array<Byte>^ key) {
	if (schemaType_ == CursorSchemaType::KeyAndValue)
		throw gcnew WiredTigerException("invalid Insert overload, current schema is [CursorSchemaType.KeyAndValue] so use Insert(byte[],byte[]) instead");
	pin_ptr<Byte> keyPtr = &key[0];
	INVOKE_NATIVE(cursor_->Insert(keyPtr, key->Length))
}

bool Cursor::Next() {
	INVOKE_NATIVE(return cursor_->Next())
}

bool Cursor::Prev() {
	INVOKE_NATIVE(return cursor_->Prev())
}

void Cursor::Remove(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	INVOKE_NATIVE(cursor_->Remove(keyPtr, key->Length))
}

void Cursor::Reset() {
	INVOKE_NATIVE(cursor_->Reset())
}

bool Cursor::Search(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	INVOKE_NATIVE(return cursor_->Search(keyPtr, key->Length))
}

bool Cursor::SearchNear(array<Byte>^ key, [System::Runtime::InteropServices::OutAttribute] int% result) {
	int exact;
	pin_ptr<Byte> keyPtr = &key[0];

	INVOKE_NATIVE({
		if (!cursor_->SearchNear(keyPtr, key->Length, &exact))
		return false;
		result = exact;
		return true;
	})
}

long Cursor::GetTotalCount(Range range) {
	RANGE_UNWRAP(range)

	INVOKE_NATIVE(return cursor_->GetTotalCount(leftPtr, leftSize, range.Left.HasValue && range.Left.Value.Inclusive,
		rightPtr, rightSize, range.Right.HasValue && range.Right.Value.Inclusive));
}

bool Cursor::IterationBegin(Range range, Direction direction) {
	RANGE_UNWRAP(range)

	NativeDirection nativeDirection = direction == Direction::Ascending ? Ascending : Descending;

	INVOKE_NATIVE(return cursor_->IterationBegin(leftPtr, leftSize, range.Left.HasValue && range.Left.Value.Inclusive,
		rightPtr, rightSize, range.Right.HasValue && range.Right.Value.Inclusive, nativeDirection, true));
}

bool Cursor::IterationMove() {
	INVOKE_NATIVE(return cursor_->IterationMove())
}

array<Byte>^ Cursor::GetKey() {
	WT_ITEM item = { 0 };
	INVOKE_NATIVE(cursor_->GetKey(&item));
	array<Byte>^ result = gcnew array<Byte>(item.size);
	if (item.size > 0) {
		pin_ptr<Byte> resultPtr = &result[0];
		memcpy(resultPtr, item.data, item.size);
	}
	return result;
}

array<Byte>^ Cursor::GetValue() {
	if (schemaType_ == CursorSchemaType::KeyOnly)
		throw gcnew WiredTigerException("for current schema [CursorSchemaType.KeyOnly] value is not defined");
	WT_ITEM item = { 0 };
	INVOKE_NATIVE(cursor_->GetValue(&item))
		array<Byte>^ result = gcnew array<Byte>(item.size);
	if (item.size > 0) {
		pin_ptr<Byte> resultPtr = &result[0];
		memcpy(resultPtr, item.data, item.size);
	}
	return result;
}

// *************
// Session
// *************

Session::Session(WT_SESSION *session) : session_(session) {
}

void Session::Close() {
	if (session_ != nullptr) {
		session_->close(session_, nullptr);
		session_ = nullptr;
	}
}

void Session::BeginTran() {
	int r = session_->begin_transaction(session_, nullptr);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "session->begin_transaction");
}

void Session::CommitTran() {
	int r = session_->commit_transaction(session_, nullptr);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "session->commit_transaction");
}

void Session::RollbackTran() {
	int r = session_->rollback_transaction(session_, nullptr);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "session->rollback_transaction");
}

void Session::Checkpoint() {
	int r = session_->checkpoint(session_, nullptr);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "session->checkpoint");
}

static std::string str_or_empty(System::String^ s) {
	std::string result(msclr::interop::marshal_as<std::string>(s == nullptr ? "" : s));
	return result;
}

static std::string str_or_die(System::String^ s, System::String^ parameterName) {
	if (s == nullptr)
		throw gcnew System::InvalidOperationException("parameter [" + parameterName + "] can't be null");
	std::string result(msclr::interop::marshal_as<std::string>(s));
	return result;
}

void Session::Create(System::String^ name, System::String^ config) {
	std::string nameStr(str_or_die(name, "name"));
	std::string configStr(str_or_empty(config));
	int r = session_->create(session_, nameStr.c_str(), configStr.c_str());
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "session->create");
}

Cursor^ Session::OpenCursor(System::String^ name) {
	std::string nameStr(str_or_die(name, "name"));
	return gcnew Cursor(OpenNativeCursor(session_, nameStr.c_str(), nullptr));
}

Cursor^ Session::OpenCursor(System::String^ name, System::String^ config) {
	std::string nameStr(str_or_die(name, "name"));
	std::string configStr(str_or_die(config, "config"));
	return gcnew Cursor(OpenNativeCursor(session_, nameStr.c_str(), configStr.c_str()));
}

// *************
// Connection
// *************
template<typename T>
static T to_pointer(System::Delegate^ d) {
	return (T)System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(d).ToPointer();
}

Connection::Connection(IEventHandler^ eventHandler)
	:eventHandler_(eventHandler),
	onErrorDelegate_(gcnew OnErrorDelegate(this, &Connection::OnError)),
	onMessageDelegate_(gcnew OnMessageDelegate(this, &Connection::OnMessage)) {
	if (eventHandler_ == nullptr)
		nativeEventHandler_ = nullptr;
	else {
		typedef int(*handle_error_t)(WT_EVENT_HANDLER *handler, WT_SESSION *session, int error, const char *message);
		typedef int(*handle_message_t)(WT_EVENT_HANDLER *handler, WT_SESSION *session, const char *message);
		nativeEventHandler_ = new WT_EVENT_HANDLER{
			to_pointer<handle_error_t>(onErrorDelegate_),
			to_pointer<handle_message_t>(onMessageDelegate_),
			nullptr,
			nullptr };
	}
}

void Connection::Close() {
	if (connection_ != nullptr)
	{
		connection_->close(connection_, nullptr);
		connection_ = nullptr;
	}
	if (nativeEventHandler_ != nullptr) {
		delete nativeEventHandler_;
		nativeEventHandler_ = nullptr;
	}
}

Session^ Connection::OpenSession() {
	WT_SESSION *session;
	int r = connection_->open_session(connection_, nullptr, nullptr, &session);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "connection->open_session");
	return gcnew Session(session);
}

System::String^ Connection::GetHome() {
	const char *home = connection_->get_home(connection_);
	return gcnew System::String(home);
}

Connection^ Connection::Open(System::String^ home, System::String^ config, IEventHandler^ eventHandler) {
	WT_CONNECTION *connectionp;
	std::string homeStr(str_or_die(home, "home"));
	std::string configStr(str_or_empty(config));
	Connection^ ret = gcnew Connection(eventHandler);
	int r = wiredtiger_open(homeStr.c_str(), ret->nativeEventHandler_, configStr.c_str(), &connectionp);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "wiredtiger_open");
	ret->connection_ = connectionp;
	return ret;
}

static bool can_use_referenced_objects() {
	bool isFinalizingForUnload = System::AppDomain::CurrentDomain->IsFinalizingForUnload();
	bool hasShutdownStarted = System::Environment::HasShutdownStarted;
	return !isFinalizingForUnload && !hasShutdownStarted;
}

int Connection::OnError(WT_EVENT_HANDLER *handler, WT_SESSION *session, int error, const char* message) {
	if (!can_use_referenced_objects())
		return -2;
	try {
		const char* err = wiredtiger_strerror(error);
		eventHandler_->OnError(error, gcnew System::String(err), gcnew System::String(message));
		return 0;
	}
	catch (System::Exception^ e) {
		//do not propagate managed exceptions as it can violate internal wt invariants
		return -1;
	}
}

int Connection::OnMessage(WT_EVENT_HANDLER *handler, WT_SESSION *session, const char* message) {
	if (!can_use_referenced_objects())
		return -2;
	try {
		eventHandler_->OnMessage(gcnew System::String(message));
		return 0;
	}
	catch (System::Exception^ e) {
		//do not propagate managed exceptions as it can violate internal wt invariants
		return -1;
	}
}