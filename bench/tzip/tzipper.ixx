
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
    void gather_words(std::ranges::input_range auto& in)
    {
        std::string word;

        for (auto c : in)
        {
            if (!std::isalnum(c))
            {
                try_emplace_char(c);

                if (!word.empty())
                {
                    emplace_or_increment(word, m_weightings);
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
            emplace_or_increment(word, m_weightings);
            word.resize(0);
        }

        gather_n_words(in, 2);
    }

    void gather_n_words(std::ranges::input_range auto& in,
                        std::size_t n)
    {
        auto words = std::vector<std::string>();

        std::string word;

        for (auto c : in)
        {
            if (!std::isalnum(c))
            {
                if (!word.empty())
                {
                    words.push_back(word);
                    word.clear();
                }

                words.emplace_back(1, c);
            }
            else
            {
                word.push_back(c);
            }
        }

        gather_n_words(n, words);
    }

    void gather_n_words(std::size_t n,
                        const std::vector<std::string>& words)
    {
        if (n != 2)
            throw std::exception("not implemented");

        if (n > words.size())
            n = words.size();

        const auto first_last_interval = 2 * n - 1;

        // ' ' space is a word
        const auto max_words = n + 1;

        auto word_count = std::size_t();

        auto first = words.cbegin();

        while (*first == " ")
            ++first;

        auto last = first + first_last_interval;

        std::string word_group;
        bool space = false;

        while (last != words.cend())
        {
            // stores group of n words
            for (auto it = first; it != last; ++it)
            {
                if (it->length() == 1)
                {
                    if (*it == " " && !space)
                    {
                        space = true;
                    }
                    else
                    {
                        word_group.clear();
                        break;
                    }
                }
                else
                {
                    space = false;
                }
                
                word_group.append(*it);
            }

            space = false;

            // saves group if valid
            if (!word_group.empty())
            {
                emplace_or_increment(word_group, m_weightings2);
                word_group.clear();
            }

            do
                ++first;
            while (*first == " ");

            last = first;

            for (auto i = first_last_interval; i > 0 && last != words.cend(); --i)
                ++last;
        }

        auto space_weight = m_char_weightings.find(' ');

        for (auto& [key, value] : m_weightings2)
        {
            auto space = std::ranges::find(key, ' ');

            const auto sv1 = std::string_view(key.cbegin(), space);
            auto first_weight = m_weightings.find(std::string(sv1));

            const auto sv2 = std::string_view(space + 1, key.cend());
            auto second_weight = m_weightings.find(std::string(sv2));

            const auto individual_weighting = first_weight->second
                + second_weight->second
                + space_weight->second;

            if (value > individual_weighting / 2)
            {
                const auto aggregate_frequency = value / key.length();

                first_weight->second -= sv1.length() * aggregate_frequency;
                --space_weight->second;
                second_weight->second -= sv2.length() * aggregate_frequency;

                m_weightings.insert(std::pair(key, value));
            }
        }
    }

    auto write_header(std::ranges::output_range<uint8_t> auto& out)
    {
        auto out_it = out.begin();

        auto weighting = std::string();

        std::size_t word_count = 0;

        for (const auto& [key, value] : m_huff_tree)
        {
            out_it = std::copy(key.cbegin(), key.cend(), out_it);
            *(out_it++) = ':';

            weighting = std::to_string(value.first);

            out_it = std::copy(weighting.cbegin(), weighting.cend(), out_it);
            *(out_it++) = ':'; // will write one too many colons...

            word_count += value.first / key.length();
            m_bytes_written += (key.length() + weighting.length() + 2);
        }

        // this overwrites last colon to signal end of huffman tree
        *(out_it - 1) = '\0';

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
                    out_it = zip_word(m_huff_tree[word].second,
                                      sr);
                    word.resize(0);
                    sr = subrange(out_it, out.end());
                }

                out_it = zip_word(m_huff_tree[std::string(1, c)].second,
                                  sr);
            }
            else
            {
                word.push_back(c);
            }
        }

        auto sr = subrange(out_it, out.end());

        if (!word.empty())
            out_it = zip_word(m_huff_tree[word].second, sr);

        if (m_byte_index > 0)
            out_it = write_byte(out_it);
    }

    auto zip(std::ranges::input_range auto& in,
             std::ranges::output_range<uint8_t> auto& out)
    {
        gather_words(in);

        const auto weightings = collate();

        m_huff_tree = huffman_tree(weightings);

        auto out_it = write_header(out);

        auto out_range = std::ranges::subrange(out_it, out.end());
        compress(in, out_range);

        return tzip_info(in.size(), m_bytes_written);
    }

    void unzip(std::ranges::input_range auto& in,
               std::ranges::output_range<uint8_t> auto& out)
    {
        auto traversal_node = m_huff_tree.get_root();

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
    void emplace_or_increment(std::string& key, auto& weightings)
    {
        const auto [it, success] = weightings.try_emplace(key, key.length());

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
    std::unordered_map<std::string, std::size_t> m_weightings2;
    uint8_t m_output_byte = 0;
    int m_byte_index = 0;
    std::size_t m_bytes_written = 0;
    huffman_tree m_huff_tree;
};
