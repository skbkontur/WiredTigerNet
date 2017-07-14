#include "NativeTiger.h"
#include <sstream>

inline int min(int a, int b) {
	return a < b ? a : b;
}

NativeCursor::NativeCursor(WT_CURSOR* cursor) :
	cursor_(cursor),
	boundary_(nullptr),
	keyIsString_(strcmp(cursor_->key_format, "S") == 0) {
}

bool NativeCursor::IterationBegin(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive, NativeDirection newDirection, bool copyBoundary) {
	int exact;
	if (newDirection == Ascending) {
		if (left != nullptr) {
			if (!SearchNear(left, leftSize, &exact) || (exact == 0 && !leftInclusive || exact < 0) && !Next())
				return false;
		}
		else if (!Next())
			return false;
		SetBoundary(right, rightSize, rightInclusive, copyBoundary);
	}
	else {
		if (right != nullptr) {
			if (!SearchNear(right, rightSize, &exact) || (exact == 0 && !rightInclusive || exact > 0) && !Prev())
				return false;
		}
		else if (!Prev())
			return false;
		SetBoundary(left, leftSize, leftInclusive, copyBoundary);
	}
	direction_ = newDirection;
	return Within();
}

bool NativeCursor::Search(Byte* key, int keyLength) {
	SetKey(key, keyLength);
	int r = cursor_->search(cursor_);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->search");
	return true;
}

bool NativeCursor::SearchNear(Byte* data, int length, int* exact) {
	SetKey(data, length);
	int r = cursor_->search_near(cursor_, exact);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->search_near");
	return true;
}

bool NativeCursor::Next() {
	int r = cursor_->next(cursor_);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->next");
	return true;
}

bool NativeCursor::Prev() {
	int r = cursor_->prev(cursor_);
	if (r == WT_NOTFOUND)
		return false;
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->prev");
	return true;
}

bool NativeCursor::Within() {
	if (boundary_ == nullptr)
		return true;
	WT_ITEM item = { 0 };
	GetKey(&item);
	int result = memcmp(item.data, boundary_, min((int)item.size, boundarySize_));
	if (result == 0 && item.size != boundarySize_)
		result = item.size < boundarySize_ ? -1 : 1;
	if (result == 0)
		return boundaryInclusive_;
	return direction_ == Ascending ? result < 0 : result > 0;
}

void NativeCursor::SetBoundary(Byte* boundary, int boundarySize, bool boundaryInclusive, bool copyBoundary) {
	if (boundary == nullptr)
		return;
	boundarySize_ = boundarySize;
	boundaryInclusive_ = boundaryInclusive;
	ownsBoundary_ = copyBoundary;
	if (copyBoundary) {
		boundary_ = new Byte[boundarySize];
		memcpy(boundary_, boundary, boundarySize);
	}
	else
		boundary_ = boundary;
}

int NativeCursor::Reset() {
	int r = cursor_->reset(cursor_);
	if (r != 0)
		return r;
	if (ownsBoundary_)
		delete[] boundary_;
	boundary_ = nullptr;
	return 0;
}

NativeCursor::~NativeCursor() {
	if (boundary_ != nullptr) {
		if (ownsBoundary_)
			delete[] boundary_;
		boundary_ = nullptr;
	}
	if (cursor_ != nullptr) {
		cursor_->close(cursor_);
		cursor_ = nullptr;
	}
}

__int64 NativeCursor::GetTotalCount(Byte* left, int leftSize, bool leftInclusive, Byte* right, int rightSize, bool rightInclusive) {
	__int64 result = 0;
	if (IterationBegin(left, leftSize, leftInclusive, right, rightSize, rightInclusive, Ascending, false))
		do
		{
			result++;
		} while (IterationMove());
	return result;
}

bool NativeCursor::IterationMove() {
	bool moved = direction_ == Ascending ? Next() : Prev();
	return moved && Within();
}

void NativeCursor::GetKey(WT_ITEM* target) {
	if (keyIsString_) {
		const char* s;
		int r = cursor_->get_key(cursor_, &s);
		if (r != 0)
			throw NativeWiredTigerApiException(r, "cursor->get_key");
		target->data = s;
		target->size = strlen(s);
	}
	else {
		int r = cursor_->get_key(cursor_, target);
		if (r != 0)
			throw NativeWiredTigerApiException(r, "cursor->get_key");
	}
}

void NativeCursor::GetValue(WT_ITEM* target) {
	int r = cursor_->get_value(cursor_, target);
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->get_value");
}

void NativeCursor::Remove(Byte* key, int keyLength) {
	SetKey(key, keyLength);
	int r = cursor_->remove(cursor_);
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->remove");
}

void NativeCursor::Insert(Byte* key, int keyLength, Byte* value, int valueLength) {
	SetKey(key, keyLength);
	SetValue(value, valueLength);
	int r = cursor_->insert(cursor_);
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->insert");
}

void NativeCursor::Insert(Byte* key, int keyLength) {
	SetKey(key, keyLength);
	cursor_->set_value(cursor_);
	int r = cursor_->insert(cursor_);
	if (r != 0)
		throw NativeWiredTigerApiException(r, "cursor->insert");
}

void NativeCursor::SetKey(Byte* data, int length) {
	if (keyIsString_) {
		const char* s = (const char *)data;
		cursor_->set_key(cursor_, s);
	}
	else {
		WT_ITEM item = { 0 };
		item.data = (void*)data;
		item.size = length;
		cursor_->set_key(cursor_, &item);
	}
}

void NativeCursor::SetValue(Byte* data, int length) {
	WT_ITEM item = { 0 };
	item.data = (void*)data;
	item.size = length;
	cursor_->set_value(cursor_, &item);
}

NativeCursor* OpenNativeCursor(WT_SESSION* session, const char* name, const char* config) {
	WT_CURSOR* cursor;
	int r = session->open_cursor(session, name, nullptr, config, &cursor);
	if (r != 0)
	{
		std::string fullApiName = "session->open_cursor";
		fullApiName.append(", ");
		fullApiName.append(name);
		throw NativeWiredTigerApiException(r, fullApiName);
	}
	return new NativeCursor(cursor);
}