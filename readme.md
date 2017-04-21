# WiredTiger.NET

WiredTiger.NET is the C++/CLI wrapper for WiredTiger key-value engine.

## Dynamic CRT dependency

Note that WiredTiger.NET has dependency on Microsoft CRT library.
We cannot compile WiredTiger.NET linked statically with CRT because of we use [C++/CLI does not support statically linked CRT](https://msdn.microsoft.com/en-us/library/ffkc918h.aspx?f=255&MSPPError=-2147217396) (mixing `/clr` and `/MD` is not supported).

Therefore, if you have some issues with WiredTiger.NET, check you have installed [Visual C++ Redistributable Packages](https://www.microsoft.com/en-us/download/details.aspx?id=40784).

## Contribute

Your Pull Requests are welcome!

## License

Distributed with [permissive MIT license](LICENSE).