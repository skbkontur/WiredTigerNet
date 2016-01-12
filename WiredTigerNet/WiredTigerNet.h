#pragma once

namespace WiredTigerNet {

	public ref class WiredTigerException : System::Exception {
	public:
		WiredTigerException(System::String^ message);
	};

	public ref class WiredTigerApiException : WiredTigerException {
	public:
		WiredTigerApiException(int errorCode, System::String^ apiName);
		property int ErrorCode {
			int get() { return errorCode_; }
		}
		property System::String^ ApiName {
			System::String^ get() override { return apiName_; }
		}
	private:
		int errorCode_;
		System::String^ apiName_;
	};

	public interface class IEventHandler {
		void OnError(int errorCode, System::String^ errorString, System::String^ message);
		void OnMessage(System::String^ message);
	};

	public enum class ErrorCodes {
		WtRollback = -31800,
		WtDuplicateKey = -31801,
		WtError = -31802,
		WtNotFound = -31803,
		WtPanic = -31804,
		WtRestart = -31805,
		WtRunRecovery = -31806,
		WtCacheFull = -31807
	};

	public enum class CursorSchemaType {
		KeyAndValue,
		KeyOnly
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
		property CursorSchemaType SchemaType {
			CursorSchemaType get() override { return schemaType_; }
		}
	internal:
		Cursor(WT_CURSOR* cursor);
	private:
		WT_CURSOR* cursor_;
		CursorSchemaType schemaType_;
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
		Cursor^ OpenCursor(System::String^ name, System::String^ config);
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
		WT_CONNECTION* connection_;
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