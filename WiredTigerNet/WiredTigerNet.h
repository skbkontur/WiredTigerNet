#pragma once

namespace WiredTigerNet {

	public ref class WiredException : System::Exception {
	public:
		WiredException(int tigerError);
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

	public ref class Cursor: public System::IDisposable {
	public:
		virtual ~Cursor();
		void Insert();
		bool Next();
		bool Prev();
		void Remove();
		void Reset();
		bool Search();
		bool SearchNear([System::Runtime::InteropServices::OutAttribute] int% result);
		void Update();
		long GetTotalCount(array<Byte>^ left, bool leftInclusive, array<Byte>^ right, bool rightInclusive);
		array<Byte>^ GetKey();
		array<Byte>^ GetValue();
		void SetKey(array<Byte>^ key);
		void SetValue(array<Byte>^ value);
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
		static Connection^ Open(System::String^ home, System::String^ config);
	private:
		WT_CONNECTION* _connection;
		Connection();
	};
}