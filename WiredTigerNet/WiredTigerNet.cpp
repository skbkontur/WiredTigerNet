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
	if (_cursor != NULL) {
		_cursor->close(_cursor);
		_cursor = NULL;
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

void Cursor::Insert(uint32_t key, array<Byte>^ value) {
	pin_ptr<Byte> valuePtr = &value[0];
	int r = NativeInsert(_cursor, key, valuePtr, value->Length);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Cursor::InsertIndex(array<Byte>^ indexKey, array<Byte>^ primaryKey) {
	pin_ptr<Byte> indexKeyPtr = &indexKey[0];
	pin_ptr<Byte> primaryKeyPtr = &primaryKey[0];
	int r = NativeInsertIndex(_cursor, indexKeyPtr, indexKey->Length, primaryKeyPtr, primaryKey->Length);
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

bool Cursor::Search(uint32_t key) {
	int r = NativeSearch(_cursor, key);
	if (r == WT_NOTFOUND) 
		return false;
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return true;
}

generic <typename ValueType>
array<ValueType>^ Cursor::Decode(array<uint32_t>^ keys) {
	array<ValueType>^ values = gcnew array<ValueType>(keys->Length);
	try
	{
		pin_ptr<uint32_t> keysPtr = &keys[0];
		pin_ptr<ValueType> valuesPtr = &values[0];
		NativeDecode(_cursor, keysPtr, keys->Length, (Byte*)valuesPtr);
	}
	catch (const NativeTigerException& e) {
		throw gcnew WiredException(0, gcnew System::String(e.Message().c_str()));
	}
	return values;
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

uint32_t Cursor::GetKeyUInt32() {
	uint32_t key;
	int r = _cursor->get_key(_cursor, &key);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return key;
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

uint32_t Cursor::GetValueUInt32() {
	uint32_t value;
	int r = _cursor->get_value(_cursor, &value);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return value;
}

// *************
// Session
// *************

Session::Session(WT_SESSION *session) : _session(session) {
}

Session::~Session() {
	if (_session != NULL) {
		_session->close(_session, NULL);
		_session = NULL;
	}
}

void Session::BeginTran() {
	int r = _session->begin_transaction(_session, NULL);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::BeginTran(System::String^ config) {
	const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
	int r = _session->begin_transaction(_session, aconfig);
	if (aconfig)
		Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::CommitTran() {
	int r = _session->commit_transaction(_session, NULL);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::RollbackTran() {
	int r = _session->rollback_transaction(_session, NULL);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Checkpoint() {
	int r = _session->checkpoint(_session, NULL);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Checkpoint(System::String^ config) {
	const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
	int r = _session->checkpoint(_session, aconfig);
	if (aconfig)
		Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Compact(System::String^ name) {
	const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
	int r = _session->compact(_session, aname, NULL);
	Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Create(System::String^ name, System::String^ config) {
	const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
	const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
	int r = _session->create(_session, aname, aconfig);
	Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
	if (aconfig)
		Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Drop(System::String^ name) {
	const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
	int r = _session->drop(_session, aname, NULL);
	Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

void Session::Rename(System::String^ oldname, System::String^ newname) {
	const char *aoldname = (char*)Marshal::StringToHGlobalAnsi(oldname).ToPointer();
	const char *anewname = (char*)Marshal::StringToHGlobalAnsi(newname).ToPointer();
	int r = _session->rename(_session, aoldname, anewname, NULL);
	Marshal::FreeHGlobal((System::IntPtr)(void*)aoldname);
	Marshal::FreeHGlobal((System::IntPtr)(void*)anewname);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
}

Cursor^ Session::OpenCursor(System::String^ name) {
	WT_CURSOR *cursor;
	const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
	int r = _session->open_cursor(_session, aname, NULL, NULL, &cursor);
	Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return gcnew Cursor(cursor);
}

Cursor^ Session::OpenCursor(System::String^ name, System::String^ config) {
	WT_CURSOR *cursor;
	const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
	const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
	int r = _session->open_cursor(_session, aname, NULL, aconfig, &cursor);
	Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
	if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
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
			NULL, 
			NULL };
	}
}

Connection::~Connection() {
	if (_connection != NULL)
	{
		_connection->close(_connection, NULL);
		_connection = NULL;
	}
	if (nativeEventHandler_ != nullptr) {
		delete nativeEventHandler_;
		nativeEventHandler_ = nullptr;
	}
}

Session^ Connection::OpenSession(System::String^ config) {
	const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
	WT_SESSION *session;
	int r = _connection->open_session(_connection, NULL, aconfig, &session);
	if (aconfig)
		Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
	return gcnew Session(session);
}

bool Connection::IsNew() {
	return _connection->is_new(_connection) ? true : false;
}

System::String^ Connection::GetHome() {
	const char *home = _connection->get_home(_connection);
	return gcnew System::String(home);
}

void Connection::AsyncFlush() {
	int r = _connection->async_flush(_connection);
	if (r != 0)
		throw gcnew WiredException(r, nullptr);
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
	const char* err = wiredtiger_strerror(error);
	eventHandler_->OnError(error, gcnew System::String(err), gcnew System::String(message));
	return 0;
}

int Connection::OnMessage(WT_EVENT_HANDLER *handler, WT_SESSION *session, const char* message) {
	eventHandler_->OnMessage(gcnew System::String(message));
	return 0;
}