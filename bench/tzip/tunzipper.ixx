export module tunzipper;

#pragma warning(disable:5050)
import std.core;

struct tzip_info
{
	std::size_t read;
	std::size_t written;
};

export class tzipper
{
public:
	tzipper() = default;

public:
	auto unzip(std::ranges::input_range auto& in,
			 std::ranges::output_range<uint8_t> auto& out)
	{
		gather_words(in);

		const auto weightings = collate();

		m_huff_tree = huffman_tree(weightings);

		compress(in, out);

		return tzip_info(in.size(), m_bytes_written);
	}

private:
	huffman_tree m_huff_tree;
};
