
#include <cassert>

#include <zlib.h>

#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
#  include <fcntl.h>
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(fileno(file), O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

#pragma warning(disable:5201)
export module zlib_test;

#pragma warning(disable:5050)
import std.core;

export auto test_zlib(std::span<uint8_t> input,
					  std::ranges::output_range<uint8_t> auto output,
					  int level)
{
	constexpr auto chunk_size = 1 << 14;

	auto stream = z_stream();
	auto index = std::size_t();

	auto bytes_written = 0;

	auto flush = Z_NO_FLUSH;

	auto rc = deflateInit(&stream, level);
	assert(rc == Z_OK);

	do
	{
		if (index + chunk_size < input.size())
		{
			stream.avail_in = chunk_size;
		}
		else
		{
			stream.avail_in = static_cast<uint32_t>(input.size() - index);
			flush = Z_FINISH;
		}

		stream.next_in = input.data() + index;
		index += chunk_size;

		do
		{
			stream.avail_out = chunk_size;
			stream.next_out = &(*(output.begin() + bytes_written));

			rc = deflate(&stream, flush);
			assert(rc != Z_STREAM_ERROR);

			bytes_written += chunk_size - stream.avail_out;

		} while (stream.avail_out == 0);

		assert(stream.avail_in == 0);

	} while (flush != Z_FINISH);

	deflateEnd(&stream);

	return bytes_written;
}
