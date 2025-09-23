#pragma once

#include "spore/proxy/detail/proxy_hash.hpp"

#include <bit>
#include <source_location>

namespace spore::proxies::detail
{
#if 0
    template <typename value_t>
    consteval std::size_t type_id()
    {
        constexpr std::size_t hash = proxies::detail::hash_string(__PRETTY_FUNCTION__);
        return static_cast<std::size_t>(hash);
    }
#elif 0

    template <typename T>
    consteval auto type_name()
    {
#    ifdef _MSC_VER
        constexpr auto prefix = std::string_view {"auto __cdecl type_name<"};
        constexpr auto suffix = std::string_view {">(void)"};
        constexpr auto function = std::string_view {__FUNCSIG__};
#    elif defined(__clang__)
        constexpr auto prefix = std::string_view {"auto type_name() [T = "};
        constexpr auto suffix = std::string_view {"]"};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#    elif defined(__GNUC__)
        constexpr auto prefix = std::string_view {"consteval auto type_name() [with T = "};
        constexpr auto suffix = std::string_view {"]"};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#    endif

        constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.rfind(suffix);

        return function.substr(start, end - start);
    }

    template <typename value_t>
    constexpr std::size_t type_id()
    {
        constexpr std::size_t hash = proxies::detail::hash_string(type_name<value_t>());
        return hash;
        // constexpr auto self = &type_id<value_t>;
        // return std::bit_cast<std::size_t>(&type_id<value_t>);
        // constexpr std::source_location location = std::source_location::current();
        // constexpr std::size_t hash = proxies::detail::hash_string(location.function_name());
        // return static_cast<std::size_t>(hash);
    }
#elif 1

    template <typename T>
    consteval auto type_signature(const std::source_location loc = std::source_location::current())
    {
#    ifdef _MSC_VER
        constexpr auto prefix = std::string_view {"auto __cdecl type_signature<"};
        constexpr auto suffix = std::string_view {">("};
        constexpr auto function = std::string_view {__FUNCSIG__};
#    elif defined(__clang__)
        constexpr auto prefix = std::string_view {"auto type_signature("};
        constexpr auto suffix = std::string_view {") [T = "};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#    elif defined(__GNUC__)
        constexpr auto prefix = std::string_view {"consteval auto type_signature("};
        constexpr auto suffix = std::string_view {") [with T = "};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#    endif

        // Combine function signature + source location pour unicité
        struct type_info
        {
            std::string_view function_sig;
            std::string_view file_name;
            std::uint32_t line;
            std::uint32_t column;

            constexpr bool operator==(const type_info& other) const
            {
                return function_sig == other.function_sig &&
                       file_name == other.file_name &&
                       line == other.line &&
                       column == other.column;
            }
        };

        return type_info {function, loc.file_name(), loc.line(), loc.column()};
    }

    // Hash function améliorée pour les informations combinées
    consteval std::size_t hash_type_info(std::string_view func, std::string_view file,
        std::uint32_t line, std::uint32_t col)
    {
        std::size_t hash = 14695981039346656037ULL;

        // Hash function signature
        for (char c : func)
        {
            hash ^= static_cast<std::size_t>(c);
            hash *= 1099511628211ULL;
        }

        // Hash file name
        for (char c : file)
        {
            hash ^= static_cast<std::size_t>(c);
            hash *= 1099511628211ULL;
        }

        // Hash line and column
        hash ^= static_cast<std::size_t>(line);
        hash *= 1099511628211ULL;
        hash ^= static_cast<std::size_t>(col);
        hash *= 1099511628211ULL;

        return hash;
    }

    // Version simple avec nom de type (pour debug)
    template <typename T>
    consteval auto type_name()
    {
#    ifdef _MSC_VER
        constexpr auto prefix = std::string_view {"auto __cdecl type_name<"};
        constexpr auto suffix = std::string_view {">(void)"};
        constexpr auto function = std::string_view {__FUNCSIG__};
#    elif defined(__clang__)
        constexpr auto prefix = std::string_view {"auto type_name() [T = "};
        constexpr auto suffix = std::string_view {"]"};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#    elif defined(__GNUC__)
        constexpr auto prefix = std::string_view {"consteval auto type_name() [with T = "};
        constexpr auto suffix = std::string_view {"]"};
        constexpr auto function = std::string_view {__PRETTY_FUNCTION__};
#    endif

        constexpr auto start = function.find(prefix) + prefix.size();
        constexpr auto end = function.rfind(suffix);
        return function.substr(start, end - start);
    }

    // Fonction typeid principale - robuste pour les lambdas
    template <typename T>
    consteval std::size_t typeid_v()
    {
        auto info = type_signature<std::remove_cv_t<std::remove_reference_t<T>>>();
        return hash_type_info(info.function_sig, info.file_name, info.line, info.column);
    }

    // Version avec nom (peut être identique pour différentes lambdas)
    template <typename T>
    consteval std::string_view typeid_name()
    {
        return type_name<std::remove_cv_t<std::remove_reference_t<T>>>();
    }

    // Classe d'aide pour capturer le type à l'endroit de l'appel
    template <typename T>
    struct type_capture
    {
        using type = T;
        std::size_t id;
        std::string_view name;

        consteval type_capture() : id(typeid_v<T>()),
                                   name(typeid_name<T>()) {}
    };

// Macro helper pour capturer le type à la ligne exacte
#    define TYPEID(T) []() consteval { return type_capture<T> {}.id; }()

        template <typename value_t>
        constexpr std::size_t type_id()
        {
            // constexpr auto self = &type_id<value_t>;
            return std::bit_cast<std::size_t>(&type_id<value_t>);
            // constexpr std::source_location location = std::source_location::current();
            // constexpr std::size_t hash = proxies::detail::hash_string(location.function_name());
            // return static_cast<std::size_t>(hash);
        }
#else
    template <typename value_t>
    constexpr std::size_t type_id()
    {
        // constexpr auto self = &type_id<value_t>;
        return std::bit_cast<std::size_t>(&type_id<value_t>);
        // constexpr std::source_location location = std::source_location::current();
        // constexpr std::size_t hash = proxies::detail::hash_string(location.function_name());
        // return static_cast<std::size_t>(hash);
    }
#endif
}