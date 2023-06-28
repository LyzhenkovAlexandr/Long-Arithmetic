#pragma once

#include "MyDumbVector.h"
#include <string_view>

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cinttypes>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <stack>
#include <stdexcept>
#include <utility>

class LN
{
  public:
	LN(const LN &other) = default;
	LN(LN &&other) = default;
	LN(long long n = 0);
	LN(const char *str);
	LN(std::string_view sv);

	LN &operator=(const LN &other) = default;
	LN &operator=(LN &&other) = default;

	LN operator+(const LN &other) const;
	LN &operator+=(const LN &other);
	LN operator-(const LN &other) const;
	LN &operator-=(const LN &other);
	LN operator*(const LN &other) const;
	LN &operator*=(const LN &other);
	LN operator/(const LN &other) const;
	LN &operator/=(const LN &other);
	LN operator%(const LN &other) const;
	LN &operator%=(const LN &other);
	LN operator-() const;
	LN operator~() const;

	std::partial_ordering operator<=>(const LN &other) const;
	bool operator<(const LN &) const;
	bool operator<=(const LN &) const;
	bool operator>(const LN &) const;
	bool operator>=(const LN &) const;
	bool operator==(const LN &other) const;
	bool operator!=(const LN &) const;

	operator long long() const;
	operator bool() const;

	bool IsNaN() const;
	std::string ToString() const;
	static LN GetNaN();

  private:
	using Block = uint32_t;
	static constexpr size_t bits_in_digit_ = 4;
	static constexpr size_t digits_in_block_ = sizeof(Block) * 8 / bits_in_digit_;

	static const uint32_t *tr_16_char_to_int();
	int sign_ = 1;
	bool is_nan_ = false;
	MyDumbVector< Block > data_;

	static const LN NaN_;
	static const LN LONG_LONG_MAX_;
	static const LN LONG_LONG_MIN_;

	std::strong_ordering abs_compare(const LN &other) const;

	Block get_block(size_t i) const;

	void Div2();
	static LN SaneAdd(const LN &left, const LN &right);

	// left > right
	static LN SaneSub(const LN &left, const LN &right);

	void BlockShift(size_t blocks);
	static LN SingleMul(const LN &num1, Block num2);

	static LN KaraMul(const LN &num1, const LN &num2);
	void Split(LN &high, LN &low, size_t m) const;

	static void divmnu(LN *q, LN *r, const LN &u, const LN &v);

	template< typename Iter >
	void FromStringLike(Iter begin, Iter end)
	{
		if (end == begin)
		{
			is_nan_ = true;
		}
		while (end > begin)
		{
			Block b = 0;
			size_t block_size = std::min< size_t >(end - begin, digits_in_block_);
			auto block_start = end - block_size;
			for (auto digit = block_start; digit < end; ++digit)
			{
				b <<= bits_in_digit_;
				Block dig_number = tr_16_char_to_int()[std::toupper(*digit)];
				if (dig_number == 0xffffffff)
				{
					is_nan_ = true;
					return;
				}
				b |= dig_number;
			}
			data_.push_back(b);
			end = block_start;
		}
	}
};

LN operator""_ln(const char *str);
