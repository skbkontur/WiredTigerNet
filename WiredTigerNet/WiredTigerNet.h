#pragma once

namespace WiredTigerNet {

	public ref class WiredException : System::Exception {
	public:
		WiredException(int tigerError, System::String^ message);
		property int TigerError {
			int get() { return tigerError_; }
		}
		property System::String^ Message {
			System::String^ get() override { return message_; }
		}
	private:
		int tigerError_;
		System::String^ message_;
	};

	public interface class IEventHandler
	{
		void OnError(int errorCode, System::String^ errorString, System::String^ message);
		void OnMessage(System::String^ message);
	};

	public ref class Cursor : public System::IDisposable {
	public:
		virtual ~Cursor();
		void Insert(array<Byte>^ key, array<Byte>^ value);
		void InsertIndex(array<Byte>^ indexKey, array<Byte>^ primaryKey);
		void Insert(array<Byte>^ key);
		void Insert(uint32_t key, array<Byte>^ value);
		bool Next();
		bool Prev();
		void Remove(array<Byte>^ key);
		void Reset();
		bool Search(array<Byte>^ key);
		bool Search(uint32_t key);
		bool SearchNear(array<Byte>^ key, [System::Runtime::InteropServices::OutAttribute] int% result);
		long GetTotalCount(array<Byte>^ left, bool leftInclusive, array<Byte>^ right, bool rightInclusive);
		array<Byte>^ GetKey();
		uint32_t GetKeyUInt32();
		array<Byte>^ GetValue();
		uint32_t GetValueUInt32();
		generic <typename ValueType>
			array<ValueType>^ Decode(array<uint32_t>^ keys);
	internal:
		Cursor(WT_CURSOR* cursor);
	private:
		WT_CURSOR* _cursor;
	};

	public ref class Session : public System::IDisposable {
	public:
		~Session();
		void BeginTran();
		void BeginTran(System::String^ config);
		void CommitTran();
		void RollbackTran();
		void Checkpoint();
		void Checkpoint(System::String^ config);
		void Compact(System::String^ name);
		void Create(System::String^ name, System::String^ config);
		void Drop(System::String^ name);
		void Rename(System::String^ oldname, System::String^ newname);
		Cursor^ OpenCursor(System::String^ name);
		Cursor^ OpenCursor(System::String^ name, System::String^ config);
	internal:
		Session(WT_SESSION *session);
	private:
		WT_SESSION* _session;
	};

	public ref class Connection : public System::IDisposable {
	public:
		~Connection();
		Session^ OpenSession(System::String^ config);
		bool IsNew();
		System::String^ GetHome();
		void AsyncFlush();
		static Connection^ Open(System::String^ home, System::String^ config, IEventHandler^ eventHandler);
	private:
		WT_CONNECTION* _connection;
		Connection();

		[System::Runtime::InteropServices::UnmanagedFunctionPointer(System::Runtime::InteropServices::CallingConvention::Cdecl)]
		delegate void OnErrorDelegate(int errorCode, System::String^ errorString, System::String^ message);
		System::Delegate^ onErrorDelegate_;

		[System::Runtime::InteropServices::UnmanagedFunctionPointer(System::Runtime::InteropServices::CallingConvention::Cdecl)]
		delegate void OnMessageDelegate(System::String^ message);
		System::Delegate^ onMessageDelegate_;

		WT_EVENT_HANDLER* eventHandler_;
	};
}