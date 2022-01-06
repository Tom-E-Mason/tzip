
export module tzipper;

#pragma warning(disable:5050)
import std.core;

import huffman_tree;

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
    auto gather_words(std::ranges::input_range auto& in)
    {
        std::string word;

        for (auto c : in)
        {
            if (!std::isalnum(c))
            {
                try_emplace_char(c);

                if (!word.empty())
                {
                    emplace_or_increment(word);
                    word.resize(0);
                }
            }
            else
            {
                word.push_back(c);
            }
        }

        if (!word.empty())
        {
            emplace_or_increment(word);
            word.resize(0);
        }
    }

    auto serialise_huffman_tree(std::ranges::output_range<uint8_t> auto& out)
    {
        auto out_it = out.begin();

        for (const auto& [key, value] : m_huff_tree)
        {
            out_it = std::copy(key.begin(), key.end(), out_it);
            *(out_it++) = ':';

            out_it = std::copy(value.begin(), value.end(), out_it);
            *(out_it++) = ':'; // will write one too many colons...
        }

        *out_it = '\0'; // this overwrites it to signal end of header

        return out_it;
    }

    auto compress(std::ranges::input_range auto& in,
                  std::ranges::output_range<uint8_t> auto& out)
    {
        using std::ranges::subrange;

        auto out_it = out.begin();
        auto word = std::string();

        int i = 0;

        for (auto c : in)
        {
            ++i;

            if (!std::isalnum(c))
            {
                auto sr = subrange(out_it, out.end());

                if (!word.empty())
                {
                    out_it = zip_word(m_huff_tree[word],
                                      sr);
                    word.resize(0);
                    sr = subrange(out_it, out.end());
                }

                out_it = zip_word(m_huff_tree[std::string(1, c)],
                                  sr);
            }
            else
            {
                word.push_back(c);
            }
        }

        auto sr = subrange(out_it, out.end());

        if (!word.empty())
            out_it = zip_word(m_huff_tree[word], sr);

        if (m_byte_index > 0)
            out_it = write_byte(out_it);
    }

    auto zip(std::ranges::input_range auto& in,
             std::ranges::output_range<uint8_t> auto& out)
    {
        gather_words(in);

        const auto weightings = collate();

        m_huff_tree = huffman_tree(weightings);

        auto out_it = serialise_huffman_tree(out);

        auto out_range = std::ranges::subrange(out_it, out.end());
        compress(in, out_range);

        return tzip_info(in.size(), m_bytes_written);
    }

    void unzip(std::ranges::input_range auto& in,
               std::ranges::output_range<uint8_t> auto& out)
    {
        auto traversal_node = m_huff_tree.get_root();
        int byte_index = 0;

        auto out_it = out.begin();

        for (auto byte : in)
        {
            for (int i = 0; i < 8; ++i)
            {
                if (!traversal_node->string().empty())
                {
                    const auto& word = traversal_node->string();
                    out_it = std::copy(word.cbegin(), word.cend(), out_it);

                    traversal_node = m_huff_tree.get_root();
                }

                auto path = bool(byte & (1 << i));

                if (path)
                    traversal_node = traversal_node->right();
                else
                    traversal_node = traversal_node->left();
            }
        }
    }

private:
    auto write_byte(std::output_iterator<uint8_t> auto out_it)
    {
        m_byte_index = 0;

        *(out_it++) = m_output_byte;
        m_output_byte = 0;
        ++m_bytes_written;

        return out_it;
    }

    auto zip_word(const std::vector<uint8_t>& bit_array,
                  std::ranges::output_range<uint8_t> auto& out)
    {
        auto out_it = out.begin();

        for (auto bit = bit_array.cbegin(); bit != bit_array.cend(); ++bit)
        {
            m_output_byte |= (*bit << m_byte_index);

            if (++m_byte_index == 8)
                out_it = write_byte(out_it);
        }

        return out_it;
    }

    auto collate() const
    {
        auto weightings = std::vector<std::pair<std::string, std::size_t>>();

        for (const auto& p : m_char_weightings)
        {
            weightings.push_back(std::pair<std::string, std::size_t>(std::string(1, p.first), p.second));
        }

        std::cout << "chars done\n";

        for (const auto& [k, v] : m_weightings)
        {
            weightings.push_back(std::pair<std::string, std::size_t>(k, v));
        }

        std::cout << "words done\n";

        return weightings;
    }

private:
    void emplace_or_increment(std::string& key)
    {
        const auto [it, success] = m_weightings.try_emplace(key, key.length());

        if (!success)
            it->second += key.length();
    }

    void try_emplace_char(uint8_t key)
    {
        const auto [it, success] = m_char_weightings.try_emplace(key, 1);

        if (!success)
            ++(it->second);
    }

private:
    std::unordered_map<uint8_t, std::size_t> m_char_weightings; // replace with flat_map
    std::unordered_map<std::string, std::size_t> m_weightings;
    uint8_t m_output_byte = 0;
    int m_byte_index = 0;
    std::size_t m_bytes_written = 0;
    huffman_tree m_huff_tree;
};
