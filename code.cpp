#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

class RefCellError : public std::runtime_error {
public:
    explicit RefCellError(const std::string& message) : std::runtime_error(message) {}
    virtual ~RefCellError() = default;
};

// invalidly call an immutable borrow
class BorrowError : public RefCellError {
public:
    explicit BorrowError(const std::string& message) : RefCellError(message) {}
};

// invalidly call a mutable borrow
class BorrowMutError : public RefCellError {
public:
    explicit BorrowMutError(const std::string& message) : RefCellError(message) {}
};

// still has refs when destructed
class DestructionError : public RefCellError {
public:
    explicit DestructionError(const std::string& message) : RefCellError(message) {}
};

template <typename T>
class RefCell {
private:
    T value;
    mutable int borrow_count; // 0: none, > 0: immutable, -1: mutable

public:
    // Forward declarations
    class Ref;
    class RefMut;

    // Constructor
    explicit RefCell(const T& initial_value) : value(initial_value), borrow_count(0) {}
    explicit RefCell(T&& initial_value) : value(std::move(initial_value)), borrow_count(0) {}

    // Disable copying and moving for simplicity
    RefCell(const RefCell&) = delete;
    RefCell& operator=(const RefCell&) = delete;
    RefCell(RefCell&&) = delete;
    RefCell& operator=(RefCell&&) = delete;

    // Borrow methods
    Ref borrow() const {
        if (borrow_count == -1) {
            throw BorrowError("Already mutably borrowed");
        }
        borrow_count++;
        return Ref(&value, &borrow_count);
    }

    std::optional<Ref> try_borrow() const {
        if (borrow_count == -1) {
            return std::nullopt;
        }
        borrow_count++;
        return Ref(&value, &borrow_count);
    }

    RefMut borrow_mut() {
        if (borrow_count != 0) {
            throw BorrowMutError("Already borrowed");
        }
        borrow_count = -1;
        return RefMut(&value, &borrow_count);
    }

    std::optional<RefMut> try_borrow_mut() {
        if (borrow_count != 0) {
            return std::nullopt;
        }
        borrow_count = -1;
        return RefMut(&value, &borrow_count);
    }

    // Inner classes for borrows
    class Ref {
    private:
        const T* ptr;
        int* count_ptr;

    public:
        Ref() : ptr(nullptr), count_ptr(nullptr) {}
        Ref(const T* p, int* c) : ptr(p), count_ptr(c) {}

        ~Ref() {
            if (count_ptr) {
                (*count_ptr)--;
            }
        }

        const T& operator*() const {
            return *ptr;
        }

        const T* operator->() const {
            return ptr;
        }

        // Allow copying
        Ref(const Ref& other) : ptr(other.ptr), count_ptr(other.count_ptr) {
            if (count_ptr) {
                (*count_ptr)++;
            }
        }
        Ref& operator=(const Ref& other) {
            if (this != &other) {
                if (count_ptr) {
                    (*count_ptr)--;
                }
                ptr = other.ptr;
                count_ptr = other.count_ptr;
                if (count_ptr) {
                    (*count_ptr)++;
                }
            }
            return *this;
        }

        // Allow moving
        Ref(Ref&& other) noexcept : ptr(other.ptr), count_ptr(other.count_ptr) {
            other.ptr = nullptr;
            other.count_ptr = nullptr;
        }

        Ref& operator=(Ref&& other) noexcept {
            if (this != &other) {
                if (count_ptr) {
                    (*count_ptr)--;
                }
                ptr = other.ptr;
                count_ptr = other.count_ptr;
                other.ptr = nullptr;
                other.count_ptr = nullptr;
            }
            return *this;
        }
    };

    class RefMut {
    private:
        T* ptr;
        int* count_ptr;

    public:
        RefMut() : ptr(nullptr), count_ptr(nullptr) {}
        RefMut(T* p, int* c) : ptr(p), count_ptr(c) {}

        ~RefMut() {
            if (count_ptr) {
                *count_ptr = 0;
            }
        }

        T& operator*() {
            return *ptr;
        }

        T* operator->() {
            return ptr;
        }

        // Disable copying to ensure correct borrow rules
        RefMut(const RefMut&) = delete;
        RefMut& operator=(const RefMut&) = delete;

        // Allow moving
        RefMut(RefMut&& other) noexcept : ptr(other.ptr), count_ptr(other.count_ptr) {
            other.ptr = nullptr;
            other.count_ptr = nullptr;
        }

        RefMut& operator=(RefMut&& other) noexcept {
            if (this != &other) {
                if (count_ptr) {
                    *count_ptr = 0;
                }
                ptr = other.ptr;
                count_ptr = other.count_ptr;
                other.ptr = nullptr;
                other.count_ptr = nullptr;
            }
            return *this;
        }
    };

    // Destructor
    ~RefCell() noexcept(false) {
        if (borrow_count != 0) {
            throw DestructionError("RefCell destroyed while borrowed");
        }
    }
};

#ifndef ONLINE_JUDGE
int main() {
    try {
        RefCell<int> cell(42);
        {
            auto r1 = cell.borrow();
            std::cout << *r1 << std::endl;
            auto r2 = cell.borrow();
            std::cout << *r2 << std::endl;
        }
        {
            auto rm = cell.borrow_mut();
            *rm = 100;
            std::cout << *rm << std::endl;
        }
        std::cout << "Success" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "Exception: " << e.what() << std::endl;
    }
    return 0;
}
#endif
