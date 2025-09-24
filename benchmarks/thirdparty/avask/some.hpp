// Copyright (C) Alexander Vaskov 2025
// (See accompanying file LICENSE.md)

#pragma once

#include <concepts>
#include <cstdint> //ints
#include <cstring> //memcpy
#include <iostream>
#include <memory> // unique_ptr
#include <type_traits>
#include <utility> // forward, move, exchange

// ===== [ MACROS ] =====
#if defined __GNUC__ // GCC, Clang
#define VX_UNREACHABLE() __builtin_unreachable()
#elif defined _MSC_VER // MSVC
#define VX_UNREACHABLE() (__assume(false))
#else
#define VX_UNREACHABLE() (void)0
#endif

#if defined VX_SOME_ENABLE_LOGGING
#define VX_SOME_LOG(expr) std::cerr << '[' << __FILE__ << ':' << __LINE__ << "] " << expr << std::endl;
#else
#define VX_SOME_LOG(expr)
#endif

#if defined VX_ENABLE_HARDENING
#define VX_HARDENED true
#else
#define VX_HARDENED false
#endif

#if defined VX_SAFER_FSOME
#define VX_FSOME_ELIDE_VCALL_ON_MOVE false
#else
#define VX_FSOME_ELIDE_VCALL_ON_MOVE not VX_HARDENED
#endif

namespace vx {

    using u16 = std::uint16_t;
    using u8 = std::uint8_t;

    namespace cfg {
        /// ===== [ SBO storage configuration ] ======
        struct SBO {
            vx::u16 size;
            vx::u16 alignment { alignof(std::max_align_t) };
        };

        struct some {
            SBO sbo {24};
            bool copy {true};
            bool move {true};
            bool empty_state {true};
            bool check_empty {VX_HARDENED};
        };

        struct fsome {
            SBO sbo {0};
            bool copy {true};
            bool move {true};
            bool empty_state {true};
            bool check_empty {VX_HARDENED};
        };
    }// namespace cfg

    /// ===== [ FWD Declarations ] =====
    template <class Trait>
    class poly_view;

    template <class Trait, typename T>
    struct impl_for;

    template <typename Trait, cfg::some>
    struct some;

    template <typename Trait, cfg::fsome>
    struct fsome;

    template <typename Trait, std::size_t SBO_capacity, std::size_t alignment>
    struct storage_for;

    namespace detail {
        template <typename> struct is_polymorphic : std::false_type {};

        template <typename Trait, cfg::some config>
        struct is_polymorphic< some<Trait, config> > : std::true_type {};

        template <typename Trait, cfg::fsome config>
        struct is_polymorphic< fsome<Trait, config> > : std::true_type {};

        template <typename Trait> struct is_polymorphic< poly_view<Trait> > : std::true_type {};
    } // namespace detail

    template <typename T>
    concept polymorphic = detail::is_polymorphic<std::remove_cvref_t<T>>::value;


    namespace detail {
        template <typename T>
        struct remove_innermost_const_impl {
            using type = T;
        };

        template <typename T>
        struct remove_innermost_const_impl<T const&> {
            using type = T&;
        };

        template <typename T>
        struct remove_innermost_const_impl<T const&&> {
            using type = T&&;
        };

        template <typename T>
        struct remove_innermost_const_impl<T const*> {
            using type = T*;
        };

        template <typename T>
        using remove_innermost_const = typename detail::remove_innermost_const_impl<T>::type;

        // =====[ pointer_like ]=====
        template <typename Ptr>
        concept pointer_like = std::is_pointer_v<Ptr> || requires (Ptr p) {
            { *p };
            { static_cast<bool>(p) };
            { p.operator->() } -> std::convertible_to<decltype( &*p )>;
        };

    }//namespace detail

    // =====[ rvalue ]=====
    template <typename T>
    concept rvalue = std::is_rvalue_reference_v<T&&> && !std::is_const_v<T>;


    /// ===== [ IMPL ] =====
    /// Impl: implementation of the trait for a given class (or any class)
    template <class Trait, typename T>
    struct impl;


    /// ====== [ Support for multiple traits ] =====
    template <typename Trait, typename... Traits> struct mix : Trait, Traits... {};

    namespace detail {
        template <typename Trait>
        struct extract_first_trait_from {
            using type = Trait;
        };

        template <typename Trait, typename... Traits>
        struct extract_first_trait_from<mix<Trait, Traits...>> {
            using type = Trait;
        };

        template <typename>
        struct mixed_traits : std::false_type {};

        template <typename... Ts>
        struct mixed_traits<vx::mix<Ts...>> : std::true_type {};

        template <typename T>
        using remove_ref_or_ptr_t = std::conditional_t<std::is_reference_v<T>,
            std::remove_reference_t<T>,
            std::conditional_t<std::is_pointer_v<T>,
                std::remove_pointer_t<T>,
                T>
            >;

        struct OperationNotSupported : std::runtime_error {};

        enum class opcode : vx::u8 {
            copy_into,
            move_into,
            fsome_move_sbo_into,
#if not VX_FSOME_ELIDE_VCALL_ON_MOVE
            fsome_move_ptr_into,
#endif
            cleanup
        };
    }//namespace detail

    template <typename T>
    using first_trait_from = typename detail::extract_first_trait_from<T>::type;


    namespace detail {
        /// is_sbo_eligible_with<X>:
        template <typename T>
        constexpr bool is_sbo_eligible_with(u16 SBO_capacity, u16 SBO_alignment) {
            return sizeof(T) <= SBO_capacity //< fits into SBO buffer
                   && alignof(T) <= SBO_alignment ///< and has lower alignment
                   && (SBO_alignment % alignof(T) == 0)
                   // either not move-constructible at all (will be checked by the ctor of some/fsome), or noexcept!
                   && (not std::is_move_constructible_v<T> || std::is_nothrow_move_constructible_v<T>);
        }

    }// namespace detail

    /// ===== [ TRAIT ] =====
    /// trait provides a virtual dtor and a do_action + prevents slicing + marks the inheriting class at the same time
    struct trait {
        trait() = default;
        trait (trait const&) = delete;
        trait (trait &&) = delete;
        virtual ~trait()=default;

      private:
        /// @note: Friend structs that should have access to the do_action:
        template <class Trait, typename T> friend struct impl_for;
        template <typename Trait, std::size_t, std::size_t> friend struct storage_for;
        template <typename Trait, cfg::fsome> friend struct fsome;

        /// @brief: do_actions handles the memory-to-memory operations: copy, move, cleanup(for fsome)
        virtual void* do_action(detail::opcode, [[maybe_unused]] void* buffer, cfg::SBO, [[maybe_unused]] void* extra=nullptr) { return nullptr; }
    };


    /// @brief A helper to facilitate simpler creation of the user-defined traits by inheriting from it
    /// @note Inheriting the impl_for constructors is adviced for most of the non-trivial traits
    /// @note Self is an object type with ref- and ptr- qualifications stripped
    /// @tparam Trait Trait type
    /// @tparam T Object type to be made polymorphic
    template <class Trait, typename T>
    struct impl_for : Trait {

        // removes const qualification from ptr- and ref- qualified types, so that it compiles in presence of
        //  an unused const method in a Trait, we will safeguard against actually using that method by other means
        using value_type = std::conditional_t<std::is_reference_v<T> || std::is_pointer_v<T>,
            detail::remove_innermost_const<T>,
            T
            >;

        // Self is an actual object type, stripped of const, pointer and reference qualification
        using Self = std::conditional_t<std::is_reference_v<value_type>,
            std::remove_reference_t<value_type>,
            std::conditional_t<std::is_pointer_v<value_type>,
                std::remove_pointer_t<value_type>,
                value_type>
            >;

        constexpr impl_for() requires std::is_default_constructible_v<T> =default;

        impl_for(impl_for const&) = default;

        /// called by owning some<T>
        explicit impl_for(std::convertible_to<T> auto&& other)
            noexcept(std::is_nothrow_constructible_v<value_type, decltype(other)>)
            : self_{std::forward<decltype(other)>(other)} {}

        const auto* operator->() const { return get(); }

        auto* operator->() { return get(); }

        auto* get() & {
            if constexpr (std::is_pointer_v<value_type>) {
                return self_;
            } else if constexpr (detail::pointer_like<value_type>) {
                return &*self_;
            } else {
                return &self_;
            }
        }

        const auto* get() const& {
            if constexpr (std::is_pointer_v<value_type>) {
                return self_;
            } else if constexpr (detail::pointer_like<value_type>) {
                return &*self_;
            } else {
                return &self_;
            }
        }

        auto& self() {
            return *get();
        }

        auto const& self() const {
            return *get();
        }

      private:
        [[no_unique_address]] value_type self_{};

      protected:
        virtual void* do_action(detail::opcode op, [[maybe_unused]] void* buffer, [[maybe_unused]] cfg::SBO sbo, [[maybe_unused]] void* extra=nullptr) override {
            VX_SOME_LOG("(vcall) do_action");
            switch (op) {
                using enum detail::opcode;
                case copy_into: if constexpr (std::is_copy_constructible_v<Self>) {
                        VX_SOME_LOG("copy_into");
                        VX_SOME_LOG("sbo{"<< sbo.size << ", " << sbo.alignment << "}");
                        // if (sizeof(T) <= sbo.size && alignof(T) <= sbo.alignment) {
                        if constexpr (std::is_pointer_v<T>) {
                            /// fsome copy
                            using Data = Self; //detail::remove_ref_or_ptr_t<T>;
                            /// The T is a pointer to a resource, so we need a deep copy
                            Data * p_object = detail::is_sbo_eligible_with<Data>(sbo.size, sbo.alignment) ?
                                                                                                         new(buffer) Data( static_cast<Data const&>(self()) )
                                                                                                         :
                                                                                                         new Data( static_cast<Data const&>(self()) );

                            auto * p_impl { static_cast<impl<Trait, T> *>( extra ) };
                            new(p_impl) impl<Trait,T> (p_object);
                        } else if constexpr (std::is_object_v<T>) {
                            /// some copy
                            /// The T is the object itself, stored inside the impl<Trait, T>
                            if (detail::is_sbo_eligible_with<T>(sbo.size, sbo.alignment)) {
                                VX_SOME_LOG("[SBO]");
                                return new(buffer) impl<Trait,T>(self_);
                            }
                            VX_SOME_LOG("[PTR]");
                            return new impl<Trait,T>(self_);
                        }
                    } break;

                ///@note move-operation for some<>
                case move_into:
                    if constexpr (vx::rvalue<T&&> && requires { impl<Trait,T>(std::move(self_)); }) {
                        VX_SOME_LOG("MOVE ");
                        if constexpr (noexcept(impl<Trait,T>(std::move(self_)))) {
                            // if (sizeof(T) <= sbo.size && alignof(T) <= sbo.alignment) {
                            if (detail::is_sbo_eligible_with<T>(sbo.size, sbo.alignment)) {
                                VX_SOME_LOG("[SBO]");
                                return new(buffer) impl<Trait,T>(std::move(self_));
                            }
                        }
                        VX_SOME_LOG("[PTR]");
                        return new impl<Trait, T>(std::move(self_));
                    } break;

                ///@note the non-SBO case will be efficiently handled w/o the vcall
                case fsome_move_sbo_into:
                    if constexpr (std::is_pointer_v<T> && std::is_move_constructible_v<Self>) {
                        VX_SOME_LOG("fsome_move_sbo_into");
                        using Data = Self; //detail::remove_ref_or_ptr_t<T>;
                        Data * p_object = detail::is_sbo_eligible_with<Self>(sbo.size, sbo.alignment) ?
                                                                                                     new(buffer) Self( std::move(self()) ) // fits into new SBO buffer => in-place move construct
                                                                                                     :
                                                                                                     new Self( std::move(self()) ); // else, allocate memory for it on the heap

                        auto * p_impl { static_cast<impl<Trait, T> *>( extra ) };
                        new(p_impl) impl<Trait,T> (p_object);
                    } break;

#if not VX_FSOME_ELIDE_VCALL_ON_MOVE
                ///@note the non-SBO case for safer fsome
                case fsome_move_ptr_into: if constexpr (std::is_pointer_v<T>) {
                        VX_SOME_LOG("fsome_move_ptr_into [safe mode]");
                        // self_ is a pointer to the heap-allocated object
                        // extra is a pointer to the target's some_ptr.iface
                        new(extra) impl<Trait, T> (std::exchange(self_, nullptr));
                    } break;
#endif

                //!@note: used exclusively in fsome
                case cleanup: [[unlikely]] {
                        VX_SOME_LOG("cleanup");
                        if constexpr (std::is_pointer_v<value_type>) {
                            using Data = std::remove_pointer_t<value_type>;
                            // if (sbo.size == 0) {
                            VX_SOME_LOG("buffer v self_ v &self");
                            VX_SOME_LOG(buffer << " v " << self_ << " v " << &self_);
                            if (buffer != self_) {
                                // allocated on the heap
                                VX_SOME_LOG("dtor::HEAP");
                                delete static_cast<value_type>(self_);
                            } else {
                                // SBO
                                VX_SOME_LOG("dtor::SBO");
                                static_cast<value_type>(self_)->~Data();
                            }
                        }
                    } break;
            }
            return nullptr;
        }
    };


    template <class CRTP, typename Trait>
    struct basic_operations_for {

        auto* operator-> () noexcept { return iface(); }

        const auto* operator-> () const noexcept { return iface(); }

        Trait& operator*() const { return *iface(); }

        template <typename Target>
        Target* try_get() {
            // std::cerr << "CRTP base: " << vx::type <CRTP> << "\n";
            // std::cerr << "IMPL TYPE: " << vx::type< typename CRTP::template impl_type<Target> > << "\n";
            auto * impl = dynamic_cast<CRTP::template impl_type<Target>*>(iface());
            return impl ? &impl->self() : nullptr;
        }

        template <typename Target>
        const Target* try_get() const noexcept {
            using X = std::remove_const_t<Target>; // for fsome, poly_view and some_ptr that store const-less type
            auto * impl = dynamic_cast<CRTP::template impl_type<X> const*>(iface());
            return impl ? &impl->self() : nullptr;
        }

        template <typename Index>
        decltype(auto) operator[] (Index&& index) requires requires(Trait & t){ t[std::forward<Index>(index)]; }
        {
            return (*iface())[std::forward<Index>(index)];
        }

        decltype(auto) operator! () requires requires(Trait & t){ !t; }
        {
            return !(*iface());
        }

        template <typename... Ts>
        decltype(auto) operator()(Ts&&... args) requires requires(Trait && t){ t(std::forward<Ts>(args)...); }
        {
            return (*iface())(std::forward<Ts>(args)...);
        }

      protected:
        auto* iface() { return static_cast<CRTP&>(*this).trait_ptr(); }
        const auto* iface() const { return static_cast<CRTP const&>(*this).trait_ptr(); }
    };


    template <class CRTP, typename>
    struct multitrait_support_for {};

    template <class CRTP, typename... Traits>
    struct multitrait_support_for<CRTP, vx::mix<Traits...>> {
        // requires (is_one_of<SubTrait, Traits...>)
        // maybe use that as a `try_as`?
        template <typename SubTrait>
        const auto* as() const {
            // if constexpr (is_one_of<SubTrait, Traits...>) {
            if constexpr ((... || std::is_same_v<SubTrait, Traits>)) {
                return static_cast<const SubTrait*>(static_cast<CRTP const&>(*this).trait_ptr());
            } else {
                return dynamic_cast<const SubTrait*>(static_cast<CRTP const&>(*this).trait_ptr());
            }
        }

        template <typename SubTrait>
        requires (!std::is_const_v<CRTP>)
            auto* as() {
            static_assert((... || std::is_same_v<SubTrait, Traits>), "Trait not in the list of mixed traits");
            auto * trait_ptr = static_cast<CRTP&>(*this).trait_ptr();
            if constexpr (std::is_const_v<std::remove_pointer_t<decltype(trait_ptr)>>) {
                return dynamic_cast<SubTrait*>(static_cast<CRTP&>(*this).trait_ptr());
            } else {
                return dynamic_cast<SubTrait*>(static_cast<CRTP&>(*this).trait_ptr());
            }
            // For try_as: (?)
            // if constexpr (is_one_of<SubTrait, Traits...>) {
            //     return static_cast<SubTrait*>(static_cast<CRTP&>(*this).trait_ptr());
            // } else {
            //     return dynamic_cast<SubTrait*>(static_cast<CRTP&>(*this).trait_ptr());
            // }
        }
    };


    struct empty_some_access : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    /// ===== [ SOME PTR ] =====
    /// Pointer to a (to-be)polymorphic object
    template <class Trait, bool checked=false>
    class some_ptr : public basic_operations_for<some_ptr<Trait>, std::remove_cv_t<Trait>> {
        friend struct basic_operations_for<some_ptr<Trait>, std::remove_cv_t<Trait>>;
        template <typename, cfg::fsome> friend struct fsome;

        using layout = struct { void* vptr; void* dptr; };

        // Though we cannot know the size of impl<Trait, T*> before we know what T is,
        // however can force all impl<Trait, T*> to be of more or less the same size, assuming that
        // they all derive from the Trait and contain a T* and no other type-specific things (which makes sense for a view)
        static constexpr auto k_trait_size = sizeof(Trait) + sizeof(void*);
        static constexpr std::size_t alignment = alignof(Trait);

        using raw_trait_t = std::remove_cv_t<Trait>;

        template <typename X>
        using impl_type = impl<raw_trait_t, X*>;

      public:
        template <typename T>
        requires( std::is_const_v<Trait> || not std::is_const_v<T> )
            void set(T * p) {
            static_assert(sizeof(impl<raw_trait_t, T*>) == k_trait_size);
            new(&iface) impl<raw_trait_t, std::remove_const_t<T>*>{const_cast<std::remove_const_t<T>*>(p)};
        }

        template <typename T>
        requires( std::is_const_v<Trait> || not std::is_const_v<T> )
            some_ptr(T * p) {
            set(p);
        }

        template <typename T, typename Del>
        requires( std::is_const_v<Trait> || not std::is_const_v<T> )
            some_ptr(std::unique_ptr<T, Del> p) {
            set(p.get());
            p.release();
        }

        some_ptr() {
            new(&iface) layout{nullptr, nullptr};
        }

        some_ptr(std::nullptr_t) : some_ptr{} {}

        template <typename T>
        requires( std::is_const_v<Trait> || not std::is_const_v<T> )
            some_ptr& operator= (T * p) {
            clear(); /// ??? Or disallow the impl<Trait, T*> to have non-trivial dtors?
            set(p);
            return *this;
        }

        template <typename T, typename Del>
        requires( std::is_const_v<Trait> || not std::is_const_v<T> )
            some_ptr& operator= (std::unique_ptr<T, Del> p) {
            clear(); /// ??? Or disallow the impl<Trait, T*> to have non-trivial dtors?
            set(p.get());
            p.release();
            return *this;
        }

        //!@TODO: Add assignment and copy-ctor to enable changing the pointer, as it should.
        // some_ptr(some_ptr const& other);

        //!@brief: Will do the first part: reinterpreting the iface as Trait*
        //! The rest will work due to the virtual destructor in Trait class.
        ~some_ptr() {
            VX_SOME_LOG(inspect().vptr << "|" << inspect().dptr);
            if (empty()) { // or check data.vptr == data.dptr
                VX_SOME_LOG("EMPTY SOME_PTR deleted");
                return;
            }
            trait_ptr()->~raw_trait_t(); // in case impl<Trait, T*> has some special destructors
        }

      protected:

        /// @brief Used by the `fsome` implementation to optimise move in the heap-allocated case
        /// @note Potential UB, only applies to fsome<>, some<> should always be UB-free.
        void steal_trait_from(some_ptr & other) {
            /// technically this is UB, however it lets us optimise the move for fsome
            /// when the data is heap-allocated, thus eliding the virtual call.
            /// Logically, however, we copy the bit representation verbatim into
            /// another object with the same type and at the same time set the old object's
            /// representation to empty, thus "moving/transfering" the object, logically that is.
            /// impl<Trait, T*> having a virtual dtor it sure ain't trivially_copyable, but
            /// practically speaking we're looking at two pointers (see `layout`) which are,
            /// indeed, pretty trivial to copy.
            std::memcpy(&iface, &other.iface, k_trait_size);
            new(&other.iface) layout{nullptr, nullptr};
            // other = some_ptr();
        }

        inline layout inspect() const noexcept {
            return std::bit_cast<layout>(iface);
        }

        bool empty() const noexcept {
            return inspect().vptr == nullptr;
        }

        void clear() noexcept(not checked) {
            VX_SOME_LOG(inspect().vptr << "|" << inspect().dptr);
            if (empty()) { // or check data.vptr == data.dptr
                VX_SOME_LOG("EMPTY SOME_PTR deleted");
                return;
            }
            trait_ptr()->~raw_trait_t(); // in case impl<Trait, T*> has some special destructors
        }

        /// technically, the iface contains an impl<Trait,T*> with its lifetime started by a placement-new operation
        /// which is in turn derived from Trait, so it *does* contain a Trait subobject and thus a
        /// static_cast<Trait*>(reinterpret_cast<impl<Trait, T*>(&iface)) would seem to be valid.
        /// That being said, we directly reinterpret_cast to Trait*, since we (obviously) don't know the type of this T*.
        /// However, this in itself doesn't change the fact that the Trait object is alive and well at that location
        /// So in threory it sounds like the usecase for std::launder(),
        /// "...to obtain a pointer to an object that already exists at the given memory location,
        /// with its lifetime already started through other means"
        std::add_pointer_t<const raw_trait_t> trait_ptr() const noexcept(not checked) {
            if constexpr (checked) {
                if (empty()) { throw empty_some_access{"empty some_ptr accessed"}; }
            }
            return std::launder(reinterpret_cast<std::add_pointer_t<const raw_trait_t>>(&iface));
        }

        std::add_pointer_t<Trait> trait_ptr() noexcept(not checked) {
            if constexpr (checked) {
                if (empty()) { throw empty_some_access{"empty some_ptr accessed"}; }
            }
            return std::launder(reinterpret_cast<std::add_pointer_t<Trait>>(&iface));
        }

        alignas(alignment) std::byte iface[k_trait_size] = {};
        /// could have a Trait* here, and that would work, but that's extra space
    };



    ///!@brief: A polymorphic view of an already existing object
    ///! <==> a reference to a (to-be)polymorphic object
    ///!@note: Assignment always applies to the view, never to the underlying object
    ///!@note:
    ///!@note: Trait can be const-qualified
    template <class Trait>
    class poly_view : public basic_operations_for<poly_view<Trait>, std::remove_cv_t<Trait>>
    {
        // We clearly cannot compute the size of impl<Trait, T> directly before we know what T is,
        // however the following is known: impl<Trait, T> is derived from Trait + holds a T*
        static constexpr auto k_trait_size = sizeof(Trait) + sizeof(void*);
        static constexpr std::size_t alignment = alignof(Trait);

        using raw_trait_t = std::remove_cv_t<Trait>;

        template <typename X>
        using impl_type = impl<raw_trait_t, X&>;

      public:

        template <typename T>
        requires (std::is_const_v<Trait> || not std::is_const_v<std::remove_reference_t<T>>)
            constexpr poly_view(T && ref) {
            static_assert(not rvalue<T&&>, "cannot be used with poly_view, use some<...> const& for that");
            using R = detail::remove_innermost_const<T>; //std::add_lvalue_reference_t<std::remove_const_t<std::remove_reference_t<T>>>; // remove inner constness
            static_assert(sizeof(impl<raw_trait_t, R>) == k_trait_size);
            new(&iface) impl<raw_trait_t, R>{const_cast<R>(ref)};
        }


        //!@brief: Will do the first part: reinterpreting the iface as Trait*
        //! The rest will work due to the virtual destructor in Trait class.
        ~poly_view() {
            trait_ptr()->~raw_trait_t();
        }

        // template <typename Target>
        // Target* try_get() {
        //     auto * impl = dynamic_cast<vx::impl<raw_trait_t, Target&>*>(trait_ptr());
        //     return impl ? &impl->self : nullptr;
        // }

      protected:
        friend struct basic_operations_for<poly_view<Trait>, std::remove_cv_t<Trait>>;

        std::add_pointer_t<const raw_trait_t> trait_ptr() const noexcept {
            return std::launder(reinterpret_cast<std::add_pointer_t<const raw_trait_t>>(&iface));
        }

        std::add_pointer_t<Trait> trait_ptr() noexcept {
            return std::launder(reinterpret_cast<std::add_pointer_t<Trait>>(&iface));
        }

        //!@note: the main part of the poly_view implementation:
        //! iface is a buffer that is initialized with the impl<Trait,T>
        //! but is later accessed through the Trait*
        //! It works since the Trait is the only non-empty base class of the impl<Trait,T>
        //! So the layout should be as expected.
        alignas(alignment) std::byte iface[k_trait_size];
    };


    /// ===== [ STORAGE ] =====
    template <typename Trait, std::size_t SBO_capacity, std::size_t alignment>
    struct storage_for {
        using main_trait_t = first_trait_from<Trait>;

        template <typename X>
        static constexpr bool is_sbo_eligible = detail::is_sbo_eligible_with<X>(SBO_capacity, alignment);

        storage_for() = default;

        template <typename T>
        explicit storage_for(T&& object) {
            using impl_type = vx::impl< Trait, std::decay_t<T> >;
            if constexpr (is_sbo_eligible<impl_type>) {
                /// [sbo] created in-place in SBO buffer
                p_trait = new(&buffer) impl_type(std::forward<T>(object));
            } else {
                /// [ptr] allocated and assigned to ptr
                p_trait = new impl_type(std::forward<T>(object));
            }
        }


        ~storage_for() noexcept {
            VX_SOME_LOG("~storage_for()");
            clear();
        }


        inline void clear() {
            if (not p_trait) { return; }
            if (this->stored_in_sbo()) {
                p_trait->~main_trait_t();
            } else {
                delete p_trait;
            }
            // p_trait = nullptr;
        }

        template <typename T>
        inline void set(T&& data) {
            using impl_type = vx::impl< Trait, std::decay_t<T> >;
            if constexpr (is_sbo_eligible<impl_type>) {
                /// [sbo] created in-place in SBO buffer
                p_trait = new(&buffer) impl_type(std::forward<T>(data));
            } else {
                /// [ptr] allocated and assigned to ptr
                p_trait = new impl_type(std::forward<T>(data));
            }
        }


        //!@note: Expects the dest to be in a reset state, i.e. the previously occuping object has been destroyed
        template <std::size_t dest_SBO, std::size_t dest_alignment>
        void copy_into(storage_for<Trait, dest_SBO, dest_alignment> & dest) const {
            dest.p_trait = (main_trait_t*)p_trait->do_action(detail::opcode::copy_into, (void*)&dest, {dest_SBO, dest_alignment});
        }


        //!@note: Expects the dest to be in a reset state, i.e. the previously occuping object has been destroyed
        template <std::size_t dest_SBO, std::size_t dest_alignment>
        void move_into(storage_for<Trait, dest_SBO, dest_alignment> & dest) && noexcept {
            if (this->stored_in_sbo()) {
                dest.p_trait = (main_trait_t*)p_trait->do_action(detail::opcode::move_into, (void*)&dest.buffer, {dest_SBO, dest_alignment});
            } else {
                dest.p_trait = std::exchange(p_trait, nullptr);
            }
        }


        bool stored_in_sbo() {
            return (void*)p_trait == (void*)&buffer;
        }


        alignas(alignment) std::byte buffer[SBO_capacity];
        main_trait_t * p_trait = nullptr;
    };


    template <typename Trait, std::size_t Alignment>
    struct storage_for<Trait, 0, Alignment> {
        using main_trait_t = first_trait_from<Trait>;
        main_trait_t *p_trait = nullptr;

        template <typename X>
        static constexpr bool is_sbo_eligible = false;

        storage_for() = default;

        template <typename T>
        explicit storage_for(T&& object) {
            using impl_type = vx::impl< Trait, std::decay_t<T> >;
            p_trait = new impl_type(std::forward<T>(object));
        }

        ~storage_for() {
            VX_SOME_LOG("~storage_for() [NO SBO]");
            clear();
        }

        inline void clear() {
            if (p_trait) delete p_trait;
        }

        template <typename T>
        inline void set(T&& data) {
            using impl_type = vx::impl< Trait, std::decay_t<T> >;
            p_trait = new impl_type(std::forward<T>(data));
        }

        //!@note: Expects the dest to be in a reset state, i.e. the previously occuping object has been destroyed
        template <std::size_t dest_SBO, std::size_t dest_alignment>
        void copy_into(storage_for<Trait, dest_SBO, dest_alignment> & dest) const {
            VX_SOME_LOG("storage_for [NO SBO]");
            dest.p_trait = (main_trait_t*)p_trait->do_action(detail::opcode::copy_into, (void*)&dest.buffer, {dest_SBO, dest_alignment});
        }

        //!@note: Expects the dest to be in a reset state, i.e. the previously occuping object has been destroyed
        template <std::size_t dest_SBO, std::size_t dest_alignment>
        void move_into(storage_for<Trait, dest_SBO, dest_alignment> & dest) && noexcept {
            dest.p_trait = std::exchange(p_trait, nullptr);
        }
    };


    /// ===== [ SOME ] =====
    /// An object (holds the ownership) with value semantics and polymorphic behaviour
    /// The Trait is an interface an object has to satisfy
    template <typename Trait=vx::trait, cfg::some config=cfg::some{}>
    struct some : basic_operations_for<some<Trait, config>, Trait>,
                  multitrait_support_for<some<Trait, config>, Trait>
    {
        // this one is needed for the CRTP class basic_operations_for<>
        template <typename X>
        using impl_type = vx::impl< Trait, std::remove_cvref_t<X> >;

        template <typename T2, cfg::some c2>
        friend struct some;


        some() requires(config.empty_state) =default;


        template <typename T>
        requires (not polymorphic<T>)
            some (T && obj) requires ((not config.copy || std::is_copy_constructible_v<std::remove_cvref_t<T>>)
                                   &&
                                   (not config.move || std::is_move_constructible_v<std::remove_cvref_t<T>>))
            : storage{std::forward<T>(obj)} {
            static_assert(not config.copy || std::is_copy_constructible_v<std::remove_cvref_t<T>>,
                "The object is required to be copyable by the configuration");
            static_assert(not config.move || std::is_move_constructible_v<std::remove_cvref_t<T>>,
                "The object is required to be move constructible by the configuration");
        }

        ~some() = default;

        some(some const& other) {
            other.storage.copy_into(this->storage);
        }

        some& operator= (some const& other) {
            storage.clear();
            other.storage.copy_into(this->storage);
            return *this;
        }

        template <cfg::some config2>
        some(some<Trait, config2> const& other) {
            other.storage.copy_into(this->storage);
        }

        template <cfg::some config2>
        some& operator= (some<Trait, config2> const& other) {
            storage.clear();
            other.storage.copy_into(this->storage);
            return *this;
        }

        ///@brief: overload for a more efficient copy- and move- assignment from a non-polymorphic object
        template <typename T> requires (not polymorphic<T>)
            some& operator= (T && obj) requires ((not config.copy || std::is_copy_constructible_v<std::remove_cvref_t<T>>)
                                        &&
                                        (not config.move || std::is_move_constructible_v<std::remove_cvref_t<T>>)) {
            storage.clear();
            storage.set(std::forward<T>(obj));
            return *this;
        }


        template <cfg::some config2>
        some(some<Trait, config2> && other) noexcept {
            std::move(other).storage.move_into(this->storage);
        }

        template <cfg::some config2>
        some& operator= (some<Trait, config2> && other) noexcept {
            storage.clear();
            std::move(other).storage.move_into(this->storage);
            return *this;
        }

      protected:
        friend struct basic_operations_for<some<Trait, config>, Trait>;
        friend struct multitrait_support_for<some<Trait, config>, Trait>;

        const auto* trait_ptr() const noexcept(not config.check_empty) {
            if constexpr (config.check_empty) {
                if (storage.p_trait == nullptr) { throw empty_some_access{"empty some<> accessed"}; }
            }
            return storage.p_trait;
        }
        auto* trait_ptr() noexcept(not config.check_empty) {
            if constexpr (config.check_empty) {
                if (storage.p_trait == nullptr) { throw empty_some_access{"empty some<> accessed"}; }
            }
            return storage.p_trait;
        }

      private:
        storage_for<Trait, config.sbo.size, config.sbo.alignment> storage;
    };


    template <typename Trait>
    struct some<Trait &> : poly_view<Trait> {
        using poly_view<Trait>::poly_view;
    };

    template <typename Trait>
    struct some<Trait const&> : poly_view<const Trait> { //some_cref<Trait> {
        // using some_cref<Trait>::some_cref;
        using poly_view<const Trait>::poly_view;
    };


    /// @brief storage policy: [SBO / heap-only]
    /// Unlike the `storage` type used in `some`, doesn't know the stored object type
    /// As such, it only allocates, the deallocation is handled in the virtual function
    /// in `trait::do_action(opcode::cleanup, ...)`
    /// @tparam capacity: SBO buffer capacity
    /// @tparam align: max supported type alignment
    template <std::size_t capacity, std::size_t alignment>
    struct fsome_storage_policy {

        template <typename X>
        static constexpr bool is_sbo_eligible = detail::is_sbo_eligible_with<X>(capacity, alignment);

        fsome_storage_policy() = default;

        void* get_sbo_buffer() noexcept { return &sbo[0]; }

        template <typename T, typename X = std::remove_cvref_t<T>>
        auto make(T && obj) {
            if constexpr (is_sbo_eligible<X>) {
                using Deleter = decltype([](X * p){ p->~X(); });
                return std::unique_ptr<X, Deleter>(new(&sbo) X(std::forward<T>(obj)));
            } else {
                return std::make_unique<X>(obj);
                // return new X(std::forward<T>(obj));
            }
        }

      private:
        alignas(alignment) std::byte sbo[capacity];
    };

    template <std::size_t alignment>
    struct fsome_storage_policy<0, alignment> {
        fsome_storage_policy() = default;

        constexpr void* get_sbo_buffer() const noexcept { return nullptr; }

        template <typename T, typename X = std::remove_cvref_t<T>>
        auto make(T && obj) {
            // return new X(std::forward<T>(obj));
            return std::make_unique<X>(std::forward<T>(obj));
        }
    };


    ///@brief fsome: a (faster/fatter) owning polymorphic object,
    ///       let's expand on those statements:
    /// - fast: it should be faster since we're shaving off an extra indirection by
    ///   keeping a vptr in the `fsome` object itself, as opposed to the `some` that
    ///   will keep a vptr inside the polymorphic object, which can be on the heap.
    /// - The second point should be obvoius by now.
    template <typename Trait=vx::trait, vx::cfg::fsome config = vx::cfg::fsome{}>
    struct fsome : public fsome_storage_policy<config.sbo.size, config.sbo.alignment>,
                   public basic_operations_for<fsome<Trait, config>, Trait> {

        template <typename, vx::cfg::fsome>
        friend struct fsome;

        /// @brief This one is needed for the `basic_operations_for` CRTP to work
        /// It converts the type X into the actual wrapped type impl<Trait, T> but here's a catch:
        /// It's not always exactly T :)
        template <typename X>
        using impl_type = some_ptr<Trait, config.check_empty>::template impl_type<X>;

        using storage_policy = fsome_storage_policy<config.sbo.size, config.sbo.alignment>;


        fsome() requires(config.empty_state) =default;


        template <typename T>
        fsome(T && obj) requires (not polymorphic<T>
                                &&
                                (not config.copy || std::is_copy_constructible_v<std::remove_cvref_t<T>>)
                                &&
                                (not config.move || std::is_move_constructible_v<std::remove_cvref_t<T>>))
            : poly_{ this->make(std::forward<T>(obj)) }
        {
            static_assert(not config.copy || std::is_copy_constructible_v<std::remove_cvref_t<T>>,
                "The object is required to be copyable by the configuration");
            static_assert(not config.move || std::is_move_constructible_v<std::remove_cvref_t<T>>,
                "The object is required to be move constructible by the configuration");
        }


        fsome(fsome const& other) : poly_{}
        {
            VX_SOME_LOG(__PRETTY_FUNCTION__);
            other.copy_into(*this);
        }


        template <cfg::fsome other_config>
        fsome(fsome<Trait, other_config> const& other) : poly_{}
        {
            VX_SOME_LOG(__PRETTY_FUNCTION__);
            other.copy_into(*this);
            /// impl<Trait, T*> holds a pointer to some newly created Data (either SBO or not) (@note: not wrapped into impl<Trait, T>!)
            /// the Data type is the same as the one in other's poly_, so the following **could** make sense (not to the TBAA however)
            // void * p_data = static_cast<Trait*>(const_cast<fsome<Trait, other_config> &>(other)->do_action(detail::opcode::copy_into, this->get_sbo_buffer(), config.sbo));
            // std::memcpy((void*)&poly_, (void*)&other.poly_, sizeof(poly_)); // starts the lifetime of other.poly_ (of type some_ptr) in our poly_'s memory location
            /// the vptr part is good, only need to swap the data pointer
            // using Layout = struct { void* vptr; void* dptr; };
            // reinterpret_cast<Layout*>(&poly_)->dptr = p_data; // attempts to sneakily swap the pointer without blowing it all up... (UB)
        }


        template <cfg::fsome other_config>
        fsome& operator= (fsome<Trait, other_config> const& other)
        {
            VX_SOME_LOG(__PRETTY_FUNCTION__);
            if constexpr (other_config.empty_state) {
                if (other.poly_.empty()) { return *this; }
            }
            clear();
            other.copy_into(*this);
            return *this;
        }

        // needed so that there is no default generated
        fsome& operator= (fsome const& other) {
            VX_SOME_LOG(__PRETTY_FUNCTION__);
            if constexpr (config.empty_state) {
                if (other.poly_.empty()) { return *this; }
            }
            clear();
            other.copy_into(*this);
            return *this;
        }

        ///@brief: overload for a more efficient copy- and move- assignment from a non-polymorphic object
        template <typename T> requires (not polymorphic<T>)
            fsome& operator= (T && obj) requires ((not config.copy || std::is_copy_constructible_v<std::remove_cvref_t<T>>)
                                        &&
                                        (not config.move || std::is_move_constructible_v<std::remove_cvref_t<T>>)) {
            clear();
            poly_ = this->make(std::forward<T>(obj));
            return *this;
        }

        fsome(fsome && other) noexcept : poly_{} {
            std::move(other).move_into(*this);
        }

        template <cfg::fsome other_config>
        fsome(fsome<Trait, other_config> && other) noexcept : poly_{}
        {
            std::move(other).move_into(*this);
        }


        template <cfg::fsome other_config>
        fsome& operator= (fsome<Trait, other_config> && other)
        {
            VX_SOME_LOG(__PRETTY_FUNCTION__);
            if constexpr (other_config.empty_state) {
                if (other.poly_.empty()) { return *this; }
            }
            clear();
            std::move(other).move_into(*this);
            return *this;
        }

        ~fsome() {
            /// cleanup will check to see it the pointer == get_sbo_buffer, if so it's in SBO, otherwise, on the heap.
            if (not poly_.empty()) { poly_->do_action(detail::opcode::cleanup, this->get_sbo_buffer(), {0,0}); }
        }

      protected:
        friend struct basic_operations_for<fsome<Trait, config>, Trait>;

        auto* trait_ptr() noexcept { return poly_.trait_ptr(); }
        auto const* trait_ptr() const noexcept { return poly_.trait_ptr(); }

        void clear() noexcept {
            if constexpr (config.empty_state) {
                if (poly_.empty()) { return; }
            }
            poly_->do_action(detail::opcode::cleanup, this->get_sbo_buffer(), config.sbo);
        }

        template <cfg::fsome other_config>
        void copy_into(fsome<Trait, other_config> & other) const& {
            if constexpr (config.empty_state) {
                if (this->poly_.empty()) { return; }
            }
            // passes the &other.poly_.iface along so the actual thing can be placement-new-constructed in there directly
            // the some_ptr class uses the std::launder anyway to stop the TBAA from intervening, so should work
            const_cast<fsome&>(*this)->do_action(
                detail::opcode::copy_into, other.get_sbo_buffer(), other_config.sbo, (void*)&other.poly_.iface);
        }

        template <cfg::fsome other_config>
        void move_into(fsome<Trait, other_config> & other) && noexcept
        {
            if constexpr (config.sbo.size == 0) {
                // No SBO even possible, runtime branch unnecessary
#if VX_FSOME_ELIDE_VCALL_ON_MOVE
                other.poly_.steal_trait_from(this->poly_);
#else
                if constexpr (config.empty_state) {
                    if (poly_.empty()) { return; }
                }
                poly_->do_action(detail::opcode::fsome_move_ptr_into,
                    nullptr, {}, (void*)&other.poly_.iface);
                poly_ = {};
#endif
            } else {
                if (this->poly_.inspect().dptr == this->get_sbo_buffer()) {
                    // SBO:
                    VX_SOME_LOG("fsome::move_into [from SBO]:\n");
                    if constexpr (config.empty_state) {
                        if (poly_.empty()) { return; }
                    }
                    poly_->do_action(detail::opcode::fsome_move_sbo_into,
                        other.get_sbo_buffer(), other_config.sbo, (void*)&other.poly_.iface);
                } else {
                    // Stored on the heap, so a quick representation swap will do.
                    // `this` will be left in an empty state
                    VX_SOME_LOG("fsome::move_into [from PTR]:\n");
#if VX_FSOME_ELIDE_VCALL_ON_MOVE
                    other.poly_.steal_trait_from(this->poly_);
#else
                    if constexpr (config.empty_state) {
                        if (poly_.empty()) { return; }
                    }
                    poly_->do_action(detail::opcode::fsome_move_ptr_into,
                        nullptr, {}, (void*)&other.poly_.iface);
                    poly_ = {};
#endif
                    // other.poly_.steal_trait_from(this->poly_);
                }
            }
        }

      private:
        some_ptr<Trait, config.check_empty> poly_{}; // {vptr + data_ptr}
        // using Layout = struct { void* vptr; void* dptr; };
    };


    /// ===== [ IMPL specializations ] =====
    ///@brief: Default do-nothing trait
    template <typename T>
    struct impl<vx::trait, T> : vx::impl_for<vx::trait, T> {
        using impl_for<vx::trait, T>::impl_for;
    };

    ///@brief: Support for mixed traits:
    template <typename Trait, typename... Traits, typename T>
    requires (sizeof...(Traits) > 1)
        struct impl<mix<Trait,Traits...>, T> : public impl<mix<Traits...>, impl<Trait, T>> {
        using impl<mix<Traits...>, impl<Trait, T>>::impl;
    };

    template <typename Trait1, typename Trait2, typename T>
    struct impl<mix<Trait1, Trait2>, T> : impl<Trait1, impl<Trait2, T>> {
        using impl<Trait1, impl<Trait2, T>>::impl;
    };

    template <typename Trait1, typename Trait2, typename T>
    struct impl_for<Trait1, impl<Trait2, T>> : Trait1, impl<Trait2, T> {
        using impl<Trait2, T>::impl;
    };

    /// ===== [ Poly helper ] =====
    template <typename X>
    struct poly;

    template <typename Trait, typename T>
    struct poly< vx::impl<Trait, T>* > {
        using Impl = vx::impl<Trait, T>;
        Impl * impl;
        auto operator->() {
            return impl->get();
        }

        auto& operator*() {
            return *impl->get();
        }
    };

    template <typename Trait, typename T>
    poly(vx::impl<Trait,T> *) -> poly<vx::impl<Trait,T> *>;


    template <typename Trait, typename T>
    struct poly< vx::impl<Trait, T> const* > {
        using Impl = const vx::impl<Trait, T>;
        Impl * impl;
        auto operator->() const {
            return impl->get();
        }

        auto const& operator*() const {
            return *impl->get();
        }
    };


    template <typename Trait, typename T>
    poly(vx::impl<Trait,T> const*) -> poly<vx::impl<Trait,T> const*>;


    struct bad_some_cast : std::runtime_error {
        using std::runtime_error::runtime_error;
    };

    template <typename T, vx::polymorphic Object>
    requires (
        not std::is_pointer_v<T>
        &&
        (not std::is_reference_v<T>
            || (std::is_const_v<std::remove_reference_t<Object>> == std::is_const_v<std::remove_reference_t<T>>))
        &&
        not (std::is_rvalue_reference_v<Object&&> && std::is_reference_v<T>)
            )
        T some_cast (Object && obj) {
        using U = std::remove_cvref_t<T>;
        if (auto p = obj.template try_get<U>(); p != nullptr) {
            return *p;
        } else {
            throw bad_some_cast{"some<> contains a different object"};
        }
    }

    template <typename T, vx::polymorphic Object>
    requires (std::is_pointer_v<T> && not rvalue<Object>)
        auto* some_cast (Object&& obj) {
        return some_cast<std::remove_pointer_t<T>>(&obj);
    }


    template <typename T>
    requires (not std::is_pointer_v<T>)
        T* some_cast (vx::polymorphic auto* obj) noexcept {
        return obj->template try_get<T>();
    }

    template <typename T>
    requires (not std::is_pointer_v<T>)
        const T* some_cast (const vx::polymorphic auto * obj) noexcept {
        return obj->template try_get<T>();
    }



} // namespace vx


#undef VX_FSOME_ELIDE_VCALL_ON_MOVE
#undef VX_HARDENED
#undef VX_SOME_LOG
#undef VX_UNREACHABLE