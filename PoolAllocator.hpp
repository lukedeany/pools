/*
 * PoolAllocator.h
 * Purpose: A simple pool allocator implementation that allows for one type of object to be stored.
 *
 * Copyright (c) 2026 Luke Deany
 *
 * This source code is licensed under the MIT license found in the
 * LICENSE file in the root directory of this source tree.
 */


#ifndef ALLOCATOR_POOLALLOCATOR_HPP
#define ALLOCATOR_POOLALLOCATOR_HPP

#include <cstddef>
#include <new>
#include <utility>

template<typename T>
class PoolAllocator {
public:
    explicit PoolAllocator(const std::size_t size) : m_chunk_count{size}, m_pool{nullptr}, m_free_list_node{nullptr} {

        // Empty pool, do nothing
        if (m_chunk_count == 0) {
            return;
        }

        m_pool = static_cast<Chunk *>(::operator new(m_chunk_count * sizeof(Chunk), std::align_val_t{alignof(Chunk)}));
        m_free_list_node = m_pool;

        // Set pool's first element to the correct value
        Chunk *pool_ptr = m_pool;
        pool_ptr->next = nullptr;
        ++pool_ptr;

        // Now loop through the remaining and set up the free list
        for (std::size_t i = 1; i < m_chunk_count; i++) {
            pool_ptr->next = m_free_list_node;
            m_free_list_node = pool_ptr;

            ++pool_ptr;
        }
    }

    // Returns a pointer to the allocated memory, or nullptr if it was unable to be created (no capacity left)
    [[nodiscard]]
    T *allocate() noexcept {
        if (m_free_list_node == nullptr) {
            // No free node available!
            return nullptr;
        }

        // Now we know we have a free node
        // Get the address of our chunk
        Chunk *chunk_ptr = m_free_list_node;

        // We know this is a pointer to a free chunk, so just move our free list pointer!
        m_free_list_node = chunk_ptr->next;

        // Finally we wanna return the value we found
        return &chunk_ptr->value;
    }

    void deallocate(T *template_ptr) noexcept {

        // We need to convert back to a chunk pointer so we can set our next
        Chunk *chunk_ptr{static_cast<Chunk *>(static_cast<void *>(template_ptr))};

        // We simply set our chunk_ptr to a pointer to our m_free_list_node
        chunk_ptr->next = m_free_list_node;

        // And then update our free list node to now point to our chunk_ptr
        m_free_list_node = chunk_ptr;
    }

    template<typename... Args>
    [[nodiscard]]
    T *construct(Args &&...args) {
        T *chunk_ptr = allocate();

        if (chunk_ptr == nullptr) {
            return nullptr;
        }

        T *holder;

        try {
            holder = new (chunk_ptr) T(std::forward<Args>(args)...);
        } catch (...) {
            // If the constructor for T fails we want to not have the memory still allocated
            deallocate(chunk_ptr);

            // Empty throw rethrows the same exception
            throw;
        }

        return holder;
    }

    void destroy(T *template_ptr) noexcept {
        template_ptr->~T();

        deallocate(template_ptr);
    }

    ~PoolAllocator() { deletePool(); }

    // We should not copy as we hold ownership of the block of data
    // Deep copying would result in a bunch of already allocated chunks existing but nothing with the pointer to them
    PoolAllocator(const PoolAllocator &other) = delete;
    PoolAllocator &operator=(const PoolAllocator &other) = delete;

    PoolAllocator(PoolAllocator &&other) noexcept :
        m_chunk_count{other.m_chunk_count}, m_pool{other.m_pool}, m_free_list_node{other.m_free_list_node} {
        other.m_pool = nullptr;
        other.m_free_list_node = nullptr;
        other.m_chunk_count = 0;
    }

    PoolAllocator &operator=(PoolAllocator &&other) noexcept {
        if (&other == this) {
            return *this;
        }

        // Clear our data
        deletePool();

        // Now set all our data up!
        m_pool = other.m_pool;
        m_free_list_node = other.m_free_list_node;
        m_chunk_count = other.m_chunk_count;

        other.m_pool = nullptr;
        other.m_free_list_node = nullptr;
        other.m_chunk_count = 0;

        return *this;
    }

private:
    // Create a union value so we can embed the free list in our empty slots
    union Chunk {
        T value;
        Chunk *next;
    };

    std::size_t m_chunk_count;

    Chunk *m_pool;
    Chunk *m_free_list_node;


    // Does not clean up all objects inside of pool
    // This means that if you delete a pool allocator make sure everything is cleaned up first!
    void deletePool() {
        // If m_pool is null, our data has been moved so we don't have to worry about it anymore!
        if (m_pool != nullptr) {
            ::operator delete(m_pool, m_chunk_count * sizeof(Chunk), std::align_val_t{alignof(Chunk)});
            m_pool = nullptr;
            m_free_list_node = nullptr;
        }
    }
};

#endif // ALLOCATOR_POOLALLOCATOR_HPP
