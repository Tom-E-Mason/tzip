export module tunzipper;

#pragma warning(disable:5050)
import std.core;

import huffman_tree;

struct tzip_info
{
	std::size_t read;
	std::size_t written;
};

namespace rng = std::ranges;

export class tunzipper
{
public:
	tunzipper() = default;

public:
	auto unzip(rng::input_range auto& in,
			   rng::output_range<uint8_t> auto& out)
	{
		auto body = parse_huffman_tree(in);
		auto traversal_node = m_huff_tree.get_root();

		auto out_it = out.begin();

		const auto word_count = m_huff_tree.get_word_count();
		auto words_written = 0;

		for (auto byte = body.begin(); byte != body.end(); ++byte)
		{
			for (int i = 0; i < 8; ++i)
			{
				if (!traversal_node->string().empty())
				{
					const auto& word = traversal_node->string();
					out_it = std::copy(word.cbegin(), word.cend(), out_it);

					++words_written;

					if (words_written == word_count)
						return byte + 1;

					traversal_node = m_huff_tree.get_root();
				}

				if (*byte & (1 << i))
					traversal_node = traversal_node->right();
				else
					traversal_node = traversal_node->left();
			}
		}

		return body.begin(); // this makes no sense, think about it
	}

private:
	auto parse_huffman_tree(rng::input_range auto& in)
	{
		enum class parse_state
		{
			START,
			WORD,
			PATH,
			COLON,
			END,
		};

		auto state = parse_state::START;
		auto word = std::string();
		auto weighting = std::string();
		auto in_it = in.begin();

		auto weightings = std::vector<std::pair<std::string, std::size_t>>();

		int count = 0;

		for (;; ++in_it)
		{
			auto c = *in_it;

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

				word.push_back(c);

				break;
			}
			case parse_state::PATH:
			{
				if (c == ':')
				{
					++count;
					weightings.emplace_back(std::piecewise_construct,
											std::forward_as_tuple(word),
											std::forward_as_tuple(std::stoull(weighting)));

					word.clear();
					weighting.clear();

					state = parse_state::COLON;
					break;
				}

				if (c == '\0')
				{
					weightings.emplace_back(std::piecewise_construct,
											std::forward_as_tuple(word),
											std::forward_as_tuple(std::stoull(weighting)));

					state = parse_state::END;
					break;
				}

				weighting.push_back(c);

				break;
			}
			case parse_state::COLON:
			{
				word.push_back(c);

				state = parse_state::WORD;

				break;
			}
			case parse_state::END:
			{
				m_huff_tree = huffman_tree(weightings);

				return rng::subrange(in_it, in.cend());
			}
			}
		}
	}

private:
	huffman_tree m_huff_tree;
};
