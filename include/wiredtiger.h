/*-
 * Copyright (c) 2014-2015 MongoDB, Inc.
 * Copyright (c) 2008-2014 WiredTiger, Inc.
 *	All rights reserved.
 *
 * See the file LICENSE for redistribution information.
 */

#ifndef	__WIREDTIGER_H_
#define	__WIREDTIGER_H_

#if defined(__cplusplus)
extern "C" {
#endif

/*******************************************
 * Version information
 *******************************************/
#define	WIREDTIGER_VERSION_MAJOR	2
#define	WIREDTIGER_VERSION_MINOR	6
#define	WIREDTIGER_VERSION_PATCH	0
#define	WIREDTIGER_VERSION_STRING	"WiredTiger 2.6.0: (May 15, 2015)"

/*******************************************
 * Required includes
 *******************************************/

#include <sys/types.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>


/*******************************************
 * Portable type names
 *******************************************/
typedef int64_t wt_off_t;



#if defined(DOXYGEN) || defined(SWIG)
#define	__F(func) func
#else
#define	__F(func) (*func)
#endif

#ifdef SWIG
%{
#include <wiredtiger.h>
%}
#endif

/*!
 * @defgroup wt WiredTiger API
 * The functions, handles and methods applications use to access and manage
 * data with WiredTiger.
 *
 * @{
 */

/*******************************************
 * Public forward structure declarations
 *******************************************/
struct __wt_async_callback;
	typedef struct __wt_async_callback WT_ASYNC_CALLBACK;
struct __wt_async_op;	    typedef struct __wt_async_op WT_ASYNC_OP;
struct __wt_collator;	    typedef struct __wt_collator WT_COLLATOR;
struct __wt_compressor;	    typedef struct __wt_compressor WT_COMPRESSOR;
struct __wt_config_item;    typedef struct __wt_config_item WT_CONFIG_ITEM;
struct __wt_config_parser;
	typedef struct __wt_config_parser WT_CONFIG_PARSER;
struct __wt_connection;	    typedef struct __wt_connection WT_CONNECTION;
struct __wt_cursor;	    typedef struct __wt_cursor WT_CURSOR;
struct __wt_data_source;    typedef struct __wt_data_source WT_DATA_SOURCE;
struct __wt_encryptor;	    typedef struct __wt_encryptor WT_ENCRYPTOR;
struct __wt_event_handler;  typedef struct __wt_event_handler WT_EVENT_HANDLER;
struct __wt_extension_api;  typedef struct __wt_extension_api WT_EXTENSION_API;
struct __wt_extractor;	    typedef struct __wt_extractor WT_EXTRACTOR;
struct __wt_item;	    typedef struct __wt_item WT_ITEM;
struct __wt_lsn;	    typedef struct __wt_lsn WT_LSN;
struct __wt_session;	    typedef struct __wt_session WT_SESSION;

#if defined(SWIGJAVA)
#define	WT_HANDLE_NULLABLE(typename)	typename##_NULLABLE
#define	WT_HANDLE_CLOSED(typename)	typename##_CLOSED
typedef WT_CURSOR			WT_CURSOR_NULLABLE;
typedef WT_CURSOR			WT_CURSOR_CLOSED;
typedef WT_SESSION			WT_SESSION_CLOSED;
typedef WT_CONNECTION			WT_CONNECTION_CLOSED;
#elif !defined(DOXYGEN)
#define	WT_HANDLE_NULLABLE(typename)	typename
#define	WT_HANDLE_CLOSED(typename)	typename
#endif

/*!
 * A raw item of data to be managed, including a pointer to the data and a
 * length.
 *
 * WT_ITEM structures do not need to be cleared before use.
 */
struct __wt_item {
	/*!
	 * The memory reference of the data item.
	 *
	 * For items returned by a WT_CURSOR, the pointer is only valid until
	 * the next operation on that cursor.  Applications that need to keep
	 * an item across multiple cursor operations must make a copy.
	 */
	const void *data;

	/*!
	 * The number of bytes in the data item.
	 *
	 * The maximum length of a single column stored in a table is not fixed
	 * (as it partially depends on the underlying file configuration), but
	 * is always a small number of bytes less than 4GB.
	 */
	size_t size;

#ifndef DOXYGEN
#define	WT_ITEM_ALIGNED					0x00000001
#define	WT_ITEM_INUSE					0x00000002
	/* This appears in the middle of the struct to avoid padding. */
	/*! Object flags (internal use). */
	uint32_t flags;

	/*! Managed memory chunk (internal use). */
	void *mem;
	/*! Managed memory size (internal use). */
	size_t memsize;
#endif
};

/*
 * We rely on this structure being aligned at 64 bits by the compiler,
 * if we were paranoid we could add an unused field to ensure the padding
 * is correct.
 *
 * NOTE:  If you change the contents of this structure you must also update
 * the macros in log.h.
 */
/*!
 * A log sequence number, representing a position in the transaction log.
 */
struct __wt_lsn {
	uint32_t        file;           /*!< Log file number */
	wt_off_t        offset;         /*!< Log file offset */
};

/*!
 * The maximum packed size of a 64-bit integer.  The ::wiredtiger_struct_pack
 * function will pack single long integers into at most this many bytes.
 */
#define	WT_INTPACK64_MAXSIZE	((int)sizeof (int64_t) + 1)

/*!
 * The maximum packed size of a 32-bit integer.  The ::wiredtiger_struct_pack
 * function will pack single integers into at most this many bytes.
 */
#define	WT_INTPACK32_MAXSIZE	((int)sizeof (int32_t) + 1)

/*!
 * A WT_CURSOR handle is the interface to a cursor.
 *
 * Cursors allow data to be searched, iterated and modified, implementing the
 * CRUD (create, read, update and delete) operations.  Cursors are opened in
 * the context of a session.  If a transaction is started, cursors operate in
 * the context of the transaction until the transaction is resolved.
 *
 * Raw data is represented by key/value pairs of WT_ITEM structures, but
 * cursors can also provide access to fields within the key and value if the
 * formats are described in the WT_SESSION::create method.
 *
 * In the common case, a cursor is used to access records in a table.  However,
 * cursors can be used on subsets of tables (such as a single column or a
 * projection of multiple columns), as an interface to statistics, configuration
 * data or application-specific data sources.  See WT_SESSION::open_cursor for
 * more information.
 *
 * <b>Thread safety:</b> A WT_CURSOR handle is not usually shared between
 * threads, see @ref threads for more information.
 */
struct __wt_cursor {
	WT_SESSION *session;	/*!< The session handle for this cursor. */

	/*!
	 * The name of the data source for the cursor, matches the \c uri
	 * parameter to WT_SESSION::open_cursor used to open the cursor.
	 */
	const char *uri;

	/*!
	 * The format of the data packed into key items.  See @ref packing for
	 * details.  If not set, a default value of "u" is assumed, and
	 * applications must use WT_ITEM structures to manipulate untyped byte
	 * arrays.
	 */
	const char *key_format;

	/*!
	 * The format of the data packed into value items.  See @ref packing
	 * for details.  If not set, a default value of "u" is assumed, and
	 * applications must use WT_ITEM structures to manipulate untyped byte
	 * arrays.
	 */
	const char *value_format;

	/*!
	 * @name Data access
	 * @{
	 */
	/*!
	 * Get the key for the current record.
	 *
	 * @snippet ex_all.c Get the cursor's string key
	 *
	 * @snippet ex_all.c Get the cursor's record number key
	 *
	 * @param cursor the cursor handle
	 * @param ... pointers to hold key fields corresponding to
	 * WT_CURSOR::key_format.
	 * @errors
	 */
	int __F(get_key)(WT_CURSOR *cursor, ...);

	/*!
	 * Get the value for the current record.
	 *
	 * @snippet ex_all.c Get the cursor's string value
	 *
	 * @snippet ex_all.c Get the cursor's raw value
	 *
	 * @param cursor the cursor handle
	 * @param ... pointers to hold value fields corresponding to
	 * WT_CURSOR::value_format.
	 * @errors
	 */
	int __F(get_value)(WT_CURSOR *cursor, ...);

	/*!
	 * Set the key for the next operation.
	 *
	 * @snippet ex_all.c Set the cursor's string key
	 *
	 * @snippet ex_all.c Set the cursor's record number key
	 *
	 * @param cursor the cursor handle
	 * @param ... key fields corresponding to WT_CURSOR::key_format.
	 *
	 * If an error occurs during this operation, a flag will be set in the
	 * cursor, and the next operation to access the key will fail.  This
	 * simplifies error handling in applications.
	 */
	void __F(set_key)(WT_CURSOR *cursor, ...);

	/*!
	 * Set the value for the next operation.
	 *
	 * @snippet ex_all.c Set the cursor's string value
	 *
	 * @snippet ex_all.c Set the cursor's raw value
	 *
	 * @param cursor the cursor handle
	 * @param ... value fields corresponding to WT_CURSOR::value_format.
	 *
	 * If an error occurs during this operation, a flag will be set in the
	 * cursor, and the next operation to access the value will fail.  This
	 * simplifies error handling in applications.
	 */
	void __F(set_value)(WT_CURSOR *cursor, ...);
	/*! @} */

	/*!
	 * @name Cursor positioning
	 * @{
	 */
	/*!
	 * Return the ordering relationship between two cursors: both cursors
	 * must have the same data source and have valid keys. (When testing
	 * only for equality, WT_CURSOR::equals may be faster.)
	 *
	 * @snippet ex_all.c Cursor comparison
	 *
	 * @param cursor the cursor handle
	 * @param other another cursor handle
	 * @param comparep the status of the comparison: < 0 if
	 * <code>cursor</code> refers to a key that appears before
	 * <code>other</code>, 0 if the cursors refer to the same key,
	 * and > 0 if <code>cursor</code> refers to a key that appears after
	 * <code>other</code>.
	 * @errors
	 */
	int __F(compare)(WT_CURSOR *cursor, WT_CURSOR *other, int *comparep);

	/*!
	 * Return the ordering relationship between two cursors, testing only
	 * for equality: both cursors must have the same data source and have
	 * valid keys.
	 *
	 * @snippet ex_all.c Cursor equality
	 *
	 * @param cursor the cursor handle
	 * @param other another cursor handle
	 * @param[out] equalp the status of the comparison: 1 if the cursors
	 * refer to the same key, otherwise 0.
	 * @errors
	 */
	int __F(equals)(WT_CURSOR *cursor, WT_CURSOR *other, int *equalp);

	/*!
	 * Return the next record.
	 *
	 * @snippet ex_all.c Return the next record
	 *
	 * @param cursor the cursor handle
	 * @errors
	 */
	int __F(next)(WT_CURSOR *cursor);

	/*!
	 * Return the previous record.
	 *
	 * @snippet ex_all.c Return the previous record
	 *
	 * @param cursor the cursor handle
	 * @errors
	 */
	int __F(prev)(WT_CURSOR *cursor);

	/*!
	 * Reset the cursor. Any resources held by the cursor are released,
	 * and the cursor's key and position are no longer valid. Subsequent
	 * iterations with WT_CURSOR::next will move to the first record, or
	 * with WT_CURSOR::prev will move to the last record.
	 *
	 * In the case of a statistics cursor, resetting the cursor refreshes
	 * the statistics information returned.
	 *
	 * @snippet ex_all.c Reset the cursor
	 *
	 * @param cursor the cursor handle
	 * @errors
	 */
	int __F(reset)(WT_CURSOR *cursor);

	/*!
	 * Return the record matching the key. The key must first be set.
	 *
	 * @snippet ex_all.c Search for an exact match
	 *
	 * On success, the cursor ends positioned at the returned record; to
	 * minimize cursor resources, the WT_CURSOR::reset method should be
	 * called as soon as the record has been retrieved and the cursor no
	 * longer needs that position.
	 *
	 * @param cursor the cursor handle
	 * @errors
	 */
	int __F(search)(WT_CURSOR *cursor);

	/*!
	 * Return the record matching the key if it exists, or an adjacent
	 * record.  An adjacent record is either the smallest record larger
	 * than the key or the largest record smaller than the key (in other
	 * words, a logically adjacent key).
	 *
	 * The key must first be set.
	 *
	 * An example of a search for an exact or adjacent match:
	 *
	 * @snippet ex_all.c Search for an exact or adjacent match
	 *
	 * An example of a forward scan through the table, where all keys
	 * greater than or equal to a specified prefix are included in the
	 * scan:
	 *
	 * @snippet ex_all.c Forward scan greater than or equal
	 *
	 * An example of a backward scan through the table, where all keys
	 * less than a specified prefix are included in the scan:
	 *
	 * @snippet ex_all.c Backward scan less than
	 *
	 * On success, the cursor ends positioned at the returned record; to
	 * minimize cursor resources, the WT_CURSOR::reset method should be
	 * called as soon as the record has been retrieved and the cursor no
	 * longer needs that position.
	 *
	 * @param cursor the cursor handle
	 * @param exactp the status of the search: 0 if an exact match is
	 * found, < 0 if a smaller key is returned, > 0 if a larger key is
	 * returned
	 * @errors
	 */
	int __F(search_near)(WT_CURSOR *cursor, int *exactp);
	/*! @} */

	/*!
	 * @name Data modification
	 * @{
	 */
	/*!
	 * Insert a record and optionally update an existing record.
	 *
	 * If the cursor was configured with "overwrite=true" (the default),
	 * both the key and value must be set; if the record already exists,
	 * the key's value will be updated, otherwise, the record will be
	 * inserted.
	 *
	 * @snippet ex_all.c Insert a new record or overwrite an existing record
	 *
	 * If the cursor was not configured with "overwrite=true", both the key
	 * and value must be set and the record must not already exist; the
	 * record will be inserted.
	 *
	 * @snippet ex_all.c Insert a new record and fail if the record exists
	 *
	 * If a cursor with record number keys was configured with
	 * "append=true" (not the default), the value must be set; a new record
	 * will be appended and the record number set as the cursor key value.
	 *
	 * @snippet ex_all.c Insert a new record and assign a record number
	 *
	 * The cursor ends with no position, and a subsequent call to the
	 * WT_CURSOR::next (WT_CURSOR::prev) method will iterate from the
	 * beginning (end) of the table.
	 *
	 * Inserting a new record after the current maximum record in a
	 * fixed-length bit field column-store (that is, a store with an
	 * 'r' type key and 't' type value) may implicitly create the missing
	 * records as records with a value of 0.
	 *
	 * When loading a large amount of data into a new object, using
	 * a cursor with the \c bulk configuration string enabled and
	 * loading the data in sorted order will be much faster than doing
	 * out-of-order inserts.  See @ref tune_bulk_load for more information.
	 *
	 * The maximum length of a single column stored in a table is not fixed
	 * (as it partially depends on the underlying file configuration), but
	 * is always a small number of bytes less than 4GB.
	 *
	 * @param cursor the cursor handle
	 * @errors
	 * In particular, if \c overwrite is not configured and a record with
	 * the specified key already exists, ::WT_DUPLICATE_KEY is returned.
	 */
	int __F(insert)(WT_CURSOR *cursor);

	/*!
	 * Update a record and optionally insert an existing record.
	 *
	 * If the cursor was configured with "overwrite=true" (the default),
	 * both the key and value must be set; if the record already exists, the
	 * key's value will be updated, otherwise, the record will be inserted.
	 *
	 * @snippet ex_all.c Update an existing record or insert a new record
	 *
	 * If the cursor was not configured with "overwrite=true", both the key
	 * and value must be set and the record must already exist; the
	 * record will be updated.
	 *
	 * @snippet ex_all.c Update an existing record and fail if DNE
	 *
	 * On success, the cursor ends positioned at the modified record; to
	 * minimize cursor resources, the WT_CURSOR::reset method should be
	 * called as soon as the cursor no longer needs that position.
	 *
	 * The maximum length of a single column stored in a table is not fixed
	 * (as it partially depends on the underlying file configuration), but
	 * is always a small number of bytes less than 4GB.
	 *
	 * @param cursor the cursor handle
	 * @errors
	 * In particular, if \c overwrite is not configured and no record with
	 * the specified key exists, ::WT_NOTFOUND is returned.
	 */
	int __F(update)(WT_CURSOR *cursor);

	/*!
	 * Remove a record.
	 *
	 * If the cursor was configured with "overwrite=true" (the default),
	 * the key must be set; the key's record will be removed if it exists,
	 * no error will be returned if the record does not exist.
	 *
	 * @snippet ex_all.c Remove a record
	 *
	 * If the cursor was not configured with "overwrite=true", the key must
	 * be set and the key's record must exist; the record will be removed.
	 *
	 * @snippet ex_all.c Remove a record and fail if DNE
	 *
	 * Removing a record in a fixed-length bit field column-store
	 * (that is, a store with an 'r' type key and 't' type value) is
	 * identical to setting the record's value to 0.
	 *
	 * On success, the cursor ends positioned at the removed record; to
	 * minimize cursor resources, the WT_CURSOR::reset method should be
	 * called as soon as the cursor no longer needs that position.
	 *
	 * @param cursor the cursor handle
	 * @errors
	 * In particular, if \c overwrite is not configured and no record with
	 * the specified key exists, ::WT_NOTFOUND is returned.
	 */
	int __F(remove)(WT_CURSOR *cursor);
	/*! @} */

	/*!
	 * Close the cursor.
	 *
	 * This releases the resources associated with the cursor handle.
	 * Cursors are closed implicitly by ending the enclosing connection or
	 * closing the session in which they were opened.
	 *
	 * @snippet ex_all.c Close the cursor
	 *
	 * @param cursor the cursor handle
	 * @errors
	 */
	int __F(close)(WT_HANDLE_CLOSED(WT_CURSOR) *cursor);

	/*!
	 * Reconfigure the cursor.
	 *
	 * The cursor is reset.
	 *
	 * @snippet ex_all.c Reconfigure a cursor
	 *
	 * @param cursor the cursor handle
	 * @configstart{WT_CURSOR.reconfigure, see dist/api_data.py}
	 * @config{append, append the value as a new record\, creating a new
	 * record number key; valid only for cursors with record number keys., a
	 * boolean flag; default \c false.}
	 * @config{overwrite, configures whether the cursor's insert\, update
	 * and remove methods check the existing state of the record.  If \c
	 * overwrite is \c false\, WT_CURSOR::insert fails with
	 * ::WT_DUPLICATE_KEY if the record exists\, WT_CURSOR::update and
	 * WT_CURSOR::remove fail with ::WT_NOTFOUND if the record does not
	 * exist., a boolean flag; default \c true.}
	 * @configend
	 * @errors
	 */
	int __F(reconfigure)(WT_CURSOR *cursor, const char *config);

	/*
	 * Protected fields, only to be used by cursor implementations.
	 */
#if !defined(SWIG) && !defined(DOXYGEN)
	/*
	 * !!!
	 * Explicit representations of structures from queue.h.
	 * TAILQ_ENTRY(wt_cursor) q;
	 */
	struct {
		WT_CURSOR *tqe_next;
		WT_CURSOR **tqe_prev;
	} q;				/* Linked list of WT_CURSORs. */

	uint64_t recno;			/* Record number, normal and raw mode */
	uint8_t raw_recno_buf[WT_INTPACK64_MAXSIZE];

	void	*json_private;		/* JSON specific storage */
	void	*lang_private;		/* Language specific private storage */

	WT_ITEM key, value;
	int saved_err;			/* Saved error in set_{key,value}. */
	/*
	 * URI used internally, may differ from the URI provided by the
	 * user on open.
	 */
	const char *internal_uri;

#define	WT_CURSTD_APPEND	0x0001
#define	WT_CURSTD_BULK		0x0002
#define	WT_CURSTD_DUMP_HEX	0x0004
#define	WT_CURSTD_DUMP_JSON	0x0008
#define	WT_CURSTD_DUMP_PRINT	0x0010
#define	WT_CURSTD_KEY_EXT	0x0020	/* Key points out of the tree. */
#define	WT_CURSTD_KEY_INT	0x0040	/* Key points into the tree. */
#define	WT_CURSTD_KEY_SET	(WT_CURSTD_KEY_EXT | WT_CURSTD_KEY_INT)
#define	WT_CURSTD_OPEN		0x0080
#define	WT_CURSTD_OVERWRITE	0x0100
#define	WT_CURSTD_RAW		0x0200
#define	WT_CURSTD_VALUE_EXT	0x0400	/* Value points out of the tree. */
#define	WT_CURSTD_VALUE_INT	0x0800	/* Value points into the tree. */
#define	WT_CURSTD_VALUE_SET	(WT_CURSTD_VALUE_EXT | WT_CURSTD_VALUE_INT)
	uint32_t flags;
#endif
};

/*! Asynchronous operation types. */
typedef enum {
	WT_AOP_NONE=0,	/*!< No operation type set */
	WT_AOP_COMPACT, /*!< WT_ASYNC_OP::compact */
	WT_AOP_INSERT,	/*!< WT_ASYNC_OP::insert */
	WT_AOP_REMOVE,	/*!< WT_ASYNC_OP::remove */
	WT_AOP_SEARCH,	/*!< WT_ASYNC_OP::search */
	WT_AOP_UPDATE	/*!< WT_ASYNC_OP::update */
} WT_ASYNC_OPTYPE;

/*!
 * A WT_ASYNC_OP handle is the interface to an asynchronous operation.
 *
 * An asynchronous operation describes a data manipulation to be performed
 * asynchronously by a WiredTiger worker thread.  These operations implement
 * the CRUD (create, read, update and delete) operations.  Each operation
 * is a self-contained work unit.  The operation will be performed in the
 * context of the worker thread's session.  Each operation is performed
 * within the context of a transaction.  The application is notified of its
 * completion with a callback.  The transaction is resolved once the callback
 * returns.
 *
 * The table referenced in an operation must already exist.
 *
 * Raw data is represented by key/value pairs of WT_ITEM structures, but
 * operations can also provide access to fields within the key and value if
 * the formats are described in the WT_SESSION::create method.
 *
 * <b>Thread safety:</b> A WT_ASYNC_OP handle may not be shared between
 * threads, see @ref threads for more information.
 */
struct __wt_async_op {
	/*! The connection for this operation. */
	WT_CONNECTION *connection;

	/*!
	 * The format of the data packed into key items.  See @ref packing for
	 * details.  If not set, a default value of "u" is assumed, and
	 * applications must use WT_ITEM structures to manipulate untyped byte
	 * arrays.
	 */
	const char *key_format;

	/*!
	 * The format of the data packed into value items.  See @ref packing
	 * for details.  If not set, a default value of "u" is assumed, and
	 * applications must use WT_ITEM structures to manipulate untyped byte
	 * arrays.
	 */
	const char *value_format;

	/*
	 * Don't expose app_private to non-C language bindings - they have
	 * their own way to attach data to an operation.
	 */
#if !defined(SWIG)
	/*!
	 * A location for applications to store information that will be
	 * available in the callback from an async operation.
	 */
	void *app_private;
#endif

	/*!
	 * @name Data access
	 * @{
	 */
	/*!
	 * Invoke the underlying WT_CURSOR::get_key method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 * @returns as described for WT_CURSOR::get_key
	 */
	int __F(get_key)(WT_ASYNC_OP *op, ...);

	/*!
	 * Invoke the underlying WT_CURSOR::get_value method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 * @returns as described for WT_CURSOR::get_value
	 */
	int __F(get_value)(WT_ASYNC_OP *op, ...);

	/*!
	 * Invoke the underlying WT_CURSOR::set_key method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 */
	void __F(set_key)(WT_ASYNC_OP *op, ...);

	/*!
	 * Invoke the underlying WT_CURSOR::set_value method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 */
	void __F(set_value)(WT_ASYNC_OP *op, ...);
	/*! @} */

	/*!
	 * @name Positioning
	 * @{
	 */
	/*!
	 * Invoke the underlying WT_CURSOR::search method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 * @returns via the callback as described for WT_CURSOR::search
	 */
	int __F(search)(WT_ASYNC_OP *op);
	/*! @} */

	/*!
	 * @name Data modification
	 * @{
	 */
	/*!
	 * Invoke the underlying WT_CURSOR::insert method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 * @returns via the callback as described for WT_CURSOR::insert
	 */
	int __F(insert)(WT_ASYNC_OP *op);

	/*!
	 * Invoke the underlying WT_CURSOR::update method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 * @returns via the callback as described for WT_CURSOR::update
	 */
	int __F(update)(WT_ASYNC_OP *op);

	/*!
	 * Invoke the underlying WT_CURSOR::remove method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 * @returns via the callback as described for WT_CURSOR::remove
	 */
	int __F(remove)(WT_ASYNC_OP *op);
	/*! @} */

	/*!
	 * @name Table operations
	 * @{
	 */
	/*!
	 * Invoke the underlying WT_SESSION::compact method; see that method
	 * for configuration, return and error values.
	 *
	 * @param op the operation handle
	 * @returns via the callback as described for WT_SESSION::compact
	 */
	int __F(compact)(WT_ASYNC_OP *op);
	/*! @} */

	/*!
	 * Get the unique identifier for this operation.
	 *
	 * @snippet ex_async.c async get identifier
	 *
	 * @param op the operation handle
	 * @returns the id of the operation
	 */
	uint64_t __F(get_id)(WT_ASYNC_OP *op);

	/*!
	 * Get the type for this operation.
	 *
	 * @snippet ex_async.c async get type
	 *
	 * @param op the operation handle
	 * @returns the ::WT_ASYNC_OPTYPE of the operation
	 */
	WT_ASYNC_OPTYPE __F(get_type)(WT_ASYNC_OP *op);

	/*
	 * Protected fields, only to be used by internal implementation.
	 * Everything we need for maintaining the key/value is part of
	 * a cursor.  So, include one here so that we can use the cursor
	 * functions to manage them.
	 */
#if !defined(SWIG) && !defined(DOXYGEN)
	WT_CURSOR	c;
#endif
};

/*!
 * All data operations are performed in the context of a WT_SESSION.  This
 * encapsulates the thread and transactional context of the operation.
 *
 * <b>Thread safety:</b> A WT_SESSION handle is not usually shared between
 * threads, see @ref threads for more information.
 */
struct __wt_session {
	/*! The connection for this session. */
	WT_CONNECTION *connection;

	/*
	 * Don't expose app_private to non-C language bindings - they have
	 * their own way to attach data to an operation.
	 */
#if !defined(SWIG)
	/*!
	 * A location for applications to store information that will be
	 * available in callbacks taking a WT_SESSION handle.
	 */
	void *app_private;
#endif

	/*!
	 * Close the session handle.
	 *
	 * This will release the resources associated with the session handle,
	 * including rolling back any active transactions and closing any
	 * cursors that remain open in the session.
	 *
	 * @snippet ex_all.c Close a session
	 *
	 * @param session the session handle
	 * @configempty{WT_SESSION.close, see dist/api_data.py}
	 * @errors
	 */
	int __F(close)(WT_HANDLE_CLOSED(WT_SESSION) *session,
	    const char *config);

	/*!
	 * Reconfigure a session handle.
	 *
	 * @snippet ex_all.c Reconfigure a session
	 *
	 * WT_SESSION::reconfigure will fail if a transaction is in progress
	 * in the session.
	 *
	 * All cursors are reset.
	 *
	 * @param session the session handle
	 * @configstart{WT_SESSION.reconfigure, see dist/api_data.py}
	 * @config{isolation, the default isolation level for operations in this
	 * session., a string\, chosen from the following options: \c
	 * "read-uncommitted"\, \c "read-committed"\, \c "snapshot"; default \c
	 * read-committed.}
	 * @configend
	 * @errors
	 */
	int __F(reconfigure)(WT_SESSION *session, const char *config);

	/*!
	 * Return information about an error as a string.
	 *
	 * @snippet ex_all.c Display an error thread safe
	 *
	 * @param session the session handle
	 * @param error a return value from a WiredTiger function
	 * @returns a string representation of the error
	 */
	const char *__F(strerror)(WT_SESSION *session, int error);

	/*!
	 * @name Cursor handles
	 * @{
	 */

	/*!
	 * Open a new cursor on a data source or duplicate an existing cursor.
	 *
	 * @snippet ex_all.c Open a cursor
	 *
	 * An existing cursor can be duplicated by passing it as the \c to_dup
	 * parameter and setting the \c uri parameter to \c NULL:
	 *
	 * @snippet ex_all.c Duplicate a cursor
	 *
	 * Cursors being duplicated must have a key set, and successfully
	 * duplicated cursors are positioned at the same place in the data
	 * source as the original.
	 *
	 * Cursor handles should be discarded by calling WT_CURSOR::close.
	 *
	 * Cursors capable of supporting transactional operations operate in the
	 * context of the current transaction, if any.
	 *
	 * WT_SESSION::rollback_transaction implicitly resets all cursors.
	 *
	 * Cursors are relatively light-weight objects but may hold references
	 * to heavier-weight objects; applications should re-use cursors when
	 * possible, but instantiating new cursors is not so expensive that
	 * applications need to cache cursors at all cost.
	 *
	 * @param session the session handle
	 * @param uri the data source on which the cursor operates; cursors
	 *  are usually opened on tables, however, cursors can be opened on
	 *  any data source, regardless of whether it is ultimately stored
	 *  in a table.  Some cursor types may have limited functionality
	 *  (for example, they may be read-only or not support transactional
	 *  updates).  See @ref data_sources for more information.
	 *  <br>
	 *  @copydoc doc_cursor_types
	 * @param to_dup a cursor to duplicate
	 * @configstart{WT_SESSION.open_cursor, see dist/api_data.py}
	 * @config{append, append the value as a new record\, creating a new
	 * record number key; valid only for cursors with record number keys., a
	 * boolean flag; default \c false.}
	 * @config{bulk, configure the cursor for bulk-loading\, a fast\,
	 * initial load path (see @ref tune_bulk_load for more information).
	 * Bulk-load may only be used for newly created objects and cursors
	 * configured for bulk-load only support the WT_CURSOR::insert and
	 * WT_CURSOR::close methods.  When bulk-loading row-store objects\, keys
	 * must be loaded in sorted order.  The value is usually a true/false
	 * flag; when bulk-loading fixed-length column store objects\, the
	 * special value \c bitmap allows chunks of a memory resident bitmap to
	 * be loaded directly into a file by passing a \c WT_ITEM to
	 * WT_CURSOR::set_value where the \c size field indicates the number of
	 * records in the bitmap (as specified by the object's \c value_format
	 * configuration). Bulk-loaded bitmap values must end on a byte boundary
	 * relative to the bit count (except for the last set of values
	 * loaded)., a string; default \c false.}
	 * @config{checkpoint, the name of a checkpoint to open (the reserved
	 * name "WiredTigerCheckpoint" opens the most recent internal checkpoint
	 * taken for the object). The cursor does not support data
	 * modification., a string; default empty.}
	 * @config{dump, configure the cursor for dump format inputs and
	 * outputs: "hex" selects a simple hexadecimal format\, "json" selects a
	 * JSON format with each record formatted as fields named by column
	 * names if available\, and "print" selects a format where only
	 * non-printing characters are hexadecimal encoded.  These formats are
	 * compatible with the @ref util_dump and @ref util_load commands., a
	 * string\, chosen from the following options: \c "hex"\, \c "json"\, \c
	 * "print"; default empty.}
	 * @config{next_random, configure the cursor to return a pseudo-random
	 * record from the object; valid only for row-store cursors.  Cursors
	 * configured with \c next_random=true only support the WT_CURSOR::next
	 * and WT_CURSOR::close methods.  See @ref cursor_random for details., a
	 * boolean flag; default \c false.}
	 * @config{overwrite, configures whether the cursor's insert\, update
	 * and remove methods check the existing state of the record.  If \c
	 * overwrite is \c false\, WT_CURSOR::insert fails with
	 * ::WT_DUPLICATE_KEY if the record exists\, WT_CURSOR::update and
	 * WT_CURSOR::remove fail with ::WT_NOTFOUND if the record does not
	 * exist., a boolean flag; default \c true.}
	 * @config{raw, ignore the encodings for the key and value\, manage data
	 * as if the formats were \c "u". See @ref cursor_raw for details., a
	 * boolean flag; default \c false.}
	 * @config{readonly, only query operations are supported by this cursor.
	 * An error is returned if a modification is attempted using the cursor.
	 * The default is false for all cursor types except for log and metadata
	 * cursors., a boolean flag; default \c false.}
	 * @config{statistics, Specify the statistics to be gathered.  Choosing
	 * "all" gathers statistics regardless of cost and may include
	 * traversing on-disk files; "fast" gathers a subset of relatively
	 * inexpensive statistics.  The selection must agree with the database
	 * \c statistics configuration specified to ::wiredtiger_open or
	 * WT_CONNECTION::reconfigure.  For example\, "all" or "fast" can be
	 * configured when the database is configured with "all"\, but the
	 * cursor open will fail if "all" is specified when the database is
	 * configured with "fast"\, and the cursor open will fail in all cases
	 * when the database is configured with "none". If \c statistics is not
	 * configured\, the default configuration is the database configuration.
	 * The "clear" configuration resets statistics after gathering them\,
	 * where appropriate (for example\, a cache size statistic is not
	 * cleared\, while the count of cursor insert operations will be
	 * cleared). See @ref statistics for more information., a list\, with
	 * values chosen from the following options: \c "all"\, \c "fast"\, \c
	 * "clear"; default empty.}
	 * @config{target, if non-empty\, backup the list of objects; valid only
	 * for a backup data source., a list of strings; default empty.}
	 * @configend
	 * @param[out] cursorp a pointer to the newly opened cursor
	 * @errors
	 */
	int __F(open_cursor)(WT_SESSION *session,
	    const char *uri, WT_HANDLE_NULLABLE(WT_CURSOR) *to_dup,
	    const char *config, WT_CURSOR **cursorp);
	/*! @} */

	/*!
	 * @name Table operations
	 * @{
	 */
	/*!
	 * Create a table, column group, index or file.
	 *
	 * @snippet ex_all.c Create a table
	 *
	 * @param session the session handle
	 * @param name the URI of the object to create, such as
	 * \c "table:stock". For a description of URI formats
	 * see @ref data_sources.
	 * @configstart{WT_SESSION.create, see dist/api_data.py}
	 * @config{allocation_size, the file unit allocation size\, in bytes\,
	 * must a power-of-two; smaller values decrease the file space required
	 * by overflow items\, and the default value of 4KB is a good choice
	 * absent requirements from the operating system or storage device., an
	 * integer between 512B and 128MB; default \c 4KB.}
	 * @config{app_metadata, application-owned metadata for this object., a
	 * string; default empty.}
	 * @config{block_allocation, configure block allocation.  Permitted
	 * values are \c "first" or \c "best"; the \c "first" configuration uses
	 * a first-available algorithm during block allocation\, the \c "best"
	 * configuration uses a best-fit algorithm., a string\, chosen from the
	 * following options: \c "first"\, \c "best"; default \c best.}
	 * @config{block_compressor, configure a compressor for file blocks.
	 * Permitted values are \c "none" or custom compression engine name
	 * created with WT_CONNECTION::add_compressor.  If WiredTiger has
	 * builtin support for \c "bzip2"\, \c "snappy"\, \c "lz4" or \c "zlib"
	 * compression\, these names are also available.  See @ref compression
	 * for more information., a string; default \c none.}
	 * @config{cache_resident, do not ever evict the object's pages from
	 * cache.  Not compatible with LSM tables; see @ref
	 * tuning_cache_resident for more information., a boolean flag; default
	 * \c false.}
	 * @config{checksum, configure block checksums; permitted values are
	 * <code>on</code> (checksum all blocks)\, <code>off</code> (checksum no
	 * blocks) and <code>uncompresssed</code> (checksum only blocks which
	 * are not compressed for any reason). The \c uncompressed setting is
	 * for applications which can rely on decompression to fail if a block
	 * has been corrupted., a string\, chosen from the following options: \c
	 * "on"\, \c "off"\, \c "uncompressed"; default \c uncompressed.}
	 * @config{colgroups, comma-separated list of names of column groups.
	 * Each column group is stored separately\, keyed by the primary key of
	 * the table.  If no column groups are specified\, all columns are
	 * stored together in a single file.  All value columns in the table
	 * must appear in at least one column group.  Each column group must be
	 * created with a separate call to WT_SESSION::create., a list of
	 * strings; default empty.}
	 * @config{collator, configure custom collation for keys.  Permitted
	 * values are \c "none" or a custom collator name created with
	 * WT_CONNECTION::add_collator., a string; default \c none.}
	 * @config{columns, list of the column names.  Comma-separated list of
	 * the form <code>(column[\,...])</code>. For tables\, the number of
	 * entries must match the total number of values in \c key_format and \c
	 * value_format.  For colgroups and indices\, all column names must
	 * appear in the list of columns for the table., a list of strings;
	 * default empty.}
	 * @config{dictionary, the maximum number of unique values remembered in
	 * the Btree row-store leaf page value dictionary; see @ref
	 * file_formats_compression for more information., an integer greater
	 * than or equal to 0; default \c 0.}
	 * @config{encryption = (, configure an encryptor for file blocks.  When
	 * a table is created\, its encryptor is not implicitly used for any
	 * related indices or column groups., a set of related configuration
	 * options defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;keyid, An
	 * identifier that identifies a unique instance of the encryptor.  It is
	 * stored in clear text\, and thus is available when the wiredtiger
	 * database is reopened.  On the first use of a (name\, keyid)
	 * combination\, the WT_ENCRYPTOR::customize function is called with the
	 * keyid as an argument., a string; default empty.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, Permitted values are \c "none"
	 * or custom encryption engine name created with
	 * WT_CONNECTION::add_encryptor.  See @ref encryption for more
	 * information., a string; default \c none.}
	 * @config{ ),,}
	 * @config{exclusive, fail if the object exists.  When false (the
	 * default)\, if the object exists\, check that its settings match the
	 * specified configuration., a boolean flag; default \c false.}
	 * @config{extractor, configure custom extractor for indices.  Permitted
	 * values are \c "none" or an extractor name created with
	 * WT_CONNECTION::add_extractor., a string; default \c none.}
	 * @config{format, the file format., a string\, chosen from the
	 * following options: \c "btree"; default \c btree.}
	 * @config{huffman_key, configure Huffman encoding for keys.  Permitted
	 * values are \c "none"\, \c "english"\, \c "utf8<file>" or \c
	 * "utf16<file>". See @ref huffman for more information., a string;
	 * default \c none.}
	 * @config{huffman_value, configure Huffman encoding for values.
	 * Permitted values are \c "none"\, \c "english"\, \c "utf8<file>" or \c
	 * "utf16<file>". See @ref huffman for more information., a string;
	 * default \c none.}
	 * @config{immutable, configure the index to be immutable - that is an
	 * index is not changed by any update to a record in the table., a
	 * boolean flag; default \c false.}
	 * @config{internal_key_max, the largest key stored in an internal
	 * node\, in bytes.  If set\, keys larger than the specified size are
	 * stored as overflow items (which may require additional I/O to
	 * access). The default and the maximum allowed value are both one-tenth
	 * the size of a newly split internal page., an integer greater than or
	 * equal to 0; default \c 0.}
	 * @config{internal_key_truncate, configure internal key truncation\,
	 * discarding unnecessary trailing bytes on internal keys (ignored for
	 * custom collators)., a boolean flag; default \c true.}
	 * @config{internal_page_max, the maximum page size for internal nodes\,
	 * in bytes; the size must be a multiple of the allocation size and is
	 * significant for applications wanting to avoid excessive L2 cache
	 * misses while searching the tree.  The page maximum is the bytes of
	 * uncompressed data\, that is\, the limit is applied before any block
	 * compression is done., an integer between 512B and 512MB; default \c
	 * 4KB.}
	 * @config{key_format, the format of the data packed into key items.
	 * See @ref schema_format_types for details.  By default\, the
	 * key_format is \c 'u' and applications use WT_ITEM structures to
	 * manipulate raw byte arrays.  By default\, records are stored in
	 * row-store files: keys of type \c 'r' are record numbers and records
	 * referenced by record number are stored in column-store files., a
	 * format string; default \c u.}
	 * @config{leaf_key_max, the largest key stored in a leaf node\, in
	 * bytes.  If set\, keys larger than the specified size are stored as
	 * overflow items (which may require additional I/O to access). The
	 * default value is one-tenth the size of a newly split leaf page., an
	 * integer greater than or equal to 0; default \c 0.}
	 * @config{leaf_page_max, the maximum page size for leaf nodes\, in
	 * bytes; the size must be a multiple of the allocation size\, and is
	 * significant for applications wanting to maximize sequential data
	 * transfer from a storage device.  The page maximum is the bytes of
	 * uncompressed data\, that is\, the limit is applied before any block
	 * compression is done., an integer between 512B and 512MB; default \c
	 * 32KB.}
	 * @config{leaf_value_max, the largest value stored in a leaf node\, in
	 * bytes.  If set\, values larger than the specified size are stored as
	 * overflow items (which may require additional I/O to access). If the
	 * size is larger than the maximum leaf page size\, the page size is
	 * temporarily ignored when large values are written.  The default is
	 * one-half the size of a newly split leaf page., an integer greater
	 * than or equal to 0; default \c 0.}
	 * @config{lsm = (, options only relevant for LSM data sources., a set
	 * of related configuration options defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;auto_throttle, Throttle inserts into
	 * LSM trees if flushing to disk isn't keeping up., a boolean flag;
	 * default \c true.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom, create bloom
	 * filters on LSM tree chunks as they are merged., a boolean flag;
	 * default \c true.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_bit_count,
	 * the number of bits used per item for LSM bloom filters., an integer
	 * between 2 and 1000; default \c 16.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_config, config string used when
	 * creating Bloom filter files\, passed to WT_SESSION::create., a
	 * string; default empty.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_hash_count, the number of hash
	 * values per item used for LSM bloom filters., an integer between 2 and
	 * 100; default \c 8.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;bloom_oldest,
	 * create a bloom filter on the oldest LSM tree chunk.  Only supported
	 * if bloom filters are enabled., a boolean flag; default \c false.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_count_limit, the maximum number
	 * of chunks to allow in an LSM tree.  This option automatically times
	 * out old data.  As new chunks are added old chunks will be removed.
	 * Enabling this option disables LSM background merges., an integer;
	 * default \c 0.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_max, the maximum
	 * size a single chunk can be.  Chunks larger than this size are not
	 * considered for further merges.  This is a soft limit\, and chunks
	 * larger than this value can be created.  Must be larger than
	 * chunk_size., an integer between 100MB and 10TB; default \c 5GB.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk_size, the maximum size of the
	 * in-memory chunk of an LSM tree.  This limit is soft - it is possible
	 * for chunks to be temporarily larger than this value.  This overrides
	 * the \c memory_page_max setting., an integer between 512K and 500MB;
	 * default \c 10MB.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge_max, the
	 * maximum number of chunks to include in a merge operation., an integer
	 * between 2 and 100; default \c 15.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge_min, the minimum number of
	 * chunks to include in a merge operation.  If set to 0 or 1 half the
	 * value of merge_max is used., an integer no more than 100; default \c
	 * 0.}
	 * @config{ ),,}
	 * @config{memory_page_max, the maximum size a page can grow to in
	 * memory before being reconciled to disk.  The specified size will be
	 * adjusted to a lower bound of <code>50 * leaf_page_max</code>\, and an
	 * upper bound of <code>cache_size / 2</code>. This limit is soft - it
	 * is possible for pages to be temporarily larger than this value.  This
	 * setting is ignored for LSM trees\, see \c chunk_size., an integer
	 * between 512B and 10TB; default \c 5MB.}
	 * @config{os_cache_dirty_max, maximum dirty system buffer cache usage\,
	 * in bytes.  If non-zero\, schedule writes for dirty blocks belonging
	 * to this object in the system buffer cache after that many bytes from
	 * this object are written into the buffer cache., an integer greater
	 * than or equal to 0; default \c 0.}
	 * @config{os_cache_max, maximum system buffer cache usage\, in bytes.
	 * If non-zero\, evict object blocks from the system buffer cache after
	 * that many bytes from this object are read or written into the buffer
	 * cache., an integer greater than or equal to 0; default \c 0.}
	 * @config{prefix_compression, configure prefix compression on row-store
	 * leaf pages., a boolean flag; default \c false.}
	 * @config{prefix_compression_min, minimum gain before prefix
	 * compression will be used on row-store leaf pages., an integer greater
	 * than or equal to 0; default \c 4.}
	 * @config{split_pct, the Btree page split size as a percentage of the
	 * maximum Btree page size\, that is\, when a Btree page is split\, it
	 * will be split into smaller pages\, where each page is the specified
	 * percentage of the maximum Btree page size., an integer between 25 and
	 * 100; default \c 75.}
	 * @config{type, set the type of data source used to store a column
	 * group\, index or simple table.  By default\, a \c "file:" URI is
	 * derived from the object name.  The \c type configuration can be used
	 * to switch to a different data source\, such as LSM or an extension
	 * configured by the application., a string; default \c file.}
	 * @config{value_format, the format of the data packed into value items.
	 * See @ref schema_format_types for details.  By default\, the
	 * value_format is \c 'u' and applications use a WT_ITEM structure to
	 * manipulate raw byte arrays.  Value items of type 't' are bitfields\,
	 * and when configured with record number type keys\, will be stored
	 * using a fixed-length store., a format string; default \c u.}
	 * @configend
	 * @errors
	 */
	int __F(create)(WT_SESSION *session,
	    const char *name, const char *config);

	/*!
	 * Compact a live row- or column-store btree or LSM tree.
	 *
	 * @snippet ex_all.c Compact a table
	 *
	 * @param session the session handle
	 * @param name the URI of the object to compact, such as
	 * \c "table:stock"
	 * @configstart{WT_SESSION.compact, see dist/api_data.py}
	 * @config{timeout, maximum amount of time to allow for compact in
	 * seconds.  The actual amount of time spent in compact may exceed the
	 * configured value.  A value of zero disables the timeout., an integer;
	 * default \c 1200.}
	 * @configend
	 * @errors
	 */
	int __F(compact)(WT_SESSION *session,
	    const char *name, const char *config);

	/*!
	 * Drop (delete) an object.
	 *
	 * @snippet ex_all.c Drop a table
	 *
	 * @param session the session handle
	 * @param name the URI of the object to drop, such as \c "table:stock"
	 * @configstart{WT_SESSION.drop, see dist/api_data.py}
	 * @config{force, return success if the object does not exist., a
	 * boolean flag; default \c false.}
	 * @config{remove_files, should the underlying files be removed?., a
	 * boolean flag; default \c true.}
	 * @configend
	 * @ebusy_errors
	 */
	int __F(drop)(WT_SESSION *session,
	    const char *name, const char *config);

	/*!
	 * Insert a ::WT_LOGREC_MESSAGE type record in the database log files
	 * (the database must be configured for logging when this method is
	 * called).
	 *
	 * @param session the session handle
	 * @param fmt a printf format specifier
	 * @errors
	 */
	int __F(log_printf)(WT_SESSION *session, const char *fmt, ...);

	/*!
	 * Rename an object.
	 *
	 * @snippet ex_all.c Rename a table
	 *
	 * @param session the session handle
	 * @param uri the current URI of the object, such as \c "table:old"
	 * @param newuri the new URI of the object, such as \c "table:new"
	 * @configempty{WT_SESSION.rename, see dist/api_data.py}
	 * @ebusy_errors
	 */
	int __F(rename)(WT_SESSION *session,
	    const char *uri, const char *newuri, const char *config);

	/*!
	 * Salvage a file or table
	 *
	 * Salvage rebuilds the file, or files of which a table is comprised,
	 * discarding any corrupted file blocks.
	 *
	 * Previously deleted records may re-appear, and inserted records may
	 * disappear, when salvage is done, so salvage should not be run
	 * unless it is known to be necessary.  Normally, salvage should be
	 * called after a file or table has been corrupted, as reported by the
	 * WT_SESSION::verify method.
	 *
	 * Files are rebuilt in place, the salvage method overwrites the
	 * existing files.
	 *
	 * @snippet ex_all.c Salvage a table
	 *
	 * @param session the session handle
	 * @param name the URI of the file or table to salvage
	 * @configstart{WT_SESSION.salvage, see dist/api_data.py}
	 * @config{force, force salvage even of files that do not appear to be
	 * WiredTiger files., a boolean flag; default \c false.}
	 * @configend
	 * @ebusy_errors
	 */
	int __F(salvage)(WT_SESSION *session,
	    const char *name, const char *config);

	/*!
	 * Truncate a file, table or cursor range.
	 *
	 * Truncate a file or table.
	 * @snippet ex_all.c Truncate a table
	 *
	 * Truncate a cursor range.  When truncating based on a cursor position,
	 * it is not required the cursor reference a record in the object, only
	 * that the key be set.  This allows applications to discard portions of
	 * the object name space without knowing exactly what records the object
	 * contains.
	 * @snippet ex_all.c Truncate a range
	 *
	 * @param session the session handle
	 * @param name the URI of the file or table to truncate
	 * @param start optional cursor marking the first record discarded;
	 * if <code>NULL</code>, the truncate starts from the beginning of
	 * the object
	 * @param stop optional cursor marking the last record discarded;
	 * if <code>NULL</code>, the truncate continues to the end of the
	 * object
	 * @configempty{WT_SESSION.truncate, see dist/api_data.py}
	 * @ebusy_errors
	 */
	int __F(truncate)(WT_SESSION *session,
	    const char *name,
	    WT_HANDLE_NULLABLE(WT_CURSOR) *start,
	    WT_HANDLE_NULLABLE(WT_CURSOR) *stop,
	    const char *config);

	/*!
	 * Upgrade a file or table.
	 *
	 * Upgrade upgrades a file or table, if upgrade is required.
	 *
	 * @snippet ex_all.c Upgrade a table
	 *
	 * @param session the session handle
	 * @param name the URI of the file or table to upgrade
	 * @configempty{WT_SESSION.upgrade, see dist/api_data.py}
	 * @ebusy_errors
	 */
	int __F(upgrade)(WT_SESSION *session,
	    const char *name, const char *config);

	/*!
	 * Verify a file or table.
	 *
	 * Verify reports if a file, or the files of which a table is
	 * comprised, have been corrupted.  The WT_SESSION::salvage method
	 * can be used to repair a corrupted file,
	 *
	 * @snippet ex_all.c Verify a table
	 *
	 * @param session the session handle
	 * @param name the URI of the file or table to verify
	 * @configstart{WT_SESSION.verify, see dist/api_data.py}
	 * @config{dump_address, Display addresses and page types as pages are
	 * verified\, using the application's message handler\, intended for
	 * debugging., a boolean flag; default \c false.}
	 * @config{dump_blocks, Display the contents of on-disk blocks as they
	 * are verified\, using the application's message handler\, intended for
	 * debugging., a boolean flag; default \c false.}
	 * @config{dump_offsets, Display the contents of specific on-disk
	 * blocks\, using the application's message handler\, intended for
	 * debugging., a list of strings; default empty.}
	 * @config{dump_pages, Display the contents of in-memory pages as they
	 * are verified\, using the application's message handler\, intended for
	 * debugging., a boolean flag; default \c false.}
	 * @config{dump_shape, Display the shape of the tree after
	 * verification\, using the application's message handler\, intended for
	 * debugging., a boolean flag; default \c false.}
	 * @configend
	 * @ebusy_errors
	 */
	int __F(verify)(WT_SESSION *session,
	    const char *name, const char *config);
	/*! @} */

	/*!
	 * @name Transactions
	 * @{
	 */
	/*!
	 * Start a transaction in this session.
	 *
	 * The transaction remains active until ended by
	 * WT_SESSION::commit_transaction or WT_SESSION::rollback_transaction.
	 * Operations performed on cursors capable of supporting transactional
	 * operations that are already open in this session, or which are opened
	 * before the transaction ends, will operate in the context of the
	 * transaction.
	 *
	 * WT_SESSION::begin_transaction will fail if a transaction is already
	 * in progress in the session.
	 *
	 * @snippet ex_all.c transaction commit/rollback
	 *
	 * @param session the session handle
	 * @configstart{WT_SESSION.begin_transaction, see dist/api_data.py}
	 * @config{isolation, the isolation level for this transaction; defaults
	 * to the session's isolation level., a string\, chosen from the
	 * following options: \c "read-uncommitted"\, \c "read-committed"\, \c
	 * "snapshot"; default empty.}
	 * @config{name, name of the transaction for tracing and debugging., a
	 * string; default empty.}
	 * @config{priority, priority of the transaction for resolving
	 * conflicts.  Transactions with higher values are less likely to
	 * abort., an integer between -100 and 100; default \c 0.}
	 * @config{sync, whether to sync log records when the transaction
	 * commits\, inherited from ::wiredtiger_open \c transaction_sync., a
	 * boolean flag; default empty.}
	 * @configend
	 * @errors
	 */
	int __F(begin_transaction)(WT_SESSION *session, const char *config);

	/*!
	 * Commit the current transaction.
	 *
	 * A transaction must be in progress when this method is called.
	 *
	 * If WT_SESSION::commit_transaction returns an error, the transaction
	 * was rolled back, not committed.
	 *
	 * @snippet ex_all.c transaction commit/rollback
	 *
	 * @param session the session handle
	 * @configempty{WT_SESSION.commit_transaction, see dist/api_data.py}
	 * @errors
	 */
	int __F(commit_transaction)(WT_SESSION *session, const char *config);

	/*!
	 * Roll back the current transaction.
	 *
	 * A transaction must be in progress when this method is called.
	 *
	 * All cursors are reset.
	 *
	 * @snippet ex_all.c transaction commit/rollback
	 *
	 * @param session the session handle
	 * @configempty{WT_SESSION.rollback_transaction, see dist/api_data.py}
	 * @errors
	 */
	int __F(rollback_transaction)(WT_SESSION *session, const char *config);

	/*!
	 * Write a transactionally consistent snapshot of a database or set of
	 * objects.  The checkpoint includes all transactions committed before
	 * the checkpoint starts.  Additionally, checkpoints may optionally be
	 * discarded.
	 *
	 * @snippet ex_all.c Checkpoint examples
	 *
	 * @param session the session handle
	 * @configstart{WT_SESSION.checkpoint, see dist/api_data.py}
	 * @config{drop, specify a list of checkpoints to drop.  The list may
	 * additionally contain one of the following keys: \c "from=all" to drop
	 * all checkpoints\, \c "from=<checkpoint>" to drop all checkpoints
	 * after and including the named checkpoint\, or \c "to=<checkpoint>" to
	 * drop all checkpoints before and including the named checkpoint.
	 * Checkpoints cannot be dropped while a hot backup is in progress or if
	 * open in a cursor., a list of strings; default empty.}
	 * @config{force, by default\, checkpoints may be skipped if the
	 * underlying object has not been modified\, this option forces the
	 * checkpoint., a boolean flag; default \c false.}
	 * @config{name, if set\, specify a name for the checkpoint (note that
	 * checkpoints including LSM trees may not be named)., a string; default
	 * empty.}
	 * @config{target, if non-empty\, checkpoint the list of objects., a
	 * list of strings; default empty.}
	 * @configend
	 * @errors
	 */
	int __F(checkpoint)(WT_SESSION *session, const char *config);

	/*!
	 * Return the transaction ID range pinned by the session handle.
	 *
	 * The ID range is approximate and is calculated based on the oldest
	 * ID needed for the active transaction in this session, compared
	 * to the newest transaction in the system.
	 *
	 * @snippet ex_all.c transaction pinned range
	 *
	 * @param session the session handle
	 * @param[out] range the range of IDs pinned by this session. Zero if
	 * there is no active transaction.
	 * @errors
	 */
	int __F(transaction_pinned_range)(WT_SESSION* session, uint64_t *range);

	/*! @} */
};

/*!
 * A connection to a WiredTiger database.  The connection may be opened within
 * the same address space as the caller or accessed over a socket connection.
 *
 * Most applications will open a single connection to a database for each
 * process.  The first process to open a connection to a database will access
 * the database in its own address space.  Subsequent connections (if allowed)
 * will communicate with the first process over a socket connection to perform
 * their operations.
 *
 * <b>Thread safety:</b> A WT_CONNECTION handle may be shared between threads,
 * see @ref threads for more information.
 */
struct __wt_connection {
	/*!
	 * @name Async operation handles
	 * @{
	 */
	/*!
	 * Wait for all outstanding operations to complete.
	 *
	 * @snippet ex_async.c async flush
	 *
	 * @param connection the connection handle
	 * @errors
	 */
	int __F(async_flush)(WT_CONNECTION *connection);

	/*!
	 * Return an async operation handle
	 *
	 * @snippet ex_async.c async handle allocation
	 *
	 * @param connection the connection handle
	 * @param uri the connection handle
	 * @configstart{WT_CONNECTION.async_new_op, see dist/api_data.py}
	 * @config{append, append the value as a new record\, creating a new
	 * record number key; valid only for operations with record number
	 * keys., a boolean flag; default \c false.}
	 * @config{overwrite, configures whether the cursor's insert\, update
	 * and remove methods check the existing state of the record.  If \c
	 * overwrite is \c false\, WT_CURSOR::insert fails with
	 * ::WT_DUPLICATE_KEY if the record exists\, WT_CURSOR::update and
	 * WT_CURSOR::remove fail with ::WT_NOTFOUND if the record does not
	 * exist., a boolean flag; default \c true.}
	 * @config{raw, ignore the encodings for the key and value\, manage data
	 * as if the formats were \c "u". See @ref cursor_raw for details., a
	 * boolean flag; default \c false.}
	 * @config{timeout, maximum amount of time to allow for compact in
	 * seconds.  The actual amount of time spent in compact may exceed the
	 * configured value.  A value of zero disables the timeout., an integer;
	 * default \c 1200.}
	 * @configend
	 * @param callback the operation callback
	 * @param[out] asyncopp the new op handle
	 * @errors
	 * If there are no available handles, \c EBUSY is returned.
	 */
	int __F(async_new_op)(WT_CONNECTION *connection,
	    const char *uri, const char *config, WT_ASYNC_CALLBACK *callback,
	    WT_ASYNC_OP **asyncopp);
	/*! @} */

	/*!
	 * Close a connection.
	 *
	 * Any open sessions will be closed.
	 *
	 * @snippet ex_all.c Close a connection
	 *
	 * @param connection the connection handle
	 * @configstart{WT_CONNECTION.close, see dist/api_data.py}
	 * @config{leak_memory, don't free memory during close., a boolean flag;
	 * default \c false.}
	 * @configend
	 * @errors
	 */
	int __F(close)(WT_HANDLE_CLOSED(WT_CONNECTION) *connection,
	    const char *config);

	/*!
	 * Reconfigure a connection handle.
	 *
	 * @snippet ex_all.c Reconfigure a connection
	 *
	 * @param connection the connection handle
	 * @configstart{WT_CONNECTION.reconfigure, see dist/api_data.py}
	 * @config{async = (, asynchronous operations configuration options., a
	 * set of related configuration options defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable asynchronous
	 * operation., a boolean flag; default \c false.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;ops_max, maximum number of expected
	 * simultaneous asynchronous operations., an integer between 1 and 4096;
	 * default \c 1024.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads, the number
	 * of worker threads to service asynchronous requests.  Each worker
	 * thread uses a session from the configured session_max., an integer
	 * between 1 and 20; default \c 2.}
	 * @config{ ),,}
	 * @config{cache_overhead, assume the heap allocator overhead is the
	 * specified percentage\, and adjust the cache usage by that amount (for
	 * example\, if there is 10GB of data in cache\, a percentage of 10
	 * means WiredTiger treats this as 11GB). This value is configurable
	 * because different heap allocators have different overhead and
	 * different workloads will have different heap allocation sizes and
	 * patterns\, therefore applications may need to adjust this value based
	 * on allocator choice and behavior in measured workloads., an integer
	 * between 0 and 30; default \c 8.}
	 * @config{cache_size, maximum heap memory to allocate for the cache.  A
	 * database should configure either \c cache_size or \c shared_cache but
	 * not both., an integer between 1MB and 10TB; default \c 100MB.}
	 * @config{checkpoint = (, periodically checkpoint the database.
	 * Enabling the checkpoint server uses a session from the configured
	 * session_max., a set of related configuration options defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;log_size, wait for this amount of log
	 * record bytes to be written to the log between each checkpoint.  A
	 * database can configure both log_size and wait to set an upper bound
	 * for checkpoints; setting this value above 0 configures periodic
	 * checkpoints., an integer between 0 and 2GB; default \c 0.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, the checkpoint name., a string;
	 * default \c "WiredTigerCheckpoint".}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait, seconds to wait between each
	 * checkpoint; setting this value above 0 configures periodic
	 * checkpoints., an integer between 0 and 100000; default \c 0.}
	 * @config{ ),,}
	 * @config{error_prefix, prefix string for error messages., a string;
	 * default empty.}
	 * @config{eviction = (, eviction configuration options., a set of
	 * related configuration options defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_max, maximum number of
	 * threads WiredTiger will start to help evict pages from cache.  The
	 * number of threads started will vary depending on the current eviction
	 * load.  Each eviction worker thread uses a session from the configured
	 * session_max., an integer between 1 and 20; default \c 1.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_min, minimum number of
	 * threads WiredTiger will start to help evict pages from cache.  The
	 * number of threads currently running will vary depending on the
	 * current eviction load., an integer between 1 and 20; default \c 1.}
	 * @config{ ),,}
	 * @config{eviction_dirty_target, continue evicting until the cache has
	 * less dirty memory than the value\, as a percentage of the total cache
	 * size.  Dirty pages will only be evicted if the cache is full enough
	 * to trigger eviction., an integer between 10 and 99; default \c 80.}
	 * @config{eviction_target, continue evicting until the cache has less
	 * total memory than the value\, as a percentage of the total cache
	 * size.  Must be less than \c eviction_trigger., an integer between 10
	 * and 99; default \c 80.}
	 * @config{eviction_trigger, trigger eviction when the cache is using
	 * this much memory\, as a percentage of the total cache size., an
	 * integer between 10 and 99; default \c 95.}
	 * @config{file_manager = (, control how file handles are managed., a
	 * set of related configuration options defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_handle_minimum, number of
	 * handles open before the file manager will look for handles to close.,
	 * an integer greater than or equal to 0; default \c 250.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_idle_time, amount of time in
	 * seconds a file handle needs to be idle before attempting to close
	 * it., an integer between 1 and 100000; default \c 30.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_scan_interval, interval in
	 * seconds at which to check for files that are inactive and close
	 * them., an integer between 1 and 100000; default \c 10.}
	 * @config{ ),,}
	 * @config{lsm_manager = (, configure database wide options for LSM tree
	 * management.  The LSM manager is started automatically the first time
	 * an LSM tree is opened.  The LSM manager uses a session from the
	 * configured session_max., a set of related configuration options
	 * defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge, merge LSM
	 * chunks where possible., a boolean flag; default \c true.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;worker_thread_max, Configure a set of
	 * threads to manage merging LSM trees in the database.  Each worker
	 * thread uses a session handle from the configured session_max., an
	 * integer between 3 and 20; default \c 4.}
	 * @config{ ),,}
	 * @config{shared_cache = (, shared cache configuration options.  A
	 * database should configure either a cache_size or a shared_cache not
	 * both.  Enabling a shared cache uses a session from the configured
	 * session_max., a set of related configuration options defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk, the granularity that a shared
	 * cache is redistributed., an integer between 1MB and 10TB; default \c
	 * 10MB.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, the name of a cache that
	 * is shared between databases or \c "none" when no shared cache is
	 * configured., a string; default \c none.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;reserve, amount of cache this
	 * database is guaranteed to have available from the shared cache.  This
	 * setting is per database.  Defaults to the chunk size., an integer;
	 * default \c 0.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;size, maximum memory
	 * to allocate for the shared cache.  Setting this will update the value
	 * if one is already set., an integer between 1MB and 10TB; default \c
	 * 500MB.}
	 * @config{ ),,}
	 * @config{statistics, Maintain database statistics\, which may impact
	 * performance.  Choosing "all" maintains all statistics regardless of
	 * cost\, "fast" maintains a subset of statistics that are relatively
	 * inexpensive\, "none" turns off all statistics.  The "clear"
	 * configuration resets statistics after they are gathered\, where
	 * appropriate (for example\, a cache size statistic is not cleared\,
	 * while the count of cursor insert operations will be cleared). When
	 * "clear" is configured for the database\, gathered statistics are
	 * reset each time a statistics cursor is used to gather statistics\, as
	 * well as each time statistics are logged using the \c statistics_log
	 * configuration.  See @ref statistics for more information., a list\,
	 * with values chosen from the following options: \c "all"\, \c "fast"\,
	 * \c "none"\, \c "clear"; default \c none.}
	 * @config{statistics_log = (, log any statistics the database is
	 * configured to maintain\, to a file.  See @ref statistics for more
	 * information.  Enabling the statistics log server uses a session from
	 * the configured session_max., a set of related configuration options
	 * defined below.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;on_close, log
	 * statistics on database close., a boolean flag; default \c false.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;path, the pathname to a file into
	 * which the log records are written\, may contain ISO C standard
	 * strftime conversion specifications.  If the value is not an absolute
	 * path name\, the file is created relative to the database home., a
	 * string; default \c "WiredTigerStat.%d.%H".}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;sources, if non-empty\, include
	 * statistics for the list of data source URIs\, if they are open at the
	 * time of the statistics logging.  The list may include URIs matching a
	 * single data source ("table:mytable")\, or a URI matching all data
	 * sources of a particular type ("table:")., a list of strings; default
	 * empty.}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;timestamp, a timestamp
	 * prepended to each log record\, may contain strftime conversion
	 * specifications., a string; default \c "%b %d %H:%M:%S".}
	 * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait, seconds to wait between each
	 * write of the log records; setting this value above 0 configures
	 * statistics logging., an integer between 0 and 100000; default \c 0.}
	 * @config{ ),,}
	 * @config{verbose, enable messages for various events.  Only available
	 * if WiredTiger is configured with --enable-verbose.  Options are given
	 * as a list\, such as <code>"verbose=[evictserver\,read]"</code>., a
	 * list\, with values chosen from the following options: \c "api"\, \c
	 * "block"\, \c "checkpoint"\, \c "compact"\, \c "evict"\, \c
	 * "evictserver"\, \c "fileops"\, \c "log"\, \c "lsm"\, \c "metadata"\,
	 * \c "mutex"\, \c "overflow"\, \c "read"\, \c "reconcile"\, \c
	 * "recovery"\, \c "salvage"\, \c "shared_cache"\, \c "split"\, \c
	 * "temporary"\, \c "transaction"\, \c "verify"\, \c "version"\, \c
	 * "write"; default empty.}
	 * @configend
	 * @errors
	 */
	int __F(reconfigure)(WT_CONNECTION *connection, const char *config);

	/*!
	 * The home directory of the connection.
	 *
	 * @snippet ex_all.c Get the database home directory
	 *
	 * @param connection the connection handle
	 * @returns a pointer to a string naming the home directory
	 */
	const char *__F(get_home)(WT_CONNECTION *connection);

	/*!
	 * Add configuration options for a method.  See
	 * @ref custom_ds_config_add for more information.
	 *
	 * @snippet ex_all.c Configure method configuration
	 *
	 * @param connection the connection handle
	 * @param method the method being configured
	 * @param uri the object type or NULL for all object types
	 * @param config the additional configuration's name and default value
	 * @param type the additional configuration's type (must be one of
	 * \c "boolean"\, \c "int", \c "list" or \c "string")
	 * @param check the additional configuration check string, or NULL if
	 * none
	 * @errors
	 */
	int __F(configure_method)(WT_CONNECTION *connection,
	    const char *method, const char *uri,
	    const char *config, const char *type, const char *check);

	/*!
	 * Return if opening this handle created the database.
	 *
	 * @snippet ex_all.c Check if the database is newly created
	 *
	 * @param connection the connection handle
	 * @returns false (zero) if the connection existed before the call to
	 * ::wiredtiger_open, true (non-zero) if it was created by opening this
	 * handle.
	 */
	int __F(is_new)(WT_CONNECTION *connection);

	/*!
	 * @name Session handles
	 * @{
	 */
	/*!
	 * Open a session.
	 *
	 * @snippet ex_all.c Open a session
	 *
	 * @param connection the connection handle
	 * @param errhandler An error handler.  If <code>NULL</code>, the
	 * connection's error handler is used
	 * @configstart{WT_CONNECTION.open_session, see dist/api_data.py}
	 * @config{isolation, the default isolation level for operations in this
	 * session., a string\, chosen from the following options: \c
	 * "read-uncommitted"\, \c "read-committed"\, \c "snapshot"; default \c
	 * read-committed.}
	 * @configend
	 * @param[out] sessionp the new session handle
	 * @errors
	 */
	int __F(open_session)(WT_CONNECTION *connection,
	    WT_EVENT_HANDLER *errhandler, const char *config,
	    WT_SESSION **sessionp);
	/*! @} */

	/*!
	 * @name Extensions
	 * @{
	 */
	/*!
	 * Load an extension.
	 *
	 * @snippet ex_all.c Load an extension
	 *
	 * @param connection the connection handle
	 * @param path the filename of the extension module, or \c "local" to
	 * search the current application binary for the initialization
	 * function, see @ref extensions for more details.
	 * @configstart{WT_CONNECTION.load_extension, see dist/api_data.py}
	 * @config{config, configuration string passed to the entry point of the
	 * extension as its WT_CONFIG_ARG argument., a string; default empty.}
	 * @config{entry, the entry point of the extension\, called to
	 * initialize the extension when it is loaded.  The signature of the
	 * function must match ::wiredtiger_extension_init., a string; default
	 * \c wiredtiger_extension_init.}
	 * @config{terminate, an optional function in the extension that is
	 * called before the extension is unloaded during WT_CONNECTION::close.
	 * The signature of the function must match
	 * ::wiredtiger_extension_terminate., a string; default \c
	 * wiredtiger_extension_terminate.}
	 * @configend
	 * @errors
	 */
	int __F(load_extension)(WT_CONNECTION *connection,
	    const char *path, const char *config);

	/*!
	 * Add a custom data source.  See @ref custom_data_sources for more
	 * information.
	 *
	 * The application must first implement the WT_DATA_SOURCE interface
	 * and then register the implementation with WiredTiger:
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE register
	 *
	 * @param connection the connection handle
	 * @param prefix the URI prefix for this data source, e.g., "file:"
	 * @param data_source the application-supplied implementation of
	 *	WT_DATA_SOURCE to manage this data source.
	 * @configempty{WT_CONNECTION.add_data_source, see dist/api_data.py}
	 * @errors
	 */
	int __F(add_data_source)(WT_CONNECTION *connection, const char *prefix,
	    WT_DATA_SOURCE *data_source, const char *config);

	/*!
	 * Add a custom collation function.
	 *
	 * The application must first implement the WT_COLLATOR interface and
	 * then register the implementation with WiredTiger:
	 *
	 * @snippet ex_all.c WT_COLLATOR register
	 *
	 * @param connection the connection handle
	 * @param name the name of the collation to be used in calls to
	 * 	WT_SESSION::create, may not be \c "none"
	 * @param collator the application-supplied collation handler
	 * @configempty{WT_CONNECTION.add_collator, see dist/api_data.py}
	 * @errors
	 */
	int __F(add_collator)(WT_CONNECTION *connection,
	    const char *name, WT_COLLATOR *collator, const char *config);

	/*!
	 * Add a compression function.
	 *
	 * The application must first implement the WT_COMPRESSOR interface
	 * and then register the implementation with WiredTiger:
	 *
	 * @snippet nop_compress.c WT_COMPRESSOR initialization structure
	 *
	 * @snippet nop_compress.c WT_COMPRESSOR initialization function
	 *
	 * @param connection the connection handle
	 * @param name the name of the compression function to be used in calls
	 *	to WT_SESSION::create, may not be \c "none"
	 * @param compressor the application-supplied compression handler
	 * @configempty{WT_CONNECTION.add_compressor, see dist/api_data.py}
	 * @errors
	 */
	int __F(add_compressor)(WT_CONNECTION *connection,
	    const char *name, WT_COMPRESSOR *compressor, const char *config);

	/*!
	 * Add an encryption function.
	 *
	 * The application must first implement the WT_ENCRYPTOR interface
	 * and then register the implementation with WiredTiger:
	 *
	 * @snippet nop_encrypt.c WT_ENCRYPTOR initialization structure
	 *
	 * @snippet nop_encrypt.c WT_ENCRYPTOR initialization function
	 *
	 * @param connection the connection handle
	 * @param name the name of the encryption function to be used in calls
	 *	to WT_SESSION::create, may not be \c "none"
	 * @param encryptor the application-supplied encryption handler
	 * @configempty{WT_CONNECTION.add_encryptor, see dist/api_data.py}
	 * @errors
	 */
	int __F(add_encryptor)(WT_CONNECTION *connection,
	    const char *name, WT_ENCRYPTOR *encryptor, const char *config);

	/*!
	 * Add a custom extractor for index keys or column groups.
	 *
	 * The application must first implement the WT_EXTRACTOR interface and
	 * then register the implementation with WiredTiger:
	 *
	 * @snippet ex_all.c WT_EXTRACTOR register
	 *
	 * @param connection the connection handle
	 * @param name the name of the extractor to be used in calls to
	 * 	WT_SESSION::create, may not be \c "none"
	 * @param extractor the application-supplied extractor
	 * @configempty{WT_CONNECTION.add_extractor, see dist/api_data.py}
	 * @errors
	 */
	int __F(add_extractor)(WT_CONNECTION *connection, const char *name,
	    WT_EXTRACTOR *extractor, const char *config);

	/*!
	 * Return a reference to the WiredTiger extension functions.
	 *
	 * @snippet ex_data_source.c WT_EXTENSION_API declaration
	 *
	 * @param wt_conn the WT_CONNECTION handle
	 * @returns a reference to a WT_EXTENSION_API structure.
	 */
	WT_EXTENSION_API *__F(get_extension_api)(WT_CONNECTION *wt_conn);
	/*! @} */
};

/*!
 * Open a connection to a database.
 *
 * @snippet ex_all.c Open a connection
 *
 * @param home The path to the database home directory.  See @ref home
 * for more information.
 * @param errhandler An error handler.  If <code>NULL</code>, a builtin error
 * handler is installed that writes error messages to stderr
 * @configstart{wiredtiger_open, see dist/api_data.py}
 * @config{async = (, asynchronous operations configuration options., a set of
 * related configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable asynchronous operation., a
 * boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;ops_max,
 * maximum number of expected simultaneous asynchronous operations., an integer
 * between 1 and 4096; default \c 1024.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads, the number of worker threads to
 * service asynchronous requests.  Each worker thread uses a session from the
 * configured session_max., an integer between 1 and 20; default \c 2.}
 * @config{
 * ),,}
 * @config{buffer_alignment, in-memory alignment (in bytes) for buffers used for
 * I/O. The default value of -1 indicates a platform-specific alignment value
 * should be used (4KB on Linux systems when direct I/O is configured\, zero
 * elsewhere)., an integer between -1 and 1MB; default \c -1.}
 * @config{cache_overhead, assume the heap allocator overhead is the specified
 * percentage\, and adjust the cache usage by that amount (for example\, if
 * there is 10GB of data in cache\, a percentage of 10 means WiredTiger treats
 * this as 11GB). This value is configurable because different heap allocators
 * have different overhead and different workloads will have different heap
 * allocation sizes and patterns\, therefore applications may need to adjust
 * this value based on allocator choice and behavior in measured workloads., an
 * integer between 0 and 30; default \c 8.}
 * @config{cache_size, maximum heap memory to allocate for the cache.  A
 * database should configure either \c cache_size or \c shared_cache but not
 * both., an integer between 1MB and 10TB; default \c 100MB.}
 * @config{checkpoint = (, periodically checkpoint the database.  Enabling the
 * checkpoint server uses a session from the configured session_max., a set of
 * related configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;log_size, wait for this amount of log record
 * bytes to be written to the log between each checkpoint.  A database can
 * configure both log_size and wait to set an upper bound for checkpoints;
 * setting this value above 0 configures periodic checkpoints., an integer
 * between 0 and 2GB; default \c 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, the
 * checkpoint name., a string; default \c "WiredTigerCheckpoint".}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait, seconds to wait between each
 * checkpoint; setting this value above 0 configures periodic checkpoints., an
 * integer between 0 and 100000; default \c 0.}
 * @config{ ),,}
 * @config{checkpoint_sync, flush files to stable storage when closing or
 * writing checkpoints., a boolean flag; default \c true.}
 * @config{config_base, write the base configuration file if creating the
 * database\, see @ref config_base for more information., a boolean flag;
 * default \c true.}
 * @config{create, create the database if it does not exist., a boolean flag;
 * default \c false.}
 * @config{direct_io, Use \c O_DIRECT to access files.  Options are given as a
 * list\, such as <code>"direct_io=[data]"</code>. Configuring \c direct_io
 * requires care\, see @ref tuning_system_buffer_cache_direct_io for important
 * warnings.  Including \c "data" will cause WiredTiger data files to use \c
 * O_DIRECT\, including \c "log" will cause WiredTiger log files to use \c
 * O_DIRECT\, and including \c "checkpoint" will cause WiredTiger data files
 * opened at a checkpoint (i.e: read only) to use \c O_DIRECT., a list\, with
 * values chosen from the following options: \c "checkpoint"\, \c "data"\, \c
 * "log"; default empty.}
 * @config{encryption = (, configure an encryptor for system wide metadata and
 * logs.  If a system wide encryptor is set\, it is also used for encrypting
 * data files and tables\, unless encryption configuration is explicitly set for
 * them when they are created with WT_SESSION::create., a set of related
 * configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;keyid,
 * An identifier that identifies a unique instance of the encryptor.  It is
 * stored in clear text\, and thus is available when the wiredtiger database is
 * reopened.  On the first use of a (name\, keyid) combination\, the
 * WT_ENCRYPTOR::customize function is called with the keyid as an argument., a
 * string; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, Permitted
 * values are \c "none" or custom encryption engine name created with
 * WT_CONNECTION::add_encryptor.  See @ref encryption for more information., a
 * string; default \c none.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;secretkey, A string
 * that is passed to the WT_ENCRYPTOR::customize function.  It is never stored
 * in clear text\, so must be given to any subsequent wiredtiger_open calls to
 * reopen the database.  It must also be provided to any "wt" commands used with
 * this database., a string; default empty.}
 * @config{ ),,}
 * @config{error_prefix, prefix string for error messages., a string; default
 * empty.}
 * @config{eviction = (, eviction configuration options., a set of related
 * configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_max, maximum number of threads
 * WiredTiger will start to help evict pages from cache.  The number of threads
 * started will vary depending on the current eviction load.  Each eviction
 * worker thread uses a session from the configured session_max., an integer
 * between 1 and 20; default \c 1.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;threads_min,
 * minimum number of threads WiredTiger will start to help evict pages from
 * cache.  The number of threads currently running will vary depending on the
 * current eviction load., an integer between 1 and 20; default \c 1.}
 * @config{
 * ),,}
 * @config{eviction_dirty_target, continue evicting until the cache has less
 * dirty memory than the value\, as a percentage of the total cache size.  Dirty
 * pages will only be evicted if the cache is full enough to trigger eviction.,
 * an integer between 10 and 99; default \c 80.}
 * @config{eviction_target, continue evicting until the cache has less total
 * memory than the value\, as a percentage of the total cache size.  Must be
 * less than \c eviction_trigger., an integer between 10 and 99; default \c 80.}
 * @config{eviction_trigger, trigger eviction when the cache is using this much
 * memory\, as a percentage of the total cache size., an integer between 10 and
 * 99; default \c 95.}
 * @config{exclusive, fail if the database already exists\, generally used with
 * the \c create option., a boolean flag; default \c false.}
 * @config{extensions, list of shared library extensions to load (using dlopen).
 * Any values specified to an library extension are passed to
 * WT_CONNECTION::load_extension as the \c config parameter (for example\,
 * <code>extensions=(/path/ext.so={entry=my_entry})</code>)., a list of strings;
 * default empty.}
 * @config{file_extend, file extension configuration.  If set\, extend files of
 * the set type in allocations of the set size\, instead of a block at a time as
 * each new block is written.  For example\,
 * <code>file_extend=(data=16MB)</code>., a list\, with values chosen from the
 * following options: \c "data"\, \c "log"; default empty.}
 * @config{file_manager = (, control how file handles are managed., a set of
 * related configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_handle_minimum, number of handles open
 * before the file manager will look for handles to close., an integer greater
 * than or equal to 0; default \c 250.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_idle_time, amount of time in seconds a
 * file handle needs to be idle before attempting to close it., an integer
 * between 1 and 100000; default \c 30.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;close_scan_interval, interval in seconds at
 * which to check for files that are inactive and close them., an integer
 * between 1 and 100000; default \c 10.}
 * @config{ ),,}
 * @config{hazard_max, maximum number of simultaneous hazard pointers per
 * session handle., an integer greater than or equal to 15; default \c 1000.}
 * @config{log = (, enable logging.  Enabling logging uses three sessions from
 * the configured session_max., a set of related configuration options defined
 * below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;archive, automatically archive
 * unneeded log files., a boolean flag; default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;compressor, configure a compressor for log
 * records.  Permitted values are \c "none" or custom compression engine name
 * created with WT_CONNECTION::add_compressor.  If WiredTiger has builtin
 * support for \c "bzip2"\, \c "snappy"\, \c "lz4" or \c "zlib" compression\,
 * these names are also available.  See @ref compression for more information.,
 * a string; default \c none.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, enable
 * logging subsystem., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;file_max, the maximum size of log files., an
 * integer between 100KB and 2GB; default \c 100MB.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;path, the path to a directory into which the
 * log files are written.  If the value is not an absolute path name\, the files
 * are created relative to the database home., a string; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;prealloc, pre-allocate log files., a boolean
 * flag; default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;recover, run recovery
 * or error if recovery needs to run after an unclean shutdown., a string\,
 * chosen from the following options: \c "error"\, \c "on"; default \c on.}
 * @config{ ),,}
 * @config{lsm_manager = (, configure database wide options for LSM tree
 * management.  The LSM manager is started automatically the first time an LSM
 * tree is opened.  The LSM manager uses a session from the configured
 * session_max., a set of related configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;merge, merge LSM chunks where possible., a
 * boolean flag; default \c true.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;worker_thread_max, Configure a set of threads
 * to manage merging LSM trees in the database.  Each worker thread uses a
 * session handle from the configured session_max., an integer between 3 and 20;
 * default \c 4.}
 * @config{ ),,}
 * @config{mmap, Use memory mapping to access files when possible., a boolean
 * flag; default \c true.}
 * @config{multiprocess, permit sharing between processes (will automatically
 * start an RPC server for primary processes and use RPC for secondary
 * processes). <b>Not yet supported in WiredTiger</b>., a boolean flag; default
 * \c false.}
 * @config{session_max, maximum expected number of sessions (including server
 * threads)., an integer greater than or equal to 1; default \c 100.}
 * @config{shared_cache = (, shared cache configuration options.  A database
 * should configure either a cache_size or a shared_cache not both.  Enabling a
 * shared cache uses a session from the configured session_max., a set of
 * related configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;chunk, the granularity that a shared cache is
 * redistributed., an integer between 1MB and 10TB; default \c 10MB.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;name, the name of a cache that is shared
 * between databases or \c "none" when no shared cache is configured., a string;
 * default \c none.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;reserve, amount of cache
 * this database is guaranteed to have available from the shared cache.  This
 * setting is per database.  Defaults to the chunk size., an integer; default \c
 * 0.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;size, maximum memory to allocate for the
 * shared cache.  Setting this will update the value if one is already set., an
 * integer between 1MB and 10TB; default \c 500MB.}
 * @config{ ),,}
 * @config{statistics, Maintain database statistics\, which may impact
 * performance.  Choosing "all" maintains all statistics regardless of cost\,
 * "fast" maintains a subset of statistics that are relatively inexpensive\,
 * "none" turns off all statistics.  The "clear" configuration resets statistics
 * after they are gathered\, where appropriate (for example\, a cache size
 * statistic is not cleared\, while the count of cursor insert operations will
 * be cleared). When "clear" is configured for the database\, gathered
 * statistics are reset each time a statistics cursor is used to gather
 * statistics\, as well as each time statistics are logged using the \c
 * statistics_log configuration.  See @ref statistics for more information., a
 * list\, with values chosen from the following options: \c "all"\, \c "fast"\,
 * \c "none"\, \c "clear"; default \c none.}
 * @config{statistics_log = (, log any statistics the database is configured to
 * maintain\, to a file.  See @ref statistics for more information.  Enabling
 * the statistics log server uses a session from the configured session_max., a
 * set of related configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;on_close, log statistics on database close.,
 * a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;path, the
 * pathname to a file into which the log records are written\, may contain ISO C
 * standard strftime conversion specifications.  If the value is not an absolute
 * path name\, the file is created relative to the database home., a string;
 * default \c "WiredTigerStat.%d.%H".}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;sources,
 * if non-empty\, include statistics for the list of data source URIs\, if they
 * are open at the time of the statistics logging.  The list may include URIs
 * matching a single data source ("table:mytable")\, or a URI matching all data
 * sources of a particular type ("table:")., a list of strings; default empty.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;timestamp, a timestamp prepended to each log
 * record\, may contain strftime conversion specifications., a string; default
 * \c "%b %d %H:%M:%S".}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;wait, seconds to wait
 * between each write of the log records; setting this value above 0 configures
 * statistics logging., an integer between 0 and 100000; default \c 0.}
 * @config{
 * ),,}
 * @config{transaction_sync = (, how to sync log records when the transaction
 * commits., a set of related configuration options defined below.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;enabled, whether to sync the log on every
 * commit by default\, can be overridden by the \c sync setting to
 * WT_SESSION::begin_transaction., a boolean flag; default \c false.}
 * @config{&nbsp;&nbsp;&nbsp;&nbsp;method, the method used to ensure log records
 * are stable on disk\, see @ref tune_durability for more information., a
 * string\, chosen from the following options: \c "dsync"\, \c "fsync"\, \c
 * "none"; default \c fsync.}
 * @config{ ),,}
 * @config{use_environment_priv, use the \c WIREDTIGER_CONFIG and \c
 * WIREDTIGER_HOME environment variables regardless of whether or not the
 * process is running with special privileges.  See @ref home for more
 * information., a boolean flag; default \c false.}
 * @config{verbose, enable messages for various events.  Only available if
 * WiredTiger is configured with --enable-verbose.  Options are given as a
 * list\, such as <code>"verbose=[evictserver\,read]"</code>., a list\, with
 * values chosen from the following options: \c "api"\, \c "block"\, \c
 * "checkpoint"\, \c "compact"\, \c "evict"\, \c "evictserver"\, \c "fileops"\,
 * \c "log"\, \c "lsm"\, \c "metadata"\, \c "mutex"\, \c "overflow"\, \c
 * "read"\, \c "reconcile"\, \c "recovery"\, \c "salvage"\, \c "shared_cache"\,
 * \c "split"\, \c "temporary"\, \c "transaction"\, \c "verify"\, \c "version"\,
 * \c "write"; default empty.}
 * @configend
 * Additionally, if files named \c WiredTiger.config or \c WiredTiger.basecfg
 * appear in the WiredTiger home directory, they are read for configuration
 * values (see @ref config_file and @ref config_base for details).
 * See @ref config_order for ordering of the configuration mechanisms.
 * @param[out] connectionp A pointer to the newly opened connection handle
 * @errors
 */
int wiredtiger_open(const char *home,
    WT_EVENT_HANDLER *errhandler, const char *config,
    WT_CONNECTION **connectionp);

/*!
 * Return information about a WiredTiger error as a string, not thread-safe.
 *
 * @snippet ex_all.c Display an error
 *
 * @param error a return value from a WiredTiger call
 * @returns a string representation of the error
 */
const char *wiredtiger_strerror(int error);

#if !defined(SWIG)
/*!
 * The interface implemented by applications to accept notifications
 * of the completion of asynchronous operations.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::async_new_op.
 *
 * @snippet ex_async.c async handle allocation
 */
struct __wt_async_callback {
	/*!
	 * Callback to receive completion notification.
	 *
	 * @param[in] op the operation handle
	 * @param[in] op_ret the result of the async operation
	 * @param[in] flags currently unused
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet ex_async.c async example callback implementation
	 */
	int (*notify)(WT_ASYNC_CALLBACK *cb, WT_ASYNC_OP *op,
	    int op_ret, uint32_t flags);
};
#endif

/*!
 * The interface implemented by applications to handle error, informational and
 * progress messages.  Entries set to NULL are ignored and the default handlers
 * will continue to be used.
 */
struct __wt_event_handler {
	/*!
	 * Callback to handle error messages; by default, error messages are
	 * written to the stderr stream.
	 *
	 * Errors that require the application to exit and restart will have
	 * their \c error value set to \c WT_PANIC. The application can exit
	 * immediately when \c WT_PANIC is passed to an error handler, there
	 * is no reason to return into WiredTiger.
	 *
	 * Error handler returns are not ignored: if the handler returns
	 * non-zero, the error may cause the WiredTiger function posting the
	 * event to fail, and may even cause operation or library failure.
	 *
	 * @param session the WiredTiger session handle in use when the error
	 * was generated. The handle may have been created by the application
	 * or automatically by WiredTiger.
	 * @param error a WiredTiger, C99 or POSIX error code, which can
	 * be converted to a string using ::wiredtiger_strerror
	 * @param message an error string
	 */
	int (*handle_error)(WT_EVENT_HANDLER *handler,
	    WT_SESSION *session, int error, const char *message);

	/*!
	 * Callback to handle informational messages; by default, informational
	 * messages are written to the stdout stream.
	 *
	 * Message handler returns are not ignored: if the handler returns
	 * non-zero, the error may cause the WiredTiger function posting the
	 * event to fail, and may even cause operation or library failure.
	 *
	 * @param session the WiredTiger session handle in use when the message
	 * was generated. The handle may have been created by the application
	 * or automatically by WiredTiger.
	 * @param message an informational string
	 */
	int (*handle_message)(WT_EVENT_HANDLER *handler,
	    WT_SESSION *session, const char *message);

	/*!
	 * Callback to handle progress messages; by default, no progress
	 * messages are written.
	 *
	 * Progress handler returns are not ignored: if the handler returns
	 * non-zero, the error may cause the WiredTiger function posting the
	 * event to fail, and may even cause operation or library failure.
	 *
	 * @param session the WiredTiger session handle in use when the
	 * progress message was generated. The handle may have been created by
	 * the application or automatically by WiredTiger.
	 * @param operation a string representation of the operation
	 * @param progress a counter
	 */
	int (*handle_progress)(WT_EVENT_HANDLER *handler,
	    WT_SESSION *session, const char *operation, uint64_t progress);

	/*!
	 * Callback to handle automatic close of a WiredTiger handle.
	 *
	 * Close handler returns are not ignored: if the handler returns
	 * non-zero, the error may cause the WiredTiger function posting the
	 * event to fail, and may even cause operation or library failure.
	 *
	 * @param session The session handle that is being closed if the
	 * cursor parameter is NULL.
	 * @param cursor The cursor handle that is being closed, or NULL if
	 * it is a session handle being closed.
	 */
	int (*handle_close)(WT_EVENT_HANDLER *handler,
	    WT_SESSION *session, WT_CURSOR *cursor);
};

/*!
 * @name Data packing and unpacking
 * @{
 */

/*!
 * Pack a structure into a buffer.
 *
 * See @ref packing for a description of the permitted format strings.
 *
 * @section pack_examples Packing Examples
 *
 * For example, the string <code>"iSh"</code> will pack a 32-bit integer
 * followed by a NUL-terminated string, followed by a 16-bit integer.  The
 * default, big-endian encoding will be used, with no alignment.  This could be
 * used in C as follows:
 *
 * @snippet ex_all.c Pack fields into a buffer
 *
 * Then later, the values can be unpacked as follows:
 *
 * @snippet ex_all.c Unpack fields from a buffer
 *
 * @param session the session handle
 * @param buffer a pointer to a packed byte array
 * @param size the number of valid bytes in the buffer
 * @param format the data format, see @ref packing
 * @errors
 */
int wiredtiger_struct_pack(WT_SESSION *session,
    void *buffer, size_t size, const char *format, ...);

/*!
 * Calculate the size required to pack a structure.
 *
 * Note that for variable-sized fields including variable-sized strings and
 * integers, the calculated sized merely reflects the expected sizes specified
 * in the format string itself.
 *
 * @snippet ex_all.c Get the packed size
 *
 * @param session the session handle
 * @param sizep a location where the number of bytes needed for the
 * matching call to ::wiredtiger_struct_pack is returned
 * @param format the data format, see @ref packing
 * @errors
 */
int wiredtiger_struct_size(WT_SESSION *session,
    size_t *sizep, const char *format, ...);

/*!
 * Unpack a structure from a buffer.
 *
 * Reverse of ::wiredtiger_struct_pack: gets values out of a
 * packed byte string.
 *
 * @snippet ex_all.c Unpack fields from a buffer
 *
 * @param session the session handle
 * @param buffer a pointer to a packed byte array
 * @param size the number of valid bytes in the buffer
 * @param format the data format, see @ref packing
 * @errors
 */
int wiredtiger_struct_unpack(WT_SESSION *session,
    const void *buffer, size_t size, const char *format, ...);

#if !defined(SWIG)

/*!
 * Streaming interface to packing.
 *
 * This allows applications to pack or unpack records one field at a time.
 * This is an opaque handle returned by ::wiredtiger_pack_start or
 * ::wiredtiger_unpack_start.  It must be closed with ::wiredtiger_pack_close.
 */
typedef struct __wt_pack_stream WT_PACK_STREAM;

/*!
 * Start a packing operation into a buffer with the given format string.  This
 * should be followed by a series of calls to ::wiredtiger_pack_item,
 * ::wiredtiger_pack_int, ::wiredtiger_pack_str or ::wiredtiger_pack_uint
 * to fill in the values.
 *
 * @param session the session handle
 * @param format the data format, see @ref packing
 * @param buffer a pointer to memory to hold the packed data
 * @param size the size of the buffer
 * @param[out] psp the new packing stream handle
 * @errors
 */
int wiredtiger_pack_start(WT_SESSION *session,
    const char *format, void *buffer, size_t size, WT_PACK_STREAM **psp);

/*!
 * Start an unpacking operation from a buffer with the given format string.
 * This should be followed by a series of calls to ::wiredtiger_unpack_item,
 * ::wiredtiger_unpack_int, ::wiredtiger_unpack_str or ::wiredtiger_unpack_uint
 * to retrieve the packed values.
 *
 * @param session the session handle
 * @param format the data format, see @ref packing
 * @param buffer a pointer to memory holding the packed data
 * @param size the size of the buffer
 * @param[out] psp the new packing stream handle
 * @errors
 */
int wiredtiger_unpack_start(WT_SESSION *session,
    const char *format, const void *buffer, size_t size, WT_PACK_STREAM **psp);

/*!
 * Close a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] usedp the number of bytes in the buffer used by the stream
 * @errors
 */
int wiredtiger_pack_close(WT_PACK_STREAM *ps, size_t *usedp);

/*!
 * Pack an item into a packing stream.
 *
 * @param ps the packing stream handle
 * @param item an item to pack
 * @errors
 */
int wiredtiger_pack_item(WT_PACK_STREAM *ps, WT_ITEM *item);

/*!
 * Pack a signed integer into a packing stream.
 *
 * @param ps the packing stream handle
 * @param i a signed integer to pack
 * @errors
 */
int wiredtiger_pack_int(WT_PACK_STREAM *ps, int64_t i);

/*!
 * Pack a string into a packing stream.
 *
 * @param ps the packing stream handle
 * @param s a string to pack
 * @errors
 */
int wiredtiger_pack_str(WT_PACK_STREAM *ps, const char *s);

/*!
 * Pack an unsigned integer into a packing stream.
 *
 * @param ps the packing stream handle
 * @param u an unsigned integer to pack
 * @errors
 */
int wiredtiger_pack_uint(WT_PACK_STREAM *ps, uint64_t u);

/*!
 * Unpack an item from a packing stream.
 *
 * @param ps the packing stream handle
 * @param item an item to unpack
 * @errors
 */
int wiredtiger_unpack_item(WT_PACK_STREAM *ps, WT_ITEM *item);

/*!
 * Unpack a signed integer from a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] ip the unpacked signed integer
 * @errors
 */
int wiredtiger_unpack_int(WT_PACK_STREAM *ps, int64_t *ip);

/*!
 * Unpack a string from a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] sp the unpacked string
 * @errors
 */
int wiredtiger_unpack_str(WT_PACK_STREAM *ps, const char **sp);

/*!
 * Unpack an unsigned integer from a packing stream.
 *
 * @param ps the packing stream handle
 * @param[out] up the unpacked unsigned integer
 * @errors
 */
int wiredtiger_unpack_uint(WT_PACK_STREAM *ps, uint64_t *up);
/*! @} */

/*!
 * @name Configuration strings
 * @{
 */

/*!
 * The configuration information returned by the WiredTiger configuration
 * parsing functions in the WT_EXTENSION_API and the public API.
 */
struct __wt_config_item {
	/*!
	 * The value of a configuration string.
	 *
	 * Regardless of the type of the configuration string (boolean, int,
	 * list or string), the \c str field will reference the value of the
	 * configuration string.
	 *
	 * The bytes referenced by \c str are <b>not</b> nul-terminated,
	 * use the \c len field instead of a terminating nul byte.
	 */
	const char *str;

	/*! The number of bytes in the value referenced by \c str. */
	size_t len;

	/*!
	 * The numeric value of a configuration boolean or integer.
	 *
	 * If the configuration string's value is "true" or "false", the
	 * \c val field will be set to 1 (true), or 0 (false).
	 *
	 * If the configuration string can be legally interpreted as an integer,
	 * using the strtoll function rules as specified in ISO/IEC 9899:1990
	 * ("ISO C90"), that integer will be stored in the \c val field.
	 */
	int64_t val;

	/*! Permitted values of the \c type field. */
	enum {
		/*! A string value with quotes stripped. */
		WT_CONFIG_ITEM_STRING,
		/*! A boolean literal ("true" or "false"). */
		WT_CONFIG_ITEM_BOOL,
		/*! An unquoted identifier: a string value without quotes. */
		WT_CONFIG_ITEM_ID,
		/*! A numeric value. */
		WT_CONFIG_ITEM_NUM,
		/*! A nested structure or list, including brackets. */
		WT_CONFIG_ITEM_STRUCT
	}
	/*!
	 * The type of value determined by the parser.  In all cases,
	 * the \c str and \c len fields are set.
	 */
	type;
};

#if !defined(SWIG) && !defined(DOXYGEN)
/*!
 * Validate a configuration string for a WiredTiger API.
 * This API is outside the scope of a WiredTiger connection handle, since
 * applications may need to validate configuration strings prior to calling
 * ::wiredtiger_open.
 * @param session the session handle (may be \c NULL if the database not yet
 * opened).
 * @param errhandler An error handler (used if \c session is \c NULL; if both
 * \c session and \c errhandler are \c NULL, error messages will be written to
 * stderr).
 * @param name the WiredTiger function or method to validate.
 * @param config the configuration string being parsed.
 * @returns zero for success, non-zero to indicate an error.
 *
 * @snippet ex_all.c Validate a configuration string
 */
int wiredtiger_config_validate(WT_SESSION *session,
    WT_EVENT_HANDLER *errhandler, const char *name, const char *config);
#endif

/*!
 * Create a handle that can be used to parse or create configuration strings
 * compatible with WiredTiger APIs.
 * This API is outside the scope of a WiredTiger connection handle, since
 * applications may need to generate configuration strings prior to calling
 * ::wiredtiger_open.
 * @param session the session handle to be used for error reporting (if NULL,
 *	error messages will be written to stderr).
 * @param config the configuration string being parsed. The string must
 *	remain valid for the lifetime of the parser handle.
 * @param len the number of valid bytes in \c config
 * @param[out] config_parserp A pointer to the newly opened handle
 * @errors
 *
 * @snippet ex_config_parse.c Create a configuration parser
 */
int wiredtiger_config_parser_open(WT_SESSION *session,
    const char *config, size_t len, WT_CONFIG_PARSER **config_parserp);

/*!
 * A handle that can be used to search and traverse configuration strings
 * compatible with WiredTiger APIs.
 * To parse the contents of a list or nested configuration string use a new
 * configuration parser handle based on the content of the ::WT_CONFIG_ITEM
 * retrieved from the parent configuration string.
 *
 * @section config_parse_examples Configuration String Parsing examples
 *
 * This could be used in C to create a configuration parser as follows:
 *
 * @snippet ex_config_parse.c Create a configuration parser
 *
 * Once the parser has been created the content can be queried directly:
 *
 * @snippet ex_config_parse.c get
 *
 * Or the content can be traversed linearly:
 *
 * @snippet ex_config_parse.c next
 *
 * Nested configuration values can be queried using a shorthand notation:
 *
 * @snippet ex_config_parse.c nested get
 *
 * Nested configuration values can be traversed using multiple
 * ::WT_CONFIG_PARSER handles:
 *
 * @snippet ex_config_parse.c nested traverse
 */
struct __wt_config_parser {

	/*!
	 * Close the configuration scanner releasing any resources.
	 *
	 * @param config_parser the configuration parser handle
	 * @errors
	 *
	 */
	int __F(close)(WT_CONFIG_PARSER *config_parser);

	/*!
	 * Return the next key/value pair.
	 *
	 * When iteration would pass the end of the configuration string
	 * ::WT_NOTFOUND will be returned.
	 *
	 * If an item has no explicitly assigned value, the item will be
	 * returned in \c key and the \c value will be set to the boolean
	 * \c "true" value.
	 *
	 * @param config_parser the configuration parser handle
	 * @param key the returned key
	 * @param value the returned value
	 * @errors
	 *
	 */
	int __F(next)(WT_CONFIG_PARSER *config_parser,
	    WT_CONFIG_ITEM *key, WT_CONFIG_ITEM *value);

	/*!
	 * Return the value of an item in the configuration string.
	 *
	 * @param config_parser the configuration parser handle
	 * @param key configuration key string
	 * @param value the returned value
	 * @errors
	 *
	 */
	int __F(get)(WT_CONFIG_PARSER *config_parser,
	    const char *key, WT_CONFIG_ITEM *value);
};

#endif /* !defined(SWIG) */
/*! @} */

/*!
 * Get version information.
 *
 * @snippet ex_all.c Get the WiredTiger library version #1
 * @snippet ex_all.c Get the WiredTiger library version #2
 *
 * @param majorp a location where the major version number is returned
 * @param minorp a location where the minor version number is returned
 * @param patchp a location where the patch version number is returned
 * @returns a string representation of the version
 */
const char *wiredtiger_version(int *majorp, int *minorp, int *patchp);

/*******************************************
 * Error returns
 *******************************************/
/*!
 * @anchor error_returns
 * @name Error returns
 * Most functions and methods in WiredTiger return an integer code indicating
 * whether the operation succeeded or failed.  A return of zero indicates
 * success, all non-zero return values indicate some kind of failure.
 *
 * WiredTiger reserves all values from -31,800 to -31,999 as possible error
 * return values.  WiredTiger may also return C99/POSIX error codes such as
 * \c ENOMEM, \c EINVAL and \c ENOTSUP, with the usual meanings.
 *
 * The following are all of the WiredTiger-specific error returns:
 * @{
 */
/*
 * DO NOT EDIT: automatically built by dist/api_err.py.
 * Error return section: BEGIN
 */
/*!
 * Conflict between concurrent operations.
 * This error is generated when an operation cannot be completed due to a
 * conflict with concurrent operations.  The operation may be retried; if a
 * transaction is in progress, it should be rolled back and the operation
 * retried in a new transaction.
 */
#define	WT_ROLLBACK	-31800
/*!
 * Attempt to insert an existing key.
 * This error is generated when the application attempts to insert a record with
 * the same key as an existing record without the 'overwrite' configuration to
 * WT_SESSION::open_cursor.
 */
#define	WT_DUPLICATE_KEY	-31801
/*!
 * Non-specific WiredTiger error.
 * This error is returned when an error is not covered by a specific error
 * return.
 */
#define	WT_ERROR	-31802
/*!
 * Item not found.
 * This error indicates an operation did not find a value to return.  This
 * includes cursor search and other operations where no record matched the
 * cursor's search key such as WT_CURSOR::update or WT_CURSOR::remove.
 */
#define	WT_NOTFOUND	-31803
/*!
 * WiredTiger library panic.
 * This error indicates an underlying problem that requires the application exit
 * and restart. The application can exit immediately when \c WT_PANIC is
 * returned from a WiredTiger interface, no further WiredTiger calls are
 * required.
 */
#define	WT_PANIC	-31804
/*! @cond internal */
/*! Restart the operation (internal). */
#define	WT_RESTART	-31805
/*! @endcond */
/*!
 * Recovery must be run to continue.
 * This error is generated when wiredtiger_open is configured to return an error
 * if recovery is required to use the database.
 */
#define	WT_RUN_RECOVERY	-31806
/*
 * Error return section: END
 * DO NOT EDIT: automatically built by dist/api_err.py.
 */
/*! @} */

#ifndef DOXYGEN
#define	WT_DEADLOCK	WT_ROLLBACK		/* Backward compatibility */
#endif

/*! @} */

/*!
 * @defgroup wt_ext WiredTiger Extension API
 * The functions and interfaces applications use to customize and extend the
 * behavior of WiredTiger.
 * @{
 */

/*******************************************
 * Forward structure declarations for the extension API
 *******************************************/
struct __wt_config_arg;     typedef struct __wt_config_arg WT_CONFIG_ARG;

/*!
 * The interface implemented by applications to provide custom ordering of
 * records.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_collator.  See @ref custom_collators for more
 * information.
 *
 * @snippet ex_extending.c add collator nocase
 *
 * @snippet ex_extending.c add collator prefix10
 */
struct __wt_collator {
	/*!
	 * Callback to compare keys.
	 *
	 * @param[out] cmp set to -1 if <code>key1 < key2</code>,
	 * 	0 if <code>key1 == key2</code>,
	 * 	1 if <code>key1 > key2</code>.
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet ex_all.c Implement WT_COLLATOR
	 *
	 * @snippet ex_extending.c case insensitive comparator
	 *
	 * @snippet ex_extending.c n character comparator
	 */
	int (*compare)(WT_COLLATOR *collator, WT_SESSION *session,
	    const WT_ITEM *key1, const WT_ITEM *key2, int *cmp);

	/*!
	 * If non-NULL, this callback is called to customize the collator
	 * for each data source.  If the callback returns a non-NULL
	 * collator, that instance is used instead of this one for all
	 * comparisons.
	 */
	int (*customize)(WT_COLLATOR *collator, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ITEM *passcfg, WT_COLLATOR **customp);

	/*!
	 * If non-NULL a callback performed when the data source is closed
	 * for customized extractors otherwise when the database is closed.
	 *
	 * The WT_COLLATOR::terminate callback is intended to allow cleanup,
	 * the handle will not be subsequently accessed by WiredTiger.
	 */
	int (*terminate)(WT_COLLATOR *collator, WT_SESSION *session);
};

/*!
 * The interface implemented by applications to provide custom compression.
 *
 * Compressors must implement the WT_COMPRESSOR interface: the
 * WT_COMPRESSOR::compress and WT_COMPRESSOR::decompress callbacks must be
 * specified, and WT_COMPRESSOR::pre_size is optional.  To build your own
 * compressor, use one of the compressors in \c ext/compressors as a template:
 * \c ext/nop_compress is a simple compressor that passes through data
 * unchanged, and is a reasonable starting point.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_compressor.
 *
 * @snippet nop_compress.c WT_COMPRESSOR initialization structure
 * @snippet nop_compress.c WT_COMPRESSOR initialization function
 */
struct __wt_compressor {
	/*!
	 * Callback to compress a chunk of data.
	 *
	 * WT_COMPRESSOR::compress takes a source buffer and a destination
	 * buffer, by default of the same size.  If the callback can compress
	 * the buffer to a smaller size in the destination, it does so, sets
	 * the \c compression_failed return to 0 and returns 0.  If compression
	 * does not produce a smaller result, the callback sets the
	 * \c compression_failed return to 1 and returns 0. If another
	 * error occurs, it returns an errno or WiredTiger error code.
	 *
	 * On entry, \c src will point to memory, with the length of the memory
	 * in \c src_len.  After successful completion, the callback should
	 * return \c 0 and set \c result_lenp to the number of bytes required
	 * for the compressed representation.
	 *
	 * On entry, \c dst points to the destination buffer with a length
	 * of \c dst_len.  If the WT_COMPRESSOR::pre_size method is specified,
	 * the destination buffer will be at least the size returned by that
	 * method; otherwise, the destination buffer will be at least as large
	 * as the length of the data to compress.
	 *
	 * If compression would not shrink the data or the \c dst buffer is not
	 * large enough to hold the compressed data, the callback should set
	 * \c compression_failed to a non-zero value and return 0.
	 *
	 * @param[in] src the data to compress
	 * @param[in] src_len the length of the data to compress
	 * @param[in] dst the destination buffer
	 * @param[in] dst_len the length of the destination buffer
	 * @param[out] result_lenp the length of the compressed data
	 * @param[out] compression_failed non-zero if compression did not
	 * decrease the length of the data (compression may not have completed)
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet nop_compress.c WT_COMPRESSOR compress
	 */
	int (*compress)(WT_COMPRESSOR *compressor, WT_SESSION *session,
	    uint8_t *src, size_t src_len,
	    uint8_t *dst, size_t dst_len,
	    size_t *result_lenp, int *compression_failed);

	/*!
	 * Callback to compress a list of byte strings.
	 *
	 * WT_COMPRESSOR::compress_raw gives applications fine-grained control
	 * over disk block size when writing row-store or variable-length
	 * column-store pages.  Where this level of control is not required by
	 * the underlying storage device, set the WT_COMPRESSOR::compress_raw
	 * callback to \c NULL and WiredTiger will internally split each page
	 * into blocks, each block then compressed by WT_COMPRESSOR::compress.
	 *
	 * WT_COMPRESSOR::compress_raw takes a source buffer and an array of
	 * 0-based offsets of byte strings in that buffer.  The callback then
	 * encodes none, some or all of the byte strings and copies the encoded
	 * representation into a destination buffer.  The callback returns the
	 * number of byte strings encoded and the bytes needed for the encoded
	 * representation.   The encoded representation has header information
	 * prepended and is written as a block to the underlying file object.
	 *
	 * On entry, \c page_max is the configured maximum size for objects of
	 * this type.  (This value is provided for convenience, and will be
	 * either the \c internal_page_max or \c leaf_page_max value specified
	 * to WT_SESSION::create when the object was created.)
	 *
	 * On entry, \c split_pct is the configured Btree page split size for
	 * this object.  (This value is provided for convenience, and will be
	 * the \c split_pct value specified to WT_SESSION::create when the
	 * object was created.)
	 *
	 * On entry, \c extra is a count of additional bytes that will be added
	 * to the encoded representation before it is written.  In other words,
	 * if the target write size is 8KB, the returned encoded representation
	 * should be less than or equal to (8KB - \c extra).  The method does
	 * not need to skip bytes in the destination buffer based on \c extra,
	 * the method should only use \c extra to decide how many bytes to store
	 * into the destination buffer for its ideal block size.
	 *
	 * On entry, \c src points to the source buffer; \c offsets is an array
	 * of \c slots 0-based offsets into \c src, where each offset is the
	 * start of a byte string, except for the last offset, which is the
	 * offset of the first byte past the end of the last byte string.  (In
	 * other words, <code>offsets[0]</code> will be 0, the offset of the
	 * first byte of the first byte string in \c src, and
	 * <code>offsets[slots]</code> is the total length of all of the byte
	 * strings in the \c src buffer.)
	 *
	 * On entry, \c dst points to the destination buffer with a length
	 * of \c dst_len.  If the WT_COMPRESSOR::pre_size method is specified,
	 * the destination buffer will be at least the size returned by that
	 * method; otherwise, the destination buffer will be at least as large
	 * as the length of the data to compress.
	 *
	 * After successful completion, the callback should return \c 0, and
	 * set \c result_slotsp to the number of byte strings encoded and
	 * \c result_lenp to the bytes needed for the encoded representation.
	 *
	 * There is no requirement the callback encode any or all of the byte
	 * strings passed by WiredTiger.  If the callback does not encode any
	 * of the byte strings and compression should not be retried, the
	 * callback should set \c result_slotsp to 0.
	 *
	 * If the callback does not encode any of the byte strings and
	 * compression should be retried with additional byte strings, the
	 * callback must return \c EAGAIN.  In that case, WiredTiger will
	 * accumulate more rows and repeat the call.
	 *
	 * If there are no more rows to accumulate or the callback indicates
	 * that it cannot be retried, WiredTiger writes the remaining rows
	 * using \c WT_COMPRESSOR::compress.
	 *
	 * On entry, \c final is zero if there are more rows to be written as
	 * part of this page (if there will be additional data provided to the
	 * callback), and non-zero if there are no more rows to be written as
	 * part of this page.  If \c final is set and the callback fails to
	 * encode any rows, WiredTiger writes the remaining rows without further
	 * calls to the callback.  If \c final is set and the callback encodes
	 * any number of rows, WiredTiger continues to call the callback until
	 * all of the rows are encoded or the callback fails to encode any rows.
	 *
	 * The WT_COMPRESSOR::compress_raw callback is intended for applications
	 * wanting to create disk blocks in specific sizes.
	 * WT_COMPRESSOR::compress_raw is not a replacement for
	 * WT_COMPRESSOR::compress: objects which WT_COMPRESSOR::compress_raw
	 * cannot handle (for example, overflow key or value items), or which
	 * WT_COMPRESSOR::compress_raw chooses not to compress for any reason
	 * (for example, if WT_COMPRESSOR::compress_raw callback chooses not to
	 * compress a small number of rows, but the page being written has no
	 * more rows to accumulate), will be passed to WT_COMPRESSOR::compress.
	 *
	 * The WT_COMPRESSOR::compress_raw callback is only called for objects
	 * where it is applicable, that is, for row-store and variable-length
	 * column-store objects, where both row-store key prefix compression
	 * and row-store and variable-length column-store dictionary compression
	 * are \b not configured.  When WT_COMPRESSOR::compress_raw is not
	 * applicable, the WT_COMPRESSOR::compress callback is used instead.
	 *
	 * @param[in] page_max the configured maximum page size for this object
	 * @param[in] split_pct the configured page split size for this object
	 * @param[in] extra the count of the additional bytes
	 * @param[in] src the data to compress
	 * @param[in] offsets the byte offsets of the byte strings in src
	 * @param[in] slots the number of entries in offsets
	 * @param[in] dst the destination buffer
	 * @param[in] dst_len the length of the destination buffer
	 * @param[in] final non-zero if there are no more rows to accumulate
	 * @param[out] result_lenp the length of the compressed data
	 * @param[out] result_slotsp the number of byte offsets taken
	 * @returns zero for success, non-zero to indicate an error.
	 */
	int (*compress_raw)(WT_COMPRESSOR *compressor, WT_SESSION *session,
	    size_t page_max, int split_pct, size_t extra,
	    uint8_t *src, uint32_t *offsets, uint32_t slots,
	    uint8_t *dst, size_t dst_len,
	    int final,
	    size_t *result_lenp, uint32_t *result_slotsp);

	/*!
	 * Callback to decompress a chunk of data.
	 *
	 * WT_COMPRESSOR::decompress takes a source buffer and a destination
	 * buffer.  The contents are switched from \c compress: the
	 * source buffer is the compressed value, and the destination buffer is
	 * sized to be the original size.  If the callback successfully
	 * decompresses the source buffer to the destination buffer, it returns
	 * 0.  If an error occurs, it returns an errno or WiredTiger error code.
	 * The source buffer that WT_COMPRESSOR::decompress takes may have a
	 * size that is rounded up from the size originally produced by
	 * WT_COMPRESSOR::compress, with the remainder of the buffer set to
	 * zeroes. Most compressors do not care about this difference if the
	 * size to be decompressed can be implicitly discovered from the
	 * compressed data.  If your compressor cares, you may need to allocate
	 * space for, and store, the actual size in the compressed buffer.  See
	 * the source code for the included snappy compressor for an example.
	 *
	 * On entry, \c src will point to memory, with the length of the memory
	 * in \c src_len.  After successful completion, the callback should
	 * return \c 0 and set \c result_lenp to the number of bytes required
	 * for the decompressed representation.
	 *
	 * If the \c dst buffer is not big enough to hold the decompressed
	 * data, the callback should return an error.
	 *
	 * @param[in] src the data to decompress
	 * @param[in] src_len the length of the data to decompress
	 * @param[in] dst the destination buffer
	 * @param[in] dst_len the length of the destination buffer
	 * @param[out] result_lenp the length of the decompressed data
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet nop_compress.c WT_COMPRESSOR decompress
	 */
	int (*decompress)(WT_COMPRESSOR *compressor, WT_SESSION *session,
	    uint8_t *src, size_t src_len,
	    uint8_t *dst, size_t dst_len,
	    size_t *result_lenp);

	/*!
	 * Callback to size a destination buffer for compression
	 *
	 * WT_COMPRESSOR::pre_size is an optional callback that, given the
	 * source buffer and size, produces the size of the destination buffer
	 * to be given to WT_COMPRESSOR::compress.  This is useful for
	 * compressors that assume that the output buffer is sized for the
	 * worst case and thus no overrun checks are made.  If your compressor
	 * works like this, WT_COMPRESSOR::pre_size will need to be defined.
	 * See the source code for the snappy compressor for an example.
	 * However, if your compressor detects and avoids overruns against its
	 * target buffer, you will not need to define WT_COMPRESSOR::pre_size.
	 * When WT_COMPRESSOR::pre_size is set to NULL, the destination buffer
	 * is sized the same as the source buffer.  This is always sufficient,
	 * since a compression result that is larger than the source buffer is
	 * discarded by WiredTiger.
	 *
	 * If not NULL, this callback is called before each call to
	 * WT_COMPRESSOR::compress to determine the size of the destination
	 * buffer to provide.  If the callback is NULL, the destination
	 * buffer will be the same size as the source buffer.
	 *
	 * The callback should set \c result_lenp to a suitable buffer size
	 * for compression, typically the maximum length required by
	 * WT_COMPRESSOR::compress.
	 *
	 * This callback function is for compressors that require an output
	 * buffer larger than the source buffer (for example, that do not
	 * check for buffer overflow during compression).
	 *
	 * @param[in] src the data to compress
	 * @param[in] src_len the length of the data to compress
	 * @param[out] result_lenp the required destination buffer size
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet nop_compress.c WT_COMPRESSOR presize
	 */
	int (*pre_size)(WT_COMPRESSOR *compressor, WT_SESSION *session,
	    uint8_t *src, size_t src_len, size_t *result_lenp);

	/*!
	 * If non-NULL, a callback performed when the database is closed.
	 *
	 * The WT_COMPRESSOR::terminate callback is intended to allow cleanup,
	 * the handle will not be subsequently accessed by WiredTiger.
	 *
	 * @snippet nop_compress.c WT_COMPRESSOR terminate
	 */
	int (*terminate)(WT_COMPRESSOR *compressor, WT_SESSION *session);
};

/*!
 * Applications can extend WiredTiger by providing new implementations of the
 * WT_DATA_SOURCE class.  Each data source supports a different URI scheme for
 * data sources to WT_SESSION::create, WT_SESSION::open_cursor and related
 * methods.  See @ref custom_data_sources for more information.
 *
 * <b>Thread safety:</b> WiredTiger may invoke methods on the WT_DATA_SOURCE
 * interface from multiple threads concurrently.  It is the responsibility of
 * the implementation to protect any shared data.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_data_source.
 *
 * @snippet ex_data_source.c WT_DATA_SOURCE register
 */
struct __wt_data_source {
	/*!
	 * Callback to create a new object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE create
	 */
	int (*create)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ARG *config);

	/*!
	 * Callback to compact an object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE compact
	 */
	int (*compact)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ARG *config);

	/*!
	 * Callback to drop an object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE drop
	 */
	int (*drop)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ARG *config);

	/*!
	 * Callback to initialize a cursor.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE open_cursor
	 */
	int (*open_cursor)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ARG *config, WT_CURSOR **new_cursor);

	/*!
	 * Callback to rename an object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE rename
	 */
	int (*rename)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, const char *newuri, WT_CONFIG_ARG *config);

	/*!
	 * Callback to salvage an object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE salvage
	 */
	int (*salvage)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ARG *config);

	/*!
	 * Callback to truncate an object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE truncate
	 */
	int (*truncate)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ARG *config);

	/*!
	 * Callback to truncate a range of an object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE range truncate
	 */
	int (*range_truncate)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    WT_CURSOR *start, WT_CURSOR *stop);

	/*!
	 * Callback to verify an object.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE verify
	 */
	int (*verify)(WT_DATA_SOURCE *dsrc, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ARG *config);

	/*!
	 * Callback to checkpoint the database.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE checkpoint
	 */
	int (*checkpoint)(
	    WT_DATA_SOURCE *dsrc, WT_SESSION *session, WT_CONFIG_ARG *config);

	/*!
	 * If non-NULL, a callback performed when the database is closed.
	 *
	 * The WT_DATA_SOURCE::terminate callback is intended to allow cleanup,
	 * the handle will not be subsequently accessed by WiredTiger.
	 *
	 * @snippet ex_data_source.c WT_DATA_SOURCE terminate
	 */
	int (*terminate)(WT_DATA_SOURCE *dsrc, WT_SESSION *session);
};

/*!
 * The interface implemented by applications to provide custom encryption.
 *
 * Encryptors must implement the WT_ENCRYPTOR interface: the
 * WT_ENCRYPTOR::encrypt, WT_ENCRYPTOR::decrypt and WT_ENCRYPTOR::sizing
 * callbacks must be specified, WT_ENCRYPTOR::customize and
 * WT_ENCRYPTOR::terminate are optional.  To build your own encryptor, use
 * one of the encryptors in \c ext/encryptors as a template:
 * \c ext/encryptors/nop_encrypt is a simple encryptor that passes through
 * data unchanged, and is a reasonable starting point;
 * \c ext/encryptors/rotn_encrypt is an encryptor implementing
 * a simple rotation cipher, it shows the use of \c keyid, \c secretkey,
 * and implements the WT_ENCRYPTOR::customize and
 * WT_ENCRYPTOR::terminate callbacks.
 *
 * Applications register their implementation with WiredTiger by calling
 * WT_CONNECTION::add_encryptor.
 *
 * @snippet nop_encrypt.c WT_ENCRYPTOR initialization structure
 * @snippet nop_encrypt.c WT_ENCRYPTOR initialization function
 */
struct __wt_encryptor {
	/*!
	 * Callback to encrypt a chunk of data.
	 *
	 * WT_ENCRYPTOR::encrypt takes a source buffer and a destination
	 * buffer.  The callback encrypts the source buffer (plain text)
	 * into the destination buffer.
	 *
	 * On entry, \c src will point to memory, with the length of the memory
	 * in \c src_len.  After successful completion, the callback should
	 * return \c 0 and set \c result_lenp to the number of bytes required
	 * for the encrypted representation.
	 *
	 * On entry, \c dst points to the destination buffer with a length
	 * of \c dst_len.  The destination buffer will be at least src_len
	 * plus the size returned by that WT_ENCRYPT::sizing.
	 *
	 * This callback cannot be NULL.
	 *
	 * @param[in] src the data to encrypt
	 * @param[in] src_len the length of the data to encrypt
	 * @param[in] dst the destination buffer
	 * @param[in] dst_len the length of the destination buffer
	 * @param[out] result_lenp the length of the encrypted data
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet nop_encrypt.c WT_ENCRYPTOR encrypt
	 */
	int (*encrypt)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
	    uint8_t *src, size_t src_len,
	    uint8_t *dst, size_t dst_len,
	    size_t *result_lenp);

	/*!
	 * Callback to decrypt a chunk of data.
	 *
	 * WT_ENCRYPTOR::decrypt takes a source buffer and a destination
	 * buffer.  The contents are switched from \c encrypt: the
	 * source buffer is the encrypted value, and the destination buffer is
	 * sized to be the original size.  If the callback successfully
	 * decrypts the source buffer to the destination buffer, it returns
	 * 0.  If an error occurs, it returns an errno or WiredTiger error code.
	 *
	 * On entry, \c src will point to memory, with the length of the memory
	 * in \c src_len.  After successful completion, the callback should
	 * return \c 0 and set \c result_lenp to the number of bytes required
	 * for the decrypted representation.
	 *
	 * If the \c dst buffer is not big enough to hold the decrypted
	 * data, the callback should return an error.
	 *
	 * This callback cannot be NULL.
	 *
	 * @param[in] src the data to decrypt
	 * @param[in] src_len the length of the data to decrypt
	 * @param[in] dst the destination buffer
	 * @param[in] dst_len the length of the destination buffer
	 * @param[out] result_lenp the length of the decrypted data
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet nop_encrypt.c WT_ENCRYPTOR decrypt
	 */
	int (*decrypt)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
	    uint8_t *src, size_t src_len,
	    uint8_t *dst, size_t dst_len,
	    size_t *result_lenp);

	/*!
	 * Callback to size a destination buffer for encryption.
	 *
	 * WT_ENCRYPTOR::sizing is an callback that returns the number
	 * of additional bytes that is needed when encrypting a
	 * text buffer.  This is always necessary, since encryptors
	 * typically generate encrypted text that is larger than the
	 * plain text input. Without such a call, WiredTiger would
	 * have no way to know the worst case for the encrypted buffer size.
	 * The WiredTiger encryption infrastructure assumes that
	 * buffer sizing is not dependent on the number of bytes
	 * of input, that there is a one to one relationship in number
	 * of bytes needed between input and output.
	 *
	 * This callback cannot be NULL.
	 *
	 * The callback should set \c expansion_constantp to the additional
	 * number of bytes needed.
	 *
	 * @param[out] expansion_constantp the additional number of bytes needed
	 *             when encrypting.
	 * @returns zero for success, non-zero to indicate an error.
	 *
	 * @snippet nop_encrypt.c WT_ENCRYPTOR sizing
	 */
	int (*sizing)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
	    size_t *expansion_constantp);

	/*!
	 * If non-NULL, this callback is called to customize the encryptor.
	 * The customize function is called whenever a keyid is used for the
	 * first time with this encryptor, whether it be in
	 * the ::wiredtiger_open call or the WT_SESSION::create
	 * call. This gives the algorithm an
	 * opportunity to retrieve and save keys in a customized encryptor.
	 * If the callback returns a non-NULL encryptor, that instance
	 * is used instead of this one for any callbacks.
	 *
	 * @param[in] encrypt_config the "encryption" portion of the
	 *            configuration from the wiredtiger_open or
	 *            WT_SESSION::create call
	 * @param[out] customp the new modified encryptor, or NULL.
	 * @returns zero for success, non-zero to indicate an error.
	 */
	int (*customize)(WT_ENCRYPTOR *encryptor, WT_SESSION *session,
	    WT_CONFIG_ARG *encrypt_config, WT_ENCRYPTOR **customp);

	/*!
	 * If non-NULL, a callback performed when the database is closed.
	 * It is called for each encryptor that was added using
	 * WT_CONNECTION::add_encryptor or returned by the
	 * WT_ENCRYPTOR::customize callback.
	 *
	 * The WT_ENCRYPTOR::terminate callback is intended to allow cleanup,
	 * the handle will not be subsequently accessed by WiredTiger.
	 *
	 * @snippet nop_encrypt.c WT_ENCRYPTOR terminate
	 */
	int (*terminate)(WT_ENCRYPTOR *encryptor, WT_SESSION *session);
};

/*!
 * The interface implemented by applications to provide custom extraction of
 * index keys or column group values.
 *
 * Applications register implementations with WiredTiger by calling
 * WT_CONNECTION::add_extractor.  See @ref custom_extractors for more
 * information.
 *
 * @snippet ex_all.c WT_EXTRACTOR register
 */
struct __wt_extractor {
	/*!
	 * Callback to extract a value for an index or column group.
	 *
	 * @errors
	 *
	 * @snippet ex_all.c WT_EXTRACTOR
	 *
	 * @param extractor the WT_EXTRACTOR implementation
	 * @param session the current WiredTiger session
	 * @param key the table key in raw format, see @ref cursor_raw for
	 *	details
	 * @param value the table value in raw format, see @ref cursor_raw for
	 *	details
	 * @param[out] result_cursor the method should call WT_CURSOR::set_key
	 *	and WT_CURSOR::insert on this cursor to return a key.  The \c
	 *	key_format of the cursor will match that passed to
	 *	WT_SESSION::create for the index.  Multiple index keys can be
	 *	created for each record by calling WT_CURSOR::insert multiple
	 *	times.
	 */
	int (*extract)(WT_EXTRACTOR *extractor, WT_SESSION *session,
	    const WT_ITEM *key, const WT_ITEM *value,
	    WT_CURSOR *result_cursor);

	/*!
	 * If non-NULL, this callback is called to customize the extractor for
	 * each index.  If the callback returns a non-NULL extractor, that
	 * instance is used instead of this one for all comparisons.
	 */
	int (*customize)(WT_EXTRACTOR *extractor, WT_SESSION *session,
	    const char *uri, WT_CONFIG_ITEM *appcfg, WT_EXTRACTOR **customp);

	/*!
	 * If non-NULL a callback performed when the index or column group
	 * is closed for customized extractors otherwise when the database
	 * is closed.
	 *
	 * The WT_EXTRACTOR::terminate callback is intended to allow cleanup,
	 * the handle will not be subsequently accessed by WiredTiger.
	 */
	int (*terminate)(WT_EXTRACTOR *extractor, WT_SESSION *session);
};

/*!
 * Entry point to an extension, called when the extension is loaded.
 *
 * @param connection the connection handle
 * @param config the config information passed to WT_CONNECTION::load_extension
 * @errors
 */
extern int wiredtiger_extension_init(
    WT_CONNECTION *connection, WT_CONFIG_ARG *config);

/*!
 * Optional cleanup function for an extension, called during
 * WT_CONNECTION::close.
 *
 * @param connection the connection handle
 * @errors
 */
extern int wiredtiger_extension_terminate(WT_CONNECTION *connection);

/*! @} */

/*******************************************
 * Statistic reference.
 *******************************************/
/*!
 * @addtogroup wt
 * @{
 */
/*
 * DO NOT EDIT: automatically built by dist/api_stat.py.
 * Statistics section: BEGIN
 */

/*!
 * @name Connection statistics
 * @anchor statistics_keys
 * @anchor statistics_conn
 * Statistics are accessed through cursors with \c "statistics:" URIs.
 * Individual statistics can be queried through the cursor using the following
 * keys.  See @ref data_statistics for more information.
 * @{
 */
/*! async: number of allocation state races */
#define	WT_STAT_CONN_ASYNC_ALLOC_RACE			1000
/*! async: number of operation slots viewed for allocation */
#define	WT_STAT_CONN_ASYNC_ALLOC_VIEW			1001
/*! async: current work queue length */
#define	WT_STAT_CONN_ASYNC_CUR_QUEUE			1002
/*! async: number of flush calls */
#define	WT_STAT_CONN_ASYNC_FLUSH			1003
/*! async: number of times operation allocation failed */
#define	WT_STAT_CONN_ASYNC_FULL				1004
/*! async: maximum work queue length */
#define	WT_STAT_CONN_ASYNC_MAX_QUEUE			1005
/*! async: number of times worker found no work */
#define	WT_STAT_CONN_ASYNC_NOWORK			1006
/*! async: total allocations */
#define	WT_STAT_CONN_ASYNC_OP_ALLOC			1007
/*! async: total compact calls */
#define	WT_STAT_CONN_ASYNC_OP_COMPACT			1008
/*! async: total insert calls */
#define	WT_STAT_CONN_ASYNC_OP_INSERT			1009
/*! async: total remove calls */
#define	WT_STAT_CONN_ASYNC_OP_REMOVE			1010
/*! async: total search calls */
#define	WT_STAT_CONN_ASYNC_OP_SEARCH			1011
/*! async: total update calls */
#define	WT_STAT_CONN_ASYNC_OP_UPDATE			1012
/*! block-manager: mapped bytes read */
#define	WT_STAT_CONN_BLOCK_BYTE_MAP_READ		1013
/*! block-manager: bytes read */
#define	WT_STAT_CONN_BLOCK_BYTE_READ			1014
/*! block-manager: bytes written */
#define	WT_STAT_CONN_BLOCK_BYTE_WRITE			1015
/*! block-manager: mapped blocks read */
#define	WT_STAT_CONN_BLOCK_MAP_READ			1016
/*! block-manager: blocks pre-loaded */
#define	WT_STAT_CONN_BLOCK_PRELOAD			1017
/*! block-manager: blocks read */
#define	WT_STAT_CONN_BLOCK_READ				1018
/*! block-manager: blocks written */
#define	WT_STAT_CONN_BLOCK_WRITE			1019
/*! cache: tracked dirty bytes in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_DIRTY			1020
/*! cache: tracked bytes belonging to internal pages in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_INTERNAL		1021
/*! cache: bytes currently in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_INUSE			1022
/*! cache: tracked bytes belonging to leaf pages in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_LEAF			1023
/*! cache: maximum bytes configured */
#define	WT_STAT_CONN_CACHE_BYTES_MAX			1024
/*! cache: tracked bytes belonging to overflow pages in the cache */
#define	WT_STAT_CONN_CACHE_BYTES_OVERFLOW		1025
/*! cache: bytes read into cache */
#define	WT_STAT_CONN_CACHE_BYTES_READ			1026
/*! cache: bytes written from cache */
#define	WT_STAT_CONN_CACHE_BYTES_WRITE			1027
/*! cache: pages evicted by application threads */
#define	WT_STAT_CONN_CACHE_EVICTION_APP			1028
/*! cache: checkpoint blocked page eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_CHECKPOINT		1029
/*! cache: unmodified pages evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_CLEAN		1030
/*! cache: page split during eviction deepened the tree */
#define	WT_STAT_CONN_CACHE_EVICTION_DEEPEN		1031
/*! cache: modified pages evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_DIRTY		1032
/*! cache: pages selected for eviction unable to be evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_FAIL		1033
/*! cache: pages evicted because they exceeded the in-memory maximum */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE		1034
/*! cache: pages evicted because they had chains of deleted items */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_DELETE	1035
/*! cache: failed eviction of pages that exceeded the in-memory maximum */
#define	WT_STAT_CONN_CACHE_EVICTION_FORCE_FAIL		1036
/*! cache: hazard pointer blocked page eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_HAZARD		1037
/*! cache: internal pages evicted */
#define	WT_STAT_CONN_CACHE_EVICTION_INTERNAL		1038
/*! cache: maximum page size at eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_MAXIMUM_PAGE_SIZE	1039
/*! cache: eviction server candidate queue empty when topping up */
#define	WT_STAT_CONN_CACHE_EVICTION_QUEUE_EMPTY		1040
/*! cache: eviction server candidate queue not empty when topping up */
#define	WT_STAT_CONN_CACHE_EVICTION_QUEUE_NOT_EMPTY	1041
/*! cache: eviction server evicting pages */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_EVICTING	1042
/*! cache: eviction server populating queue, but not evicting pages */
#define	WT_STAT_CONN_CACHE_EVICTION_SERVER_NOT_EVICTING	1043
/*! cache: eviction server unable to reach eviction goal */
#define	WT_STAT_CONN_CACHE_EVICTION_SLOW		1044
/*! cache: pages split during eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_SPLIT		1045
/*! cache: pages walked for eviction */
#define	WT_STAT_CONN_CACHE_EVICTION_WALK		1046
/*! cache: eviction worker thread evicting pages */
#define	WT_STAT_CONN_CACHE_EVICTION_WORKER_EVICTING	1047
/*! cache: in-memory page splits */
#define	WT_STAT_CONN_CACHE_INMEM_SPLIT			1048
/*! cache: percentage overhead */
#define	WT_STAT_CONN_CACHE_OVERHEAD			1049
/*! cache: tracked dirty pages in the cache */
#define	WT_STAT_CONN_CACHE_PAGES_DIRTY			1050
/*! cache: pages currently held in the cache */
#define	WT_STAT_CONN_CACHE_PAGES_INUSE			1051
/*! cache: pages read into cache */
#define	WT_STAT_CONN_CACHE_READ				1052
/*! cache: pages written from cache */
#define	WT_STAT_CONN_CACHE_WRITE			1053
/*! connection: pthread mutex condition wait calls */
#define	WT_STAT_CONN_COND_WAIT				1054
/*! cursor: cursor create calls */
#define	WT_STAT_CONN_CURSOR_CREATE			1055
/*! cursor: cursor insert calls */
#define	WT_STAT_CONN_CURSOR_INSERT			1056
/*! cursor: cursor next calls */
#define	WT_STAT_CONN_CURSOR_NEXT			1057
/*! cursor: cursor prev calls */
#define	WT_STAT_CONN_CURSOR_PREV			1058
/*! cursor: cursor remove calls */
#define	WT_STAT_CONN_CURSOR_REMOVE			1059
/*! cursor: cursor reset calls */
#define	WT_STAT_CONN_CURSOR_RESET			1060
/*! cursor: cursor search calls */
#define	WT_STAT_CONN_CURSOR_SEARCH			1061
/*! cursor: cursor search near calls */
#define	WT_STAT_CONN_CURSOR_SEARCH_NEAR			1062
/*! cursor: cursor update calls */
#define	WT_STAT_CONN_CURSOR_UPDATE			1063
/*! data-handle: connection dhandles swept */
#define	WT_STAT_CONN_DH_CONN_HANDLES			1064
/*! data-handle: connection candidate referenced */
#define	WT_STAT_CONN_DH_CONN_REF			1065
/*! data-handle: connection sweeps */
#define	WT_STAT_CONN_DH_CONN_SWEEPS			1066
/*! data-handle: connection time-of-death sets */
#define	WT_STAT_CONN_DH_CONN_TOD			1067
/*! data-handle: session dhandles swept */
#define	WT_STAT_CONN_DH_SESSION_HANDLES			1068
/*! data-handle: session sweep attempts */
#define	WT_STAT_CONN_DH_SESSION_SWEEPS			1069
/*! connection: files currently open */
#define	WT_STAT_CONN_FILE_OPEN				1070
/*! log: log buffer size increases */
#define	WT_STAT_CONN_LOG_BUFFER_GROW			1071
/*! log: total log buffer size */
#define	WT_STAT_CONN_LOG_BUFFER_SIZE			1072
/*! log: log bytes of payload data */
#define	WT_STAT_CONN_LOG_BYTES_PAYLOAD			1073
/*! log: log bytes written */
#define	WT_STAT_CONN_LOG_BYTES_WRITTEN			1074
/*! log: yields waiting for previous log file close */
#define	WT_STAT_CONN_LOG_CLOSE_YIELDS			1075
/*! log: total size of compressed records */
#define	WT_STAT_CONN_LOG_COMPRESS_LEN			1076
/*! log: total in-memory size of compressed records */
#define	WT_STAT_CONN_LOG_COMPRESS_MEM			1077
/*! log: log records too small to compress */
#define	WT_STAT_CONN_LOG_COMPRESS_SMALL			1078
/*! log: log records not compressed */
#define	WT_STAT_CONN_LOG_COMPRESS_WRITE_FAILS		1079
/*! log: log records compressed */
#define	WT_STAT_CONN_LOG_COMPRESS_WRITES		1080
/*! log: maximum log file size */
#define	WT_STAT_CONN_LOG_MAX_FILESIZE			1081
/*! log: pre-allocated log files prepared */
#define	WT_STAT_CONN_LOG_PREALLOC_FILES			1082
/*! log: number of pre-allocated log files to create */
#define	WT_STAT_CONN_LOG_PREALLOC_MAX			1083
/*! log: pre-allocated log files used */
#define	WT_STAT_CONN_LOG_PREALLOC_USED			1084
/*! log: log release advances write LSN */
#define	WT_STAT_CONN_LOG_RELEASE_WRITE_LSN		1085
/*! log: records processed by log scan */
#define	WT_STAT_CONN_LOG_SCAN_RECORDS			1086
/*! log: log scan records requiring two reads */
#define	WT_STAT_CONN_LOG_SCAN_REREADS			1087
/*! log: log scan operations */
#define	WT_STAT_CONN_LOG_SCANS				1088
/*! log: consolidated slot closures */
#define	WT_STAT_CONN_LOG_SLOT_CLOSES			1089
/*! log: logging bytes consolidated */
#define	WT_STAT_CONN_LOG_SLOT_CONSOLIDATED		1090
/*! log: consolidated slot joins */
#define	WT_STAT_CONN_LOG_SLOT_JOINS			1091
/*! log: consolidated slot join races */
#define	WT_STAT_CONN_LOG_SLOT_RACES			1092
/*! log: slots selected for switching that were unavailable */
#define	WT_STAT_CONN_LOG_SLOT_SWITCH_FAILS		1093
/*! log: record size exceeded maximum */
#define	WT_STAT_CONN_LOG_SLOT_TOOBIG			1094
/*! log: failed to find a slot large enough for record */
#define	WT_STAT_CONN_LOG_SLOT_TOOSMALL			1095
/*! log: consolidated slot join transitions */
#define	WT_STAT_CONN_LOG_SLOT_TRANSITIONS		1096
/*! log: log sync operations */
#define	WT_STAT_CONN_LOG_SYNC				1097
/*! log: log sync_dir operations */
#define	WT_STAT_CONN_LOG_SYNC_DIR			1098
/*! log: log server thread advances write LSN */
#define	WT_STAT_CONN_LOG_WRITE_LSN			1099
/*! log: log write operations */
#define	WT_STAT_CONN_LOG_WRITES				1100
/*! LSM: sleep for LSM checkpoint throttle */
#define	WT_STAT_CONN_LSM_CHECKPOINT_THROTTLE		1101
/*! LSM: sleep for LSM merge throttle */
#define	WT_STAT_CONN_LSM_MERGE_THROTTLE			1102
/*! LSM: rows merged in an LSM tree */
#define	WT_STAT_CONN_LSM_ROWS_MERGED			1103
/*! LSM: application work units currently queued */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_APP			1104
/*! LSM: merge work units currently queued */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_MANAGER		1105
/*! LSM: tree queue hit maximum */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_MAX			1106
/*! LSM: switch work units currently queued */
#define	WT_STAT_CONN_LSM_WORK_QUEUE_SWITCH		1107
/*! LSM: tree maintenance operations scheduled */
#define	WT_STAT_CONN_LSM_WORK_UNITS_CREATED		1108
/*! LSM: tree maintenance operations discarded */
#define	WT_STAT_CONN_LSM_WORK_UNITS_DISCARDED		1109
/*! LSM: tree maintenance operations executed */
#define	WT_STAT_CONN_LSM_WORK_UNITS_DONE		1110
/*! connection: memory allocations */
#define	WT_STAT_CONN_MEMORY_ALLOCATION			1111
/*! connection: memory frees */
#define	WT_STAT_CONN_MEMORY_FREE			1112
/*! connection: memory re-allocations */
#define	WT_STAT_CONN_MEMORY_GROW			1113
/*! thread-yield: page acquire busy blocked */
#define	WT_STAT_CONN_PAGE_BUSY_BLOCKED			1114
/*! thread-yield: page acquire eviction blocked */
#define	WT_STAT_CONN_PAGE_FORCIBLE_EVICT_BLOCKED	1115
/*! thread-yield: page acquire locked blocked */
#define	WT_STAT_CONN_PAGE_LOCKED_BLOCKED		1116
/*! thread-yield: page acquire read blocked */
#define	WT_STAT_CONN_PAGE_READ_BLOCKED			1117
/*! thread-yield: page acquire time sleeping (usecs) */
#define	WT_STAT_CONN_PAGE_SLEEP				1118
/*! connection: total read I/Os */
#define	WT_STAT_CONN_READ_IO				1119
/*! reconciliation: page reconciliation calls */
#define	WT_STAT_CONN_REC_PAGES				1120
/*! reconciliation: page reconciliation calls for eviction */
#define	WT_STAT_CONN_REC_PAGES_EVICTION			1121
/*! reconciliation: split bytes currently awaiting free */
#define	WT_STAT_CONN_REC_SPLIT_STASHED_BYTES		1122
/*! reconciliation: split objects currently awaiting free */
#define	WT_STAT_CONN_REC_SPLIT_STASHED_OBJECTS		1123
/*! connection: pthread mutex shared lock read-lock calls */
#define	WT_STAT_CONN_RWLOCK_READ			1124
/*! connection: pthread mutex shared lock write-lock calls */
#define	WT_STAT_CONN_RWLOCK_WRITE			1125
/*! session: open cursor count */
#define	WT_STAT_CONN_SESSION_CURSOR_OPEN		1126
/*! session: open session count */
#define	WT_STAT_CONN_SESSION_OPEN			1127
/*! transaction: transaction begins */
#define	WT_STAT_CONN_TXN_BEGIN				1128
/*! transaction: transaction checkpoints */
#define	WT_STAT_CONN_TXN_CHECKPOINT			1129
/*! transaction: transaction checkpoint generation */
#define	WT_STAT_CONN_TXN_CHECKPOINT_GENERATION		1130
/*! transaction: transaction checkpoint currently running */
#define	WT_STAT_CONN_TXN_CHECKPOINT_RUNNING		1131
/*! transaction: transaction checkpoint max time (msecs) */
#define	WT_STAT_CONN_TXN_CHECKPOINT_TIME_MAX		1132
/*! transaction: transaction checkpoint min time (msecs) */
#define	WT_STAT_CONN_TXN_CHECKPOINT_TIME_MIN		1133
/*! transaction: transaction checkpoint most recent time (msecs) */
#define	WT_STAT_CONN_TXN_CHECKPOINT_TIME_RECENT		1134
/*! transaction: transaction checkpoint total time (msecs) */
#define	WT_STAT_CONN_TXN_CHECKPOINT_TIME_TOTAL		1135
/*! transaction: transactions committed */
#define	WT_STAT_CONN_TXN_COMMIT				1136
/*! transaction: transaction failures due to cache overflow */
#define	WT_STAT_CONN_TXN_FAIL_CACHE			1137
/*! transaction: transaction range of IDs currently pinned by a checkpoint */
#define	WT_STAT_CONN_TXN_PINNED_CHECKPOINT_RANGE	1138
/*! transaction: transaction range of IDs currently pinned */
#define	WT_STAT_CONN_TXN_PINNED_RANGE			1139
/*! transaction: transactions rolled back */
#define	WT_STAT_CONN_TXN_ROLLBACK			1140
/*! connection: total write I/Os */
#define	WT_STAT_CONN_WRITE_IO				1141

/*!
 * @}
 * @name Statistics for data sources
 * @anchor statistics_dsrc
 * @{
 */
/*! block-manager: file allocation unit size */
#define	WT_STAT_DSRC_ALLOCATION_SIZE			2000
/*! block-manager: blocks allocated */
#define	WT_STAT_DSRC_BLOCK_ALLOC			2001
/*! block-manager: checkpoint size */
#define	WT_STAT_DSRC_BLOCK_CHECKPOINT_SIZE		2002
/*! block-manager: allocations requiring file extension */
#define	WT_STAT_DSRC_BLOCK_EXTENSION			2003
/*! block-manager: blocks freed */
#define	WT_STAT_DSRC_BLOCK_FREE				2004
/*! block-manager: file magic number */
#define	WT_STAT_DSRC_BLOCK_MAGIC			2005
/*! block-manager: file major version number */
#define	WT_STAT_DSRC_BLOCK_MAJOR			2006
/*! block-manager: minor version number */
#define	WT_STAT_DSRC_BLOCK_MINOR			2007
/*! block-manager: file bytes available for reuse */
#define	WT_STAT_DSRC_BLOCK_REUSE_BYTES			2008
/*! block-manager: file size in bytes */
#define	WT_STAT_DSRC_BLOCK_SIZE				2009
/*! LSM: bloom filters in the LSM tree */
#define	WT_STAT_DSRC_BLOOM_COUNT			2010
/*! LSM: bloom filter false positives */
#define	WT_STAT_DSRC_BLOOM_FALSE_POSITIVE		2011
/*! LSM: bloom filter hits */
#define	WT_STAT_DSRC_BLOOM_HIT				2012
/*! LSM: bloom filter misses */
#define	WT_STAT_DSRC_BLOOM_MISS				2013
/*! LSM: bloom filter pages evicted from cache */
#define	WT_STAT_DSRC_BLOOM_PAGE_EVICT			2014
/*! LSM: bloom filter pages read into cache */
#define	WT_STAT_DSRC_BLOOM_PAGE_READ			2015
/*! LSM: total size of bloom filters */
#define	WT_STAT_DSRC_BLOOM_SIZE				2016
/*! btree: btree checkpoint generation */
#define	WT_STAT_DSRC_BTREE_CHECKPOINT_GENERATION	2017
/*! btree: column-store variable-size deleted values */
#define	WT_STAT_DSRC_BTREE_COLUMN_DELETED		2018
/*! btree: column-store fixed-size leaf pages */
#define	WT_STAT_DSRC_BTREE_COLUMN_FIX			2019
/*! btree: column-store internal pages */
#define	WT_STAT_DSRC_BTREE_COLUMN_INTERNAL		2020
/*! btree: column-store variable-size leaf pages */
#define	WT_STAT_DSRC_BTREE_COLUMN_VARIABLE		2021
/*! btree: pages rewritten by compaction */
#define	WT_STAT_DSRC_BTREE_COMPACT_REWRITE		2022
/*! btree: number of key/value pairs */
#define	WT_STAT_DSRC_BTREE_ENTRIES			2023
/*! btree: fixed-record size */
#define	WT_STAT_DSRC_BTREE_FIXED_LEN			2024
/*! btree: maximum tree depth */
#define	WT_STAT_DSRC_BTREE_MAXIMUM_DEPTH		2025
/*! btree: maximum internal page key size */
#define	WT_STAT_DSRC_BTREE_MAXINTLKEY			2026
/*! btree: maximum internal page size */
#define	WT_STAT_DSRC_BTREE_MAXINTLPAGE			2027
/*! btree: maximum leaf page key size */
#define	WT_STAT_DSRC_BTREE_MAXLEAFKEY			2028
/*! btree: maximum leaf page size */
#define	WT_STAT_DSRC_BTREE_MAXLEAFPAGE			2029
/*! btree: maximum leaf page value size */
#define	WT_STAT_DSRC_BTREE_MAXLEAFVALUE			2030
/*! btree: overflow pages */
#define	WT_STAT_DSRC_BTREE_OVERFLOW			2031
/*! btree: row-store internal pages */
#define	WT_STAT_DSRC_BTREE_ROW_INTERNAL			2032
/*! btree: row-store leaf pages */
#define	WT_STAT_DSRC_BTREE_ROW_LEAF			2033
/*! cache: bytes read into cache */
#define	WT_STAT_DSRC_CACHE_BYTES_READ			2034
/*! cache: bytes written from cache */
#define	WT_STAT_DSRC_CACHE_BYTES_WRITE			2035
/*! cache: checkpoint blocked page eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_CHECKPOINT		2036
/*! cache: unmodified pages evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_CLEAN		2037
/*! cache: page split during eviction deepened the tree */
#define	WT_STAT_DSRC_CACHE_EVICTION_DEEPEN		2038
/*! cache: modified pages evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_DIRTY		2039
/*! cache: data source pages selected for eviction unable to be evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_FAIL		2040
/*! cache: hazard pointer blocked page eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_HAZARD		2041
/*! cache: internal pages evicted */
#define	WT_STAT_DSRC_CACHE_EVICTION_INTERNAL		2042
/*! cache: pages split during eviction */
#define	WT_STAT_DSRC_CACHE_EVICTION_SPLIT		2043
/*! cache: in-memory page splits */
#define	WT_STAT_DSRC_CACHE_INMEM_SPLIT			2044
/*! cache: overflow values cached in memory */
#define	WT_STAT_DSRC_CACHE_OVERFLOW_VALUE		2045
/*! cache: pages read into cache */
#define	WT_STAT_DSRC_CACHE_READ				2046
/*! cache: overflow pages read into cache */
#define	WT_STAT_DSRC_CACHE_READ_OVERFLOW		2047
/*! cache: pages written from cache */
#define	WT_STAT_DSRC_CACHE_WRITE			2048
/*! compression: raw compression call failed, no additional data available */
#define	WT_STAT_DSRC_COMPRESS_RAW_FAIL			2049
/*! compression: raw compression call failed, additional data available */
#define	WT_STAT_DSRC_COMPRESS_RAW_FAIL_TEMPORARY	2050
/*! compression: raw compression call succeeded */
#define	WT_STAT_DSRC_COMPRESS_RAW_OK			2051
/*! compression: compressed pages read */
#define	WT_STAT_DSRC_COMPRESS_READ			2052
/*! compression: compressed pages written */
#define	WT_STAT_DSRC_COMPRESS_WRITE			2053
/*! compression: page written failed to compress */
#define	WT_STAT_DSRC_COMPRESS_WRITE_FAIL		2054
/*! compression: page written was too small to compress */
#define	WT_STAT_DSRC_COMPRESS_WRITE_TOO_SMALL		2055
/*! cursor: create calls */
#define	WT_STAT_DSRC_CURSOR_CREATE			2056
/*! cursor: insert calls */
#define	WT_STAT_DSRC_CURSOR_INSERT			2057
/*! cursor: bulk-loaded cursor-insert calls */
#define	WT_STAT_DSRC_CURSOR_INSERT_BULK			2058
/*! cursor: cursor-insert key and value bytes inserted */
#define	WT_STAT_DSRC_CURSOR_INSERT_BYTES		2059
/*! cursor: next calls */
#define	WT_STAT_DSRC_CURSOR_NEXT			2060
/*! cursor: prev calls */
#define	WT_STAT_DSRC_CURSOR_PREV			2061
/*! cursor: remove calls */
#define	WT_STAT_DSRC_CURSOR_REMOVE			2062
/*! cursor: cursor-remove key bytes removed */
#define	WT_STAT_DSRC_CURSOR_REMOVE_BYTES		2063
/*! cursor: reset calls */
#define	WT_STAT_DSRC_CURSOR_RESET			2064
/*! cursor: search calls */
#define	WT_STAT_DSRC_CURSOR_SEARCH			2065
/*! cursor: search near calls */
#define	WT_STAT_DSRC_CURSOR_SEARCH_NEAR			2066
/*! cursor: update calls */
#define	WT_STAT_DSRC_CURSOR_UPDATE			2067
/*! cursor: cursor-update value bytes updated */
#define	WT_STAT_DSRC_CURSOR_UPDATE_BYTES		2068
/*! LSM: sleep for LSM checkpoint throttle */
#define	WT_STAT_DSRC_LSM_CHECKPOINT_THROTTLE		2069
/*! LSM: chunks in the LSM tree */
#define	WT_STAT_DSRC_LSM_CHUNK_COUNT			2070
/*! LSM: highest merge generation in the LSM tree */
#define	WT_STAT_DSRC_LSM_GENERATION_MAX			2071
/*! LSM: queries that could have benefited from a Bloom filter that did
 * not exist */
#define	WT_STAT_DSRC_LSM_LOOKUP_NO_BLOOM		2072
/*! LSM: sleep for LSM merge throttle */
#define	WT_STAT_DSRC_LSM_MERGE_THROTTLE			2073
/*! reconciliation: dictionary matches */
#define	WT_STAT_DSRC_REC_DICTIONARY			2074
/*! reconciliation: internal page multi-block writes */
#define	WT_STAT_DSRC_REC_MULTIBLOCK_INTERNAL		2075
/*! reconciliation: leaf page multi-block writes */
#define	WT_STAT_DSRC_REC_MULTIBLOCK_LEAF		2076
/*! reconciliation: maximum blocks required for a page */
#define	WT_STAT_DSRC_REC_MULTIBLOCK_MAX			2077
/*! reconciliation: internal-page overflow keys */
#define	WT_STAT_DSRC_REC_OVERFLOW_KEY_INTERNAL		2078
/*! reconciliation: leaf-page overflow keys */
#define	WT_STAT_DSRC_REC_OVERFLOW_KEY_LEAF		2079
/*! reconciliation: overflow values written */
#define	WT_STAT_DSRC_REC_OVERFLOW_VALUE			2080
/*! reconciliation: pages deleted */
#define	WT_STAT_DSRC_REC_PAGE_DELETE			2081
/*! reconciliation: page checksum matches */
#define	WT_STAT_DSRC_REC_PAGE_MATCH			2082
/*! reconciliation: page reconciliation calls */
#define	WT_STAT_DSRC_REC_PAGES				2083
/*! reconciliation: page reconciliation calls for eviction */
#define	WT_STAT_DSRC_REC_PAGES_EVICTION			2084
/*! reconciliation: leaf page key bytes discarded using prefix compression */
#define	WT_STAT_DSRC_REC_PREFIX_COMPRESSION		2085
/*! reconciliation: internal page key bytes discarded using suffix
 * compression */
#define	WT_STAT_DSRC_REC_SUFFIX_COMPRESSION		2086
/*! session: object compaction */
#define	WT_STAT_DSRC_SESSION_COMPACT			2087
/*! session: open cursor count */
#define	WT_STAT_DSRC_SESSION_CURSOR_OPEN		2088
/*! transaction: update conflicts */
#define	WT_STAT_DSRC_TXN_UPDATE_CONFLICT		2089
/*! @} */
/*
 * Statistics section: END
 * DO NOT EDIT: automatically built by dist/api_stat.py.
 */
/*!
 * @name Log record and operation types
 * @anchor log_types
 * @{
 */
/*
 * DO NOT EDIT: automatically built by dist/log.py.
 * Log record declarations: BEGIN
 */
/*! invalid operation */
#define	WT_LOGOP_INVALID	0
/*! checkpoint */
#define	WT_LOGREC_CHECKPOINT	0
/*! transaction commit */
#define	WT_LOGREC_COMMIT	1
/*! file sync */
#define	WT_LOGREC_FILE_SYNC	2
/*! message */
#define	WT_LOGREC_MESSAGE	3
/*! column put */
#define	WT_LOGOP_COL_PUT	1
/*! column remove */
#define	WT_LOGOP_COL_REMOVE	2
/*! column truncate */
#define	WT_LOGOP_COL_TRUNCATE	3
/*! row put */
#define	WT_LOGOP_ROW_PUT	4
/*! row remove */
#define	WT_LOGOP_ROW_REMOVE	5
/*! row truncate */
#define	WT_LOGOP_ROW_TRUNCATE	6
/*
 * Log record declarations: END
 * DO NOT EDIT: automatically built by dist/log.py.
 */
/*! @} */
/*! @} */

#undef __F

#if defined(__cplusplus)
}
#endif
#endif /* __WIREDTIGER_H_ */
