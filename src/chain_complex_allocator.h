#ifndef CHAIN_COMPLEX_ALLOCATOR_H
#define CHAIN_COMPLEX_ALLOCATOR_H

#include <memory>

class chain_complex_allocator { 
    unsigned char* _buf;
    chain_complex_allocator* _end{ };
    std::size_t _brk{ };

    static constexpr auto end_segment_size() {
        return sizeof(chain_complex_allocator);
    }

public:
    constexpr chain_complex_allocator(std::size_t initial_size) {
        _buf = ::new unsigned char[initial_size + end_segment_size()];
    }

    template <class T = void> constexpr T* end() const noexcept {
        if constexpr (std::is_void_v<T>) {
            return _buf + _brk; 
        } else {
            return static_cast<T*>(_buf + _brk - sizeof(T));
        }
    }

    template <class T, class ...Args> constexpr std::pair<T*, T*> construct_from_n(T* from, std::size_t n, Args&& ...args) {
        if (_buf + _brk + (n * (alignof(T) + sizeof(T))) + end_segment_size() > end()) {
            chain_complex_allocator* end = ::new(_buf + _brk) chain_complex_allocator(_brk * 2);
            return end->construct_n<T, Args...>(n, std::forward<Args>(args)...);
        } else {
            T* ptr = std::construct_at<T>(_buf + _brk, std::forward<Args>(args)...);
            if (n > 1) {
                _brk += sizeof(T);
                return construct_n<T>(from, n - 1, std::forward<Args>(args)...);
            }
            return from;
        }
    }

    template <class T, class ...Args> constexpr T* construct_n(std::size_t n, Args&& ...args) {
        return  std::get<1>(construct_from_n<T>(end<T>(), std::forward<Args>(args)...));
    }

    template <class T, class ...Args> constexpr T* construct(Args&& ...args) {
        return construct_n(1, std::forward<Args>(args)...);
    }
    
    constexpr ~chain_complex_allocator() {
        ::delete[] _buf;
    }
};

#endif

