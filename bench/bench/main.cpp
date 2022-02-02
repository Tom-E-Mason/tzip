
#pragma warning(disable:5050)
import std.core;

import timer;
import tzip_test;
import zlib_test;

void print_result(std::size_t original_size,
				  std::size_t compressed_size,
				  std::string name);
auto load_file(std::string filename) -> std::vector<uint8_t>;
auto percentage(std::size_t num, std::size_t dom) -> float;

int main()
{
	std::cout << "Hello, world\n";

	using namespace std::chrono_literals;

	auto file_data = load_file("../test.txt");
	const auto original_size = file_data.size();

	std::cout << std::format("original size: {} bytes\n", original_size);

	auto output_buffer = std::vector<uint8_t>(original_size * 2);

	const auto zlib_output_size = [&]
	{
		auto t = timer("zlib");
		return test_zlib(file_data, output_buffer, 9);
	}();

	print_result(original_size, zlib_output_size, "zlib");

	std::ranges::fill(output_buffer, 0);

	const auto tzip_output_size = [&]
	{
		auto t = timer("tzip");
		return test_tzip(file_data, output_buffer);
	}();

	print_result(original_size, tzip_output_size, "tzip");
}

auto load_file(std::string filename)->std::vector<uint8_t>
{
	auto bytes = std::vector<uint8_t>();
	auto ifs = std::ifstream(filename);

	uint8_t byte;
	while (ifs >> std::noskipws >> byte)
		bytes.push_back(byte);

	return bytes;
}

auto percentage(std::size_t num, std::size_t dom) -> float
{
	return static_cast<float>(num) / static_cast<float>(dom) * 100.0f;
}

void print_result(std::size_t original_size,
				  std::size_t compressed_size,
				  std::string name)
{
	const auto percent = percentage(compressed_size, original_size);
	std::cout << std::format("{}: {} bytes ({}%)\n",
							 name,
							 compressed_size,
							 percent);
}
