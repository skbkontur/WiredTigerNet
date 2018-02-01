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
			System::String^ get() { return apiName_; }
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

	public value class Boundary {
	public:
		Boundary(array<Byte>^ bytes, bool inclusive);
		property array<Byte>^ Bytes {
			array<Byte>^ get() { return bytes_; }
		}
		property bool Inclusive {
			bool get() { return inclusive_; }
		}
	private:
		array<Byte>^ bytes_;
		bool inclusive_;
	};

	public value class Range {
	public:
		Range(System::Nullable<Boundary> left, System::Nullable<Boundary> right);
		property System::Nullable<Boundary> Left {
			System::Nullable<Boundary> get() { return left_; }
		}
		property System::Nullable<Boundary> Right {
			System::Nullable<Boundary> get() { return right_; }
		}
		static Range Segment(array<Byte>^ left, array<Byte>^ right);
		static Range PositiveRay(array<Byte>^ left);
		static Range NegativeRay(array<Byte>^ right);
		static Range NegativeOpenRay(array<Byte>^ right);
		static Range Line();
		static Range Empty();
		static Range PositiveOpenRay(array<Byte>^ left);
		static Range LeftOpenSegment(array<Byte>^ left, array<Byte>^ right);
		static Range RightOpenSegment(array<Byte>^ left, array<Byte>^ right);
		static Range Interval(array<Byte>^ left, array<Byte>^ right);
		static Range Prefix(array<Byte>^ prefix);
		static Range Prefix(array<Byte>^ left, array<Byte>^ right);
		static array<Byte>^ IncrementBytes(array<Byte>^ source);
		static array<Byte>^ DecrementBytes(array<Byte>^ source);
	private:
		System::Nullable<Boundary> left_;
		System::Nullable<Boundary> right_;

		static Range line = Range(System::Nullable<Boundary>(), System::Nullable<Boundary>());
		static Boundary exclusiveOne = Boundary(gcnew array<Byte>(1) { 1 }, false);
		static Range emptyRange = Range(exclusiveOne, exclusiveOne);
	};

	[System::Runtime::CompilerServices::ExtensionAttribute]
	public ref class RangeOperators abstract sealed {
	public:
		[System::Runtime::CompilerServices::ExtensionAttribute]
		static Range Inclusive(Range range);
		[System::Runtime::CompilerServices::ExtensionAttribute]
		static Range Prepend(Range range, array<Byte>^ prefix);
		[System::Runtime::CompilerServices::ExtensionAttribute]
		static Range IntersectWith(Range a, Range b);
		[System::Runtime::CompilerServices::ExtensionAttribute]
		static bool IsEmpty(Range range);
	private:
		static System::Nullable<Boundary> MaxLeft(System::Nullable<Boundary> a, System::Nullable<Boundary> b);
		static System::Nullable<Boundary> MinRight(System::Nullable<Boundary> a, System::Nullable<Boundary> b);
		static int CompareLeft(System::Nullable<Boundary> a, System::Nullable<Boundary> b);
		static int CompareRight(System::Nullable<Boundary> a, System::Nullable<Boundary> b);
		static System::Nullable<Boundary> Prepend(System::Nullable<Boundary> boundary, array<Byte>^ prefix);
		static System::Nullable<Boundary> Inclusive(System::Nullable<Boundary> boundary);
	};

	public ref class WiredTigerComponent abstract  {
	public:
		WiredTigerComponent(WiredTigerComponent^ parent);
		~WiredTigerComponent();
		!WiredTigerComponent();
	protected:
		virtual void Close() abstract;
	internal:
		bool IsDisposed();
	private:
		bool disposed_;
		WiredTigerComponent^ parent_;
	};

	public enum class Direction {
		Ascending = 0,
		Descending = 1,
	};

	public ref class Cursor : public WiredTigerComponent {
	public:
		void Insert(array<Byte>^ key, array<Byte>^ value);
		void Insert(array<Byte>^ key);
		bool Next();
		bool Prev();
		void Remove(array<Byte>^ key);
		void Reset();
		bool Search(array<Byte>^ key);
		bool SearchNear(array<Byte>^ key, [System::Runtime::InteropServices::OutAttribute] int% result);
		__int64 GetTotalCount(Range range);
		__int64 GetTotalCountMax(Range range, __int64 maxCount);
		array<Byte>^ GetKey();
		array<Byte>^ GetValue();
		bool IterationBegin(Range range, Direction direction);
		bool IterationMove();
		property CursorSchemaType SchemaType {
			CursorSchemaType get() { return schemaType_; }
		}
	protected:
		virtual void Close() override;
	internal:
		Cursor(NativeCursor* cursor, WiredTigerComponent^ session);
	private:
		NativeCursor* cursor_;
		CursorSchemaType schemaType_;
	};

	public ref class Session : public WiredTigerComponent {
	public:
		void BeginTran();
		void CommitTran();
		void RollbackTran();
		void Checkpoint(System::String^ config);
		void Compact(System::String^ name, System::String^ config);
		void Create(System::String^ name, System::String^ config);
		void Drop(System::String^ name, System::String^ config);
		void Rename(System::String^ oldName, System::String^ newName, System::String^ config);
		void Upgrade(System::String^ name, System::String^ config);
		void Verify(System::String^ name, System::String^ config);
		Cursor^ OpenCursor(System::String^ name);
		Cursor^ OpenCursor(System::String^ name, System::String^ config);
	protected:
		virtual void Close() override;
	internal:
		Session(WT_SESSION *session, WiredTigerComponent^ connection);
	private:
		WT_SESSION* session_;
	};

	public ref class Connection : public WiredTigerComponent {
	public:
		Session^ OpenSession();
		System::String^ GetHome();
		static Connection^ Open(System::String^ home, System::String^ config, IEventHandler^ eventHandler);
	protected:
		virtual void Close() override;
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