using System.Globalization;
using System.IO;
using System.Linq;
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
			using (var session = connection.OpenSession(""))
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
					cursor.AssertValues("k", "a", "b");
			}
		}

		[Test]
		public void CorrectlyLogErrorWhenTargetDirectoryNotExist()
		{
			var eventHandler = new TestingEventHandler();
			var exception = Assert.Throws<WiredException>(() => Connection.Open(Path.Combine(testDirectory, "inexistentFolder"),
				"", eventHandler));
			const int expectedErrorCode = -28997;
			Assert.That(exception.Message, Is.StringContaining("The system cannot find the path specified")
				.Or.StringContaining("найти указанный файл"));
			Assert.That(exception.Message, Is.StringContaining(expectedErrorCode.ToString(CultureInfo.InvariantCulture)));
			Assert.That(eventHandler.loggedEvents.Count, Is.EqualTo(1));
			var loggedEvent = (TestingEventHandler.ErrorEvent) eventHandler.loggedEvents.Single();
			Assert.That(loggedEvent.errorCode, Is.EqualTo(expectedErrorCode));
			Assert.That(loggedEvent.errorString, Is.StringContaining("The system cannot find the path specified")
				.Or.StringContaining("найти указанный файл"));
			Assert.That(loggedEvent.message, Is.StringContaining(testDirectory));
		}
	}
}