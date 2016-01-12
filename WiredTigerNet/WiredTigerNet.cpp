#include "NativeTiger.h"
#include "WiredTigerNet.h"
#include "msclr\marshal_cppstd.h"
#include "msclr\marshal.h"
#include <string>

using namespace WiredTigerNet;

// *************
// WiredTigerApiException
// *************

static System::String^ FormatMessage(int errorCode, System::String^ apiName) {
	const char* err = wiredtiger_strerror(errorCode);
	return System::String::Format("error code [{0}], error message [{1}], api name [{2}]",
		errorCode, gcnew System::String(err), apiName);
}

WiredTigerApiException::WiredTigerApiException(int errorCode, System::String^ apiName)
	: errorCode_(errorCode), apiName_(apiName), System::Exception(FormatMessage(errorCode, apiName)) {
}

// *************
// Cursor
// *************

Cursor::Cursor(WT_CURSOR* cursor) : _cursor(cursor) {
}

Cursor::~Cursor() {
	if (_cursor != nullptr) {
		_cursor->close(_cursor);
		_cursor = nullptr;
	}
}

void Cursor::Insert(array<Byte>^ key, array<Byte>^ value) {
	pin_ptr<Byte> keyPtr = &key[0];
	pin_ptr<Byte> valuePtr = &value[0];
	int r = NativeInsert(_cursor, keyPtr, key->Length, valuePtr, value->Length);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->insert");
}

void Cursor::Insert(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeInsert(_cursor, keyPtr, key->Length);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->insert");
}

bool Cursor::Next() {
	int r = _cursor->next(_cursor);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->next");
	return true;
}

bool Cursor::Prev() {
	int r = _cursor->prev(_cursor);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->prev");
	return true;
}

void Cursor::Remove(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeRemove(_cursor, keyPtr, key->Length);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->remove");
}

void Cursor::Reset() {
	int r = _cursor->reset(_cursor);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->reset");
}

bool Cursor::Search(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeSearch(_cursor, keyPtr, key->Length);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->search");
	return true;
}

bool Cursor::SearchNear(array<Byte>^ key, [System::Runtime::InteropServices::OutAttribute] int% result) {
	int exact;
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeSearchNear(_cursor, keyPtr, key->Length, &exact);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->search_near");
	result = exact;
	return true;
}

long Cursor::GetTotalCount(array<Byte>^ left, bool leftInclusive, array<Byte>^ right, bool rightInclusive) {
	pin_ptr<Byte> leftPtr;
	int leftSize;
	if (left != nullptr && left->Length > 0) {
		leftSize = left->Length;
		leftPtr = &left[0];
	}
	else {
		leftSize = 0;
		leftPtr = nullptr;
	}
	pin_ptr<Byte> rightPtr;
	int rightSize;
	if (right != nullptr && right->Length) {
		rightSize = right->Length;
		rightPtr = &right[0];
	}
	else {
		rightSize = 0;
		rightPtr = nullptr;
	}
	try {
		return NativeGetTotalCount(_cursor, leftPtr, leftSize, leftInclusive, rightPtr, rightSize, rightInclusive);
	}
	catch (const NativeWiredTigerApiException& e) {
		throw gcnew WiredTigerApiException(e.ErrorCode(), gcnew System::String(e.ApiName().c_str()));
	}
}

array<Byte>^ Cursor::GetKey() {
	WT_ITEM item = { 0 };
	int r = _cursor->get_key(_cursor, &item);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->get_key");
	array<Byte>^ result = gcnew array<Byte>(item.size);
	if (item.size > 0) {
		pin_ptr<Byte> resultPtr = &result[0];
		memcpy(resultPtr, item.data, item.size);
	}
	return result;
}

array<Byte>^ Cursor::GetValue() {
	WT_ITEM item = { 0 };
	int r = _cursor->get_value(_cursor, &item);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "cursor->get_value");
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

Session::~Session() {
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
	WT_CURSOR* cursor;
	std::string nameStr(str_or_die(name, "name"));
	int r = session_->open_cursor(session_, nameStr.c_str(), nullptr, nullptr, &cursor);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "session->open_cursor");
	return gcnew Cursor(cursor);
}

Cursor^ Session::OpenCursor(System::String^ name, System::String^ config) {
	WT_CURSOR* cursor;
	std::string nameStr(str_or_die(name, "name"));
	std::string configStr(str_or_die(config, "config"));
	int r = session_->open_cursor(session_, nameStr.c_str(), nullptr, configStr.c_str(), &cursor);
	if (r != 0)
		throw gcnew WiredTigerApiException(r, "session->open_cursor");
	return gcnew Cursor(cursor);
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

Connection::~Connection() {
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