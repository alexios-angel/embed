#ifndef PHD_EXAMPLES_STD_LUA_CTLUA_EXECUTE_HPP
#define PHD_EXAMPLES_STD_LUA_CTLUA_EXECUTE_HPP

#include <ctlua/parse.hpp>
#include <ctlua/core.hpp>

namespace ctlua {

	struct execution {
		std::array<value, 256> results = {};
		std::string_view error = "";
	};

	namespace dtl {
		constexpr execution exec (syntax_tree& tree) {
			execution result;
			return result;
		}
	}

	constexpr execution execute(std::string_view file) {
		auto tree = parse(file);
		return dtl::exec(tree);
	}

} // namespace ctlua

#endif
