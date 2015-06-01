// WiredTigerNet.h

#pragma once

using namespace System::Runtime::InteropServices;

namespace WiredTigerNet {

	public ref class WiredException : System::Exception
	{
		int _tigerError;
		System::String^ _message;
	public:
		WiredException(int tigerError) : _tigerError(tigerError)
		{
			const char *err = wiredtiger_strerror(tigerError);
			_message = System::String::Format("WiredTiger Error {0} : [{1}]", tigerError, gcnew System::String(err));
		}

		property int TigerError {
			int get() { return _tigerError; }
		}

		property System::String^ Message {
			System::String^ get() override
			{
				return _message;
			}
		}
	};

	ref class Session;
	public ref class CursorBase abstract : public System::IDisposable
	{
	protected:
		WT_CURSOR* _cursor;
		CursorBase(WT_CURSOR* cursor) : _cursor(cursor){}
	public:
		virtual ~CursorBase()
		{
			if (_cursor != NULL)
			{
				_cursor->close(_cursor);
				_cursor = NULL;
			}
		}

		void Insert()
		{
			int r = _cursor->insert(_cursor);
			if (r != 0) throw gcnew WiredException(r);
		}

		bool Next()
		{
			int r = _cursor->next(_cursor);
			if (r == WT_NOTFOUND) return false;
			if (r != 0) throw gcnew WiredException(r);
			return true;
		}

		bool Prev()
		{
			int r = _cursor->prev(_cursor);
			if (r == WT_NOTFOUND) return false;
			if (r != 0) throw gcnew WiredException(r);
			return true;
		}

		void Remove()
		{
			int r = _cursor->remove(_cursor);
			if (r != 0) throw gcnew WiredException(r);
		}
		void Reset()
		{
			int r = _cursor->reset(_cursor);
			if (r != 0) throw gcnew WiredException(r);
		}

		bool Search()
		{
			int r = _cursor->search(_cursor);
			if (r == WT_NOTFOUND) return false;
			if (r != 0) throw gcnew WiredException(r);
			return true;
		}

		bool SearchNear([System::Runtime::InteropServices::OutAttribute] int% result)
		{
			int exact;
			int r = _cursor->search_near(_cursor, &exact);
			if (r == WT_NOTFOUND)
				return false;
			if (r != 0) throw gcnew WiredException(r);
			result = exact;
			return true;
		}

		void Update()
		{
			int r = _cursor->update(_cursor);
			if (r != 0) throw gcnew WiredException(r);
		}
	};

	public ref class Cursor  : public CursorBase
	{
	internal:
		Cursor(WT_CURSOR* cursor) : CursorBase(cursor)
		{
			/*if (strcmp(cursor->key_format, "u") != 0 || strcmp(cursor->value_format, "u") != 0)
			{
				throw gcnew Exception("Key or Value Format incompatible with this cursor!");
			}*/
		}
	public:
		long GetTotalCount(array<Byte>^ left, bool leftInclusive, array<Byte>^ right, bool rightInclusive) {
			pin_ptr<Byte> leftPtr;
			int leftSize;
			if (left != nullptr) {
				leftSize = left->Length;
				if (leftSize > 0)
					leftPtr = &left[0];
			}
			else {
				leftSize = 0;
				leftPtr = nullptr;
			}

			pin_ptr<Byte> rightPtr;
			int rightSize;
			if (right != nullptr) {
				rightSize = right->Length;
				if (rightSize > 0)
					rightPtr = &right[0];
			}
			else {
				rightSize = 0;
				rightPtr = nullptr;
			}

			return NativeGetTotalCount(_cursor, leftPtr, leftSize, leftInclusive, rightPtr, rightSize, rightInclusive);
		}

		array<Byte>^ GetKey()
		{
			WT_ITEM item = { 0 };
			int r=_cursor->get_key(_cursor, &item);
			if (r != 0) throw gcnew WiredException(r);

			array<Byte>^ buffer = gcnew array<Byte>((int)item.size);
			Marshal::Copy((System::IntPtr)(void*)item.data, buffer, 0, buffer->Length);
			return buffer;
		}
		void GetKey([System::Runtime::InteropServices::OutAttribute] array<Byte>^% item1, [System::Runtime::InteropServices::OutAttribute] array<Byte>^% item2)
		{
			WT_ITEM wt_item1 = { 0 };
			WT_ITEM wt_item2 = { 0 };
			int r = _cursor->get_key(_cursor, &wt_item1, &wt_item2);
			if (r != 0) throw gcnew WiredException(r);
			
			item1 = gcnew array<Byte>((int)wt_item1.size);
			Marshal::Copy((System::IntPtr)(void*)wt_item1.data, item1, 0, item1->Length);

			item2 = gcnew array<Byte>((int)wt_item2.size);
			Marshal::Copy((System::IntPtr)(void*)wt_item2.data, item2, 0, item2->Length);
		}
		array<Byte>^ GetValue()
		{
			WT_ITEM item = { 0 };
			int r =_cursor->get_value(_cursor, &item);
			if (r != 0) throw gcnew WiredException(r);

			array<Byte>^ buffer = gcnew array<Byte>((int)item.size);
			Marshal::Copy((System::IntPtr)(void*)item.data, buffer, 0, buffer->Length);
			return buffer;
		}

		void SetKey(System::IntPtr data, int length)
		{
			WT_ITEM item = { 0 };
			item.data = (void*)data;
			item.size = length;
			_cursor->set_key(_cursor, &item);
		}
		void SetKey(array<Byte>^ key)
		{
			pin_ptr<Byte> pkey = &key[0];
			WT_ITEM item = { 0 };
			item.data = (void*)pkey;
			item.size = key->Length;
			_cursor->set_key(_cursor, &item);
		}
		void SetKey(array<Byte>^ key1, array<Byte>^ key2)
		{
			pin_ptr<Byte> pkey1 = &key1[0];
			WT_ITEM item1 = { 0 };
			item1.data = (void*)pkey1;
			item1.size = key1->Length;

			pin_ptr<Byte> pkey2 = &key2[0];
			WT_ITEM item2 = { 0 };
			item2.data = (void*)pkey2;
			item2.size = key2->Length;

			_cursor->set_key(_cursor, &item1, &item2);
		}

		void Info()
		{
			System::Console::WriteLine("key_format [{0}], value_format [{1}]", gcnew System::String(_cursor->key_format), gcnew System::String(_cursor->value_format));
		}

		void SetValue(System::IntPtr data, int length)
		{
			WT_ITEM item = { 0 };
			item.data = (void*)data;
			item.size = length;
			_cursor->set_value(_cursor, &item);
		}
		void SetValue(array<Byte>^ value)
		{
			pin_ptr<Byte> pvalue = &value[0];
			WT_ITEM item = { 0 };
			item.data = (void*)pvalue;
			item.size = value->Length;
			_cursor->set_value(_cursor, &item);
		}
		void SetValue()
		{			
			_cursor->set_value(_cursor);
		}
	};

	public ref class CursorLong : public CursorBase
	{
	internal:
		CursorLong(WT_CURSOR* cursor) : CursorBase(cursor){
			if (strcmp(cursor->key_format, "q") != 0 || strcmp(cursor->value_format, "u") != 0)
			{
				throw gcnew System::Exception("Key or Value Format incompatible with this cursor!");
			}
		}
	public:
		System::Int64 GetKey()
		{
			int64_t key;
			int r = _cursor->get_key(_cursor, &key);
			if (r != 0) throw gcnew WiredException(r);
			return key;
		}
		array<Byte>^ GetValue()
		{
			WT_ITEM item = { 0 };
			int r = _cursor->get_value(_cursor, &item);
			if (r != 0) throw gcnew WiredException(r);

			array<Byte>^ buffer = gcnew array<Byte>((int)item.size);
			Marshal::Copy((System::IntPtr)(void*)item.data, buffer, 0, buffer->Length);
			return buffer;
		}

		void SetKey(System::Int64 key)
		{
			int64_t vkey = key;
			_cursor->set_key(_cursor, vkey);
		}
		void SetValue(System::IntPtr data, int length)
		{
			WT_ITEM item = { 0 };
			item.data = (void*)data;
			item.size = length;
			_cursor->set_value(_cursor, &item);
		}
		void SetValue(array<Byte>^ value)
		{
			pin_ptr<Byte> pvalue = &value[0];
			WT_ITEM item = { 0 };
			item.data = (void*)pvalue;
			item.size = value->Length;
			_cursor->set_value(_cursor, &item);
		}
	};


	public ref class Session : public System::IDisposable
	{
		WT_SESSION *_session;
	internal:
		Session(WT_SESSION *session) : _session(session)
		{}
	public:	
		~Session()
		{
			if (_session != NULL)
			{
				_session->close(_session, NULL);
				_session = NULL;
			}
		}

		void BeginTran()
		{
			int r = _session->begin_transaction(_session, NULL);
			if (r != 0) throw gcnew WiredException(r);
		}

		void BeginTran(System::String^ config)
		{
			const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
			int r = _session->begin_transaction(_session, aconfig);
			if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
			if (r != 0) throw gcnew WiredException(r);
		}

		void CommitTran()
		{
			int r = _session->commit_transaction(_session, NULL);
			if (r != 0) throw gcnew WiredException(r);
		}
		void RollbackTran()
		{
			int r = _session->rollback_transaction(_session, NULL);
			if (r != 0) throw gcnew WiredException(r);
		}

		void Checkpoint()
		{
			int r = _session->checkpoint(_session, NULL);
			if (r != 0) throw gcnew WiredException(r);
		}
		void Checkpoint(System::String^ config)
		{
			const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
			int r = _session->checkpoint(_session, aconfig);
			if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
			if (r != 0) throw gcnew WiredException(r);
		}

		void Compact(System::String^ name)
		{
			const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
			int r = _session->compact(_session, aname,NULL);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
			if (r != 0) throw gcnew WiredException(r);
		}

		void Create(System::String^ name, System::String^ config)
		{
			const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
			const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
			int r = _session->create(_session, aname, aconfig);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
			if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
			if (r != 0) throw gcnew WiredException(r);
		}

		void Drop(System::String^ name)
		{
			const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
			int r = _session->drop(_session, aname, NULL);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
			if (r != 0) throw gcnew WiredException(r);
		}

		void Rename(System::String^ oldname, System::String^ newname)
		{
			const char *aoldname = (char*)Marshal::StringToHGlobalAnsi(oldname).ToPointer();
			const char *anewname = (char*)Marshal::StringToHGlobalAnsi(newname).ToPointer();
			int r = _session->rename(_session, aoldname, anewname, NULL);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aoldname);
			Marshal::FreeHGlobal((System::IntPtr)(void*)anewname);
			if (r != 0) throw gcnew WiredException(r);
		}

		Cursor^ OpenCursor(System::String^ name)
		{
			WT_CURSOR *cursor;
			const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
			int r = _session->open_cursor(_session, aname, NULL, NULL, &cursor);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
			if (r != 0) throw gcnew WiredException(r);

			return gcnew Cursor(cursor);
		}
		Cursor^ OpenCursor(System::String^ name, System::String^ config)
		{
			WT_CURSOR *cursor;
			const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
			const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
			int r = _session->open_cursor(_session, aname, NULL, aconfig, &cursor);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
			if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
			if (r != 0) throw gcnew WiredException(r);

			return gcnew Cursor(cursor);
		}
		CursorLong^ OpenCursorLK(System::String^ name)
		{
			WT_CURSOR *cursor;
			const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
			int r = _session->open_cursor(_session, aname, NULL, NULL, &cursor);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
			if (r != 0) throw gcnew WiredException(r);

			return gcnew CursorLong(cursor);
		}
		CursorLong^ OpenCursorLK(System::String^ name, System::String^ config)
		{
			WT_CURSOR *cursor;
			const char *aname = (char*)Marshal::StringToHGlobalAnsi(name).ToPointer();
			const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
			int r = _session->open_cursor(_session, aname, NULL, aconfig, &cursor);
			Marshal::FreeHGlobal((System::IntPtr)(void*)aname);
			if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
			if (r != 0) throw gcnew WiredException(r);

			return gcnew CursorLong(cursor);
		}
	};

	public ref class Connection : public System::IDisposable
	{
		WT_CONNECTION *_connection;
		
		Connection(){}

	public:
		~Connection()
		{
			if (_connection != NULL)
			{
				_connection->close(_connection, NULL);
				_connection = NULL;
			}
		}

		Session^ OpenSession(System::String^ config)
		{
			const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();
			WT_SESSION *session;
			int r = _connection->open_session(_connection, NULL, aconfig, &session);
			if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
			if (r != 0) throw gcnew WiredException(r);

			return gcnew Session(session);
		}

		bool IsNew()
		{
			return _connection->is_new(_connection)?true:false;
		}

		System::String^ GetHome()
		{
			const char *home = _connection->get_home(_connection);
			return gcnew System::String(home);
		}

		void AsyncFlush()
		{
			int r = _connection->async_flush(_connection);
			if (r != 0) throw gcnew WiredException(r);
		}


		static Connection^ Open(System::String^ home, System::String^ config)
		{
			WT_CONNECTION *connectionp;

			const char *ahome = (char*)Marshal::StringToHGlobalAnsi(home).ToPointer();
			const char *aconfig = System::String::IsNullOrWhiteSpace(config) ? NULL : (char*)Marshal::StringToHGlobalAnsi(config).ToPointer();

			Connection^ ret = gcnew Connection();

			int r= wiredtiger_open(ahome,NULL /*WT_EVENT_HANDLER *errhandler*/, aconfig,&connectionp);
			Marshal::FreeHGlobal((System::IntPtr)(void*)ahome);
			if (aconfig) Marshal::FreeHGlobal((System::IntPtr)(void*)aconfig);
			if (r != 0) throw gcnew WiredException(r);

			ret->_connection = connectionp;

			return ret;
		}
	};

}
