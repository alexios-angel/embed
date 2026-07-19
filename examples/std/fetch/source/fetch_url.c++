// std::fetch smoke example: fetch https://example.com/ DURING CONSTANT
// EVALUATION and embed the response bytes. Compile with
//   clang++ -std=c++2d --fetch-allow='https://example.com/' \
//           --fetch-allow='https://example.com/**' fetch_url.c++
// Without --fetch-allow the first static_assert fires (NotAuthorized) —
// the build scripts rely on that as the negative test.

#if !defined(__has_builtin) || !__has_builtin(__builtin_std_fetch)
#error "this compiler has no __builtin_std_fetch"
#endif

#include <phd/fetch.hpp>

#include <cstddef>
#include <cstdio>
#include <span>
#include <string_view>

namespace {
	// mirrors the status codes in clang's BI__builtin_std_fetch evaluator
	enum fetch_status : int {
		fetch_ok             = 1,
		fetch_not_authorized = 2,
		fetch_failed         = 3,
		fetch_empty          = 4,
		fetch_hash_mismatch  = 5
	};

	struct fetch_result {
		int status;
		std::span<const char> body;
	};

	consteval fetch_result fetch(std::string_view url) noexcept {
		int status           = -1;
		std::size_t len      = 0;
		const char* res      = nullptr;
		res = __builtin_std_fetch(0u, status, len, res, url.size(), url.data());
		return { status, { res, res == nullptr ? 0 : len } };
	}
} // namespace

int main(int, char*[]) {
	static constexpr fetch_result page = fetch("https://example.com/");
	static_assert(page.status == fetch_ok,
	     "compile-time fetch of https://example.com/ must succeed "
	     "(needs network and --fetch-allow='https://example.com/')");
	static_assert(page.body.size() > 0);

	// a URL outside the allow-list must come back NotAuthorized, not fetched
	static constexpr fetch_result denied = fetch("https://not-allow-listed.invalid/");
	static_assert(denied.status == fetch_not_authorized);
	static_assert(denied.body.data() == nullptr);

	// the public wrapper agrees with the raw builtin
	static constexpr std::span<const char> via_header = phd::fetch<char>("https://example.com/");
	static_assert(via_header.size() == page.body.size());

	std::printf("FETCH-SMOKE-PASSED (%zu bytes fetched at compile time)\n",
	     page.body.size());
	return 0;
}
