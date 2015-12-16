#include "NativeTiger.h"

enum Direction {
	Ascending,
	Descending
};

inline int min(int a, int b) {
	return a < b ? a : b;
}

class NativeCursor {
public:
	NativeCursor(WT_CURSOR* cursor) :cursor_(cursor), currentKey_(nullptr) {
	}

	long GetTotalCount(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive) {
		long result = 0;
		if (BeginIteration(left, leftSize, leftInclusive, right, rightSize, rightInclusive, Ascending))
			do
			{
				result++;
			} while (Move());
		return result;
	}

	bool BeginIteration(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive, Direction newDirection) {
		int exact;
		if (newDirection == Ascending) {
			if (left != nullptr) {
				SetKey(left, leftSize);
				if (!SearchNear(&exact) || (exact == 0 && !leftInclusive || exact < 0) && !Next())
					return false;
			}
			else if (!Next())
				return false;
			boundary_ = right;
			boundarySize_ = rightSize;
			boundaryInclusive_ = rightInclusive;
		}
		else {
			if (right != nullptr) {
				SetKey(right, rightSize);
				if (!SearchNear(&exact) || (exact == 0 && !rightInclusive || exact > 0) && !Prev())
					return false;
			}
			else if (!Prev())
				return false;
			boundary_ = left;
			boundarySize_ = leftSize;
			boundaryInclusive_ = leftInclusive;
		}
		GetKeyWithCopy(&currentKey_, &currentKeySize_);
		direction_ = newDirection;
		return Within();
	}

	bool Move() {
		bool moved = direction_ == Ascending ? Next() : Prev();
		if (!moved)
			return false;
		ReadKey();
		return Within();
	}

	~NativeCursor() {
		if (currentKey_ != nullptr) {
			delete[] currentKey_;
			currentKey_ = nullptr;
		}
		cursor_->reset(cursor_);
	}
private:
	WT_CURSOR* cursor_;
	Direction direction_;
	
	Byte* boundary_;
	int boundarySize_;
	bool boundaryInclusive_;

	Byte* currentKey_;
	int currentKeySize_;

	void SetKey(Byte* data, int length) {
		WT_ITEM item = { 0 };
		item.data = (void*)data;
		item.size = length;
		cursor_->set_key(cursor_, &item);
	}

	void GetKeyWithCopy(Byte** data, int* length) {
		WT_ITEM item = { 0 };
		int r = cursor_->get_key(cursor_, &item);
		if (r != 0)
			throw NativeTigerException("get_key failed with code " + r);
		*data = new Byte[item.size];
		*length = item.size;
		memcpy(*data, item.data, item.size);
	}

	void ReadKey() {
		if (currentKey_ != nullptr) {
			delete[] currentKey_;
			currentKey_ = nullptr;
		}
		GetKeyWithCopy(&currentKey_, &currentKeySize_);
	}

	bool SearchNear(int* exact) {
		int r = cursor_->search_near(cursor_, exact);
		if (r == WT_NOTFOUND)
			return false;
		if (r != 0)
			throw NativeTigerException("search_near failed with code " + r);
		return true;
	}

	bool Next() {
		int r = cursor_->next(cursor_);
		if (r == WT_NOTFOUND) 
			return false;
		if (r != 0) 
			throw NativeTigerException("next failed with code " + r);
		return true;
	}

	bool Prev() {
		int r = cursor_->prev(cursor_);
		if (r == WT_NOTFOUND) 
			return false;
		if (r != 0) 
			throw NativeTigerException("next failed with code " + r);
		return true;
	}

	bool Within() {
		if (boundary_ == nullptr)
			return true;
		int result = memcmp(currentKey_, boundary_, min(currentKeySize_, boundarySize_));
		if (result == 0)
			return boundaryInclusive_;
		return direction_ == Ascending ? result < 0 : result > 0;
	}
};

static void SetKey(WT_CURSOR* cursor, Byte* key, int keyLength) {
	WT_ITEM keyItem = { 0 };
	keyItem.data = (void*)key;
	keyItem.size = keyLength;
	cursor->set_key(cursor, &keyItem);
}

static void SetValue(WT_CURSOR* cursor, Byte* value, int valueLength) {
	WT_ITEM valueItem = { 0 };
	valueItem.data = (void*)value;
	valueItem.size = valueLength;
	cursor->set_value(cursor, &valueItem);
}

long NativeGetTotalCount(WT_CURSOR* cursor, Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive) {
	NativeCursor nativeCursor(cursor);
	return nativeCursor.GetTotalCount(left, leftSize, leftInclusive, right, rightSize, rightInclusive);
}

int NativeInsert(WT_CURSOR* cursor, Byte* key, int keyLength, Byte* value, int valueLength) {
	SetKey(cursor, key, keyLength);
	SetValue(cursor, value, valueLength);
	return cursor->insert(cursor);
}

int NativeRemove(WT_CURSOR* cursor, Byte* key, int keyLength) {
	SetKey(cursor, key, keyLength);
	return cursor->remove(cursor);
}

int NativeSearch(WT_CURSOR* cursor, Byte* key, int keyLength) {
	SetKey(cursor, key, keyLength);
	return cursor->search(cursor);
}

int NativeSearchNear(WT_CURSOR* cursor, Byte* key, int keyLength, int *exactp) {
	SetKey(cursor, key, keyLength);
	return cursor->search_near(cursor, exactp);
}