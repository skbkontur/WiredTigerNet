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