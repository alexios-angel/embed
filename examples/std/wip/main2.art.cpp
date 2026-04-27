#include <phd/embed.hpp>

int main (int, char*[]) {
	constexpr std::span<const char> data = phd::embed("art.txt");
	return data[0];
}
