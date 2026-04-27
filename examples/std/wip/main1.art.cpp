#include <phd/embed.hpp>
	

int main (int, char*[]) {
	constexpr std::span<const char> data = phd::embed("foo/art.txt");
	static_assert(data[0] == ' ');
	return 0;
}
