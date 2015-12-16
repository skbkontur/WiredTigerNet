using System;
using System.Diagnostics;
using WiredTigerNet;

namespace ConsoleApplication1
{
	public static class Program
	{
		public static void Main(string[] args)
		{
			//CreateNewDatabaseAndDie();
			OpenExistingDatabase();
		}

		public static void CreateNewDatabaseAndDie()
		{
			var connection = Connection.Open("C:\\TestWTShit", "create,cache_size=100MB");
			var session = connection.OpenSession("");
			session.Create("table:documents", "key_format=u,value_format=u,block_compressor=snappy,columns=(key,scopeKey)");
			var cursor = session.OpenCursor("table:documents", "");
			var key = new byte[40];
			var value = new byte[1];
			value[0] = 76;
			for (var i = 0; i < 100; i++)
			{
				key[0] = (byte) i;
				cursor.SetKey(key);
				cursor.SetValue(value);
				cursor.Insert();
			}
			//cursor.Dispose();
			//session.Dispose();
			//connection.Dispose();
			session.Checkpoint();
			Process.GetCurrentProcess().Kill();
		}

		public static void OpenExistingDatabase()
		{
			var connection = Connection.Open("C:\\TestWTShit", "cache_size=100MB");
			var session = connection.OpenSession("");
			var cursor = session.OpenCursor("table:documents");
			var index = 0;
			while (cursor.Next())
			{
				var key = cursor.GetKey();
				if (key.Length != 40)
					throw new InvalidOperationException("shit2");
				if (key[0] != index)
					throw new InvalidOperationException("shit3");
				var value = cursor.GetValue();
				if (value.Length != 1)
					throw new InvalidOperationException("shit4");
				if (value[0] != 76)
					throw new InvalidOperationException("shit5");
				index++;
			}
			if (index != 100)
				throw new InvalidOperationException("shit6");
			cursor.Dispose();
			session.Dispose();
			connection.Dispose();
			Console.Out.WriteLine("checkpoint ok!");
		}
	}
}