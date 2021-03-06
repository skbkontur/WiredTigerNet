﻿using System;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using NUnit.Framework;
using WiredTigerNet;

namespace Tests
{
	[TestFixture]
	public class BasicTest
	{
		private string testDirectory;

		[SetUp]
		public void SetUp()
		{
			testDirectory = Path.GetFullPath(".testData");
			if (Directory.Exists(testDirectory))
				Directory.Delete(testDirectory, true);
			Directory.CreateDirectory(testDirectory);
		}

		[TearDown]
		public void TearDown()
		{
			if (Directory.Exists(testDirectory))
				Directory.Delete(testDirectory, true);
		}

		[Test]
		public void Simple()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test",
					"key_format=u,value_format=u,prefix_compression=true,block_compressor=snappy,columns=(key,scopeKey)");
				session.Create("index:test:byScopeKey", "prefix_compression=true,block_compressor=snappy,columns=(scopeKey)");

				using (var cursor = session.OpenCursor("table:test"))
				{
					cursor.Insert("a", "k");
					cursor.Insert("b", "k");
				}

				using (var cursor = session.OpenCursor("index:test:byScopeKey(key)"))
					cursor.AssertKeyValues("k", "a", "b");
			}
		}

		[Test]
		public void FinalizeCanTakeOverForForgottenDispose()
		{
			var connection = Connection.Open(testDirectory, "create", null);
			var session = connection.OpenSession();
			session.Create("table:test", null);
			var cursor = session.OpenCursor("table:test");
			cursor.Insert("a", "b");

			//emulate standard finalization with random ordering
			CallFinalizer(session);
			CallFinalizer(connection);
			CallFinalizer(cursor);

			using(var connection2 = Connection.Open(testDirectory, "create", null))
			using (var session2 = connection2.OpenSession())
			using (var cursor2 = session2.OpenCursor("table:test"))
				cursor2.AssertAllKeysAndValues("a->b");
		}

		public static void CallFinalizer(object target)
		{
			var finalizer = target.GetType().GetMethod("Finalize", BindingFlags.NonPublic | BindingFlags.Instance);
			finalizer.Invoke(target, new object[0]);
		}

		[Test]
		public void CanHandleStringKeys()
		{
			const string testString = "test string";

			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", "key_format=S,value_format=,columns=(k)");
				using (var cursor = session.OpenCursor("table:test"))
				{
					var testStringNullTerminatedBytes = new byte[Encoding.ASCII.GetByteCount(testString) + 1];
					var writtenBytesCount = Encoding.ASCII.GetBytes(testString, 0, testString.Length, testStringNullTerminatedBytes, 0);
					if (writtenBytesCount != testString.Length)
						throw new InvalidOperationException();
					testStringNullTerminatedBytes[testStringNullTerminatedBytes.Length - 1] = 0;
					cursor.Insert(testStringNullTerminatedBytes);
				}
			}

			using (var connection = Connection.Open(testDirectory, null, null))
			using (var session = connection.OpenSession())
			using (var cursor = session.OpenCursor("table:test"))
			{
				if (!cursor.Next())
					throw new InvalidOperationException();
				var keyBytes = cursor.GetKey();
				Assert.That(Encoding.ASCII.GetString(keyBytes), Is.EqualTo(testString));
			}
		}

		[Test]
		public void BulkInsert()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", null);
				using (var cursor = session.OpenCursor("table:test", "bulk=true"))
				{
					cursor.Insert("b", "k");
					var exception = Assert.Throws<WiredTigerApiException>(() => cursor.Insert("a", "k"));
					Assert.That(exception.ApiName, Is.EqualTo("cursor->insert"));
					Assert.That(exception.ErrorCode, Is.EqualTo(22));
					Assert.That(exception.Message,
						Is.EqualTo("error code [22], error message [Invalid argument], api name [cursor->insert]"));
					cursor.Insert("c", "k");
				}
				using (var cursor = session.OpenCursor("table:test"))
					cursor.AssertAllKeysAndValues("b->k", "c->k");
			}
		}

		[Test]
		public void SimpleWithTran()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test",
					"key_format=u,value_format=u,prefix_compression=true,block_compressor=snappy,columns=(key,scopeKey)");
				session.Create("index:test:byScopeKey", "prefix_compression=true,block_compressor=snappy,columns=(scopeKey)");

				using (var cursor = session.OpenCursor("table:test"))
				{
					cursor.Insert("a", "k");
					cursor.Insert("b", "k");
				}
				session.BeginTran();
				using (var cursor = session.OpenCursor("table:test"))
					cursor.Insert("c", "k");
				using (var cursor = session.OpenCursor("index:test:byScopeKey(key)"))
					cursor.AssertKeyValues("k", "a", "b", "c");
				session.RollbackTran();
				using (var cursor = session.OpenCursor("index:test:byScopeKey(key)"))
					cursor.AssertKeyValues("k", "a", "b");
			}
		}

		[Test]
		public void SessionCreateConfigParameterIsNullable()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", null);
				using (var cursor = session.OpenCursor("table:test"))
				{
					cursor.Insert("a", "k");
					cursor.Insert("b", "k");
				}
				using (var cursor = session.OpenCursor("table:test"))
					cursor.AssertAllKeysAndValues("a->k", "b->k");
			}
		}

		[Test]
		public void ConnectionOpenConfigParameterIsNullable()
		{
			using (Connection.Open(testDirectory, "create", null))
			{
			}

			using (var connection = Connection.Open(testDirectory, null, null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", null);
				using (var cursor = session.OpenCursor("table:test"))
					cursor.Insert("a", "b");
				using (var cursor = session.OpenCursor("table:test"))
					cursor.AssertAllKeysAndValues("a->b");
			}
		}

		[Test]
		public void CorrectlyLogErrorWhenTargetDirectoryNotExist()
		{
			var eventHandler = new LoggingEventHandler();
			var exception =
				Assert.Throws<WiredTigerApiException>(() => Connection.Open(Path.Combine(testDirectory, "inexistentFolder"),
					"", eventHandler));
			const int expectedErrorCode = 2;
			Assert.That(exception.Message, Is.StringContaining("No such file or directory")
				.Or.StringContaining("найти указанный путь"));
			Assert.That(exception.Message, Is.StringContaining(expectedErrorCode.ToString(CultureInfo.InvariantCulture)));
			Assert.That(exception.ApiName, Is.EqualTo("wiredtiger_open"));
			Assert.That(exception.ErrorCode, Is.EqualTo(expectedErrorCode));
			Assert.That(eventHandler.loggedEvents.Count, Is.EqualTo(1));
			var loggedEvent = (LoggingEventHandler.ErrorEvent) eventHandler.loggedEvents.Single();
			Assert.That(loggedEvent.errorCode, Is.EqualTo(expectedErrorCode));
			Assert.That(loggedEvent.errorString, Is.StringContaining("No such file or directory")
				.Or.StringContaining("найти указанный путь"));
			Assert.That(loggedEvent.message, Is.StringContaining(testDirectory));
		}

		[Test]
		public void CheckKeyOnlyCursorSchema()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:keyOnly", "key_format=u,value_format=,columns=(k)");
				using (var cursor = session.OpenCursor("table:keyOnly"))
				{
					Assert.That(cursor.SchemaType, Is.EqualTo(CursorSchemaType.KeyOnly));
					var exception = Assert.Throws<WiredTigerException>(() => cursor.Insert("a", "b"));
					Assert.That(exception.Message,
						Is.EqualTo("invalid Insert overload, current schema is [CursorSchemaType.KeyOnly] so use Insert(byte[]) instead"));
					cursor.Insert("a");
				}
				using (var cursor = session.OpenCursor("table:keyOnly"))
				{
					Assert.That(cursor.Search("a"));
					Assert.That(cursor.GetKeyString(), Is.EqualTo("a"));
					var exception = Assert.Throws<WiredTigerException>(() => cursor.GetValue());
					Assert.That(exception.Message,
						Is.EqualTo("for current schema [CursorSchemaType.KeyOnly] value is not defined"));
				}
			}
		}

		[Test]
		public void CheckKeyAndValueCursorSchema()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:keyAndValue", "key_format=u,value_format=u,columns=(k,v)");
				using (var cursor = session.OpenCursor("table:keyAndValue"))
				{
					Assert.That(cursor.SchemaType, Is.EqualTo(CursorSchemaType.KeyAndValue));
					var exception = Assert.Throws<WiredTigerException>(() => cursor.Insert("a"));
					Assert.That(exception.Message,
						Is.EqualTo(
							"invalid Insert overload, current schema is [CursorSchemaType.KeyAndValue] so use Insert(byte[],byte[]) instead"));
					cursor.Insert("a", "b");
				}
				using (var cursor = session.OpenCursor("table:keyAndValue"))
					cursor.AssertAllKeysAndValues("a->b");
			}
		}

		[Test]
		public void ValidateSupportedCursorSchema()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", "key_format=u,value_format=uu,columns=(key,scopeKey,scopeKey2)");
				var exception = Assert.Throws<WiredTigerException>(() => session.OpenCursor("table:test"));
				Assert.That(exception.Message,
					Is.EqualTo("unsupported cursor schema (key_format->value_format) = (u->uu), expected (u->u) or (u->)"));
			}
		}

		[Test]
		public void GetTotalCount()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", "key_format=u,value_format=u,columns=(k,v)");

				using (var cursor = session.OpenCursor("table:test"))
				{
					cursor.Insert("a", "v");
					cursor.Insert("b", "v");
					cursor.Insert("c", "v");
				}

				using (var cursor = session.OpenCursor("table:test"))
				{
					Assert.That(cursor.GetTotalCount(Range.Segment("d".B(), "d".B())), Is.EqualTo(0));
					Assert.That(cursor.GetTotalCount(Range.Segment("c".B(), "c".B())), Is.EqualTo(1));
					Assert.That(cursor.GetTotalCount(Range.Segment("b".B(), "f".B())), Is.EqualTo(2));
					Assert.That(cursor.GetTotalCount(Range.Line()), Is.EqualTo(3));
					Assert.That(cursor.GetTotalCount(Range.Segment("d".B(), "d".B()), 5), Is.EqualTo(0));
					Assert.That(cursor.GetTotalCount(Range.Segment("b".B(), "f".B()), 1), Is.EqualTo(1));
				}
			}
		}

		[TestCase(0, 0)]
		[TestCase(2, 2)]
		[TestCase(3, 3)]
		[TestCase(4, 3)]
		public void GetTotalCountWithUpperBound(long maxCount, long expectedResult)
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", "key_format=u,value_format=u,columns=(k,v)");

				using (var cursor = session.OpenCursor("table:test"))
				{
					cursor.Insert("a", "v");
					cursor.Insert("b", "v");
					cursor.Insert("c", "v");
				}

				using (var cursor = session.OpenCursor("table:test"))
				{
					Assert.That(cursor.GetTotalCount(Range.Line(), maxCount), Is.EqualTo(expectedResult));
				}
			}
		}

		[Test]
		public void GetTotalCountForNonInclusiveRightBoundary()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				session.Create("table:test", "key_format=u,value_format=,columns=(k)");

				using (var cursor = session.OpenCursor("table:test"))
					cursor.Insert("a");

				using (var cursor = session.OpenCursor("table:test"))
					Assert.That(cursor.GetTotalCount(Range.RightOpenSegment("a".B(), "ab".B())), Is.EqualTo(1));
			}
		}

		[Test]
		public void HandleCrashesOfErrorHandler()
		{
			var eventHandler = new CrashingEventHandler();
			var exception =
				Assert.Throws<WiredTigerApiException>(() => Connection.Open(Path.Combine(testDirectory, "inexistentFolder"),
					"", eventHandler));
			const int expectedErrorCode = 2;
			Assert.That(exception.Message, Is.StringContaining("No such file or directory")
				.Or.StringContaining("найти указанный путь"));
			Assert.That(exception.Message, Is.StringContaining(expectedErrorCode.ToString(CultureInfo.InvariantCulture)));
		}

		[Test]
		public void HandleGracefullyOpenCursorCrash()
		{
			using (var connection = Connection.Open(testDirectory, "create", null))
			using (var session = connection.OpenSession())
			{
				var exception =
					Assert.Throws<WiredTigerApiException>(() => session.OpenCursor("table:test", "checkpoint=inexistent"));
				Assert.That(exception.Message, Is.StringContaining("error code [2]"));
				Assert.That(exception.Message, Is.StringContaining("api name [session->open_cursor, table:test]"));

				using (session.OpenCursor("backup:"))
				{
					exception = Assert.Throws<WiredTigerApiException>(() => session.OpenCursor("backup:"));
					Assert.That(exception.Message, Is.StringContaining("error code [22]"));
					Assert.That(exception.Message, Is.StringContaining("api name [session->open_cursor, backup:]"));
				}
			}
		}

		[Test]
		public void CanDisposeWiredTigerComponentsInAnyOrder()
		{
			var connection = Connection.Open(testDirectory, "create", null);
			var session = connection.OpenSession();
			session.Create("table:test", null);
			var cursor = session.OpenCursor("table:test");
			cursor.Insert("a", "b");
			session.Dispose();
			connection.Dispose();
			cursor.Dispose();

			using (var connection2 = Connection.Open(testDirectory, "create", null))
			using (var session2 = connection2.OpenSession())
			using (var cursor2 = session2.OpenCursor("table:test"))
				cursor2.AssertAllKeysAndValues("a->b");
		}
	}
}