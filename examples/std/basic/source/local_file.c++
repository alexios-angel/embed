#include <phd/embed.hpp>

#depend "a_very_unique_file.*.txt"

int main(int, char*[]) {
	static constexpr std::span<const std::byte> data = phd::embed("a_very_unique_file.meow.txt");
	static_assert(data[0] == (std::byte)'m');
	static_assert(data[1] == (std::byte)'e');
	static_assert(data[2] == (std::byte)'o');
	static_assert(data[3] == (std::byte)'w');
	static_assert(data[4] == (std::byte)'!');
	static_assert(data[5] == (std::byte)'~');
	static_assert(data.size() == 6);
	return 0;
}
