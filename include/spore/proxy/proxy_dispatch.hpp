#pragma once

#include "spore/proxy/detail/proxy_counter.hpp"
#include "spore/proxy/detail/proxy_no_lock.hpp"
#include "spore/proxy/detail/proxy_once.hpp"
#include "spore/proxy/detail/proxy_spin_lock.hpp"
#include "spore/proxy/detail/proxy_type_id.hpp"
#include "spore/proxy/detail/proxy_type_set.hpp"
#include "spore/proxy/proxy_base.hpp"
#include "spore/proxy/proxy_macros.hpp"

#include <array>
#include <cstddef>
#include <mutex>
#include <type_traits>
#include <unordered_map>

#ifndef SPORE_PROXY_DISPATCH_DEFAULT
#    define SPORE_PROXY_DISPATCH_DEFAULT proxy_dispatch_static2<128, 128, 128>
#endif

#include <fstream>
//
// #pragma section("_spore_proxy$a", read)
// #pragma section("_spore_proxy$b", read)
// #pragma section("_spore_proxy$c", read)

namespace spore
{
    namespace proxies::detail
    {
        template <typename facade_t, typename func_t, typename self_t, typename signature_t>
        struct dispatch_mapping;

        template <typename facade_t, typename func_t, typename self_t, typename return_t, typename... args_t>
        struct dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>
        {
            using facade_type = facade_t;

            template <typename value_t>
            static constexpr auto func = [](void* ptr, args_t&&... args) -> return_t {
                if constexpr (std::is_const_v<std::remove_reference_t<self_t>>)
                {
                    return func_t {}(*static_cast<const value_t*>(ptr), std::forward<args_t>(args)...);
                }
                else if constexpr (std::is_lvalue_reference_v<self_t>)
                {
                    return func_t {}(*static_cast<value_t*>(ptr), std::forward<args_t>(args)...);
                }
                else
                {
                    return func_t {}(std::move(*static_cast<value_t*>(ptr)), std::forward<args_t>(args)...);
                }
            };
        };

//        template <bool const_v, bool temp_v>
//        struct forward_traits
//        {
//            static constexpr bool is_const = const_v;
//            static constexpr bool is_temp = temp_v;
//        };
//
//        template <forward_traits traits_v>
//        struct dispatcher
//        {
//            template <typename value_t, typename result_t, typename... args_t>
//            constexpr result_t operator()(void* ptr, args_t&&... args) const
//            {
//                if constexpr (traits_v.is_const)
//                {
//                    return func_t {}(*static_cast<const value_t*>(ptr), std::forward<args_t>(args)...);
//                }
//                else if constexpr (traits_v.is_temp)
//                {
//                    return func_t {}(std::move(*static_cast<value_t*>(ptr)), std::forward<args_t>(args)...);
//                }
//                else
//                {
//                    return func_t {}(*static_cast<value_t*>(ptr), std::forward<args_t>(args)...);
//                }
//            }
//        };

        template <typename tag_t>
        struct index_impl
        {
            static inline std::atomic<std::uint32_t> seed;

            template <typename value_t>
            static inline std::uint32_t value = seed++;
        };

        struct facade_index_tag;

        template <typename facade_t>
        struct type_index_tag;

        template <typename facade_t>
        struct mapping_index_tag;

        template <typename facade_t>
        std::uint32_t facade_index()
        {
            // return counters::get<facade_t>();
            return index_impl<facade_index_tag>::value<facade_t>;
        }

        template <typename facade_t, typename mapping_t>
        std::uint32_t mapping_index()
        {
            // return counters::get<mapping_index_tag<facade_t>>();
            return index_impl<mapping_index_tag<facade_t>>::template value<mapping_t>;
        }

        template <typename facade_t, typename value_t>
        std::uint32_t type_index()
        {
            // return counters::get<mapping_index_tag<facade_t>>();
            return index_impl<type_index_tag<facade_t>>::template value<value_t>;
        }
#if 0
        struct linker_key
        {
            std::size_t mapping_id = 0;
            std::size_t type_id = 0;

            bool valid() const noexcept
            {
                return mapping_id > 0 and type_id > 0;
            }

            constexpr bool operator==(const linker_key& other) const
            {
                return std::tie(mapping_id, type_id) == std::tie(other.mapping_id, other.type_id);
            }
        };

        struct linker_hash
        {
            constexpr std::size_t operator()(const linker_key& key) const
            {
                return proxies::detail::hash_combine(key.type_id, key.mapping_id);
            }
        };

        struct linker_entry
        {
            linker_key key;
            void* ptr;

            bool valid() const noexcept
            {
                return key.valid() and ptr != nullptr;
            }
        };

        __declspec(allocate("_spore_proxy$a")) linker_entry entry_begin {
            .key = {
                .mapping_id = 0,
                .type_id = 0,
            },
            .ptr = nullptr,
        };

        template <typename value_t, typename mapping_t>
        __declspec(allocate("_spore_proxy$b")) linker_entry entry {
            .key = {
                .mapping_id = 0,
                .type_id = 0,
            },
            .ptr = nullptr,
            // .key = {
            //     .mapping_id = typeid(mapping_t).hash_code(),
            //     .type_id = typeid(value_t).hash_code(),
            // },
            // .ptr = (void*) +mapping_t::template functor<value_t>,
            // .key = {
            //     .mapping_id = proxies::detail::type_id<mapping_t>(),
            //     .type_id = proxies::detail::type_id<value_t>(),
            // },
            // .ptr = proxies::detail::get_mapping_ptr<value_t, mapping_t>(),
        };

        __declspec(allocate("_spore_proxy$c")) linker_entry entry_end {
            .key = {
                .mapping_id = 0,
                .type_id = 0,
            },
            .ptr = nullptr,
        };

        //        template <typename value_t, typename mapping_t>
        //        struct force_emit {
        //            static linker_entry& ref() {
        //                return entry<value_t, mapping_t>;
        //            }
        //        };
#endif
    }

    template <template <typename...> typename unordered_map_t = std::unordered_map, typename mutex_t = proxies::detail::spin_lock>
    struct proxy_dispatch_dynamic
    {
        static void* get_ptr(const std::uint32_t mapping_id, const std::uint32_t type_id) noexcept
        {
            const dispatch_key dispatch_key {mapping_id, type_id};
            // const std::lock_guard lock {_mutex};
            const auto it_ptr = _ptr_map.find(dispatch_key);
            return it_ptr != _ptr_map.end() ? it_ptr->second : nullptr;
        }

        static void set_ptr(const std::uint32_t mapping_id, const std::uint32_t type_id, void* ptr) noexcept
        {
            const dispatch_key dispatch_key {mapping_id, type_id};
            // const std::lock_guard lock {_mutex};
            _ptr_map.insert_or_assign(dispatch_key, ptr);
        }

      private:
        struct dispatch_key
        {
            std::uint32_t mapping_id = 0;
            std::uint32_t type_id = 0;

            constexpr bool operator==(const dispatch_key& other) const
            {
                return std::tie(mapping_id, type_id) == std::tie(other.mapping_id, other.type_id);
            }
        };

        struct dispatch_hash
        {
            constexpr std::size_t operator()(const dispatch_key& key) const
            {
                return proxies::detail::hash_combine(key.type_id, key.mapping_id);
            }
        };

        static inline mutex_t _mutex;
        static inline thread_local unordered_map_t<dispatch_key, void*, dispatch_hash> _ptr_map;
    };

    template <std::size_t max_type_v>
    struct proxy_dispatch_static4
    {
        template <typename facade_t, typename mapping_t>
        static void* get_ptr(const std::uint32_t type_index) noexcept
        {
            SPORE_PROXY_ASSERT(type_index < max_type_v);
            return ptrs<facade_t, mapping_t>[type_index];
        }

        template <typename facade_t, typename mapping_t>
        static void set_ptr(const std::uint32_t type_index, void* ptr) noexcept
        {
            SPORE_PROXY_ASSERT(mapping_index < max_mapping_v);
            SPORE_PROXY_ASSERT(type_index < max_type_v);

            ptrs<facade_t, mapping_t>[type_index] = ptr;
        }

        template <typename facade_t, typename value_t>
        static void** get_ptr_base()
        {
            return nullptr;
        }

        template <typename facade_t, typename mapping_t>
        static inline void* ptrs[max_type_v];
    };

    template <std::size_t max_mapping_v, std::size_t max_type_v>
    struct proxy_dispatch_static3
    {
        template <typename facade_t>
        static void* get_ptr(const std::uint32_t mapping_index, const std::uint32_t type_index) noexcept
        {
            SPORE_PROXY_ASSERT(mapping_index < max_mapping_v);
            SPORE_PROXY_ASSERT(type_index < max_type_v);

            return ptrs<facade_t>[type_index][mapping_index];
        }

        template <typename facade_t>
        static void set_ptr(const std::uint32_t mapping_index, const std::uint32_t type_index, void* ptr) noexcept
        {
            SPORE_PROXY_ASSERT(mapping_index < max_mapping_v);
            SPORE_PROXY_ASSERT(type_index < max_type_v);

            ptrs<facade_t>[type_index][mapping_index] = ptr;
        }

        template <typename facade_t, typename value_t>
        static void** get_ptr_base()
        {
            const std::uint32_t type_index = proxies::detail::type_index<facade_t, value_t>();

            SPORE_PROXY_ASSERT(type_index < max_type_v);

            return ptrs<facade_t>[type_index];
        }

        template <typename facade_t>
        static inline void* ptrs[max_type_v][max_mapping_v];
    };

    template <std::size_t facades_v, std::size_t mapping_v, std::size_t value_v>
    struct proxy_dispatch_static2
    {
        static void* get_ptr(const std::uint32_t facade_index, const std::uint32_t mapping_index, const std::uint32_t type_index) noexcept
        {
            SPORE_PROXY_ASSERT(facade_index < facades_v);
            SPORE_PROXY_ASSERT(mapping_index < mapping_v);
            SPORE_PROXY_ASSERT(type_index < value_v);

            const std::uint32_t ptr_index = (facade_index * facades_v * value_v) + (mapping_index * value_v) + type_index;
            return ptrs2[ptr_index];
            // return ptrs[facade_index][type_index][mapping_index];
        }

        static void set_ptr(const std::uint32_t facade_index, const std::uint32_t mapping_index, const std::uint32_t type_index, void* ptr) noexcept
        {
            SPORE_PROXY_ASSERT(facade_index < facades_v);
            SPORE_PROXY_ASSERT(mapping_index < mapping_v);
            SPORE_PROXY_ASSERT(type_index < value_v);

            const std::uint32_t ptr_index = (facade_index * facades_v * value_v) + (mapping_index * value_v) + type_index;
            ptrs2[ptr_index] = ptr;
            // ptrs[facade_index][type_index][mapping_index] = ptr;
        }

        static inline void* ptrs[facades_v][value_v][mapping_v];
        static inline void* ptrs2[facades_v * value_v * mapping_v];
    };

    template <std::size_t size_v, typename mutex_t = proxies::detail::spin_lock>
    struct proxy_dispatch_static
    {
        static std::size_t do_work(std::size_t size)
        {
            std::size_t result = 0;

            for (std::size_t index = 0; index < size; index++)
            {
                result += index;
            }

            return result;
        }

        static void* get_ptr(const std::uint32_t mapping_id, const std::uint32_t type_id) noexcept
        {
#if 0
            return (void*) &do_work;
#elif 1
            const std::uint32_t dispatch_id = mapping_id ^ (mapping_id + 0x9e3779b9 + (type_id << 6) + (type_id >> 2));
            const std::uint32_t dispatch_index = dispatch_id % size_v;
            return _ptr_map2[dispatch_index];
#elif 1
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::size_t dispatch_index = dispatch_id % size_v;
            const std::lock_guard lock {_mutex};
            return _ptr_map2[dispatch_index];
#else
            // while (_modifying.test(std::memory_order_acquire))
            // {
            // }

            // std::atomic_thread_fence(std::memory_order_acquire);

            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::uint16_t checksum = make_checksum(mapping_id, type_id);

            std::size_t dispatch_index = dispatch_id % size_v;
            std::size_t seed_index = 0;

            const std::lock_guard lock {_mutex};

            do
            {
                const dispatch_ptr& ptr_data = _ptr_map[dispatch_index];

                if (not ptr_data.valid() or checksum == ptr_data.checksum())
                {
                    return ptr_data.ptr();
                }

                dispatch_index = (dispatch_id ^ _seeds[seed_index++]) % size_v;
            } while (seed_index < std::size(_seeds));

            return nullptr;
#endif
        }

        static void set_ptr(const std::uint32_t mapping_id, const std::uint32_t type_id, void* ptr) noexcept
        {
#if 1
            const std::uint32_t dispatch_id = mapping_id ^ (mapping_id + 0x9e3779b9 + (type_id << 6) + (type_id >> 2));
            const std::uint32_t dispatch_index = dispatch_id % size_v;
            _ptr_map2[dispatch_index] = ptr;
#elif 1
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::size_t dispatch_index = dispatch_id % size_v;
            const std::lock_guard lock {_mutex};
            _ptr_map2[dispatch_index] = ptr;
#else
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::uint16_t checksum = make_checksum(mapping_id, type_id);

            std::size_t dispatch_index = dispatch_id % size_v;
            std::size_t seed_index = 0;

            // while (_modifying.test_and_set(std::memory_order_acquire))
            // {
            // }
            const std::lock_guard lock {_mutex};

            do
            {
                dispatch_ptr& ptr_data = _ptr_map[dispatch_index];

                if (not ptr_data.valid() or checksum != ptr_data.checksum())
                {
                    ptr_data = dispatch_ptr {ptr, checksum};
                    // _modifying.clear();
                    return;
                }

                dispatch_index = (dispatch_id ^ _seeds[seed_index++]) % size_v;
            } while (seed_index < std::size(_seeds));

            // _modifying.clear();
            SPORE_PROXY_ASSERT(false);
#endif
        }

      private:
        struct dispatch_ptr
        {
            static constexpr std::intptr_t valid_mask = 0x8000000000000000ULL;
            static constexpr std::intptr_t checksum_mask = 0x7fff000000000000ULL;
            static constexpr std::intptr_t ptr_mask = 0x0000ffffffffffffULL;

            dispatch_ptr() = default;

            dispatch_ptr(void* ptr, const std::uint16_t checksum)
            {
                _valid = static_cast<std::intptr_t>(true);
                _checksum = static_cast<std::intptr_t>(checksum); // & checksum_mask;
                _ptr = std::bit_cast<std::intptr_t>(ptr) & ptr_mask;
            }

            bool valid() const noexcept
            {
                return _valid;
            }

            std::uint16_t checksum() const noexcept
            {
                return _checksum;
            }

            void* ptr() const noexcept
            {
                std::intptr_t ptr = _ptr;

#if defined(__x86_64__) || defined(_M_X64)
                constexpr std::intptr_t sign_bit = 1ULL << 47ULL;
                constexpr std::intptr_t sign_extension_bits = 0xffff000000000000;

                if (ptr & sign_bit)
                {
                    ptr |= sign_extension_bits;
                }
#endif
                return std::bit_cast<void*>(ptr);
            }

          private:
            std::uintptr_t _valid : 1;
            std::uintptr_t _checksum : 16;
            std::uintptr_t _ptr : 48;
        };

        // static inline std::atomic_flag _modifying {};
        static inline mutex_t _mutex;
        static inline std::array<dispatch_ptr, size_v> _ptr_map {};
        static inline std::array<void*, size_v> _ptr_map2 {};

        static constexpr std::size_t _seeds[] {
            0x65d859d632a0a935ULL,
            0x40dc6be7c399f54aULL,
            0xa21e06cc24770a56ULL,
            0xa73a74747989357dULL,
            0xe3d482338fcde2bbULL,
            0x144bdebc9fa82bf6ULL,
            0x80feaaeb42ac942fULL,
            0xb3e9e45e5b4689f4ULL,
            0x93c836e6d3ab52a3ULL,
            0x8dc4e3c4856b16ceULL,
            0x409989a18ae7049bULL,
            0x5b547d631c3a9c8aULL,
            0xa33950eba26453edULL,
            0x9a9e29da9bde239bULL,
            0x1b2d691230396398ULL,
            0xc79bcfade4a11e6dULL,
        };

        static constexpr std::uint16_t make_checksum(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            return static_cast<std::uint16_t>((mapping_id ^ type_id) & std::numeric_limits<std::uint16_t>::max());
        }
    };

#if 0
    struct proxy_dispatch_linker
    {
        static inline std::unordered_map<proxies::detail::linker_key, void*, proxies::detail::linker_hash> _map;

        static void initialize()
        {
            auto begin = reinterpret_cast<proxies::detail::linker_entry*>(&proxies::detail::entry_begin) + 1;
            auto end = reinterpret_cast<proxies::detail::linker_entry*>(&proxies::detail::entry_end);

            for (auto it = begin; it != end; ++it)
            {
                const proxies::detail::linker_entry& entry = *it;
                _map.emplace(entry.key, entry.ptr);
            }
        }
    };

    struct proxy_dispatch_map
    {
        static void* get_ptr(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            std::lock_guard lock {_mutex};
            const dispatch_key dispatch_key {mapping_id, type_id};
            const auto it_func = _func_map.find(dispatch_key);
            return it_func != _func_map.end() ? it_func->second : nullptr;
        }

        static void set_ptr(const std::size_t mapping_id, const std::size_t type_id, void* ptr) noexcept
        {
            std::lock_guard lock {_mutex};

            // _file_ << mapping_id << " " << type_id << " " << ptr << std::endl;

            const dispatch_key dispatch_key {mapping_id, type_id};
            _func_map.insert_or_assign(dispatch_key, ptr);
        }

      private:
        struct dispatch_key
        {
            std::size_t mapping_id;
            std::size_t type_id;

            constexpr bool operator==(const dispatch_key& other) const
            {
                return std::tie(mapping_id, type_id) == std::tie(other.mapping_id, other.type_id);
            }
        };

        struct dispatch_hash
        {
            constexpr std::size_t operator()(const dispatch_key& key) const
            {
                return key.mapping_id ^ (key.mapping_id + 0x9e3779b9 + (key.type_id << 6) + (key.type_id >> 2));
            }
        };

        static inline std::recursive_mutex _mutex;
        static inline std::unordered_map<dispatch_key, void*, dispatch_hash> _func_map;
    };
#endif
#if 0
    template <std::size_t size_v>
    struct proxy_dispatch_static2
    {
#    if 0
        static void* get_ptr(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::size_t dispatch_index = dispatch_id % size_v;
            return _ptr_map[dispatch_index];
        }

        static void set_ptr(const std::size_t mapping_id, const std::size_t type_id, void* ptr) noexcept
        {
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::size_t dispatch_index = dispatch_id % size_v;
            _ptr_map[dispatch_index] = ptr;
        }
#    else
        static void* get_ptr(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::size_t dispatch_index = dispatch_id % size_v;
            const dispatch_ptr dispatch_ptr = _ptr_map[dispatch_index].load();
            SPORE_PROXY_ASSERT(actual_ptr.checksum() == make_checksum(mapping_id, type_id));
            return dispatch_ptr.ptr();
        }

        static void set_ptr(const std::size_t mapping_id, const std::size_t type_id, void* ptr) noexcept
        {
            const std::size_t dispatch_id = proxies::detail::hash_combine(mapping_id, type_id);
            const std::size_t dispatch_index = dispatch_id % size_v;
            const std::uint16_t checksum = make_checksum(mapping_id, type_id);

            dispatch_ptr expected_ptr {nullptr, 0};
            dispatch_ptr desired_ptr {ptr, checksum};
            dispatch_ptr actual_ptr = _ptr_map[dispatch_index].load();

            while (not _ptr_map[dispatch_index].compare_exchange_weak(expected_ptr, desired_ptr))
            {
                if (actual_ptr == desired_ptr)
                {
                    break;
                }
            }
        }
#    endif

      private:
        struct dispatch_ptr
        {
            static constexpr std::intptr_t valid_mask = 0x8000000000000000ULL;
            static constexpr std::intptr_t checksum_mask = 0x7fff000000000000ULL;
            static constexpr std::intptr_t ptr_mask = 0x0000ffffffffffffULL;

            dispatch_ptr() = default;

            dispatch_ptr(void* ptr, const std::uint16_t checksum)
            {
                _valid = static_cast<std::intptr_t>(true);
                _checksum = static_cast<std::intptr_t>(checksum); // & checksum_mask;
                _ptr = std::bit_cast<std::intptr_t>(ptr) & ptr_mask;
            }

            bool valid() const noexcept
            {
                return _valid;
            }

            std::uint16_t checksum() const noexcept
            {
                return _checksum;
            }

            void* ptr() const noexcept
            {
                std::intptr_t ptr = _ptr;

#    if defined(__x86_64__) || defined(_M_X64)
                constexpr std::intptr_t sign_bit = 1ULL << 47ULL;
                constexpr std::intptr_t sign_extension_bits = 0xffff000000000000;

                if (ptr & sign_bit)
                {
                    ptr |= sign_extension_bits;
                }
#    endif
                return std::bit_cast<void*>(ptr);
            }

            bool operator==(const dispatch_ptr& other)
            {
                return std::memcmp(this, &other, sizeof(dispatch_ptr)) == 0;
            }

          private:
            std::uintptr_t _valid : 1;
            std::uintptr_t _checksum : 16;
            std::uintptr_t _ptr : 48;
        };

#    if 0
        static inline std::array<void*, size_v> _ptr_map {};
#    else
        static inline std::array<std::atomic<dispatch_ptr>, size_v> _ptr_map {};
#    endif

        static constexpr std::size_t _seeds[] {
            0x65d859d632a0a935ULL,
            0x40dc6be7c399f54aULL,
            0xa21e06cc24770a56ULL,
            0xa73a74747989357dULL,
            0xe3d482338fcde2bbULL,
            0x144bdebc9fa82bf6ULL,
            0x80feaaeb42ac942fULL,
            0xb3e9e45e5b4689f4ULL,
            0x93c836e6d3ab52a3ULL,
            0x8dc4e3c4856b16ceULL,
            0x409989a18ae7049bULL,
            0x5b547d631c3a9c8aULL,
            0xa33950eba26453edULL,
            0x9a9e29da9bde239bULL,
            0x1b2d691230396398ULL,
            0xc79bcfade4a11e6dULL,
        };

        static constexpr std::uint16_t make_checksum(const std::size_t mapping_id, const std::size_t type_id) noexcept
        {
            return static_cast<std::uint16_t>((mapping_id ^ type_id) & std::numeric_limits<std::uint16_t>::max());
        }
    };
#endif

    using proxy_dispatch_default = proxy_dispatch_static4<64>;

    namespace proxies
    {
        namespace detail
        {
            // one per translation unit
            namespace
            {
                struct facade_tag;

                template <typename...>
                struct once_tag;

                template <typename facade_t>
                struct mapping_tag
                {
                };

                template <typename facade_t>
                struct value_tag
                {
                };

                template <typename facade_t>
                struct base_tag
                {
                };
            }

            template <typename func_t, typename return_t>
            struct dispatch_or_throw
            {
                template <typename... args_t>
                constexpr return_t operator()(args_t&&... args) const noexcept(std::is_invocable_r_v<return_t, func_t, args_t&&...>)
                {
                    if constexpr (std::is_invocable_r_v<return_t, func_t, args_t&&...>)
                    {
                        return func_t {}(std::forward<args_t>(args)...);
                    }
                    else
                    {
                        SPORE_PROXY_THROW("not callable");
                    }
                }
            };

            template <typename func_t, typename return_t>
            struct dispatch_or_default
            {
                template <typename... args_t>
                constexpr return_t operator()(args_t&&... args) const noexcept(std::is_invocable_r_v<return_t, func_t, args_t&&...>)
                {
                    if constexpr (std::is_invocable_r_v<return_t, func_t, args_t&&...>)
                    {
                        return func_t {}(std::forward<args_t>(args)...);
                    }
                    else if constexpr (not std::is_void_v<return_t>)
                    {
                        static_assert(std::is_default_constructible_v<return_t>);
                        return return_t {};
                    }
                }
            };

#if 0
            template <typename value_t, typename mapping_t>
            void* get_mapping_ptr() noexcept
            {
                constexpr auto unwrap_mapping = []</*typename facade_t, */ std::size_t index_v, typename func_t, typename self_t, typename return_t, typename... args_t>(const dispatch_mapping</*facade_t, */ index_v, func_t, self_t, return_t(args_t...)>) {
                    using void_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
                    const auto func = [](void_t* ptr, args_t... args) -> return_t {
                        if constexpr (std::is_const_v<std::remove_reference_t<self_t>>)
                        {
                            return func_t {}(*static_cast<const value_t*>(ptr), std::forward<args_t&&>(args)...);
                        }
                        else if constexpr (std::is_lvalue_reference_v<self_t>)
                        {
                            return func_t {}(*static_cast<value_t*>(ptr), std::forward<args_t&&>(args)...);
                        }
                        else
                        {
                            return func_t {}(std::move(*static_cast<value_t*>(ptr)), std::forward<args_t&&>(args)...);
                        }
                    };

                    return reinterpret_cast<void*>(+func);
                };

                return unwrap_mapping(mapping_t {});
            }
#endif

            template <typename facade_t>
            consteval void add_facade()
            {
                proxies::detail::type_sets::emplace<proxies::detail::facade_tag, facade_t>();
                proxies::detail::type_sets::for_each<proxies::detail::facade_tag>([]<typename other_facade_t> {
                    if constexpr (not std::is_same_v<facade_t, other_facade_t>)
                    {
                        if constexpr (std::derived_from<facade_t, other_facade_t>)
                        {
                            proxies::detail::type_sets::emplace<proxies::detail::base_tag<facade_t>, other_facade_t>();
                        }

                        if constexpr (std::derived_from<other_facade_t, facade_t>)
                        {
                            proxies::detail::type_sets::emplace<proxies::detail::base_tag<other_facade_t>, facade_t>();
                        }
                    }
                });
            }

            template <typename value_t, typename mapping_t>
            void add_value_mapping_once() noexcept
            {
                using facade_t = typename mapping_t::facade_type;
                // using dispatch_t = typename facade_t::dispatch_type;
                using dispatch_t = proxy_dispatch_default;
                using once_tag_t = proxies::detail::once_tag<value_t, mapping_t>;

                [[maybe_unused]] static const once<once_tag_t> once = [] {
                    void* ptr = reinterpret_cast<void*>(+mapping_t::template func<value_t>);
                    dispatch_t::set_ptr<facade_t, mapping_t>(proxies::detail::type_index<facade_t, value_t>(), ptr);
                };
            }

            template <typename facade_t, typename value_t>
            void add_facade_value_once() noexcept
            {
                using once_tag_t = proxies::detail::once_tag<facade_t, value_t>;

                [[maybe_unused]] static const once<once_tag_t> once = [] {
                    proxies::detail::add_facade<facade_t>();
                    proxies::detail::type_sets::emplace<proxies::detail::value_tag<facade_t>, value_t>();

                    proxies::detail::type_sets::for_each<proxies::detail::mapping_tag<facade_t>>([]<typename mapping_t> {
                        proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                    });

                    proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_facade_t> {
                        proxies::detail::add_facade_value_once<base_facade_t, value_t>();
                    });
                };
            }

            template <typename facade_t, typename mapping_t>
            void add_facade_mapping_once() noexcept
            {
                using once_tag_t = proxies::detail::once_tag<facade_t, mapping_t>;

                [[maybe_unused]] static const once<once_tag_t> once = [] {
                    proxies::detail::add_facade<facade_t>();
                    proxies::detail::type_sets::emplace<proxies::detail::mapping_tag<facade_t>, mapping_t>();

                    proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
                        proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                    });

                    proxies::detail::type_sets::for_each<proxies::detail::base_tag<facade_t>>([]<typename base_t> {
                        proxies::detail::type_sets::for_each<proxies::detail::value_tag<base_t>>([]<typename value_t> {
                            proxies::detail::add_value_mapping_once<value_t, mapping_t>();
                        });
                    });
                };
            }

            // static inline std::recursive_mutex _mutex;
            // static inline std::ofstream _file_ {"C:/Dev/wtf.txt"};
            //
            //
            //            template <typename value_t>
            //            std::string function_name()
            //            {
            //                constexpr std::source_location location = std::source_location::current();
            //                return location.function_name();
            //            }

            template <typename return_t, typename func_t, typename self_t, typename... args_t>
            /*constexpr*/ return_t dispatch_impl(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
            {
                using facade_t = std::decay_t<self_t>;
                // using dispatch_t = typename facade_t::dispatch_type;
                using dispatch_t = proxy_dispatch_default;
                using mapping_t = proxies::detail::dispatch_mapping<facade_t, func_t, self_t, return_t(args_t...)>;

                // {
                //     std::lock_guard lock {_mutex};
                //     _file_ << proxies::detail::type_id<mapping_t>() << std::endl;
                //     _file_ << typeid(mapping_t).name() << std::endl;
                //     _file_  << std::endl;
                //
                // }

                static_assert(std::is_empty_v<func_t>);
                static_assert(std::is_empty_v<facade_t>);

                // std::lock_guard lock {_mutex};

                proxies::detail::add_facade_mapping_once<facade_t, mapping_t>();

                using proxy_base_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const proxy_base, proxy_base>;

                proxy_base_t& proxy = reinterpret_cast<proxy_base_t&>(self);

#if 0
                using void_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
                const auto index = proxies::detail::mapping_index<facade_t, mapping_t>();
                const auto func = reinterpret_cast<return_t (*const)(void_t*, args_t&&...)>(proxy.vptr()[index]);
                return func(proxy.ptr(), std::forward<args_t>(args)...);
#elif 1
                void* ptr = dispatch_t::get_ptr<facade_t, mapping_t>(proxy.type_index());

                SPORE_PROXY_ASSERT(ptr != nullptr);

                using void_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
                const auto func = reinterpret_cast<return_t (*)(void_t*, args_t...)>(ptr);
                return func(proxy.ptr(), std::forward<args_t>(args)...);
#else
                void* ptr = dispatch_t::get_ptr<facade_t>(proxies::detail::mapping_index<facade_t, mapping_t>(), proxy.type_index());

                SPORE_PROXY_ASSERT(ptr != nullptr);

                using void_t = std::conditional_t<std::is_const_v<std::remove_reference_t<self_t>>, const void, void>;
                const auto func = reinterpret_cast<return_t (*)(void_t*, args_t...)>(ptr);
                return func(proxy.ptr(), std::forward<args_t>(args)...);
#endif
            }
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t dispatch(const func_t& func, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(func, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t dispatch_or_throw(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(
                proxies::detail::dispatch_or_throw<func_t, return_t> {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }

        template <typename return_t = void, typename func_t, typename self_t, typename... args_t>
        constexpr return_t dispatch_or_default(const func_t&, self_t&& self, args_t&&... args) SPORE_PROXY_THROW_SPEC
        {
            return proxies::detail::dispatch_impl<return_t>(
                proxies::detail::dispatch_or_default<func_t, return_t> {}, std::forward<self_t>(self), std::forward<args_t>(args)...);
        }
    }
}

// #pragma init_seg(lib)
//
// namespace spore::proxies::detail
//{
//     struct init
//     {
//         init()
//         {
//             proxies::detail::type_sets::for_each<proxies::detail::facade_tag>([]<typename facade_t> {
//                 proxies::detail::type_sets::for_each<proxies::detail::value_tag<facade_t>>([]<typename value_t> {
//                     proxies::detail::type_sets::for_each<proxies::detail::mapping_tag<facade_t>>([]<typename mapping_t> {
//                         proxy_dispatch_default::set_ptr(
//                             proxies::detail::type_id<mapping_t>(),
//                             proxies::detail::type_id<value_t>(),
//                             proxies::detail::get_mapping_ptr<value_t, mapping_t>());
//                     });
//                 });
//             });
//
//             std::atomic_thread_fence(std::memory_order::release);
//         }
//     };
//
//     static init run_before_main;
// }
