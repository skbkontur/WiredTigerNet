#include "NativeTiger.h"
#include "WiredTigerNet.h"
#include "msclr\marshal_cppstd.h"
#include "msclr\marshal.h"
#include <string>

using namespace System::Runtime::InteropServices;
using namespace WiredTigerNet;

// *************
// WiredException
// *************

WiredException::WiredException(int tigerError, System::String^ message) : tigerError_(tigerError) {
	const char *err = wiredtiger_strerror(tigerError);
	message_ = message == nullptr
		? System::String::Format("WiredTiger Error {0} : [{1}]", tigerError, gcnew System::String(err))
		: message;
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
		throw gcnew WiredException(r, nullptr);
}

void Cursor::Insert(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeInsert(_cursor, keyPtr, key->Length);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

bool Cursor::Next() {
	int r = _cursor->next(_cursor);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return true;
}

bool Cursor::Prev() {
	int r = _cursor->prev(_cursor);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return true;
}

void Cursor::Remove(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeRemove(_cursor, keyPtr, key->Length);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Cursor::Reset() {
	int r = _cursor->reset(_cursor);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

bool Cursor::Search(array<Byte>^ key) {
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeSearch(_cursor, keyPtr, key->Length);
	if (r == WT_NOTFOUND) 
		return false;
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return true;
}

bool Cursor::SearchNear(array<Byte>^ key, [System::Runtime::InteropServices::OutAttribute] int% result) {
	int exact;
	pin_ptr<Byte> keyPtr = &key[0];
	int r = NativeSearchNear(_cursor, keyPtr, key->Length, &exact);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
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
	return NativeGetTotalCount(_cursor, leftPtr, leftSize, leftInclusive, rightPtr, rightSize, rightInclusive);
}

array<Byte>^ Cursor::GetKey() {
	WT_ITEM item = { 0 };
	int r = _cursor->get_key(_cursor, &item);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
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
		throw gcnew WiredException(r, nullptr);
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
		throw gcnew WiredException(r, nullptr);
}

void Session::CommitTran() {
	int r = session_->commit_transaction(session_, nullptr);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::RollbackTran() {
	int r = session_->rollback_transaction(session_, nullptr);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Checkpoint() {
	int r = session_->checkpoint(session_, nullptr);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Create(System::String^ name, System::String^ config) {
	std::string nameStr(msclr::interop::marshal_as<std::string>(name));
	std::string configStr(msclr::interop::marshal_as<std::string>(config));
	int r = session_->create(session_, nameStr.c_str(), configStr.c_str());
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

Cursor^ Session::OpenCursor(System::String^ name) {
	WT_CURSOR* cursor;
	std::string nameStr(msclr::interop::marshal_as<std::string>(name));
	int r = session_->open_cursor(session_, nameStr.c_str(), nullptr, nullptr, &cursor);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return gcnew Cursor(cursor);
}

// *************
// Connection
// *************
template<typename T>
static T to_pointer(System::Delegate^ d) {
	return (T)System::Runtime::InteropServices::Marshal::GetFunctionPointerForDelegate(d).ToPointer();
}

Connection::Connection(IEventHandler^ eventHandler) :
	eventHandler_(eventHandler),
	onErrorDelegate_(gcnew OnErrorDelegate(this, &Connection::OnError)),
	onMessageDelegate_(gcnew OnMessageDelegate(this, &Connection::OnMessage)) {
	if (eventHandler_ == nullptr)
		nativeEventHandler_ = nullptr;
	else {
		typedef int(*handle_error_t)(WT_EVENT_HANDLER *handler, WT_SESSION *session, int error, const char *message);
		typedef int(*handle_message_t)(WT_EVENT_HANDLER *handler, WT_SESSION *session, const char *message);
		nativeEventHandler_ = new WT_EVENT_HANDLER { 
			to_pointer<handle_error_t>(onErrorDelegate_),
			to_pointer<handle_message_t>(onMessageDelegate_),
			nullptr, 
			nullptr };
	}
}

Connection::~Connection() {
	if (_connection != nullptr)
	{
		_connection->close(_connection, nullptr);
		_connection = nullptr;
	}
	if (nativeEventHandler_ != nullptr) {
		delete nativeEventHandler_;
		nativeEventHandler_ = nullptr;
	}
}

Session^ Connection::OpenSession() {
	WT_SESSION *session;
	int r = _connection->open_session(_connection, nullptr, nullptr, &session);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return gcnew Session(session);
}

System::String^ Connection::GetHome() {
	const char *home = _connection->get_home(_connection);
	return gcnew System::String(home);
}

Connection^ Connection::Open(System::String^ home, System::String^ config, IEventHandler^ eventHandler) {
	WT_CONNECTION *connectionp;
	std::string homeStr(msclr::interop::marshal_as<std::string>(home));
	std::string configStr(msclr::interop::marshal_as<std::string>(config));
	Connection^ ret = gcnew Connection(eventHandler);
	int r = wiredtiger_open(homeStr.c_str(), ret->nativeEventHandler_, configStr.c_str(), &connectionp);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	ret->_connection = connectionp;
	return ret;
}

int Connection::OnError(WT_EVENT_HANDLER *handler, WT_SESSION *session, int error, const char* message) {
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
	try {
		eventHandler_->OnMessage(gcnew System::String(message));
		return 0;
	}
	catch (System::Exception^ e) {
		//do not propagate managed exceptions as it can violate internal wt invariants
		return -1;
	}
}