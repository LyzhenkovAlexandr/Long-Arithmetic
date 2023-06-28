#pragma once

#include <algorithm>
#include <cstring>
#include <utility>

template< typename T >
class MyDumbVector
{
  public:
	MyDumbVector() : size_(0), cap_(0), data_(nullptr) {}

	MyDumbVector(const MyDumbVector< T >& other) : size_(other.size_), cap_(other.cap_), data_(new T[cap_]())
	{
		memcpy(data_, other.data_, other.size_ * sizeof(T));
	}

	MyDumbVector(MyDumbVector< T >&& other) noexcept : MyDumbVector()
	{
		std::swap(size_, other.size_);
		std::swap(cap_, other.cap_);
		std::swap(data_, other.data_);
	}

	MyDumbVector(size_t sz) : size_(sz), cap_(std::max(size_, size_t(1))), data_(new T[cap_]()) {}

	~MyDumbVector()
	{
		if (data_)
		{
			delete[] data_;
		}
	}

	MyDumbVector< T >& operator=(const MyDumbVector< T >& other)
	{
		if (&other != this)
		{
			if (data_)
			{
				delete[] data_;
			}
			size_ = other.size_;
			cap_ = other.cap_;
			data_ = new T[cap_];
			memcpy(data_, other.data_, size_ * sizeof(T));
		}
		return *this;
	}

	MyDumbVector< T >& operator=(MyDumbVector< T >&& other) noexcept
	{
		if (&other != this)
		{
			if (data_)
			{
				delete[] data_;
				data_ = nullptr;
			}
			size_ = 0;
			cap_ = 0;
			data_ = nullptr;
			std::swap(size_, other.size_);
			std::swap(cap_, other.cap_);
			std::swap(data_, other.data_);
		}
		return *this;
	}

	T& operator[](size_t i) { return data_[i]; }

	const T& operator[](size_t i) const { return data_[i]; }

	void push_back(const T& elem)
	{
		try_resize();
		data_[size_++] = elem;
	}

	void pop()
	{
		try_resize();
		--size_;
	}

	size_t get_size() const { return size_; }

  private:
	size_t size_;
	size_t cap_;
	T* data_;

	void try_resize()
	{
		size_t new_cap = 0;
		T* new_data;

		if (size_ >= cap_)
		{
			if (cap_ == 0)
			{
				new_cap = 1;
			}
			else
			{
				new_cap = cap_ * 2;
			}
		}
		else if (cap_ > 8 && size_ <= cap_ / 4)
		{
			new_cap = cap_ / 2;
		}
		else
		{
			return;
		}

		new_data = new T[new_cap]();

		if (data_ != nullptr)
		{
			memcpy(new_data, data_, size_ * sizeof(T));
			delete[] data_;
		}

		data_ = new_data;
		cap_ = new_cap;
	}
};
