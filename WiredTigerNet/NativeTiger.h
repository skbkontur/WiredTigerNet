#include <wiredtiger.h>
#include <string>

typedef unsigned char Byte;

class NativeTigerException : public std::exception {
public:
	NativeTigerException(const std::string message) :message_(message) {
	}
	const std::string& Message() const { return message_; }
private:
	const std::string message_;
};

long NativeGetTotalCount(WT_CURSOR* cursor, Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive);
int NativeInsert(WT_CURSOR* cursor, Byte* key, int keyLength, Byte* value, int valueLength);
int NativeInsert(WT_CURSOR* cursor, Byte* key, int keyLength);
int NativeInsert(WT_CURSOR* cursor, uint32_t key, Byte* value, int valueLength);
int NativeInsertIndex(WT_CURSOR* cursor, Byte* indexKey, int indexKeyLength, Byte* primaryKey, int primaryKeyLength);
int NativeRemove(WT_CURSOR* cursor, Byte* key, int keyLength);
int NativeSearch(WT_CURSOR* cursor, Byte* key, int keyLength);
int NativeSearch(WT_CURSOR* cursor, uint32_t key);
int NativeSearchNear(WT_CURSOR* cursor, Byte* key, int keyLength, int *exactp);
void NativeDecode(WT_CURSOR* cursor, const uint32_t* keys, int keysCount, Byte* values);