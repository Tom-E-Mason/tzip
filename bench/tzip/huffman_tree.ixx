
export module huffman_tree;

#pragma warning(disable:5050)
import std.core;
import ordered_vector;

export using pair_type = std::pair<std::string, std::size_t>;

export class node
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

export class huffman_tree
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
