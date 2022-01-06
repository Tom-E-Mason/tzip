export module tunzipper;

#pragma warning(disable:5050)
import std.core;

import huffman_tree;

struct tzip_info
{
	std::size_t read;
	std::size_t written;
};

export class tunzipper
{
public:
	tunzipper() = default;

public:
	auto unzip(std::ranges::input_range auto& in,
			 std::ranges::output_range<uint8_t> auto& out)
	{
		enum class parse_state
		{
			START,
			WORD,
			PATH,
			END,
		};

		auto state = parse_state::START;
		auto word = std::string();
		auto bit_string = std::vector<uint8_t>();
		auto in_it = in.begin();

		for (;;)
		{
			auto c = *in_it++;

			switch (state)
			{
			case parse_state::START:
			{
				// assume ':' not first character for proof of concept
				if (c == ':') throw std::exception("assumption failed idiot");

				word.push_back(c);
				state = parse_state::WORD;

				break;
			}
			case parse_state::WORD:
			{
				if (c == ':')
				{
					state = parse_state::PATH;
					break;
				}
				else if (c == '\0')
				{
					state = parse_state::END;
				}

				word.push_back(c);

				break;
			}
			case parse_state::PATH:
			{
				if (c == ':')
				{


					state = parse_state::WORD;
					break;
				}

				bit_string.push_back(c - '0');

				break;
			}
			}
		}
	}

private:
	huffman_tree m_huff_tree;
};
