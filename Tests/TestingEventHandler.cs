using System.Collections.Generic;
using WiredTigerNet;

namespace Tests
{
	public class TestingEventHandler : IEventHandler
	{
		public readonly List<object> loggedEvents = new List<object>();

		public void OnError(int errorCode, string errorString, string message)
		{
			loggedEvents.Add(new ErrorEvent {errorCode = errorCode, errorString = errorString, message = message});
		}

		public void OnMessage(string message)
		{
			loggedEvents.Add(new MessageEvent {message = message});
		}

		public class ErrorEvent
		{
			public int errorCode;
			public string errorString;
			public string message;
		}

		public class MessageEvent
		{
			public string message;
		}
	}
}