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

long NativeGetTotalCount(WT_CURSOR* cursor, Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive);
int NativeInsert(WT_CURSOR* cursor, Byte* key, int keyLength, Byte* value, int valueLength);
int NativeInsert(WT_CURSOR* cursor, Byte* key, int keyLength);
int NativeRemove(WT_CURSOR* cursor, Byte* key, int keyLength);
int NativeSearch(WT_CURSOR* cursor, Byte* key, int keyLength);
int NativeSearchNear(WT_CURSOR* cursor, Byte* key, int keyLength, int *exactp);