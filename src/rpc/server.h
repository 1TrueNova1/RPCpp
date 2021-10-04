#pragma once

#include "../networking/socket.h"
#include "rpc/miscellaneous.h"

#include <map>
#include <thread>

namespace rpc
{
	// TODO: add another level of abstraction for handling return type or its absense

	class server
	{
	private:

		net::socket<net::protocols::TCP, true> socket;
	public:

		template<typename ...Func>
		server(const net::address<net::IPv::IPv4>& address, const std::pair<const char*, Func>&... pairs)
		{
			create(address, pairs...);
		}

		template<typename ...Func>
		bool create(const net::address<net::IPv::IPv4>& address, const std::pair<const char*, Func>&... pairs)
		{
			if (!socket.create(address)) {
				std::cout << "Something happened while creating socket for server\n";
				return false;
			}
			(register_function(pairs.first, pairs.second), ...);
			return true;
		}

		template<typename Func>
		void register_function(std::string_view func_name, Func&& func)
		{
			register_function(std::hash<std::string_view>{}(func_name), std::move(func));
		}
		template<typename Func>
		void register_function(const id_t func_id, Func&& func)
		{
			auto lambda = [this, &func](misc::buffer<>& buffer) -> misc::buffer<>
			{
				std::function f{ func };
				using meta_info = function_meta_info<decltype(f)>;
				using ret_type = typename meta_info::return_type;
				using args_types_tuple = typename meta_info::arguments_types;
				args_types_tuple args;

				unpack(args, buffer, std::make_index_sequence<std::tuple_size_v<args_types_tuple>>{});

				ret_type ret_val = std::apply(std::move(f), std::move(args));
				misc::buffer<> ret_buffer(sizeof ret_type);
				ret_buffer.add(ret_val);
				return ret_buffer;
			};

			functions.emplace(func_id, std::move(lambda));
		}
		template<typename ...Args, typename std::size_t ...Indices>
		void unpack(std::tuple<Args...>& tuple, misc::buffer<>& buffer, std::index_sequence<Indices...>)
		{
			auto unpack_one_parameter = [counter = 0, &buffer](auto& value) mutable
			{
				value = misc::get<decltype(value)>((void*)buffer.data(), counter);
			};

			(unpack_one_parameter(std::get<Indices>(tuple)), ...);
		}

		template<typename T, typename Ret, typename ...Args>
		void register_type(std::string_view name, Ret(T::* init)(Args...))
		{
			register_type<T>(std::hash<std::string_view>{}(name), init);
		}
		template<typename T, typename Ret, typename ...Args>
		void register_type(id_t type_id, Ret(T::* init)(Args...))
		{
			auto alloc_and_init = [this](misc::buffer<>& arguments) -> void*
			{
				using meta = function_meta_info<decltype(init)>;
				using ret_type = meta::return_type;
				using args_types_tuple = meta::arguments_types;
				args_types_tuple args;

				unpack(args, arguments, std::make_index_sequence<std::tuple_size_v<args_types_tuple>>{});
				return static_cast<void*>(new_tuple<T>(args));
			};
			types.emplace(type_id, alloc_and_init);
		}

		template<typename Ret, typename T, typename ...Args>
		void register_method(std::string_view method_name, Ret(T::* method)(Args...)) {
			register_method(std::hash<std::string_view>{}(method_name), method);
		}
		template<typename Ret, typename T, typename ...Args>
		void register_method(id_t method_id, Ret(T::* g_method)(Args...))
		{
			auto lambda = [this, m_method = g_method](void* object_v, misc::buffer<>& arguments) -> misc::buffer<>
			{
				//copying member function pointer to eliminate a bug with losing its value
				Ret(__thiscall T:: * method)(Args...) = m_method; 
				using class_type = T;
				using meta = function_meta_info<decltype(g_method)>;
				using ret_type = meta::return_type;
				using args_types_tuple = meta::arguments_types;
				args_types_tuple args;

				unpack(args, arguments, std::make_index_sequence<std::tuple_size_v<args_types_tuple>>{});

				class_type* object = static_cast<class_type*>(object_v);
				assert(object != nullptr);

				auto caller = [&object, &method](auto... args) -> ret_type { return (object->*method)(args...); };
				ret_type ret_val = std::apply(caller, args); 
				misc::buffer<> ret_buffer(sizeof ret_type);
				ret_buffer.add(ret_val);
				return ret_buffer;
			};
			methods.emplace(method_id, lambda);
		}

		void run()
		{
			misc::buffer<> buffer(net::kilobyte);

			if (socket.accept() == false) {
				std::cout << "Something happened while accepting new client in run method of server\n";
				int error_code = WSAGetLastError();
				std::cout << "Error code: " << error_code << '\n';
			}
			while (!is_stopped) {
				auto return_code = socket.receive(buffer.data_nc(), buffer.capacity());
				if (return_code == 0) {
					std::cout << "Received 0 bytes of information\n";
					continue;
				}
				else if (return_code < 0) {
					std::cout << "Something has happened with server socket while receiving client call\n";
					int error_code = WSAGetLastError();
					std::cout << "Error code: " << error_code << '\n';
					continue;
				}

				buffer.set_size(return_code);

				const std::uint8_t* data = buffer.data();
				const opcode_t opcode = misc::get<opcode_t>(data, 0);
				buffer.left_shift(sizeof opcode_t);

				misc::buffer<> return_buffer;
				switch (opcode) {
				case opcodes::call_function: {
					const id_t func_id = misc::get<id_t>(data, 0);
					buffer.left_shift(sizeof id_t);
					return_buffer = call_function(func_id, buffer);
					break;
				}
				case opcodes::call_method: {
					const id_t func_id = misc::get<id_t>(data, 0);
					buffer.left_shift(sizeof id_t);
					const id_t object_id = misc::get<id_t>(data, 0);
					buffer.left_shift(sizeof id_t);
					return_buffer = call_method(func_id, object_id, buffer);
					break;
				}
				case opcodes::create_object: {
					const id_t type_id = misc::get<id_t>(data, 0);
					buffer.left_shift(sizeof id_t);
					const id_t name_id = misc::get<id_t>(data, 0);
					buffer.left_shift(sizeof id_t);
					create_object(type_id, name_id, buffer);
					break;
				}
				}

				if (return_buffer.is_empty())
					continue;
				if (!socket.send(return_buffer.data(), return_buffer.size())) {
					std::cout << "Some error occured while sending return value to client\n";
					continue;
				}

			}
		}

		misc::buffer<> call_function(id_t func_id, misc::buffer<>& args)
		{
			assert(functions.find(func_id) != functions.end());
			return functions[func_id](args);
		}

		void create_object(id_t type_id, id_t name_id, misc::buffer<>& args)
		{
			assert(types.find(type_id) != types.end());
			objects.emplace(name_id, types[type_id](args));
		}

		misc::buffer<> call_method(id_t method_id, id_t object_id, misc::buffer<>& args)
		{
			return methods[method_id](objects[object_id], args);
		}

		bool is_stopped = false;
		std::map<id_t, void*> objects;
		std::map<id_t, std::function<void* (misc::buffer<>&)>> types;
		std::map<id_t, std::function<misc::buffer<>(misc::buffer<>&)>> functions;
		std::map<id_t, std::function<misc::buffer<>(void*, misc::buffer<>&)>> methods;
	};
}