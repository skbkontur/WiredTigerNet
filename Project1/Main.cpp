#include <iostream>
#include <exception>
#include <sstream>
#include <Windows.h>
#include "wiredtiger.h"

using namespace std;

void check_code(int code) {
	if (code != 0) {
		ostringstream s;
		s << "shit " << code;
		throw logic_error(s.str());
	}
}

void create_new_database_and_die() {
	WT_CONNECTION* conn;
	check_code(wiredtiger_open("C:\\TestWTShit", NULL, "create,cache_size=100MB", &conn));

	WT_SESSION* session;
	check_code(conn->open_session(conn, NULL, NULL, &session));

	check_code(session->create(session, "table:documents", "key_format=u,value_format=u,block_compressor=snappy,columns=(key,scopeKey)"));

	WT_CURSOR* cursor;
	check_code(session->open_cursor(session, "table:documents", NULL, NULL, &cursor));
	char* key = new char[40];
	char* value = new char[1];
	value[0] = 76;
	for (int i = 0; i < 100; i++) {
		key[0] = i;
		WT_ITEM keyItem = { 0 };
		keyItem.data = (void*)key;
		keyItem.size = 40;
		cursor->set_key(cursor, keyItem);

		WT_ITEM valueItem = { 0 };
		valueItem.data = (void*)value;
		valueItem.size = 1;
		cursor->set_value(cursor, valueItem);

		check_code(cursor->insert(cursor));
	}
	session->checkpoint(session, NULL);
	TerminateProcess(GetCurrentProcess(), -1);
}

bool move_cursor_next(WT_CURSOR* cursor) {
	int apiCode = cursor->next(cursor);
	if (apiCode == WT_NOTFOUND)
		return false;
	check_code(apiCode);
	return true;
}

void open_existing_database() {
	WT_CONNECTION* conn;
	check_code(wiredtiger_open("C:\\TestWTShit", NULL, "cache_size=100MB", &conn));

	WT_SESSION* session;
	check_code(conn->open_session(conn, NULL, NULL, &session));

	WT_CURSOR* cursor;
	check_code(session->open_cursor(session, "table:documents", NULL, NULL, &cursor));
	char* key = new char[40];
	char* value = new char[1];
	int index = 0;
	while (move_cursor_next(cursor)) {
		WT_ITEM keyItem = { 0 };
		check_code(cursor->get_key(cursor, &keyItem));
		if (keyItem.size != 40)
			throw logic_error("shit2");
		char* keyData = (char*)keyItem.data;
		if (keyData[0] != index)
			throw logic_error("shit3");

		WT_ITEM valueItem = { 0 };
		check_code(cursor->get_value(cursor, &valueItem));
		if (valueItem.size != 1)
			throw logic_error("shit4");
		char* valueData = (char*)valueItem.data;
		if (valueData[0] != 76)
			throw logic_error("shit5");
		index++;
	}
	if (index != 100)
		throw logic_error("shit6");
	cout << "checkpoint ok!" << endl;
}

int main() {
	try
	{
		open_existing_database();
		//create_new_database_and_die();
		return 0;
	}
	catch (const logic_error& e) {
		cout << "error occurs " << e.what() << endl;
		return -1;
	}
}