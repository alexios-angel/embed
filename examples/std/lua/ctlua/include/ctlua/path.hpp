#ifndef PHD_EXAMPLES_STD_LUA_CTLUA_PATH_HPP
#define PHD_EXAMPLES_STD_LUA_CTLUA_PATH_HPP

#include <string_view>

namespace ctlua {

	constexpr std::string_view stem_root(std::string_view file) noexcept {
		const std::size_t last_index = file.find_last_of("\\/");
		if (last_index == std::string_view::npos) {
			return std::string_view(file.data(), 0);
		}
		return std::string_view(file.data(), last_index);
	}

} // namespace ctlua

#endif
