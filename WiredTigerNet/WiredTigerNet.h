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

	public interface class IEventHandler {
		void OnError(int errorCode, System::String^ errorString, System::String^ message);
		void OnMessage(System::String^ message);
	};

	public ref class Cursor : public System::IDisposable {
	public:
		virtual ~Cursor();
		void Insert(array<Byte>^ key, array<Byte>^ value);
		void Insert(array<Byte>^ key);
		bool Next();
		bool Prev();
		void Remove(array<Byte>^ key);
		void Reset();
		bool Search(array<Byte>^ key);
		bool SearchNear(array<Byte>^ key, [System::Runtime::InteropServices::OutAttribute] int% result);
		long GetTotalCount(array<Byte>^ left, bool leftInclusive, array<Byte>^ right, bool rightInclusive);
		array<Byte>^ GetKey();
		array<Byte>^ GetValue();
	internal:
		Cursor(WT_CURSOR* cursor);
	private:
		WT_CURSOR* _cursor;
	};

	public ref class Session : public System::IDisposable {
	public:
		~Session();
		void BeginTran();
		void CommitTran();
		void RollbackTran();
		void Checkpoint();
		void Create(System::String^ name, System::String^ config);
		Cursor^ OpenCursor(System::String^ name);
	internal:
		Session(WT_SESSION *session);
	private:
		WT_SESSION* session_;
	};

	public ref class Connection : public System::IDisposable {
	public:
		~Connection();
		Session^ OpenSession();
		System::String^ GetHome();
		static Connection^ Open(System::String^ home, System::String^ config, IEventHandler^ eventHandler);
	private:
		WT_CONNECTION* _connection;
		Connection(IEventHandler^ eventHandler);

		[System::Runtime::InteropServices::UnmanagedFunctionPointer(System::Runtime::InteropServices::CallingConvention::Cdecl)]
		delegate int OnErrorDelegate(WT_EVENT_HANDLER *handler, WT_SESSION *session, int error, const char* message);
		int OnError(WT_EVENT_HANDLER *handler, WT_SESSION *session, int error, const char* message);
		OnErrorDelegate^ onErrorDelegate_;

		[System::Runtime::InteropServices::UnmanagedFunctionPointer(System::Runtime::InteropServices::CallingConvention::Cdecl)]
		delegate int OnMessageDelegate(WT_EVENT_HANDLER *handler, WT_SESSION *session, const char* message);
		int OnMessage(WT_EVENT_HANDLER *handler, WT_SESSION *session, const char* message);
		OnMessageDelegate^ onMessageDelegate_;

		WT_EVENT_HANDLER* nativeEventHandler_;
		IEventHandler^ eventHandler_;
	};
}