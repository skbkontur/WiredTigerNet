using System;
using System.Linq;
using NUnit.Framework;
using WiredTigerNet;

namespace Tests
{
	[TestFixture]
	public class RangesTest
	{
		[Test]
		public void Prefix_EmptyArray_Infinity()
		{
			Assert.That(Range.Prefix(new byte[0]).Right, Is.Null);
		}

		[Test]
		public void Prefix_Simple()
		{
			var prefix = Range.Prefix(new byte[] { 1 });
			Assert.That(prefix.Left.Value.Bytes, Is.EqualTo(new byte[] { 1 }));
			Assert.That(prefix.Right.Value.Bytes, Is.EqualTo(new byte[] { 2 }));
		}

		[Test]
		public void Prefix_CanIncrementSecondByte()
		{
			Assert.That(Range.Prefix(new byte[] { 1, Byte.MaxValue }).Right.Value.Bytes, Is.EqualTo(new byte[] { 2, 0 }));
		}

		[Test]
		public void Prefix_CannotIncrement_ReturnInfinity()
		{
			var prefix = Range.Prefix(new[] { Byte.MaxValue, Byte.MaxValue });
			Assert.That(prefix.Left.Value.Bytes, Is.EqualTo(new[] { Byte.MaxValue, Byte.MaxValue }));
			Assert.That(prefix.Right, Is.Null);
		}

		[Test]
		public void Prepend()
		{
			var a = Range.Segment(ToBytes(1), ToBytes(2));
			var result = a.Prepend(new byte[] { 100 });
			Assert.That(result.Left.Value.Bytes, Is.EqualTo(new byte[] { 100, 1 }));
			Assert.That(result.Right.Value.Bytes, Is.EqualTo(new byte[] { 100, 2 }));
		}

		[Test]
		public void PrependNonInclusive()
		{
			var a = Range.LeftOpenSegment(AsBytes(1), AsBytes(2));
			var result = a.Prepend(new byte[] { 100 });
			Assert.That(result.Left.Value.Inclusive, Is.False);
			Assert.That(result.Right.Value.Inclusive);
		}

		[Test]
		public void PrependUnbounded()
		{
			var a = Range.NegativeRay(AsBytes(1));
			var result = a.Prepend(new byte[] { 100 });
			Assert.That(result.Left, Is.Null);
			Assert.That(result.Right.Value.Bytes, Is.EqualTo(new byte[] { 100, 1 }));
		}

		[Test]
		public void IntersectSimple()
		{
			var a = Range.Segment(AsBytes(1), AsBytes(2));
			var b = Range.Segment(AsBytes(2), AsBytes(3));
			var result = a.IntersectWith(b);
			Assert.That(result.Left.Value.Bytes.Single(), Is.EqualTo(2));
			Assert.That(result.Right.Value.Bytes.Single(), Is.EqualTo(2));
		}

		public static byte[] ToBytes(int b)
		{
			return new[] { (byte)b };
		}

		[Test]
		public void InclusiveIntersectWithNotInclusive_NotInclusiveWins()
		{
			var a = Range.LeftOpenSegment(AsBytes(1), AsBytes(2));
			var b = Range.Segment(AsBytes(1), AsBytes(3));
			var result = a.IntersectWith(b);
			Assert.That(result.Left.Value.Bytes.Single(), Is.EqualTo(1));
			Assert.That(result.Left.Value.Inclusive, Is.False);
			Assert.That(result.Right.Value.Bytes.Single(), Is.EqualTo(2));
			Assert.That(result.Right.Value.Inclusive);
		}

		private static void AssertSame(Range a, Range b)
		{
			AssertSame(a.Left, b.Left);
			AssertSame(a.Right, b.Right);
		}

		private static void AssertSame(Boundary? a, Boundary? b)
		{
			if (a.HasValue && b.HasValue)
			{
				Assert.That(a.Value.Inclusive, Is.EqualTo(b.Value.Inclusive));
				Assert.That(a.Value.Bytes, Is.SameAs(b.Value.Bytes));
			}
			else
			{
				Assert.That(a.HasValue, Is.False);
				Assert.That(b.HasValue, Is.False);
			}
		}

		[Test]
		public void IntersectNestedRanges()
		{
			var inner = Range.Segment(AsBytes(3), AsBytes(4));
			var outer = Range.Segment(AsBytes(1), AsBytes(5));

			AssertSame(inner.IntersectWith(outer), inner);
			AssertSame(outer.IntersectWith(inner), inner);
		}

		[Test]
		public void InfinityIntersectsWithNotInfinity_NotInfinityWins()
		{
			var a = Range.PositiveOpenRay(AsBytes(1));
			var b = Range.NegativeRay(AsBytes(2));
			var result = a.IntersectWith(b);
			Assert.That(result.Left.Value.Bytes.Single(), Is.EqualTo(1));
			Assert.That(result.Left.Value.Inclusive, Is.False);
			Assert.That(result.Right.Value.Bytes.Single(), Is.EqualTo(2));
			Assert.That(result.Right.Value.Inclusive);
		}

		[Test]
		public void IsEmptyTests()
		{
			Assert.That(Range.Segment(AsBytes(1), AsBytes(2)).IsEmpty(), Is.False);
			Assert.That(Range.Segment(AsBytes(2), AsBytes(1)).IsEmpty(), Is.True);
			Assert.That(Range.PositiveRay(AsBytes(1)).IsEmpty(), Is.False);
			Assert.That(Range.Line().IsEmpty(), Is.False);
			Assert.That(Range.NegativeRay(AsBytes(2)).IsEmpty(), Is.False);
			Assert.That(Range.Segment(AsBytes(1), AsBytes(1)).IsEmpty(), Is.False);
			Assert.That(Range.Interval(AsBytes(1), AsBytes(1)).IsEmpty(), Is.True);
			Assert.That(Range.LeftOpenSegment(AsBytes(1), AsBytes(1)).IsEmpty(), Is.True);
			Assert.That(Range.RightOpenSegment(AsBytes(1), AsBytes(1)).IsEmpty(), Is.True);
		}

		public static byte[] AsBytes(int b)
		{
			return new[] { (byte)b };
		}
	}
}