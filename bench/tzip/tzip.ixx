
export module tzip;

#pragma warning(disable:5050)
import std.core;

import ordered_vector;

using pair_type = std::pair<std::string, std::size_t>;

class node
{
public:
	node(pair_type data)
		: m_data(data)
	{
	}

public:
	auto string() const -> const std::string&
	{
		return m_data.first;
	}

	auto weighting() const -> std::size_t
	{
		return m_data.second;
	}

	void set_children(const std::shared_ptr<node>& left, 
					  const std::shared_ptr<node>& right)
	{
		m_left = left;
		m_right = right;
	}

	const auto& left() const
	{
		return m_left;
	}

	const auto& right() const
	{
		return m_right;
	}

private:
	pair_type m_data;
	std::shared_ptr<node> m_left;
	std::shared_ptr<node> m_right;
};

class huffman_tree
{
public:
	struct comp
	{
		constexpr bool operator()(const auto& l, const auto& r) const noexcept
		{
			return l.second < r.second || (l.second == r.second && l.first < r.first);
		}
	};

	struct ptr_comp
	{
		constexpr bool operator()(const auto& l, const auto& r) const noexcept
		{
			return l->weighting() < r->weighting() || (l->weighting() == r->weighting() && l->string() < r->string());
		}
	};

	// TODO REMOVE
	auto get_root()
	{
		return m_root;
	}

	huffman_tree() = default;

	huffman_tree(const std::vector<pair_type>& weightings_vec)
	{
		auto weightings_set = std::set<pair_type, comp>();

		for (auto& [string, weighting] : weightings_vec)
			weightings_set.insert(pair_type(string, weighting));

		auto nodes = ordered_vector<std::shared_ptr<node>, ptr_comp>();

		auto weightings_it = weightings_set.begin();

		auto smallest = weightings_set.extract(weightings_it++);
		auto next_smallest = weightings_set.extract(weightings_it);

		auto left_child = std::make_shared<node>(smallest.value());
		auto right_child = std::make_shared<node>(next_smallest.value());

		nodes.insert(make_parent(left_child, right_child));

		while (!weightings_set.empty())
		{
			weightings_it = weightings_set.begin();

			if (weightings_it->second < nodes[0]->weighting())
			{
				smallest = weightings_set.extract(weightings_it++);
				left_child = make_child(smallest.value());

				if (weightings_it != weightings_set.end()
					&& weightings_it->second < nodes[0]->weighting())
				{
					next_smallest = weightings_set.extract(weightings_it);
					right_child = make_child(next_smallest.value());
				}
				else
				{
					right_child = nodes.shift();
				}
			}
			else
			{
				left_child = nodes.shift();

				if (!nodes.is_empty() && nodes[0]->weighting() < weightings_it->second)
				{
					right_child = nodes.shift();
				}
				else
				{
					next_smallest = weightings_set.extract(weightings_it);
					right_child = make_child(next_smallest.value());
				}
			}

			nodes.insert(make_parent(left_child, right_child));
		}

		while (nodes.size() > 1)
		{
			left_child = nodes.shift();
			right_child = nodes.shift();

			nodes.insert(make_parent(left_child, right_child));
		}

		m_root = nodes[0];

		//print_tree();

		make_tree_patterns();
	}

public:
	const auto& operator[](const std::string& key) const
	{
		return m_tree_patterns.at(key);
	}

private:
	auto make_parent(const std::shared_ptr<node>& left,
					 const std::shared_ptr<node>& right) -> std::shared_ptr<node>
	{
		const auto weighting = left->weighting() + right->weighting();
		auto parent = std::make_shared<node>(pair_type("", weighting));

		parent->set_children(left, right);

		return parent;
	}

	auto make_child(pair_type data) -> std::shared_ptr<node>
	{
		return std::make_shared<node>(data);
	}

	void make_tree_patterns()
	{
		using node_path = std::pair<std::shared_ptr<node>, std::vector<uint8_t>>;

		auto stack = std::stack<node_path>();

		stack.push(node_path(m_root, std::vector<uint8_t>()));

		while (!stack.empty())
		{
			auto top = stack.top();
			stack.pop();

			if (!top.first->string().empty())
			{
				m_tree_patterns.emplace(top.first->string(), std::move(top.second));
				continue;
			}
			
			if (top.first->left()) {
				auto v = top.second;
				v.push_back(0);
				stack.push(node_path(top.first->left(), std::move(v)));
			}
			if (top.first->right())
			{
				auto v = top.second;
				v.push_back(1);
				stack.push(node_path(top.first->right(), std::move(v)));
			}
		}
	}

	void print_tree()
	{
		auto q = std::queue<std::shared_ptr<node>>();
		q.push(m_root);

		auto level = 0;

		while (!q.empty())
		{
			auto size = q.size();

			std::cout << std::format("level: {} ", level++);

			while (size--)
			{
				auto front = q.front();
				q.pop();

				std::cout << std::format("{}: {} ",
										 front->string(),
										 front->weighting());

				if (front->left())
					q.push(front->left());
				if (front->right())
					q.push(front->right());
			}

			std::cout << '\n';
		}
	}

private:
	std::shared_ptr<node> m_root;
	std::unordered_map<std::string, std::vector<uint8_t>> m_tree_patterns;
};

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

		compress(in, out);

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
