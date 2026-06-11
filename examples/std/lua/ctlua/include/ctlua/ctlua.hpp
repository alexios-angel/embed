#ifndef PHD_EXAMPLES_STD_LUA_CTLUA_CTLUA_HPP
#define PHD_EXAMPLES_STD_LUA_CTLUA_CTLUA_HPP

#include <ctlua/core.hpp>
#include <ctlua/path.hpp>
#include <ctlua/parse.hpp>
#include <ctlua/execute.hpp>

#include <phd/embed.hpp>

#include <string_view>
#include <string>
#include <vector>
#include <array>
#include <variant>
#include <optional>
#include <cstddef>
#include <climits>
#include <algorithm>


namespace ctlua {

	using big_buffer = std::array<char, 8192>;

	consteval void recursive_parse_lua(std::string_view file, big_buffer& buf, std::size_t& buf_i, int depth) {
		using filename_buffer = std::array<char, 512>;
		auto data = phd::embed<char>(file);
		for (std::size_t i = 0; i < data.size(); ++i) {
			const char& c = data[i];
			switch (c) {
			case 'r': {
				std::size_t characters_left = data.size() - i;
				if (characters_left < 10) {
					buf[buf_i] = c;
					++buf_i;
					break;
				}
				std::string_view maybe_requires(&c, 9);
				const bool single_quote = maybe_requires == "require '";
				const bool double_quote = maybe_requires == "require \"";
				if (!single_quote && !double_quote) {
					buf[buf_i] = c;
					++buf_i;
					break;
				}
				// we start a "require" check
				const char* new_file_start  = &c + 9;
				const char* new_file_finish = new_file_start;
				std::size_t file_read_i     = i + 9;
				for (; file_read_i < data.size(); ++file_read_i) {
					const char& fc = data[file_read_i];
					if (single_quote && fc == '\'') {
						new_file_finish = &fc;
						break;
					}
					if (double_quote && fc == '"') {
						new_file_finish = &fc;
						break;
					}
					if (fc == '\n' || fc == '\r') {
						// directive is over, probably an error
						throw "bad end of require directive";
					}
				}
				if (new_file_start == new_file_finish) {
					throw "require directive must be supplied a proper string name";
				}
				auto base                       = stem_root(file);
				filename_buffer new_file_buffer = { };
				std::size_t new_file_buffer_i   = 0;
				buf[buf_i]                      = '-';
				++buf_i;
				buf[buf_i] = '-';
				++buf_i;
				buf[buf_i] = ' ';
				++buf_i;
				if (!base.empty()) {
					for (auto base_i = base.cbegin(), base_finish = base.cend(); base_i != base_finish; ++base_i) {
						new_file_buffer[new_file_buffer_i] = *base_i;
						++new_file_buffer_i;
					}
					new_file_buffer[new_file_buffer_i] = '/';
					++new_file_buffer_i;
				}
				for (const char* new_file_i = new_file_start; new_file_i != new_file_finish; ++new_file_i) {
					buf[buf_i] = *new_file_i;
					++buf_i;
					new_file_buffer[new_file_buffer_i] = *new_file_i;
					++new_file_buffer_i;
				}
				buf[buf_i] = '\n';
				++buf_i;

				std::string_view new_file = std::string_view(new_file_buffer.data(), new_file_buffer_i);
				recursive_parse_lua(new_file, buf, buf_i, depth + 1);
				i = file_read_i;
				break;
			}
			default:
				buf[buf_i] = c;
				++buf_i;
				break;
			}
		}
	}

	consteval big_buffer recursive_parse_lua(std::string_view file) {
		std::size_t buf_i = 0;
		big_buffer buffer = { };
		recursive_parse_lua(file, buffer, buf_i, 0);
		return buffer;
	}
} // namespace ctlua

#endif
