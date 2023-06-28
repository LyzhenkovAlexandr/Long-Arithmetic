#include "LN.h"

const char HEX_DIGITS[] = "0123456789ABCDEF";
const LN LN::LONG_LONG_MAX_ = { std::numeric_limits< long long >::max() };
const LN LN::LONG_LONG_MIN_ = { std::numeric_limits< long long >::min() };
const LN LN::NaN_ = LN::GetNaN();

using std::uint32_t;
int nlz1(uint32_t x)
{
	int n;
	if (x == 0)
		return (32);
	n = 0;
	if (x <= 0x0000FFFF)
	{
		n = n + 16;
		x = x << 16;
	}
	if (x <= 0x00FFFFFF)
	{
		n = n + 8;
		x = x << 8;
	}
	if (x <= 0x0FFFFFFF)
	{
		n = n + 4;
		x = x << 4;
	}
	if (x <= 0x3FFFFFFF)
	{
		n = n + 2;
		x = x << 2;
	}
	if (x <= 0x7FFFFFFF)
	{
		n = n + 1;
	}
	return n;
}

LN::LN(long long n)
{
	is_nan_ = false;
	bool min_value = false;
	if (n == std::numeric_limits< long long >::min())
	{
		n += 1;
		min_value = true;
	}

	if (n < 0)
	{
		sign_ = -1;
		n = -n;
	}
	else
	{
		sign_ = 1;
	}

	constexpr Block all_ones = std::numeric_limits< Block >::max();
	size_t bits_in_block = sizeof(Block) * 8;

	while (n > 0)
	{
		data_.push_back(static_cast< Block >(n & all_ones));
		n >>= bits_in_block;
	}

	if (min_value)
	{
		*this -= 1LL;
	}
}

LN::LN(const char *str)
{
	if (!str)
	{
		return;
	}
	if (*str == '-')
	{
		sign_ = -1;
		str++;
	}

	const char *end = str;
	while (*end)
	{
		++end;
	}

	FromStringLike(str, end);
}

LN::LN(std::string_view sv)
{
	if (sv.empty())
	{
		return;
	}
	auto it = std::begin(sv);

	if (*it == '-')
	{
		sign_ = -1;
		++it;
	}

	FromStringLike(it, std::end(sv));
}

LN LN::operator+(const LN &other) const
{
	if (is_nan_ || other.is_nan_)
	{
		return NaN_;
	}
	else if (sign_ == other.sign_)
	{
		LN result = SaneAdd(*this, other);
		result.sign_ = sign_;	 // == other.sign_
		return result;
	}
	else
	{
		auto order = abs_compare(other);
		if (order == std::strong_ordering::equivalent)
		{	 //==
			return { 0LL };
		}
		else if (order == std::strong_ordering::less)
		{	 //<
			LN result = SaneSub(other, *this);
			result.sign_ = other.sign_;
			return result;
		}
		else
		{	 //>
			LN result = SaneSub(*this, other);
			result.sign_ = sign_;
			return result;
		}
	}
}

LN &LN::operator+=(const LN &other)
{
	*this = *this + other;
	return *this;
}

LN LN::operator-(const LN &other) const
{
	if (is_nan_ || other.is_nan_)
	{
		return NaN_;
	}
	else if (sign_ != other.sign_)
	{	 // - x - y => -(x + y); x - (-y) = x + y
		LN result = SaneAdd(*this, other);
		result.sign_ = sign_;
		return result;
	}
	else
	{
		auto order = abs_compare(other);
		if (order == std::strong_ordering::equivalent)
		{
			return { 0LL };
		}
		else if (order == std::strong_ordering::less)
		{
			LN result = SaneSub(other, *this);
			result.sign_ = -sign_;
			return result;
		}
		else
		{
			LN result = SaneSub(*this, other);
			result.sign_ = sign_;
			return result;
		}
	}
}

LN &LN::operator-=(const LN &other)
{
	*this = *this - other;
	return *this;
}

LN LN::operator*(const LN &other) const
{
	if (is_nan_ || other.is_nan_)
	{
		return NaN_;
	}
	else
	{
		LN result = KaraMul(*this, other);
		result.sign_ = sign_ * other.sign_;
		return result;
	}
}

LN &LN::operator*=(const LN &other)
{
	*this = *this * other;
	return *this;
}

LN LN::operator/(const LN &other) const
{
	LN quotitent;
	int new_sign = sign_ * other.sign_;
	divmnu(&quotitent, nullptr, *this, other);
	quotitent.sign_ = new_sign;
	return quotitent;
}

LN &LN::operator/=(const LN &other)
{
	int new_sign = sign_ * other.sign_;
	divmnu(this, nullptr, *this, other);
	sign_ = new_sign;
	return *this;
}

LN LN::operator%(const LN &other) const
{
	int new_sign = sign_ * other.sign_;
	LN quotitent;
	LN remainder;
	divmnu(&quotitent, &remainder, *this, other);
	remainder.sign_ = new_sign;
	return remainder;
}

LN &LN::operator%=(const LN &other)
{
	int new_sign = sign_ * other.sign_;
	LN quotitent;
	divmnu(&quotitent, this, *this, other);
	sign_ = new_sign;
	return *this;
}

LN LN::operator-() const
{
	LN result = *this;
	result.sign_ = -sign_;
	return result;
}

LN LN::operator~() const
{
	size_t n = data_.get_size();
	if (sign_ == -1)
	{
		return NaN_;
	}
	else if (n == 0)
	{
		return LN{ 0LL };
	}
	LN x;
	static const LN LN1{ 1LL };
	static const LN LN0{ 0LL };
	size_t init_size = (n + 1) / 2 + 1;
	x.data_ = MyDumbVector< Block >(init_size);
	x.data_[init_size - 1] = 1;	   // std::numeric_limits<Block>::max() / 2;
	while (true)
	{
		LN next = x;
		next += *this / next;
		next.Div2();

		LN diff = next - x;
		if (diff == LN1 || diff == LN0)
		{
			return x;
		}
		x = next;
	}
}

std::partial_ordering LN::operator<=>(const LN &other) const
{
	if (is_nan_ || other.is_nan_)
	{
		return std::partial_ordering::unordered;
	}
	else if (data_.get_size() == 0 && other.data_.get_size() == 0)
	{
		return std::partial_ordering::equivalent;
	}
	else if (sign_ == -1 && other.sign_ == 1)
	{
		return std::partial_ordering::less;
	}
	else if (sign_ == 1 && other.sign_ == -1)
	{
		return std::partial_ordering::greater;
	}
	else
	{
		std::strong_ordering abs_order = abs_compare(other);
		if (abs_order == std::strong_ordering::greater)
		{
			if (sign_ == 1)
			{
				return std::partial_ordering::greater;
			}
			else
			{
				return std::partial_ordering::less;
			}
		}
		else if (abs_order == std::strong_ordering::less)
		{
			if (sign_ == 1)
			{
				return std::partial_ordering::less;
			}
			else
			{
				return std::partial_ordering::greater;
			}
		}
		else
		{
			return std::partial_ordering::equivalent;
		}
	}
}

bool LN::operator<(const LN &) const = default;
bool LN::operator<=(const LN &) const = default;
bool LN::operator>(const LN &) const = default;
bool LN::operator>=(const LN &) const = default;
bool LN::operator==(const LN &other) const
{
	return (*this <=> other) == std::partial_ordering::equivalent;
}
bool LN::operator!=(const LN &) const = default;

LN::operator long long() const
{
	if (*this > LONG_LONG_MAX_ || *this < LONG_LONG_MIN_)
	{
		throw std::domain_error("Number is too large to be converted in long long");
	}
	else if (*this == LONG_LONG_MIN_)
	{
		return std::numeric_limits< long long >::min();
	}
	else if (is_nan_)
	{
		throw std::domain_error("NaN is not representable by long long");
	}
	else
	{
		long long result = 0;
		size_t blocks = data_.get_size();
		for (size_t i = blocks; i != 0; --i)
		{
			result <<= sizeof(Block) * 8;
			result |= static_cast< long long >(data_[i - 1]);
		}
		return result * sign_;
	}
}

LN::operator bool() const
{
	return data_.get_size() == 0;
}

std::string LN::ToString() const
{
	if (is_nan_)
	{
		return "NaN";
	}
	else if (*this == LN{ 0LL } || data_.get_size() == 0)
	{
		return "0";
	}
	else
	{
		std::string result;
		size_t blocks = data_.get_size();
		for (size_t i = 0; i < blocks; ++i)
		{
			Block b = data_[i];
			for (int j = 0; j < sizeof(Block) * 8; j += 4)
			{
				result.push_back(HEX_DIGITS[(b >> j) & 0xf]);
			}
		}
		while (result.back() == '0')
		{
			result.pop_back();
		}
		if (sign_ == -1)
		{
			result.push_back('-');
		}
		std::reverse(std::begin(result), std::end(result));
		return result;
	}
}

bool LN::IsNaN() const
{
	return is_nan_;
}

LN LN::GetNaN()
{
	LN nan;
	nan.is_nan_ = true;
	return nan;
}

std::strong_ordering LN::abs_compare(const LN &other) const
{
	std::strong_ordering abs_order = data_.get_size() <=> other.data_.get_size();
	if (abs_order == std::strong_ordering::equivalent)
	{
		size_t i = data_.get_size();
		do
		{
			i--;
			abs_order = data_[i] <=> other.data_[i];
		} while (i != 0 && abs_order == std::strong_ordering::equivalent);
	}
	return abs_order;
}

LN::Block LN::get_block(size_t i) const
{
	return i >= data_.get_size() ? 0 : data_[i];
}

void LN::Div2()
{
	size_t n = data_.get_size();
	for (size_t i = 0; i < n; ++i)
	{
		data_[i] = (data_[i] >> 1) | (get_block(i + 1) & 1);
	}
	if (data_[n - 1] == 0)
	{
		data_.pop();
	}
}

LN LN::SaneAdd(const LN &left, const LN &right)
{
	LN result;

	size_t i;
	size_t ls = left.data_.get_size();
	size_t rs = right.data_.get_size();

	Block carry = 0;

	for (i = 0; i < ls || i < rs || carry; ++i)
	{
		Block new_block;

		uint64_t el = left.get_block(i);
		uint64_t er = right.get_block(i);
		uint64_t ex = el + er;

		ex += carry;
		carry = 0;
		new_block = static_cast< Block >(ex & std::numeric_limits< Block >::max());
		carry = static_cast< Block >(ex >> sizeof(Block) * 8);
		result.data_.push_back(new_block);
	}

	return result;
}

// left > right
LN LN::SaneSub(const LN &left, const LN &right)
{
	LN result;

	size_t i;
	size_t ls = left.data_.get_size();
	size_t rs = right.data_.get_size();

	Block carry = 0;

	for (i = 0; i < ls || i < rs || carry; ++i)
	{
		Block new_block;

		uint64_t el = left.get_block(i);
		uint64_t er = right.get_block(i);

		er += carry;
		carry = 0;

		if (er > el)
		{
			el |= static_cast< uint64_t >(1) << (sizeof(Block) * 8);
			carry = 1;
		}

		el -= er;
		new_block = static_cast< Block >(el);
		result.data_.push_back(new_block);
	}

	while (result.data_.get_size() != 0 && result.data_[result.data_.get_size() - 1] == 0)
	{
		result.data_.pop();
	}

	return result;
}

void LN::BlockShift(size_t blocks)
{
	if (data_.get_size() == 0)
	{
		return;
	}
	for (size_t i = 0; i < blocks; ++i)
	{
		data_.push_back(0);
	}
	for (size_t i = data_.get_size(); i > blocks; --i)
	{
		std::swap(data_[i - 1], data_[i - blocks - 1]);
	}
}

LN LN::SingleMul(const LN &num1, Block num2)
{
	LN result;

	size_t i;
	size_t n = num1.data_.get_size();

	Block carry = 0;
	uint64_t er = static_cast< uint64_t >(num2);

	for (i = 0; i < n || carry; ++i)
	{
		Block new_block;

		uint64_t el = num1.get_block(i);
		uint64_t ex = el * er + carry;
		carry = 0;
		new_block = static_cast< Block >(ex & std::numeric_limits< Block >::max());
		carry = static_cast< Block >(ex >> sizeof(Block) * 8);
		result.data_.push_back(new_block);
	}

	return result;
}

LN LN::KaraMul(const LN &num1, const LN &num2)
{
	if (num1.data_.get_size() == 0 || num2.data_.get_size() == 0)
	{
		return { 0LL };
	}
	else if (num1.data_.get_size() == 1)
	{
		return SingleMul(num2, num1.data_[0]);
	}
	else if (num2.data_.get_size() == 1)
	{
		return SingleMul(num1, num2.data_[0]);
	}
	LN h1, h2, l1, l2;
	size_t m = std::max(num1.data_.get_size(), num2.data_.get_size()) / 2;
	num1.Split(h1, l1, m);
	num2.Split(h2, l2, m);
	LN z2 = KaraMul(h1, h2);
	LN z0 = KaraMul(l1, l2);
	LN z1 = KaraMul(l1 + h1, l2 + h2) - z2 - z0;
	z2.BlockShift(m);
	z2 += z1;
	z2.BlockShift(m);
	z2 += z0;
	return z2;
}

void LN::Split(LN &high, LN &low, size_t m) const
{
	size_t n = data_.get_size();
	for (size_t i = 0; i < std::min(m, n); ++i)
	{
		low.data_.push_back(data_[i]);
	}
	for (size_t i = m; i < n; ++i)
	{
		high.data_.push_back(data_[i]);
	}
}

/*
 * solves (u = v * q + r) for known u v
 * q -- quotitent
 * r -- remainder (can be null if not needed)
 * u -- divident
 * v -- divisor
 */
void LN::divmnu(LN *q, LN *r, const LN &u, const LN &v)
{
	if (q == nullptr)
	{
		return;
	}
	if (v == LN{ 0LL })
	{
		q->is_nan_ = true;
		if (r != nullptr)
		{
			r->is_nan_ = true;
		}
		return;
	}
	size_t m = u.data_.get_size();
	size_t n = v.data_.get_size();

	if (m < n)
	{
		*q = { 0LL };
		if (r != nullptr)
		{
			*r = u;
		}
		return;
	}
	else if (n <= 1)
	{
		auto qdata = MyDumbVector< Block >(m);
		uint64_t vb = v.data_[0];
		uint64_t carry = 0;
		for (int i = m - 1; i >= 0; --i)
		{
			uint64_t x = carry << sizeof(Block) * 8 | u.data_[i];
			carry = x % vb;
			x /= vb;
			qdata[i] = x;
		}
		while (q->data_.get_size() != 0 && qdata[q->data_.get_size() - 1] == 0)
		{
			qdata.pop();
		}
		q->data_ = std::move(qdata);
		if (r)
		{
			*r = LN{};
			if (carry != 0)
			{
				r->data_.push_back(carry);
			}
		}
		return;
	}

	const uint64_t b = static_cast< uint64_t >(1) << (sizeof(Block) * 8);

	int s = nlz1(v.data_[n - 1]);
	uint32_t *vn = new uint32_t[4 * n]();
	uint32_t *un = new uint32_t[4 * (m + 1)]();
	for (int i = n - 1; i > 0; i--)
	{
		vn[i] = (v.data_[i] << s) | (static_cast< uint64_t >(v.data_[i - 1]) >> (32 - s));
	}

	vn[0] = v.data_[0] << s;

	un[m] = static_cast< uint64_t >(u.data_[m - 1]) >> (32 - s);

	for (int i = m - 1; i > 0; --i)
	{
		un[i] = (u.data_[i] << s) | (static_cast< uint64_t >(u.data_[i - 1]) >> (32 - s));
	}

	un[0] = u.data_[0] << s;

	uint64_t qhat;
	uint64_t rhat;
	uint64_t p;
	int64_t t, k;

	q->data_ = MyDumbVector< Block >(m - n + 1);
	for (int j = m - n; j >= 0; --j)
	{
		qhat = (un[j + n] * b + un[j + n - 1]) / vn[n - 1];
		rhat = (un[j + n] * b + un[j + n - 1]) - qhat * vn[n - 1];

		do
		{
			if (qhat >= b || (qhat * vn[n - 2] > b * rhat + un[j + n - 2]))
			{
				--qhat;
				rhat += vn[n - 1];
				if (rhat < b)
				{
					continue;
				}
			}
		} while (false);

		k = 0;
		for (int i = 0; i < n; ++i)
		{
			p = qhat * vn[i];
			t = un[i + j] - k - (p & 0xFFFFFFFFULL);
			un[i + j] = t;
			k = (p >> 32) - (t >> 32);
		}

		t = un[j + n] - k;
		un[j + n] = t;

		q->data_[j] = qhat;
		if (t < 0)
		{
			--q->data_[j];
			k = 0;
			for (int i = 0; i < n; ++i)
			{
				t = static_cast< uint64_t >(un[i + j]) + vn[i] + k;
				un[i + j] = t;
				k = t >> 32;
			}
			un[j + n] += k;
		}
	}

	while (q->data_.get_size() != 0 && q->data_[q->data_.get_size() - 1] == 0)
	{
		q->data_.pop();
	}

	if (r != nullptr)
	{
		r->data_ = MyDumbVector< Block >(n);
		for (int i = 0; i < n - 1; ++i)
		{
			r->data_[i] = (un[i] >> s) | (static_cast< uint64_t >(un[i + 1]) << (32 - s));
		}
		r->data_[n - 1] = un[n - 1] >> s;
		while (r->data_.get_size() != 0 && r->data_[r->data_.get_size() - 1] == 0)
		{
			r->data_.pop();
		}
	}

	delete[] un;
	delete[] vn;
}

const uint32_t *LN::tr_16_char_to_int()
{
	static bool called = false;
	static uint32_t res[256];
	if (!called)
	{
		for (size_t i = 0; i < 256; ++i)
		{
			res[i] = 0xffffffff;
		}
		for (uint32_t i = 0; i < sizeof(HEX_DIGITS); ++i)
		{
			res[HEX_DIGITS[i]] = i;
		}
		called = true;
	}
	return res;
}

LN operator""_ln(const char *str)
{
	return LN(str);
}
