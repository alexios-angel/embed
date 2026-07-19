// =============================================================================
// phd::fetch
//
// Written in 2026 by the std::embed / std::fetch toolchain authors.
//
// To the extent possible under law, the author(s) have dedicated all copyright and related
// and neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.

// You should have received a copy of the CC0 Public Domain Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

#ifndef PHD_FETCH_HPP
#define PHD_FETCH_HPP

#pragma once

// std::embed's network sibling: perform an HTTP(S) GET DURING CONSTANT
// EVALUATION and hand back the response bytes as a span into
// compiler-materialized storage.
//
//   constexpr auto page = phd::fetch("https://example.com/");
//   constexpr auto pinned
//        = phd::fetch("https://example.com/", "aabb...ff"); // SHA-256 hex
//
//  - consteval only; there is no runtime cost and no runtime networking.
//  - the compiler must authorize every URL: pass one or more
//    --fetch-allow=<url-glob> (`*` matches within a path segment, `**`
//    matches across '/'). An un-authorized URL fails the compile.
//  - there is NO caching: every build performs the fetch. Pin content
//    with the optional lowercase-hex SHA-256 argument for reproducible
//    builds.
//  - a clang built without curl support diagnoses the fetch itself; this
//    header only compiles where __builtin_std_fetch exists.

#if !defined(PHD_FETCH_HAS_BUILTIN)
#if defined(__has_builtin)
#define PHD_FETCH_HAS_BUILTIN(...) __has_builtin(__VA_ARGS__)
#else
#define PHD_FETCH_HAS_BUILTIN(...) 0
#endif // __has_builtin ability
#endif // undefined PHD_FETCH_HAS_BUILTIN

#if !defined(PHD_FETCH_HAS_CONSTEXPR_EXCEPTIONS)
#if defined(__cpp_exceptions) && (__cpp_exceptions != 0) && defined(__cpp_constexpr_exceptions) \
     && (__cpp_constexpr_exceptions) != 0 && defined(__cpp_lib_constexpr_exceptions)            \
     && (__cpp_lib_constexpr_exceptions) != 0
#define PHD_FETCH_HAS_CONSTEXPR_EXCEPTIONS_I 1
#else
#define PHD_FETCH_HAS_CONSTEXPR_EXCEPTIONS_I 0
#endif
#elif PHD_FETCH_HAS_CONSTEXPR_EXCEPTIONS != 0
#define PHD_FETCH_HAS_CONSTEXPR_EXCEPTIONS_I 1
#else
#define PHD_FETCH_HAS_CONSTEXPR_EXCEPTIONS_I 0
#endif

#if (PHD_FETCH_HAS_BUILTIN(__builtin_std_fetch))

#include <cstddef>
#include <span>
#include <type_traits>
#include <string_view>
#include <string>
#include <optional>
#include <cassert>
#include <cstdlib>
#include <exception>
#include <source_location>

#if PHD_FETCH_HAS_BUILTIN(__builtin_abort)
#define PHD_FETCH_VERBOSE_ABORT(...) \
	do {                            \
		assert(__VA_ARGS__);       \
		__builtin_abort();         \
	} while (0)
#else
#define PHD_FETCH_VERBOSE_ABORT(...) \
	do {                            \
		assert(__VA_ARGS__);       \
		::std::abort();            \
	} while (0)
#endif

#define PHD_FETCH_CONCAT_(_XTOK, _YTOK) _XTOK##_YTOK
#define PHD_FETCH_CONCAT(_XTOK, _YTOK) PHD_FETCH_CONCAT_(_XTOK, _YTOK)

#if PHD_FETCH_HAS_CONSTEXPR_EXCEPTIONS_I != 0
#define PHD_FETCH_FAIL_WITH(_TY, _MSG, _LOC) throw _TY(PHD_FETCH_CONCAT(u8, _MSG), _LOC)
#else
#define PHD_FETCH_FAIL_WITH(_TY, _MSG, _LOC) PHD_FETCH_VERBOSE_ABORT(_MSG)
#endif

namespace phd {

	class fetch_error : public std::exception {
	private:
		::std::optional<::std::string> __what;
		::std::u8string __u8what;
		::std::source_location __where;

	public:
		consteval fetch_error(::std::u8string_view __in_u8what,
		     ::std::source_location __in_where = ::std::source_location::current()) noexcept
		: __what(), __u8what(__in_u8what), __where(::std::move(__in_where)) {
			// FIXME: we are just assuming the ordinary literal encoding can handle UTF-8.
			const ::std::size_t __u8what_size = __u8what.size();
			__what.emplace(__u8what_size, '\0');
			::std::string& __wh = *__what;
			for (::std::size_t __idx = 0; __idx < __u8what_size; ++__idx)
				__wh[__idx] = static_cast<char>(__u8what[__idx]);
		}

		consteval fetch_error(::std::string_view __in_what,
		     ::std::source_location __in_where = ::std::source_location::current()) noexcept
		: __what(__in_what), __u8what(), __where(::std::move(__in_where)) {
			const ::std::size_t __what_size = __what->size();
			__u8what.resize(__what_size);
			::std::string& __wh = *__what;
			for (::std::size_t __idx = 0; __idx < __what_size; ++__idx)
				__u8what[__idx] = __wh[__idx];
		}

		fetch_error(const fetch_error&) = default;
		fetch_error(fetch_error&&)      = default;

		fetch_error& operator=(const fetch_error&) = default;
		fetch_error& operator=(fetch_error&&)      = default;

		constexpr const char* what() const noexcept override {
			return __what ? __what->c_str() : "";
		}
		consteval ::std::u8string_view u8what() const noexcept {
			return __u8what;
		}
		consteval ::std::source_location where() const noexcept {
			return __where;
		}
	};

	// the URL matched no --fetch-allow=<url-glob>
	class not_authorized_error : public fetch_error {
	public:
		using fetch_error::fetch_error;
	};

	// the HTTP(S) request completed with a non-200 response
	// (transport-level failures are diagnosed by the compiler itself)
	class fetch_failed_error : public fetch_error {
	public:
		using fetch_error::fetch_error;
	};

	// the response body's SHA-256 did not match the requested pin
	class hash_mismatch_error : public fetch_error {
	public:
		using fetch_error::fetch_error;
	};

	namespace __fetch_detail {
		// a URL is absolute: there is no quoted-search anchor to honor
		inline constexpr const unsigned int __absolute_locus = 0u;

		enum __builtin_result {
			__ok             = 1,
			__not_authorized = 2,
			__fetch_failed   = 3,
			__empty          = 4,
			__hash_mismatch  = 5
		};

		template <typename _Ty, ::std::size_t _Extent, typename _StrView>
		inline consteval ::std::span<const _Ty, _Extent> __fetch(
		     const _StrView& __url, ::std::string_view __sha256_hex) noexcept {
			static_assert(::std::is_same_v<_Ty, unsigned char> || ::std::is_same_v<_Ty, char>
			          || ::std::is_same_v<_Ty, ::std::byte>,
			     "Type must be either 'char', 'unsigned char', or 'std::byte'");
			int __status     = -1;
			const _Ty* __res = nullptr;
			::std::size_t __res_len = 0;
			if (__sha256_hex.empty()) {
				__res = __builtin_std_fetch(__absolute_locus, __status, __res_len, __res,
				     __url.size(), __url.data());
			}
			else {
				__res = __builtin_std_fetch(__absolute_locus, __status, __res_len, __res,
				     __url.size(), __url.data(), __sha256_hex.size(), __sha256_hex.data());
			}
			if (__status == __not_authorized) {
				[[maybe_unused]] auto __loc = ::std::source_location::current();
				PHD_FETCH_FAIL_WITH(::phd::not_authorized_error,
				     "URL matches no --fetch-allow=<url-glob> given to the compiler", __loc);
			}
			else if (__status == __fetch_failed) {
				[[maybe_unused]] auto __loc = ::std::source_location::current();
				PHD_FETCH_FAIL_WITH(
				     ::phd::fetch_failed_error, "the HTTP(S) request returned a non-200 response", __loc);
			}
			else if (__status == __hash_mismatch) {
				[[maybe_unused]] auto __loc = ::std::source_location::current();
				PHD_FETCH_FAIL_WITH(::phd::hash_mismatch_error,
				     "the fetched bytes do not match the expected SHA-256 content hash", __loc);
			}
			if constexpr (_Extent != ::std::dynamic_extent) {
				if (__res_len < _Extent) {
					PHD_FETCH_VERBOSE_ABORT(
					     "the size of the fetched resource is too small for "
					     "the fixed-size span's extent");
				}
			}
			if (__status == __empty) {
				return ::std::span<const _Ty, _Extent>();
			}
			assert(__status == __ok
			     && "status value should be `1` at this point, indicating a successful "
			        "authorized fetch with a non-empty response body");
			return ::std::span<const _Ty, _Extent>(__res, __res_len);
		}
	} // namespace __fetch_detail

	template <typename _Ty = ::std::byte>
	[[nodiscard]]
	inline consteval ::std::span<const _Ty> fetch(
	     ::std::string_view __url, ::std::string_view __sha256_hex = {}) noexcept {
		return __fetch_detail::__fetch<_Ty, ::std::dynamic_extent>(::std::move(__url), __sha256_hex);
	}

#ifdef __cpp_char8_t

	template <typename _Ty = ::std::byte>
	[[nodiscard]]
	inline consteval ::std::span<const _Ty> fetch(
	     ::std::u8string_view __url, ::std::string_view __sha256_hex = {}) noexcept {
		return __fetch_detail::__fetch<_Ty, ::std::dynamic_extent>(::std::move(__url), __sha256_hex);
	}

#endif // char8_t shenanigans

	template <::std::size_t _Extent, typename _Ty = ::std::byte>
	[[nodiscard]]
	inline consteval ::std::span<const _Ty, _Extent> fetch(
	     ::std::string_view __url, ::std::string_view __sha256_hex = {}) noexcept {
		return __fetch_detail::__fetch<_Ty, _Extent>(::std::move(__url), __sha256_hex);
	}

#ifdef __cpp_char8_t

	template <::std::size_t _Extent, typename _Ty = ::std::byte>
	[[nodiscard]]
	inline consteval ::std::span<const _Ty, _Extent> fetch(
	     ::std::u8string_view __url, ::std::string_view __sha256_hex = {}) noexcept {
		return __fetch_detail::__fetch<_Ty, _Extent>(::std::move(__url), __sha256_hex);
	}

#endif // char8_t shenanigans

} // namespace phd

#undef PHD_FETCH_CONCAT
#undef PHD_FETCH_CONCAT_
#undef PHD_FETCH_FAIL_WITH
#undef PHD_FETCH_VERBOSE_ABORT

#else

#error This compiler does not support the required __builtin_std_fetch functionality

#endif

#endif // PHD_FETCH_HPP
