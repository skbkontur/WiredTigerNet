using System;
using WiredTigerNet;

namespace Tests
{
	public class CrashingEventHandler : IEventHandler
	{
		public void OnError(int errorCode, string errorString, string message)
		{
			throw new InvalidOperationException("test OnError crash");
		}

		public void OnMessage(string message)
		{
			throw new InvalidOperationException("test OnMessage crash");
		}
	}
}