using System;
using System.Linq;
using System.Text;
using NUnit.Framework;
using WiredTigerNet;

namespace Tests
{
	public static class TestHelpers
	{
		public static byte[] B(this string s)
		{
			return Encoding.ASCII.GetBytes(s);
		}

		public static string S(this byte[] b)
		{
			return Encoding.ASCII.GetString(b);
		}

		public static void Insert(this Cursor cursor, string key, string value)
		{
			cursor.Insert(key.B(), value.B());
		}
		
		public static void Insert(this Cursor cursor, string key)
		{
			cursor.Insert(key.B());
		}

		public static bool Search(this Cursor cursor, string key)
		{
			return cursor.Search(key.B());
		}

		public static string GetKeyString(this Cursor cursor)
		{
			return cursor.GetKey().S();
		}

		public static string GetValueString(this Cursor cursor)
		{
			return cursor.GetValue().S();
		}

		public static void AssertKeyValues(this Cursor cursor, string key, params string[] values)
		{
			Assert.That(cursor.Search(key));
			cursor.AssertKeysAndValues(values.Select(x => key + "->" + x).ToArray());
		}

		public static void AssertAllKeysAndValues(this Cursor cursor, params string[] keysAndValues)
		{
			var nextPositioned = cursor.Next();
			if (keysAndValues.Length == 0)
				Assert.That(!nextPositioned);
			else
			{
				Assert.That(nextPositioned);
				cursor.AssertKeysAndValues(keysAndValues);
			}
		}

		private static void AssertKeysAndValues(this Cursor cursor, params string[] keysAndValues)
		{
			for (var i = 0; i < keysAndValues.Length; i++)
			{
				var iterationMessage = "iteration " + i;
				var keyValue = keysAndValues[i].Split(new[] {"->"}, StringSplitOptions.None);
				if (keyValue.Length != 2)
					throw new InvalidOperationException(string.Format("invalid keyValue [{0}]", keysAndValues[i]));
				Assert.That(cursor.GetKeyString(), Is.EqualTo(keyValue[0]), iterationMessage);
				Assert.That(cursor.GetValueString(), Is.EqualTo(keyValue[1]), iterationMessage);
				if (i < keysAndValues.Length - 1)
					Assert.That(cursor.Next(), iterationMessage);
			}
		}
	}
}