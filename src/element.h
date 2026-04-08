#ifndef ELEMENT_H
#define ELEMENT_H

#include "aspect.h"
#include "index_sequence_polyfill.h"
#include <concepts>
#include <meta>

namespace detail {
template <std::size_t I, std::meta::info T> consteval std::string child_name() {
    static_assert(is_type(T));
    constexpr auto ii = std::meta::reflect_constant(I);

    std::string name;
    if constexpr (std::meta::has_template_arguments(T)) {
        name = identifier_of(template_of(T));
    } else if constexpr (has_identifier(T)) {
        name = identifier_of(T);
    } else {
        name = display_string_of(T);
    }

    return name + display_string_of(ii);
}

struct binding_index {
    std::size_t index;
};

template <class... A> struct with_aspects_impl {
    struct aspects_impl;
    struct children_impl;

    consteval {
        std::vector<std::meta::info> mems;
        std::vector<std::meta::info> child_types;

        // this was originally an expansion statement but it causes a crash in clangd that i don't understand so we're
        // back to doing it this way.
        [&]<std::size_t ...Is>(std::index_sequence<Is...>){
            ([&]<std::size_t I, class T>(){
                constexpr auto a = ^^T;

                if constexpr (is_aspect_type(a)) {
                    mems.push_back(std::meta::data_member_spec(a, { .name = aspect_name<typename[:a:]> }));
                } else if constexpr (is_type(a)) {
                    child_types.push_back(std::meta::data_member_spec(a, { .name = child_name<I, a>() }));
                }
            }.template operator()<Is, A>(), ...);
        }(std::make_index_sequence<sizeof...(A)>{ });

        define_aggregate(^^aspects_impl, mems);
        define_aggregate(^^children_impl, child_types);
    }
};

} // namespace detail
template <class... A> using with_aspects = detail::with_aspects_impl<A...>::aspects_impl;
template <class... A> using with_children = detail::with_aspects_impl<A...>::children_impl;

template <class El> class element;

template <template <class...> class El, class... A> class element<El<A...>>: public with_aspects<A...>,
                                                                             public with_children<A...> {
    using el_type = El<A...>;

    static consteval void traverse_bases(std::meta::info type, std::vector<std::meta::info>& v) {
        constexpr auto ctx = std::meta::access_context::unchecked();

        for (auto base : bases_of(type, ctx)) { traverse_bases(type_of(base), v); }
        v.push_back(dealias(type));
    }

    template <class B = el_type> static consteval decltype(auto) aspect_storage_types() {
        std::vector<std::meta::info> v{};
        traverse_bases(dealias(^^B), v);
        return define_static_array(v);
    }

    template <class Gen> static consteval auto aspect_info() {
        constexpr auto ctx = std::meta::access_context::unchecked();

        constexpr auto storage_candidates = aspect_storage_types();
        static_assert(storage_candidates.size() >= 1);

        template for (constexpr std::meta::info base : storage_candidates) {
            static_assert(is_type(base));
            constexpr auto storage = (base);
            constexpr auto mems = define_static_array(nonstatic_data_members_of(storage, ctx));
            template for (constexpr auto mem : mems) {
                constexpr auto mem_type = type_of(mem);
                if constexpr (is_aspect_type(mem_type)) {
                    if constexpr (get_generator_info(mem_type) == ^^Gen) { return mem; }
                }
            }
        }
        return std::meta::info{};
    }

    static consteval auto children_in() {
        constexpr auto ctx = std::meta::access_context::unchecked();
        std::vector<std::meta::info> c;

        constexpr auto bases = aspect_storage_types<el_type>();

        template for (constexpr std::meta::info base : aspect_storage_types<el_type>()) {
            constexpr auto mems = define_static_array(nonstatic_data_members_of(base, ctx));
            template for (constexpr auto mem : mems) {
                if constexpr (!is_aspect_type(type_of(mem))) { c.push_back(mem); }
            }
        }
        return define_static_array(c);
    }

    template <std::size_t I> static consteval auto child_index_from_raw_index() {
        std::size_t c = I;
        template for (constexpr auto J : std::make_index_sequence<I>{}) {
            if constexpr (aspect_type<A...[J]>) { --c; }
        }
        return c;
    }

    template <class T, std::size_t I> constexpr auto initialize(T&& t) {
        constexpr auto info = ^^T;
        if constexpr (is_aspect_type(info)) {
            constexpr auto generator_info = get_generator_info(info);
            std::construct_at(std::addressof(this->[:aspect_info<typename[:generator_info:]>():]), std::forward<T>(t));
        } else {
            std::construct_at(std::addressof(this->child<child_index_from_raw_index<I>()>()), std::forward<T>(t));
        }
    }

public:
    constexpr element() = default;
    constexpr element(const element&) = default;
    constexpr element(element&&) = default;

    constexpr element(A&&... args) {
        template for (constexpr auto I : std::make_index_sequence<sizeof...(A)>{}) {
            this->template initialize<A...[I], I>(std::forward<A...[I]>(args...[I]));
        }
    }

    template <class G, class Self> constexpr auto& get(this Self& self) {
        constexpr auto info = aspect_info<G>();
        static_assert(is_nonstatic_data_member(info), static_cast<std::string_view>(G::name()));
        return self.[:info:].value;
    }

    template <aspect_gen_type G> constexpr auto& operator[](this auto& self, G) {
        return self.template get<G>();
    }

    template <aspect_gen_type G> static consteval bool has(G) {
        constexpr auto info = aspect_info<G>();
        if constexpr (is_nonstatic_data_member(info)) { return true; }
        return false;
    }

    constexpr decltype(auto) aspects() const { return static_cast<const with_aspects<A...>&>(*this); }

    constexpr decltype(auto) aspects() { return static_cast<with_aspects<A...>&>(*this); }

    static consteval auto count_aspects() {
        return nonstatic_data_members_of(^^with_aspects<A...>, std::meta::access_context::unchecked()).size();
    }

    template <const auto& Gen, class Func> constexpr auto map(this auto& self, Func&& func)
        requires(aspect_gen_type<std::decay_t<decltype(Gen)>>)
    {
        if constexpr (has(Gen)) {
            if constexpr (std::invocable<Func, decltype(self[Gen])&>) {
                return std::invoke(std::forward<Func>(func), self[Gen]);
            }
        }
    }

    template <std::size_t I> constexpr auto& child(this auto& self) { return self.[:children_in()[I]:]; }

    constexpr decltype(auto) children() const { return static_cast<const with_children<A...>&>(*this); }

    constexpr decltype(auto) children() { return static_cast<with_children<A...>&>(*this); }

    static consteval auto count_children() { return children_in().size(); }

    template <class... Sig> constexpr void signal(this auto& self, Sig&&... sig) {
        if constexpr (requires { requires self(std::forward<Sig>(sig)...); }) {
            self(std::forward<Sig>(sig)...);
        } else {
            self.signal_children(std::forward<Sig>(sig)...);
        }
    }

    template <class Self, class... Sig> constexpr void signal_childen(this Self& self, Sig&&... sig) {
        template for (auto& child : self.children()) {
            if constexpr (requires { requires child(std::forward<Sig>(sig)...); }) {
                child(std::forward<Sig>(sig)...);
            }
        }
    }
};

template <auto Gen, class El> constexpr auto& get(const element<El>& el) {
    static_assert(el.has(Gen));
    return el[Gen];
}

template <class T> concept element_type = std::derived_from<T, element<T>>;

#endif

