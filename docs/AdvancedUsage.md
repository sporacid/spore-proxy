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

Proxies are the main 

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

# 🗣️Semantics

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

# ⏱️ Benchmarks