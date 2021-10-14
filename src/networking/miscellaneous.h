#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <cassert>
#include <bit>

#include <new>

namespace misc
{

	template<typename T, typename = void>
	struct is_iterable : std::false_type {};
	template<typename T>
	struct is_iterable<T, std::void_t<
		decltype(std::declval<T>().begin()),
		decltype(std::declval<T>().end())
		>> : std::true_type {};

	//template<typename T, typename Ty = void>
	//using is_iterable_v = is_iterable<T, Ty>::value;

	// is needed at all?
	template<typename T, typename = void>
	struct is_container : std::false_type {};
	template<typename T>
	struct is_container<T, std::void_t<
		decltype(std::declval<T>().size()),
		decltype(std::declval<T>().data())
		>> : std::true_type {};

	template<typename T, typename Ty = void>
	using is_container_v = is_container<T, Ty>::value;


	template<typename T>
	std::size_t sizeof_v(const T& value)
	{
		if constexpr (misc::is_iterable<T>::value) {
			return sizeof T::size_type + value.size() * sizeof T::value_type;
		}
		return sizeof T;
	}
	template<typename T, typename ...Args>
	std::size_t sizeof_v(const T& value, const Args&... values)
	{
		return sizeof_v(value) + sizeof_v(values...);
	}

	template<typename T, typename offset_t>
	T get(const void* from, offset_t&& offset)
	{
		using decayed = std::decay_t<T>;
		if constexpr (!std::is_rvalue_reference_v<offset_t>) {
			offset += sizeof decayed;
			return *(decayed*)(&((uint8_t*)from)[offset - sizeof T]);
		}
		else {
			return *(decayed*)(&((uint8_t*)from)[offset]);
		}
	}
	template<typename offset_t>
	void* get(const void* from, std::size_t size, offset_t&& offset)
	{
		void* data = new(std::nothrow) std::uint8_t[size];
		if (data) {
			std::memcpy(data, static_cast<const std::byte*>(from) + offset, size);
			if constexpr (!std::is_rvalue_reference_v<offset_t>)
				offset += size;
		}
		return data;
	}

	template<typename T, typename offset_t>
	void set(void* to, offset_t&& offset, const T& value)
	{
		*(T*)((uint8_t*)to + offset) = value;
		if constexpr (!std::is_rvalue_reference_v<offset_t>)
			offset += sizeof T;
	}

	template<typename T, typename ...Args, typename offset_t>
	void set(void* to, offset_t&& offset, const T& value, const Args&... values)
	{
		*(T*)((uint8_t*)to + offset) = value;
		if constexpr (!std::is_rvalue_reference_v<offset_t>)
			offset += sizeof T;
		set(to, offset, values...);
	}
	template<typename offset_t>
	void set(void* to, offset_t&& offset)
	{
		//std::cout << "End of writing\n";
	}


	template<std::size_t capacity = 0>
	class buffer
	{
	private:
		std::uint8_t m_data[capacity];
		std::size_t m_size = 0;
	public:
		buffer() {}
		~buffer() {}
		template<typename T, typename ...Args>
		void add(const T& value, const Args&... values)
		{
			*(T*)(m_data + m_size) = value;
			m_size += sizeof T;
			add(values...);
		}
		void add() { }

		const std::uint8_t* data() const { return m_data; }
		std::size_t capacity() const { return capacity; }
		std::size_t size() const { return m_size; }
	};

	template<>
	class buffer<>
	{
	private:
		std::uint8_t* m_data = nullptr;
		std::size_t m_size = 0;
		std::size_t m_capacity = 0;
	public:
		// create must be called
		buffer() {}

		buffer(std::size_t capacity)
		{
			create(capacity);
		}
		buffer(std::uint8_t* data, std::size_t capacity)
			: m_data(data), m_capacity(capacity)
		{}
		buffer(const buffer& buffer)
		{
			if (buffer.m_capacity > 0) {
				delete[] m_data;
				m_size = buffer.m_size;
				m_capacity = buffer.m_capacity;

				if (buffer.m_size > 0) {
					this->m_data = new std::uint8_t[buffer.m_size];
					std::memcpy(this->m_data, buffer.m_data, buffer.m_size);
				}
			}
		}
		buffer& operator=(const buffer& buffer)
		{
			if (buffer.m_capacity > 0) {
				delete[] m_data;
				m_size = buffer.m_size;
				m_capacity = buffer.m_capacity;

				if (buffer.m_size > 0) {
					this->m_data = new std::uint8_t[buffer.m_size];
					std::memcpy(this->m_data, buffer.m_data, buffer.m_size);
				}
			}
			return *this;
		}
		buffer(buffer&& buffer) noexcept
		{
			delete[] m_data;

			m_data = buffer.m_data;
			m_size = buffer.m_size;
			m_capacity = buffer.m_capacity;

			buffer.m_data = nullptr;
			buffer.m_size = 0;
			buffer.m_capacity = 0;
		}
		buffer& operator=(buffer&& buffer) noexcept
		{
			delete[] m_data;

			m_data = buffer.m_data;
			m_size = buffer.m_size;
			m_capacity = buffer.m_capacity;

			buffer.m_data = nullptr;
			buffer.m_size = 0;
			buffer.m_capacity = 0;
			return *this;
		}
		~buffer()
		{
			delete[] m_data;
		}
		bool create(size_t capacity)
		{
			m_capacity = capacity;
			m_data = new (std::nothrow)std::uint8_t[m_capacity];
			return !!m_data;
		}
		template<typename T, typename ...Args>
		std::enable_if_t<!std::is_pointer_v<T>> add(const T& value, const Args&... values)
		{
			assert(m_size + sizeof_v(value) <= m_capacity && "Out of range error");
			if constexpr (misc::is_iterable<T>::value) {
				// adding size of data
				const std::size_t size = value.size() * sizeof T::value_type;
				std::memcpy(m_data + m_size, &size, sizeof std::size_t);
				m_size += sizeof std::size_t;
				// adding data itself
				std::memcpy(m_data + m_size, value.data(), size);
				m_size += size;
			}
			if constexpr (!misc::is_iterable<T>::value) {
				*(T*)(m_data + m_size) = value;
				m_size += sizeof T;
			}
			add(values...);
		}
		template<typename Pointer, typename Size, typename ...Args>
		std::enable_if_t<std::is_pointer_v<Pointer>&& std::is_integral_v<Size>> add(const Pointer pointer, const Size size, const Args&... values) {
			assert(m_size + size <= m_capacity && "Out of range error");
			std::memcpy(m_data + m_size, pointer, size);
			m_size += size;
			add(values...);
		}
		void add(const buffer& buffer)
		{
			assert(m_size + buffer.m_size <= m_capacity && "Out of range error");
			std::memcpy(m_data + m_size, buffer.m_data, buffer.m_size);
			m_size += buffer.m_size;
		}
		void add() {}

		void left_shift(std::size_t shift)
		{
			assert(!is_null());
			if (shift > m_size)
			{
				m_size = 0;
				return;
			}
			std::memcpy(m_data, m_data + shift, m_size - shift);
			m_size -= shift;
		}

		void clear() { m_size = 0; }

		bool is_empty() const { return m_size == 0; }
		bool is_null() const { return m_data == nullptr; }

		template<typename T>
		T cast()
		{
			assert(!is_null());

			if constexpr (misc::is_container<T>::value) {
				std::size_t offset = 0;
				typename T::size_type size = misc::get<T::size_type>(this->m_data, offset);
				typename T::value_type* data =
					static_cast<typename T::value_type*>(misc::get(this->m_data, size, offset));
				assert(data);
				T container(data, data + size / sizeof T::value_type);
				delete[] data;
				return container;
			}
			T value;
			std::memcpy(&value, m_data, m_size);
			return value;
		}

		const std::uint8_t* data() const { return m_data; }
		std::uint8_t* data_nc() { return m_data; }
		std::size_t capacity() const { return m_capacity; }
		std::size_t size() const { return m_size; }

		void set_size(std::size_t new_size) { m_size = new_size; }
	};

} // namespace misc
