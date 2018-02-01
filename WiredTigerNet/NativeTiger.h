#include <wiredtiger.h>
#include <string>

typedef unsigned char Byte;

class NativeWiredTigerApiException : public std::exception {
public:
	NativeWiredTigerApiException(int errorCode, const std::string apiName) :errorCode_(errorCode), apiName_(apiName) {
	}
	const int ErrorCode() const { return errorCode_; }
	const std::string& ApiName() const { return apiName_; }
	NativeWiredTigerApiException& operator=(const NativeWiredTigerApiException&) = delete;
private:
	int errorCode_;
	const std::string apiName_;
};

enum NativeDirection {
	Ascending,
	Descending
};

class NativeCursor {
public:
	~NativeCursor();
	bool IterationBegin(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive, NativeDirection newDirection, bool copyBoundary);
	bool IterationMove();
	__int64 GetTotalCount(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive);
	__int64 GetTotalCountMax(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive, __int64 maxCount);
	int Reset();
	const char* KeyFormat() const { return cursor_->key_format; }
	const char* ValueFormat() const { return cursor_->value_format; }
	bool Search(Byte* key, int keyLength);
	bool SearchNear(Byte* data, int length, int* exact);
	bool Next();
	bool Prev();
	void Insert(Byte* key, int keyLength, Byte* value, int valueLength);
	void Insert(Byte* key, int keyLength);
	void Remove(Byte* key, int keyLength);
	void GetKey(WT_ITEM* target);
	void GetValue(WT_ITEM* target);

	friend NativeCursor* OpenNativeCursor(WT_SESSION* session, const char* name, const char* config);
private:
	NativeCursor(WT_CURSOR* cursor);
	WT_CURSOR* cursor_;
	bool keyIsString_;
	NativeDirection direction_;
	Byte* boundary_;
	int boundarySize_;
	bool boundaryInclusive_;
	bool ownsBoundary_;
	bool Within();
	void SetKey(Byte* data, int length);
	void SetValue(Byte* data, int length);
	void SetBoundary(Byte* boundary, int boundarySize, bool boundaryInclusive, bool ownsBoundary);
};

NativeCursor* OpenNativeCursor(WT_SESSION* session, const char* name, const char* config);