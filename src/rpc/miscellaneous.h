#pragma once

#include <cstdint>
#include <functional>
#include "../networking/miscellaneous.h"

namespace rpc
{

	const std::uint16_t default_port = 9000;

	template<typename>
	struct function_meta_info;

	template<typename Ret, typename ...Args>
	struct function_meta_info<std::function<Ret(Args...)>> {
		using return_type = std::decay_t<Ret>;
		using arguments_types = std::tuple<std::decay_t<Args> ...>;
	};

	template<typename Ret, typename ...Args>
	struct function_meta_info<Ret __cdecl(Args...)> {
		using return_type = std::decay_t<Ret>;
		using arguments_types = std::tuple<std::decay_t<Args> ...>;
	};

	template<typename Ret, typename ...Args>
	struct function_meta_info<Ret(__cdecl*)(Args...)> {
		using return_type = std::decay_t<Ret>;
		using arguments_types = std::tuple<std::decay_t<Args> ...>;
	};

	template<typename S, typename Ret, typename ...Args>
	struct function_meta_info<Ret(__thiscall S::*)(Args...)> {
		using return_type = std::decay_t<Ret>;
		using arguments_types = std::tuple<std::decay_t<Args> ...>;
	};

	using id_t = std::size_t;

	template<typename T, typename ...Args, typename std::size_t ...Indices>
	T* new_tuple_invoker(const std::tuple<Args...>& tuple, std::index_sequence<Indices...>)
	{
		return new T(std::get<Indices>(tuple)...);
	}
	template<typename T, typename ...Args>
	T* new_tuple(const std::tuple<Args...>& tuple)
	{
		return new_tuple_invoker<T>(tuple, std::make_index_sequence<std::tuple_size_v<std::decay_t<decltype(tuple)>>>{});
	}

	template<typename T>
	std::size_t sizeof_pack(const T& value)
	{
		return sizeof T;
	}
	template<typename T, typename ...Args>
	std::size_t sizeof_pack(const T& value, const Args&... values)
	{
		return sizeof T + sizeof_pack(values...);
	}


	using opcode_t = std::uint8_t;
	namespace opcodes
	{
		const opcode_t call_function = 0;
		const opcode_t call_method = 1;
		const opcode_t create_object = 2;
	}
	template<typename ...Args>
	misc::buffer<> form_packet(opcode_t opcode, Args&&... args)
	{
		std::size_t size = sizeof_pack(args...) + sizeof opcode_t;
		misc::buffer<> packet(size);
		packet.add(opcode);
		packet.add(args...);
		return packet;
	}
}