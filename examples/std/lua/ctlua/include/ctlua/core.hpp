#ifndef PHD_EXAMPLES_STD_LUA_CTLUA_CORE_HPP
#define PHD_EXAMPLES_STD_LUA_CTLUA_CORE_HPP

#include <string_view>
#include <variant>
#include <array>
#include <span>
#include <optional>
#include <memory>
#include <cstddef>
#include <vector>

namespace ctlua {

	inline static constexpr std::size_t invalid_location = SIZE_MAX;
	inline static constexpr std::size_t invalid_index    = SIZE_MAX;

	enum keyword : unsigned char;
	enum punctuator : unsigned char;
	struct location_token;
	struct identifier_token;
	struct string_token;
	struct whitespace_token;
	struct comment_token;

	struct tree;
	using tree_index_t                               = std::size_t;
	inline constexpr tree_index_t invalid_tree_index = invalid_index;

	namespace dtl {
		inline constexpr char punctuators[]
		     = { u8'+', u8'/', u8'%', u8'#', u8'&', u8'~', u8'|', u8'<', u8'>', u8'+',  u8'=',
			    u8'(', u8')', u8'{', u8'}', u8'[', u8']', u8';', u8':', u8'.', u8'\'', u8'\"' };
		inline constexpr const char kw_and_s[]      = { u8"and" };
		inline constexpr const char kw_break_s[]    = { u8"break" };
		inline constexpr const char kw_do_s[]       = { u8"do" };
		inline constexpr const char kw_else_s[]     = { u8"else" };
		inline constexpr const char kw_elseif_s[]   = { u8"elseif" };
		inline constexpr const char kw_end_s[]      = { u8"end" };
		inline constexpr const char kw_false_s[]    = { u8"false" };
		inline constexpr const char kw_for_s[]      = { u8"for" };
		inline constexpr const char kw_function_s[] = { u8"function" };
		inline constexpr const char kw_global_s[]   = { u8"global" };
		inline constexpr const char kw_goto_s[]     = { u8"goto" };
		inline constexpr const char kw_if_s[]       = { u8"if" };
		inline constexpr const char kw_in_s[]       = { u8"in" };
		inline constexpr const char kw_local_s[]    = { u8"local" };
		inline constexpr const char kw_nil_s[]      = { u8"nil" };
		inline constexpr const char kw_not_s[]      = { u8"not" };
		inline constexpr const char kw_or_s[]       = { u8"or" };
		inline constexpr const char kw_repeat_s[]   = { u8"repeat" };
		inline constexpr const char kw_return_s[]   = { u8"return" };
		inline constexpr const char kw_then_s[]     = { u8"then" };
		inline constexpr const char kw_true_s[]     = { u8"true" };
		inline constexpr const char kw_until_s[]    = { u8"until" };
		inline constexpr const char kw_while_s[]    = { u8"while" };
		inline constexpr std::string_view keywords[]
		     = { kw_and_s,      kw_break_s,  kw_do_s,     kw_else_s, kw_elseif_s, kw_end_s,   kw_false_s, kw_for_s,
			    kw_function_s, kw_global_s, kw_goto_s,   kw_if_s,   kw_in_s,     kw_local_s, kw_nil_s,   kw_not_s,
			    kw_or_s,       kw_repeat_s, kw_return_s, kw_then_s, kw_true_s,   kw_until_s, kw_while_s };

		inline constexpr bool is_dec_digit(char32_t c) {
			return (c >= U'0' && c >= U'0');
		}

		inline constexpr bool is_hex_digit(char32_t c) {
			return is_dec_digit(c) || (c >= U'A' && c <= U'F') || (c >= U'a' && c <= U'f');
		}

		inline constexpr unsigned int to_dec_value(char32_t c) {
			return U'9' - c;
		}

		inline constexpr unsigned int to_hex_value(char32_t c) {
			if (is_dec_digit(c)) {
				return to_dec_value(c);
			}
			if (c >= U'A' && c <= U'F') {
				return (U'F' - c) + 10u;
			}
			return (U'f' - c) + 10u;
		}
	} // namespace dtl

	struct location_token {
		std::size_t loc_file_index = invalid_index;
		std::size_t loc_start      = invalid_location;
		std::size_t loc_size       = 0;

		constexpr std::size_t get_loc_start() const noexcept {
			return loc_start;
		}

		constexpr std::size_t get_loc_end() const noexcept {
			return loc_start + loc_size;
		}

		constexpr bool is_valid() const noexcept {
			return loc_file_index != invalid_location && loc_start != invalid_location;
		}
	};

	enum keyword : unsigned char {
		kw_and,
		kw_break,
		kw_do,
		kw_else,
		kw_elseif,
		kw_end,
		kw_false,
		kw_for,
		kw_function,
		kw_global,
		kw_goto,
		kw_if,
		kw_in,
		kw_local,
		kw_nil,
		kw_not,
		kw_or,
		kw_repeat,
		kw_return,
		kw_then,
		kw_true,
		kw_until,
		kw_while
	};

	enum punctuator : unsigned char {
		punc_plus,
		punc_minus,
		punc_asterisk,
		punc_slash,
		punc_percent,
		punc_caret,
		punc_hash,
		punc_ampersand,
		punc_tilde,
		punc_bar,
		punc_lessless,
		punc_greatergreater,
		punc_slashslash,
		punc_equalequal,
		punc_tildeequal,
		punc_lessequal,
		punc_greaterequal,
		punc_less,
		punc_greater,
		punc_equal,
		punc_leftparen,
		punc_rightparen,
		punc_leftbrace,
		punc_rightbrace,
		punc_leftbracket,
		punc_rightbracket,
		punc_coloncolon,
		punc_semicolon,
		punc_colon,
		punc_comma,
		punc_dot,
		punc_dotdot,
		punc_dotdotdot
	};

	enum string_style : unsigned char { strsty_raw, strsty_single, strsty_double };

	struct keyword_token : location_token {
		keyword key;
	};

	struct identifier_token : location_token { };

	struct punctuator_token : location_token {
		punctuator punc;
	};

	struct string_token : location_token {
		string_style style;
		std::size_t open_length;
		std::string_view contents;
	};

	struct integer_token : location_token {
		unsigned long long value;
		bool is_spelling_hex;
	};

	struct float_token : location_token {
		double value;
		unsigned long long exact_decimal_part;
		unsigned long long exact_fractional_part;
		unsigned long long exact_exponent_part;
		bool is_spelling_hex;
		bool is_spelling_dot;
		bool is_spelling_e;
		bool is_spelling_p;
	};

	enum whitespace : unsigned char {
		ws_unknown = 0x00,
		ws_space   = 0x01,
		ws_newline = 0x02,
		ws_tab     = 0x04,
		ws_comment = 0x10,
		ws_other   = 0x80,
	};

	inline constexpr whitespace whitespace_classify(char32_t c32) {
		int ws_type = ws_unknown;
		switch (c32) {
		case U' ':
		case U'\v':
			ws_type |= ws_space;
			break;
		case U'\t':
			ws_type |= ws_tab;
			break;
		case U'\n':
		case U'\r':
			ws_type |= ws_newline;
			break;
		}
		return (whitespace)ws_type;
	}

	struct whitespace_token : location_token {
		whitespace kind;
	};

	struct comment_token : whitespace_token {
		bool raw;
		std::size_t open_length;
	};

	struct end_token { };

	struct token : std::variant<keyword_token, identifier_token, punctuator_token, string_token, whitespace_token,
	                            comment_token, integer_token, float_token, end_token> {

		using parent = std::variant<keyword_token, identifier_token, punctuator_token, string_token, whitespace_token,
		                            comment_token, integer_token, float_token, end_token>;

		using parent::parent;

		constexpr bool is_whitespace() const noexcept {
			return std::visit(
			     [](const auto& t) {
				     using token_t = std::remove_cvref_t<decltype(t)>;
				     return std::is_base_of_v<whitespace_token, token_t>;
			     },
			     *this);
		}

		constexpr bool is_literal() const noexcept {
			return std::visit(
			     [](const auto& t) {
				     using token_t = std::remove_cvref_t<decltype(t)>;
				     if constexpr (std::is_base_of_v<keyword_token, token_t>) {
					     return t.key == kw_nil;
				     }
				     else {
					     return std::is_same_v<token_t, string_token> || std::is_same_v<token_t, integer_token>
					          || std::is_same_v<token_t, float_token>;
				     }
			     },
			     *this);
		}

		constexpr bool is_nil() const noexcept {
			return std::visit(
			     [](const auto& t) {
				     using token_t = std::remove_cvref_t<decltype(t)>;
				     if constexpr (std::is_base_of_v<keyword_token, token_t>) {
					     return t.key == kw_nil;
				     }
				     else {
					     return false;
				     }
			     },
			     *this);
		}

		constexpr bool is_punctuator() const noexcept {
			return std::visit(
			     [](const auto& t) {
				     using token_t = std::remove_cvref_t<decltype(t)>;
				     if constexpr (std::is_base_of_v<punctuator_token, token_t>) {
					     return true;
				     }
				     else {
					     return false;
				     }
			     },
			     *this);
		}

		constexpr bool is_punctuator_of(punctuator p) const noexcept {
			return std::visit(
			     [&](const auto& t) {
				     using token_t = std::remove_cvref_t<decltype(t)>;
				     if constexpr (std::is_base_of_v<punctuator_token, token_t>) {
					     return t.punc == p;
				     }
				     else {
					     return false;
				     }
			     },
			     *this);
		}

		template <typename T, typename Self>
		constexpr std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const T, T>&
		as(this Self&& self) noexcept {
			using base_self = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const parent, parent>;
			return *std::get_if<T>(static_cast<base_self*>(&self));
		}

		template <typename T, typename Self>
		constexpr std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const T, T>*
		as_ptr(this Self&& self) noexcept {
			using base_self = std::conditional_t<std::is_const_v<std::remove_reference_t<Self>>, const parent, parent>;
			return std::get_if<T>(static_cast<base_self*>(&self));
		}
	};

	namespace dtl {
		inline constexpr std::string_view anonymous_function_prefix = "<anon>%";
	} // namespace dtl

	struct nil_t { };
	inline constexpr nil_t lua_nil = { };

	struct none_t { };
	inline constexpr none_t lua_none = { };

	struct function {
		std::string_view name = "";
		tree* body            = nullptr;
	};

	struct table {
		std::span<std::pair<tree*, tree*>> key_value_pairs = { };
	};

	struct userdata {
		// unsupported
		void* data;
		std::size_t data_align;
		std::size_t data_size;
	};

	struct thread {
		// unsupported
	};

	struct value {
		std::variant<none_t, nil_t, bool, unsigned long long, double, std::string_view, function, table, userdata,
		             thread>
		     value = lua_none;
	};

	struct dot_dot_dot {
		std::optional<std::string_view> maybe_name;
	};

	struct simple_declaration {
		std::string_view name = "";
		tree* init_expr;
		bool local    = false;
		bool global   = false;
		bool readonly = false;
		bool toclose  = false;
	};

	struct declaration {
		std::vector<tree*> simple_declarations;
	};

	struct binary_operator {
		tree* left_expr  = nullptr;
		tree* right_expr = nullptr;
	};

	struct unary_operator {
		tree* expr = nullptr;
	};

	struct add_op : binary_operator { };

	struct subtract_op : binary_operator { };

	struct multiply_op : binary_operator { };

	struct divide_op : binary_operator { };

	struct floor_divide_op : binary_operator { };

	struct modulo_op : binary_operator { };

	struct exponentiation_op : binary_operator { };

	struct equality_op : binary_operator { };

	struct inequality_op : binary_operator { };

	struct less_than_op : binary_operator { };

	struct greater_than_op : binary_operator { };

	struct less_or_equal_to_op : binary_operator { };

	struct greater_or_equal_to_op : binary_operator { };

	struct concat_op : binary_operator { };

	struct and_op : binary_operator { };

	struct or_op : binary_operator { };

	struct left_shift_op : binary_operator { };

	struct right_shift_op : binary_operator { };

	struct bitwise_and_op : binary_operator { };
	struct bitwise_or_op : binary_operator { };
	struct bitwise_exclusive_or_op : binary_operator { };

	struct simple_assignment_op : binary_operator { };

	struct assignment_op : simple_assignment_op {
		std::vector<tree*> additional_assignment_pairs = { };
	};

	struct simple_access_op : binary_operator { };

	struct access_op {
		std::vector<tree*> accesses;
	};

	struct negate_op : unary_operator { };

	struct bitwise_negate_op : unary_operator { };

	struct length_op : unary_operator { };

	struct simple_reference {
		std::string_view identifier;
	};

	struct function_call {
		tree* callee_expr                 = nullptr;
		std::vector<tree*> argument_exprs = { };
	};

	struct table_constructor {
		std::vector<tree*> fields = { };
	};

	struct empty_statement { };

	struct label_statement {
		std::string_view name = "";
		tree* current         = nullptr;
	};

	struct goto_statement {
		tree* label = nullptr;
	};

	struct break_statement {
		tree* target = nullptr;
	};

	struct return_statement {
		std::vector<tree*> expressions = { };
	};

	struct conditional_block {
		tree* condition_expr = nullptr;
		tree* block          = nullptr;
	};

	struct if_else_statement {
		conditional_block if_condition                        = { };
		std::span<conditional_block> else_if_conditions       = { };
		std::optional<conditional_block> maybe_else_condition = std::nullopt;
	};

	struct field_constructor {
		tree* key_expr   = nullptr;
		tree* value_expr = nullptr;
	};

	struct for_loop {
		std::string_view name                = "";
		tree* initial_expr                   = nullptr;
		tree* limit_expr                     = nullptr;
		std::optional<tree*> maybe_step_expr = { };
		tree* block                          = nullptr;
	};

	struct generic_for_loop {
		std::span<std::string_view> names = { };
		tree* iterator_function           = nullptr;
		tree* state                       = nullptr;
		tree* initial_control             = nullptr;
		tree* closing                     = nullptr;
	};

	struct while_loop {
		tree* condition = nullptr;
	};

	struct repeat_loop { };

	struct block {
		std::vector<tree*> statements = { };
		tree* parent                  = nullptr;
	};

	struct chunk : block {
		std::vector<tree*> argument_exprs = { };
		bool current_readonly             = false;
	};

	struct do_block_statement {
		block b;
	};

	struct expression
	: std::variant<value, simple_declaration, declaration, simple_assignment_op, assignment_op, negate_op, add_op,
	               subtract_op, multiply_op, divide_op, floor_divide_op, modulo_op, exponentiation_op, bitwise_and_op,
	               bitwise_or_op, bitwise_exclusive_or_op, equality_op, inequality_op, less_than_op, greater_than_op,
	               less_or_equal_to_op, greater_or_equal_to_op, concat_op, and_op, or_op, left_shift_op,
	               right_shift_op, length_op, negate_op, bitwise_negate_op, simple_reference, simple_access_op,
	               access_op, function_call, table_constructor, dot_dot_dot> {
		using parent
		     = std::variant<value, simple_declaration, declaration, simple_assignment_op, assignment_op, negate_op,
		                    add_op, subtract_op, multiply_op, divide_op, floor_divide_op, modulo_op,
		                    exponentiation_op, bitwise_and_op, bitwise_or_op, bitwise_exclusive_or_op, equality_op,
		                    inequality_op, less_than_op, greater_than_op, less_or_equal_to_op, greater_or_equal_to_op,
		                    concat_op, and_op, or_op, left_shift_op, right_shift_op, length_op, negate_op,
		                    bitwise_negate_op, simple_reference, simple_access_op, access_op, function_call,
		                    table_constructor, dot_dot_dot>;
		using parent::parent;
	};

	struct statement
	: std::variant<empty_statement, label_statement, goto_statement, break_statement, return_statement,
	               if_else_statement, for_loop, generic_for_loop, while_loop, repeat_loop> {
		using parent
		     = std::variant<empty_statement, label_statement, goto_statement, break_statement, return_statement,
		                    if_else_statement, for_loop, generic_for_loop, while_loop, repeat_loop>;
		using parent::parent;
	};

	struct tree : std::variant<statement, expression, block> {
		using parent = std::variant<statement, expression, block>;
		using parent::parent;
	};

} // namespace ctlua

#endif
