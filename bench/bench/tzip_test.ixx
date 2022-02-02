
export module tzip_test;

#pragma warning(disable:5050)
import std.core;

import tzip;

export auto test_tzip(std::ranges::input_range auto& input,
					  std::ranges::output_range<uint8_t> auto& output)
{
	constexpr auto chunk_size = 1 << 14;
	auto dbg = &(*output.begin());

	auto zipper = tzipper();

	auto bytes_written = 0;

	auto [read, written] = zipper.zip(input, output);

	auto decompression_buffer = std::vector<uint8_t>(input.size());

	auto unzipper = tunzipper();

	auto decompression_input = output;
	decompression_input.resize(output.size() / 2);

	unzipper.unzip(decompression_input, decompression_buffer);

	//auto range = std::ranges::subrange(output.begin(), output.begin() + written);
	//zipper.unzip(range, decompression_buffer);

	if (!std::ranges::equal(input, decompression_buffer))
		throw std::exception();

	/*for (auto c : decompression_buffer)
	{
		std::cout << c;
	}

	std::cout << '\n';*/

	return written;
}