# 📦 Spore Proxy - Template-Friendly Runtime Polymorphism

`spore-proxy` is a C++20, header-only, no-dependencies, type-erasure and runtime polymorphism library that allows
blazing fast
virtual template dispatching.

---

## 🚀 Features

- ✅ Template function dispatching
- ✅ Type-safe API
- ✅ Zero dependencies
- ✅ Cross-platform support
- ✅ Header-only

---

## 📥 Installation

Simply copy the content of `include` directory in your project, and you're set up! Simply include the `proxy.hpp` header
to have access to all features.

```cpp
#include "spore/proxy/proxy.hpp"
```

---

## 🧪 Usage

```cpp
#include <iostream>

#include "spore/proxy/proxy.hpp"

using namespace spore;

struct facade : proxy_facade<facade>
{
    void act() const
    {
        constexpr auto f = [](const auto& self) { self.act(); };
        proxies::dispatch(f, *this);
    }
};

struct impl
{
    void act() const
    {
        std::cout << "Action!" << std::endl;
    }
};

int main()
{
    value_proxy<facade> p = proxies::make_value<facade, impl>();
    p.act();

    return 0;
}
```

More examples can be found in the [examples](examples) directory, or you can have a look
at [tests](tests/src/t_proxy.cpp) for more advanced usage!

---

## 📚 Documentation

Full documentation is [available here](docs/AdvancedUsage.md).

---

## 🧰 Requirements

- C++20 or later

---

## 🧑‍💻 Contributing

This project welcomes contributions and suggestions.

To report bugs or request features, open an issue.

---

## 🛡️ License

This project is licensed under the [Boost License](LICENSE).

Feel free to use, modify, and distribute.

---

## ❤️ Acknowledgments

- Inspired by [boost-ext/te](https://github.com/boost-ext/te) and [microsoft/proxy](https://github.com/microsoft/proxy)
- Inspired by [this article](https://mc-deltat.github.io/articles/stateful-metaprogramming-cpp20)
  by [Reece Jones](https://github.com/ReeceJones) to implement stateful meta-programming at scale
- Thanks to [Kris Jusiak](https://github.com/kris-jusiak) for making awesome libraries and inspiring me to implement
  the craziest ideas