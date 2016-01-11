using System.Text;
using NUnit.Framework;
using WiredTigerNet;

namespace Tests
{
	public static class TestHelpers
	{
		public static void Insert(this Cursor cursor, string key, string value)
		{
			cursor.Insert(Encoding.ASCII.GetBytes(key), Encoding.ASCII.GetBytes(value));
		}

		public static bool Search(this Cursor cursor, string key)
		{
			return cursor.Search(Encoding.ASCII.GetBytes(key));
		}

		public static string GetKeyString(this Cursor cursor)
		{
			return Encoding.ASCII.GetString(cursor.GetKey());
		}

		public static string GetValueString(this Cursor cursor)
		{
			return Encoding.ASCII.GetString(cursor.GetValue());
		}

		public static void AssertValues(this Cursor cursor, string key, params string[] values)
		{
			Assert.That(cursor.Search(key));
			for (var i = 0; i < values.Length; i++)
			{
				var iterationMessage = "iteration " + i;
				Assert.That(cursor.GetKeyString(), Is.EqualTo(key), iterationMessage);
				Assert.That(cursor.GetValueString(), Is.EqualTo(values[i]), iterationMessage);
				if (i < values.Length - 1)
					Assert.That(cursor.Next(), iterationMessage);
			}
		}
	}
}