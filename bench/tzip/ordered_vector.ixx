
export module ordered_vector;

#pragma warning(disable:5050)
import std.core;

export template <typename T, typename Compare = std::less<>>
class ordered_vector
{
public:
	ordered_vector()
	{
	}

public:
	auto begin()  { return m_data.begin();  }
	auto end()    { return m_data.end();    }
	auto cbegin() { return m_data.cbegin(); }
	auto cend()   { return m_data.cend();   }

	auto insert(T item)
	{
		namespace rng = std::ranges;
		return m_data.insert(rng::upper_bound(m_data,
											  item,
											  Compare()),
							 std::move(item));
	}

	void pop_front()
	{
		m_data.erase(begin());
	}

	auto shift()
	{
		auto front = m_data[0];
		pop_front();

		return front;
	}

	auto is_empty() { return m_data.empty(); }
	auto size() { return m_data.size(); }

	auto& operator[](std::size_t idx)
	{
		return m_data[idx];
	}

private:
	std::vector<T> m_data;
};
