//   ----------
// | phd::embed |
//   ----------

// Written in 2026 by Björkus "ThePhD" Dorkus <phdofthehouse@gmail.com>

// To the extent possible under law, the author(s) have dedicated all copyright and related
// and neighboring rights to this software to the public domain worldwide. This software is
// distributed without any warranty.

// You should have received a copy of the CC0 Public Domain Dedication along with this software.
// If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.

#ifndef PHD_EMBED_HPP
#define PHD_EMBED_HPP

#pragma once

#include <phd/make_static.hpp>

#if !defined(PHD_EMBED_HAS_INCLUDE)
#if defined(__has_include)
#define PHD_EMBED_HAS_INCLUDE(...) __has_include(__VA_ARGS__)
#else
#define PHD_EMBED_HAS_INCLUDE(...) 0
#endif // __has_include ability
#endif // undefined PHD_EMBED_HAS_INCLUDE

#if !defined(PHD_EMBED_HAS_BUILTIN)
#if defined(__has_include)
#define PHD_EMBED_HAS_BUILTIN(...) __has_builtin(__VA_ARGS__)
#else
#define PHD_EMBED_HAS_BUILTIN(...) 0
#endif // __has_builtin ability
#endif // undefined PHD_EMBED_HAS_BUILTIN

#if defined(PHD_EMBED_HAS_INCLUDE) && (PHD_EMBED_HAS_INCLUDE(<version>))
#include <version>
#endif // <version> check

#if defined(PHD_EMBED_HAS_BUILTIN_STD_EMBED)
#if PHD_EMBED_HAS_BUILTIN_STD_EMBED != 0
#define PHD_EMBED_HAS_BUILTIN_STD_EMBED_I 1
#else
#define PHD_EMBED_HAS_BUILTIN_STD_EMBED_I 0
#endif
#elif PHD_EMBED_HAS_BUILTIN(__builtin_std_embed)
#define PHD_EMBED_HAS_BUILTIN_STD_EMBED_I 1
#else
#define PHD_EMBED_HAS_BUILTIN_STD_EMBED_I 0
#endif // __has_builtin test

#if defined(PHD_EMBED_USE_STD_EMBED)
#if PHD_EMBED_USE_STD_EMBED != 0
#define PHD_EMBED_USE_STD_EMBED_I 1
#else
#define PHD_EMBED_USE_STD_EMBED_I 0
#endif
#elif (defined(__cpp_lib_embed) && (__cpp_lib_embed >= 202611L)) && PHD_EMBED_HAS_INCLUDE(<embed>)
#define PHD_EMBED_USE_STD_EMBED_I 1
#else
#define PHD_EMBED_USE_STD_EMBED_I 0
#endif

#if defined(PHD_EMBED_HAS_BUILTIN_ABORT)
#if defined(PHD_EMBED_HAS_BUILTIN_ABORT) && PHD_EMBED_HAS_BUILTIN_ABORT != 0
#define PHD_EMBED_HAS_BUILTIN_ABORT_I 1
#else
#define PHD_EMBED_HAS_BUILTIN_ABORT_I 0
#endif
#elif PHD_EMBED_HAS_BUILTIN(__builtin_abort)
#define PHD_EMBED_HAS_BUILTIN_ABORT_I 1
#else
#define PHD_EMBED_HAS_BUILTIN_ABORT_I 0
#endif

#if defined(PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS)
#if defined(PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS) && PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS != 0
#define PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS_I 1
#else
#define PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS_I 0
#endif
#elif defined(__cpp_exceptions) && (__cpp_exceptions != 0) && defined(__cpp_constexpr_exceptions) \
     && (__cpp_constexpr_exceptions) != 0 && defined(__cpp_lib_constexpr_exceptions)              \
     && (__cpp_lib_constexpr_exceptions) != 0
#define PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS_I 1
#else
#define PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS_I 0
#endif

#if defined(PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT)
#if defined(PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT) && PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT != 0
#define PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT_I 1
#else
#define PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT_I 0
#endif
#elif PHD_EMBED_HAS_BUILTIN(__builtin_source_location_at)
#define PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT_I 1
#else
#define PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT_I 0
#endif

#if PHD_EMBED_USE_STD_EMBED_I == 1

#include <embed>

namespace phd { inline namespace v0 {

	using ::std::embed;

}} // namespace phd::v0

#elif (PHD_EMBED_HAS_BUILTIN_STD_EMBED_I != 0)

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

#if PHD_EMBED_HAS_BUILTIN_ABORT_I != 0
#define PHD_EMBED_VERBOSE_ABORT(...) \
	do {                            \
		assert(__VA_ARGS__);       \
		__builtin_abort();         \
	} while (0)
#else
#define PHD_EMBED_VERBOSE_ABORT(...) \
	do {                            \
		assert(__VA_ARGS__);       \
		::std::abort();            \
	} while (0)
#endif

#define PHD_EMBED_CONCAT_(_XTOK, _YTOK) _XTOK##_YTOK
#define PHD_EMBED_CONCAT(_XTOK, _YTOK) PHD_EMBED_CONCAT_(_XTOK, _YTOK)

#if PHD_EMBED_HAS_CONSTEXPR_EXCEPTIONS_I != 0
#define PHD_EMBED_FAIL_WITH(_TY, _MSG, _LOC) throw _TY(PHD_EMBED_CONCAT(u8, _MSG), _LOC)
#else
#define PHD_EMBED_FAIL_WITH(_TY, _MSG, _LOC) PHD_EMBED_VERBOSE_ABORT(_MSG)
#endif

namespace phd {

	class file_not_found_error : public std::exception {
	private:
		::std::optional<::std::string> __what;
		::std::u8string __u8what;
		::std::source_location __where;

	public:
		consteval file_not_found_error(::std::u8string_view __in_u8what,
		     ::std::source_location __in_where = ::std::source_location::current()) noexcept
		: __what(), __u8what(__in_u8what), __where(std::move(__in_where)) {
			const ::std::size_t __u8what_size = __u8what.size();
			__what.emplace(__u8what_size, '\0');
			::std::string& __wh = *__what;
			for (::std::size_t __idx = 0; __idx < __u8what_size; ++__idx)
				__wh[__idx] = static_cast<char>(__u8what[__idx]);
		}

		consteval file_not_found_error(::std::string_view __in_what,
		     ::std::source_location __in_where = ::std::source_location::current()) noexcept
		: __what(__in_what), __u8what(), __where(::std::move(__in_where)) {
			const ::std::size_t __what_size = __what->size();
			__u8what.resize(__what_size);
			::std::string& __wh = *__what;
			for (::std::size_t __idx = 0; __idx < __what_size; ++__idx)
				__u8what[__idx] = __wh[__idx];
		}

		file_not_found_error(const file_not_found_error&) = default;
		file_not_found_error(file_not_found_error&&)      = default;

		file_not_found_error& operator=(const file_not_found_error&) = default;
		file_not_found_error& operator=(file_not_found_error&&)      = default;

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

	class dependency_error : public std::exception {
	private:
		::std::optional<::std::string> __what;
		::std::u8string __u8what;
		::std::source_location __where;

	public:
		consteval dependency_error(::std::u8string_view __in_what,
		     ::std::source_location __in_where = ::std::source_location::current()) noexcept
		: __what(), __u8what(__in_what), __where(::std::move(__in_where)) {
			// FIXME: we are just assuming the ordinary literal encoding can handle UTF-8.
			const ::std::size_t __u8what_size = __u8what.size();
			__what.emplace(__u8what_size, '\0');
			::std::string& __wh = *__what;
			for (::std::size_t __idx = 0; __idx < __u8what_size; ++__idx)
				__wh[__idx] = static_cast<char>(__u8what[__idx]);
		}

		consteval dependency_error(::std::string_view __in_what,
		     ::std::source_location __in_where = ::std::source_location::current()) noexcept
		: __what(__in_what), __u8what(), __where(::std::move(__in_where)) {
			// FIXME: get specific about `__u8what` only containing UTF-8, or doing nothing when the ordinary literal
			// encoding is not proper
			const ::std::size_t __what_size = __what->size();
			__u8what.resize(__what_size);
			::std::string& __wh = *__what;
			for (::std::size_t __idx = 0; __idx < __what_size; ++__idx)
				__u8what[__idx] = __wh[__idx];
		}

		dependency_error(const dependency_error&) = default;
		dependency_error(dependency_error&&)      = default;

		dependency_error& operator=(const dependency_error&) = default;
		dependency_error& operator=(dependency_error&&)      = default;

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

	namespace __detail {
		inline constexpr const unsigned int __local_lookup = 0b101u;
		// how many constexpr call frames __embed sits below the user's
		// call site: the public embed() wrapper, then __embed itself. This
		// is the "call-local-distance" half of the locus (locus >> 1).
		inline constexpr const unsigned long long __call_stack_distance = __local_lookup >> 1u;

		enum __builtin_result {
			__file_not_found           = 0,
			__file_found               = 1,
			__file_found_but_no_depend = 2,
			__file_found_and_empty     = 3
		};

		template <typename _Ty, ::std::size_t _Extent, typename _StrView>
		inline consteval ::std::span<const _Ty, _Extent> __embed(const _StrView& __resource_name,
		     ::std::size_t __offset, const ::std::optional<::std::size_t>& __limit) noexcept {
#if 0
			static_assert(
			     (::std::is_integral_v<_Ty> || ::std::is_enum_v<_Ty>) && alignof(_Ty) == 1 && sizeof(_Ty) == 1,
			     "Type must have sizeof(T) == 1, alignof(T) == 1, and it must be an integral or "
			     "enumeration type");
#else
			static_assert(::std::is_same_v<_Ty, unsigned char> || ::std::is_same_v<_Ty, char>
			          || ::std::is_same_v<_Ty, ::std::byte>,
			     "Type must be either 'char', 'unsigned char', or 'std::byte'");
#endif
			int __status     = -1;
			const _Ty* __res = nullptr;
			::std::size_t __res_len = 0;
			if constexpr (_Extent != ::std::dynamic_extent) {
				__res = __builtin_std_embed(__local_lookup, __status, __res_len, __res, __resource_name.size(),
				     __resource_name.data(), __offset, _Extent);
			}
			else {
				if (__limit) {
					__res = __builtin_std_embed(__local_lookup, __status, __res_len, __res, __resource_name.size(),
					     __resource_name.data(), __offset, *__limit);
				}
				else {
					__res = __builtin_std_embed(__local_lookup, __status, __res_len, __res, __resource_name.size(),
					     __resource_name.data(), __offset);
				}
			}
			if (__status == __file_not_found) {
#if PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT_I != 0
				// FIXME: kind of cheating to use current() to do this, but that's how it is!
				auto __builtin_loc          = __builtin_source_location_at(__call_stack_distance);
				[[maybe_unused]] auto __loc = ::std::source_location::current(__builtin_loc);
#else
				[[maybe_unused]] auto __loc = ::std::source_location::current();
#endif
				PHD_EMBED_FAIL_WITH(::phd::file_not_found_error, "file not found", __loc);
			}
			else if (__status == __file_found_but_no_depend) {
#if PHD_EMBED_HAS_BUILTIN_SOURCE_LOCATION_AT_I != 0
				// FIXME: kind of cheating to use current() to do this, but that's how it is!
				auto __builtin_loc          = __builtin_source_location_at(__call_stack_distance);
				[[maybe_unused]] auto __loc = ::std::source_location::current(__builtin_loc);
#else
				[[maybe_unused]] auto __loc = ::std::source_location::current();
#endif
				PHD_EMBED_FAIL_WITH(
				     ::phd::dependency_error, "file found but not appropriately '#depend <>'-ed", __loc);
			}
			if constexpr (_Extent != ::std::dynamic_extent) {
				if (__res_len < _Extent) {
					PHD_EMBED_VERBOSE_ABORT(
					     "the size of the resource is too small for "
					     "the fixed-size span's extent");
				}
			}
			if (__status == __file_found_and_empty) {
				return ::std::span<const _Ty, _Extent>();
			}
			assert(__status == __file_found
			     && "status value should be `1` at this point, indicating a successful "
			        "finding of a file with data that has been depended on");
			return ::std::span<const _Ty, _Extent>(__res, __res_len);
		}
	} // namespace __detail

	template <typename _Ty = std::byte>
	[[nodiscard]]
	inline constexpr ::std::span<const _Ty> embed(::std::string_view __resource_name, ::std::size_t __offset = 0,
	     ::std::optional<::std::size_t> __limit = ::std::nullopt) noexcept {
		return __detail::__embed<_Ty, ::std::dynamic_extent>(::std::move(__resource_name), __offset, __limit);
	}

	template <typename _Ty = std::byte>
	[[nodiscard]]
	inline consteval ::std::span<const _Ty> embed(::std::wstring_view __resource_name, ::std::size_t __offset = 0,
	     ::std::optional<::std::size_t> __limit = ::std::nullopt) noexcept {
		return __detail::__embed<_Ty, ::std::dynamic_extent>(::std::move(__resource_name), __offset, __limit);
	}

#ifdef __cpp_char8_t

	template <typename _Ty = ::std::byte>
	[[nodiscard]]
	inline consteval ::std::span<const _Ty> embed(::std::u8string_view __resource_name, ::std::size_t __offset = 0,
	     ::std::optional<::std::size_t> __limit = ::std::nullopt) noexcept {
		return __detail::__embed<_Ty, ::std::dynamic_extent>(::std::move(__resource_name), __offset, __limit);
	}

#endif // char8_t shenanigans

	template <::std::size_t _Extent, typename _Ty = ::std::byte>
	[[nodiscard]]
	inline constexpr ::std::span<const _Ty, _Extent> embed(
	     ::std::string_view __resource_name, ::std::size_t __offset = 0) noexcept {
		return __detail::__embed<_Ty, _Extent>(::std::move(__resource_name), __offset, ::std::nullopt);
	}

	template <::std::size_t _Extent, typename _Ty = ::std::byte>
	inline consteval ::std::span<const _Ty, _Extent> embed(
	     ::std::wstring_view __resource_name, ::std::size_t __offset = 0) noexcept {
		return __detail::__embed<_Ty, _Extent>(::std::move(__resource_name), __offset, ::std::nullopt);
	}

#ifdef __cpp_char8_t

	template <::std::size_t _Extent, typename _Ty = ::std::byte>
	[[nodiscard]]
	inline consteval ::std::span<const _Ty, _Extent> embed(
	     ::std::u8string_view __resource_name, ::std::size_t __offset = 0) noexcept {
		return __detail::__embed<_Ty, _Extent>(::std::move(__resource_name), __offset, ::std::nullopt);
	}

#endif // char8_t shenanigans

} // namespace phd

#undef PHD_EMBED_CONCAT
#undef PHD_EMBED_CONCAT_
#undef PHD_EMBED_FAIL_WITH
#undef PHD_EMBED_VERBOSE_ABORT

#else

#error This compiler does not support the required <embed>/__builtin_std_embed functionality

#endif

#endif // PHD_EMBED_HPP
