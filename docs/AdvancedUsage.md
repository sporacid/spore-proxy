# Table of Content

- [How it works](#how-it-works)

----

# ⚙️ How it works

`spore-proxy` uses stateful meta-programming to build compile-time type sets of known facades, implementations and
dispatch mappings. All this data is iterated once per translation unit to register mappings into a dispatch table
implementation. Since the dispatch expects idempotency, registering the same mapping multiple times is not an issue.

When the library sees a call to `spore::proxies::dispatch`, a new mapping is registered at compile time. When the
dispatch call is invoked at runtime, the mapping is registered for all known implementations of this mapping. This
allows to register template instantiation without any issues!

# 🪞 Proxies

Proxies are the main type to interact with. It encloses a [facade](#-facades), a [storage](#-storages) and
a [semantics](#-semantics) type and is constructed from a concrete implementation.

## Proxy Types

The library provides standard typedefs for most commonly used proxy types.

| Type            | Storage                                      | Semantics                   | Factory                        | Notes                                                                                   |
|-----------------|----------------------------------------------|-----------------------------|--------------------------------|-----------------------------------------------------------------------------------------|
| `value_proxy`   | `proxy_storage_sbo` or `proxy_storage_value` | `proxy_value_semantics`     | `spore::proxies::make_value`   | If the value is small enough, small buffer optimization will be used.                   |
| `inline_proxy`  | `proxy_storage_inline`                       | `proxy_value_semantics`     | `spore::proxies::make_inline`  | N/A                                                                                     |
| `shared_proxy`  | `proxy_storage_shared`                       | `proxy_pointer_semantics`   | `spore::proxies::make_shared`  | N/A                                                                                     |
| `unique_proxy`  | `proxy_storage_unique`                       | `proxy_pointer_semantics`   | `spore::proxies::make_unique`  | N/A                                                                                     |
| `view_proxy`    | `proxy_storage_non_owning`                   | `proxy_pointer_semantics`   | `spore::proxies::make_view`    | Non-owning, so cheap to copy.                                                           |
| `forward_proxy` | `proxy_storage_non_owning`                   | `proxy_reference_semantics` | `spore::proxies::make_forward` | Non-owning, so cheap to copy. Will behave the same way as its forwarded implementation. |

# 🏛️ Facades

A facade define a callable interface that dispatches to implementation. Dispatching doesn't have to be on a function
member, it can dispatch however it likes. Facades must derive from `proxy_facade`, which takes the facade as a
template parameter, plus optionally, additional facade to derived from. Here is a typical example of a facade:

```cpp
struct facade : proxy_facade<facade>
{
    void act() const
    {
        constexpr auto f = [](const auto& self) { self.act(); };
        proxies::dispatch(f, *this);
    }
};
```

## Facade inheritance

Facades can inherit from one another, but not directly. It must be done with `proxy_facade` to make sure that base
facades are visible to the dispatching system.

```cpp
struct base : proxy_facade<base> {};
struct facade : proxy_facade<facade, base> {};
```

In this example, `facade` would inherit from `base_facade` and implementations are expected to implement both facades.

# 📨 Dispatching

When a facade invokes `spore::proxies::dispatch`, there's both a compile-time and a runtime side effect occurring. At
compile-time, a new unique mapping is created with the invocation's unique information. At runtime, the mapping will be
registered once per translation unit, then, there will be a lookup for a dispatch function for the specific
implementation. Finally, the dispatch function will be invoked with forwarded arguments from the facade.

The dispatching system uses an ever-increasing zero-based index to ensure fast lookup. Therefore, finding a specific
dispatch function for an implementation is O(1) and is guaranteed, under the current dispatch implementations, to be a
random access lookup into a contiguous memory region.

## Static Dispatcher

The static dispatcher implementation, `proxy_dispatch_static`, uses a static array to store dispatch functions. There is
one such array per mapping type, e.g.

```cpp
template <typename mapping_t>
static inline proxies::detail::dispatch_type<mapping_t> dispatches[size_v] {};
```

The size limits therefore limits the number of implementations for a specific mapping. For example,
`proxy_dispatch_static<16>` means that for any single mapping type, a maximum of 16 implementations are possible. You
can use this dispatcher type when the number of implementation of a given facade is small and known beforehand.

## Dynamic Dispatcher

The dynamic dispatcher implementation, `proxy_dispatch_dynamic`, uses one `std::vector` to store dispatch functions.
There is one vector per mapping type, per-thread, to ensure thread safety without locks, e.g.

```cpp
template <typename mapping_t>
static inline thread_local std::vector<proxies::detail::dispatch_type<mapping_t>> dispatches {size_v};
```

You can use this dispatcher type when the number of implementation of a given facade is unknown.

## Default Dispatcher

The default dispatcher can be overridden with the macro definition `SPORE_PROXY_DISPATCH_DEFAULT`, e.g.

```cpp
#define SPORE_PROXY_DISPATCH_DEFAULT spore::proxy_dispatch_static<16>

#include "spore/proxy/proxy.hpp"
```

By default, `proxy_dispatch_dynamic` is used as a default.

## Overriding Dispatchers

Dispatchers can be overridden for each facade by declaring a `dispatch_type` typedef. The inheritance chain will be
looked up for an override if the current facade doesn't override the dispatcher. If no override is found, the default
dispatcher will be used for this facade.

```cpp
struct facade : proxy_facade<facade>
{
    using dispatch_type = proxy_dispatch_static<16>;
};
```

## Thread Safety

All dispatch implementations are guaranteed to be thread-safe (hopefully).

## Template Dispatching

There is no limitations on how templates dispatching is used. It's important to note that since template instantiation
is lazy, all translation units will end-up with their own set of template mappings. However, since all translation units
initialize the dispatcher independently, all translation units should be able to dispatch to template functions.

```cpp
struct facade : proxy_facade<facade>
{
    template <typename value_t>
    void act() const
    {
        constexpr auto f = [](const auto& self) { self.template act<value_t>(); };
        proxies::dispatch(f, *this);
    }
};
```

## Special dispatching

### Dispatch or Throw

`spore::proxies::dispatch_or_throw` can be used to weakly dispatch a function if the implementation has it, or throw at
runtime.

```cpp
template <typename value_t>
concept actable = requires(const value& value) {
    { value.act() };
};

struct facade : proxy_facade<facade>
{
    void act() const
    {
        constexpr auto f = []<actable self_t>(const self_t& self) { self.act(); };
        proxies::dispatch_or_throw(f, *this);
    }
};

struct valid_impl
{
    void act() const {}
};

struct invalid_impl
{
};

proxies::make_value<facade>(valid_impl {}).act();   // ✅ ok
proxies::make_value<facade>(invalid_impl {}).act(); // ❌ throws
```

### Dispatch or Default

`spore::proxies::dispatch_or_default` can be used to weakly dispatch a function if the implementation has it, or return
a default value otherwise.

```cpp
template <typename value_t>
concept actable = requires(const value& value) {
    { value.act() };
};

struct facade : proxy_facade<facade>
{
    void act() const
    {
        constexpr auto f = []<actable self_t>(const self_t& self) { self.act(); };
        proxies::dispatch_or_default(f, *this);
    }
};

struct valid_impl
{
    void act() const {}
};

struct invalid_impl
{
};

proxies::make_value<facade>(valid_impl {}).act();   // ✅ ok
proxies::make_value<facade>(invalid_impl {}).act(); // ✅ no-op
```

# 💾 Storages

Common storage implementations are available to customize how facade implementations are stored.

## Shared storage

Ref-counting storage, similar to `std::shared_ptr`.

## Unique storage

Unique, move-only storage, similar to `std::unique_ptr`.

## Value storage

Value-semantics storage, that deep-copies or move its pointer.

## Inline storage

Value-semantics, automatic storage that doesn't type erase its value.

## SBO storage

Value-semantics, automatic storage that type-erase its value.

## Chain storage

Special type of storage that store its value in the first compatible storage.

# 🗣️ Semantics

Semantics implementations allow to customize how to interact with the facade from a proxy.

## Value semantics

The proxy inherits directly from the facade and all its members will be visible from the dot operator. The facade will
behave the same way as its enclosing proxy, i.e. if the proxy is const, only const facade functions can be called.

```cpp
void function(const value_proxy<facade>& p)
{
    // value semantics: const facade&
}

void function(value_proxy<facade>&& p)
{
    // value semantics: facade&&
}
```

## Pointer semantics

The proxy doesn't inherit directly from the facade, and it will be visible through its arrow or star operator. The
facade will always behave as if it was a const or non-const value, despite its enclosing proxy's state.

```cpp
void function(const shared_proxy<facade>& p)
{
    // pointer semantics: facade*
}

void function(shared_proxy<const facade>&& p)
{
    // pointer semantics: const facade*
}
```

## Reference semantics

The proxy doesn't inherit directly from the facade, and it will be visible through its star operator or the function
`get`. The facade will always behave like its ref type, despite its enclosing proxy's state.

```cpp
void function(const forward_proxy<facade&&>& p)
{
    // reference semantics: facade&&
}

void function(forward_proxy<const facade&>&& p)
{
    // reference semantics: const facade&
}
```

# 🔃 Conversions

Proxies can convert between one another depending on the proxy's types. For more detail on which conversions are
available, you can have a look at [this file](../include/spore/proxy/proxy_conversions.hpp). The most common conversion
would be from a derived facade to a base facade, or from any proxy to a proxy view.

```cpp
value_proxy<facade> proxy;

// Moves the implementation into a proxy of base facade
value_proxy<base> base = std::move(proxy);

// Creates a non-owning proxy that points to the same object as proxy, with pointer semantics
view_proxy<facade> view = proxy;

// Creates a non-owning proxy that points to the same object as proxy, with reference semantics
forward_proxy<facade&> = proxy;
```

# ⏱️ Benchmarks

[This project](../benchmarks/src/main.cpp) benchmarks this implementation against other popular libraries and more
typical C++ feature such as virtual interfaces and CRTP.

Here are the results of these benchmarks, in release mode.

## Test

The test does exactly the same thing across all implementations. It does a 1,000 warmup iterations, followed by
100,000,000 timed iterations. `std::chrono::steady_clock` was used for timings. The function being dispatched is this:

```cpp
std::size_t do_work() noexcept
{
    std::size_t result = 0;

    for (std::size_t index = 0; index < 10; index++)
    {
        result += index;
    }

    return result;
}
```

## Hardware

The tests were done on this hardware:

- Intel(R) Core(TM) i9-12900K, 3200 Mhz, 16 Core(s), 24 Logical Processor(s)
- 32 GB RAM
- Windows 11 Version 10.0.26100

The executable was built with these settings:

- Release mode
- Clang-cl compiler
- Maximum optimization and inlining settings ([exact compile options here](../benchmarks/CMakeLists.txt))

## Results

| Name                    | Seconds |
|-------------------------|---------|
| virtual                 | 0.9700  |
| non-virtual             | 0.7611  |
| crtp                    | 0.7698  |
| functional              | 0.9339  |
| microsoft               | 0.9615  |
| avask                   | 0.9790  |
| dyno                    | 0.9429  |
| spore static (inline)   | 0.9229  |
| spore static (value)    | 0.9307  |
| spore static (unique)   | 0.9275  |
| spore static (shared)   | 0.9033  |
| spore static (view)     | 0.8901  |
| spore static (forward)  | 0.8869  |
| spore dynamic (inline)  | 0.9400  |
| spore dynamic (value)   | 0.9274  |
| spore dynamic (unique)  | 0.9238  |
| spore dynamic (shared)  | 0.9241  |
| spore dynamic (view)    | 0.8951  |
| spore dynamic (forward) | 0.9179  |
