#include <wiredtiger.h>
#include <string>

typedef unsigned char Byte;

class NativeWiredTigerApiException : public std::exception {
public:
	NativeWiredTigerApiException(int errorCode, const std::string apiName) :errorCode_(errorCode), apiName_(apiName) {
	}
	const int ErrorCode() const { return errorCode_; }
	const std::string& ApiName() const { return apiName_; }
private:
	int errorCode_;
	const std::string apiName_;
};

enum Direction {
	Ascending,
	Descending
};

class NativeCursor {
public:
	~NativeCursor();
	bool BeginIteration(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive, Direction newDirection, bool copyBoundary);
	bool Move();
	long GetTotalCount(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive);
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
	Direction direction_;
	
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