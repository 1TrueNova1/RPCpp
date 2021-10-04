#pragma once

#include <cstdint>
#include <cstring>
#include <vector>
#include <new>
#include <cassert>

#include <bit>

namespace misc
{
	using opcode_t = uint8_t;

	namespace opcodes {
		class client
		{
		public:
			static const opcode_t connection_request = 0;
		};
		class server
		{
		public:
			static const opcode_t connection_response = 0;
		};

		enum class client_t : misc::opcode_t
		{
			connection_request,
		};
		enum class server_t : misc::opcode_t
		{
			connection_response,
		};
	};


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
			//std::cout << "Not a hello, world\n";
			assert(m_size + sizeof value <= m_capacity && "Out of range error");
			*(T*)(m_data + m_size) = value;
			m_size += sizeof T;
			add(values...);
		}
		template<typename Pointer, typename Size>
		std::enable_if_t<std::is_pointer_v<Pointer> && std::is_integral_v<Size>> add(const Pointer pointer, const Size size) {
			assert(m_size + size <= m_capacity && "Out of range error");
			std::memcpy(m_data + m_size, pointer, size);
			m_size += size;
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
		std::cout << "End of writing\n";
	}
} // namespace misc
