#ifndef MEMORY_H
#define MEMORY_H

#include <cstdint>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace utility
{
  namespace memory
  {
    template <class>
    class IterablePoolIterator;

    /**
     * A class that allocates a fixed-size chunk of memory on the heap for
     * storing elements of type T. An iterator over the currently-allocated
     * (non-hidden) elements is provided.
     */
    template <class T>
    class IterablePool
    {
    private:
      typedef std::allocator<T> allocator;
      typedef std::allocator_traits<allocator> allocator_traits;

    public:
      typedef typename allocator_traits::size_type size_type;

      explicit IterablePool(size_type);

      ~IterablePool() noexcept(false);

      IterablePoolIterator<T> begin() const &;
      IterablePoolIterator<T> end() const &;

      size_type getIndex(const T &value) const;

      template <typename... Targs>
      T &construct(Targs &&...) &;

      void hide(T &) &;
      void unhide(T &) &;

      void destroy(T &) &;

    private:
      /**
       * A vector containing for each memory slot a pointer to the next slot in
       * the linked list containing the element. This can either be the linked
       * list of allocated slots or the linked list of unallocated slots.
       */
      std::vector<T *> forwardLinks;
      /**
       * A vector containing for each allocated memory slot a pointer to the
       * previous slot in the linked list of allocated elements.
       */
      std::vector<T *> backwardLinks;
      /**
       * The beginning of the allocated memory.
       */
      T &storage;
      /**
       * The head of the linked list of iterable allocated elements.
       */
      T *head{ };
      /**
       * The tail of the linked list of allocated elements.
       */
      T *tail{ };
      /**
       * The head of the linked list of allocated elements.
       */
      T *allocatedHead{ };
      /**
       * The head of the linked list of unallocated elements.
       */
      T *unallocatedHead;

      template <class>
      friend class IterablePoolIterator;
    };

    /**
     * An iterator over the non-hidden allocated elements of an IterablePool.
     */
    template <class T>
    class IterablePoolIterator
    {
    public:
      typedef void difference_type;
      typedef T value_type;
      typedef T *pointer;
      typedef T &reference;
      typedef std::bidirectional_iterator_tag iterator_category;

      IterablePoolIterator() noexcept = default;
      explicit
          IterablePoolIterator(
            const IterablePool<T> &iterablePool_,
            pointer value_)
        : iterablePool(&iterablePool_), value(value_) { }

      bool operator==(const IterablePoolIterator<T> that) const
      {
        return value == that.value;
      }
      bool operator!=(const IterablePoolIterator<T> that) const
      {
        return value != that.value;
      }

      reference operator*() const
      {
        return *value;
      }
      pointer operator->() const
      {
        return value;
      }

      IterablePoolIterator<T> &operator++() &
      {
        value = iterablePool->forwardLinks[value - &iterablePool->storage];
        return *this;
      }
      IterablePoolIterator<T> operator++() &&
      {
        ++*this;
        return *this;
      }
      IterablePoolIterator<T> operator++(int) &
      {
        IterablePoolIterator<T> result = *this;
        ++*this;
        return result;
      }

      IterablePoolIterator<T> &operator--() &
      {
        value = iterablePool->backwardLinks[value - &iterablePool->storage];
        return *this;
      }
      IterablePoolIterator<T> operator--() &&
      {
        --*this;
        return *this;
      }
      IterablePoolIterator<T> operator--(int) &
      {
        IterablePoolIterator<T> result = *this;
        --*this;
        return result;
      }

    private:
      const IterablePool<T> *iterablePool;
      pointer value{ };
    };

    /**
     * Construct a new IterablePool with the specified capacity.
     */
    template <class T>
    inline IterablePool<T>::IterablePool(const size_type size)
      : forwardLinks(size),
        backwardLinks(size),
        storage(*allocator().allocate(size)),
        unallocatedHead(&storage)
    {
      if (size > std::numeric_limits<std::ptrdiff_t>::max())
      {
        throw std::length_error("");
      }
      for (size_type index = 1; index < size; ++index)
      {
        forwardLinks[index - 1u] = index + &storage;
      }
    }

    /**
     * Destroy all existing objects and deallocate the memory.
     */
    template <class T>
    inline IterablePool<T>::~IterablePool() noexcept(false)
    {
      static_assert(noexcept(std::declval<T>().~T()), "Unhandled exceptions.");
      allocator allocator;
      while (allocatedHead)
      {
        allocator_traits::destroy(allocator, allocatedHead);
        allocatedHead = forwardLinks[allocatedHead - &storage];
      }
      allocator_traits
        ::deallocate(allocator, &storage, forwardLinks.size());
    }

    template <class T>
    inline IterablePoolIterator<T> IterablePool<T>::begin() const &
    {
      return IterablePoolIterator<T>(*this, head);
    }
    template <class T>
    inline IterablePoolIterator<T> IterablePool<T>::end() const &
    {
      return IterablePoolIterator<T>(*this, nullptr);
    }

    /**
     * Return an index for the object that is unique during its lifetime.
     */
    template <class T>
    inline auto IterablePool<T>::getIndex(const T &value) const -> size_type
    {
      return &value - &storage;
    }

    /**
     * Construct a new object of type T in an unallocated memory location, and
     * add it to the linked list.
     */
    template <class T>
    template <typename... Targs>
    inline T &IterablePool<T>::construct(Targs&&... targs) &
    {
      assert(unallocatedHead);

      std::unique_ptr<T> pointer(unallocatedHead);

      unallocatedHead = forwardLinks[unallocatedHead - &storage];

      allocator allocator;
      allocator_traits::construct(
        allocator,
        pointer.get(),
        std::forward<Targs>(targs)...);

      if (tail)
      {
        forwardLinks[tail - &storage] = pointer.get();
      }
      backwardLinks[pointer.get() - &storage] = tail;

      tail = pointer.release();
      forwardLinks[tail - &storage] = nullptr;

      if (!head)
      {
        head = tail;
      }
      if (!allocatedHead)
      {
        allocatedHead = tail;
      }

      return *tail;
    }

    /**
     * Remove value from the list of iterable objects, but do not destory it.
     */
    template <class T>
    inline void IterablePool<T>::hide(T &value) &
    {
      T *&forwardLink = forwardLinks[&value - &storage];
      T *&backwardLink = backwardLinks[&value - &storage];
      if (forwardLink)
      {
        backwardLinks[forwardLink - &storage] = backwardLink;
      }
      else
      {
        tail = backwardLink;
      }
      if (backwardLink)
      {
        forwardLinks[backwardLink - &storage] = forwardLink;
      }
      else
      {
        allocatedHead = forwardLink;
      }
      if (head == &value)
      {
        head = forwardLink;
      }

      if (allocatedHead)
      {
        backwardLinks[allocatedHead - &storage] = &value;
      }
      forwardLink = allocatedHead;

      allocatedHead = &value;
      backwardLink = nullptr;

      if (!tail)
      {
        tail = allocatedHead;
      }
    }

    /**
     * Destory the object, freeing its memory for future calls to construct.
     */
    template <class T>
    inline void IterablePool<T>::destroy(T &value) &
    {
      T *&forwardLink = forwardLinks[&value - &storage];
      T *&backwardLink = backwardLinks[&value - &storage];
      if (forwardLink)
      {
        backwardLinks[forwardLink - &storage] = backwardLink;
      }
      else
      {
        tail = backwardLink;
      }
      if (backwardLink)
      {
        forwardLinks[backwardLink - &storage] = forwardLink;
      }
      else
      {
        allocatedHead = forwardLink;
      }
      if (head == &value)
      {
        head = forwardLink;
      }

      allocator allocator;
      allocator_traits::destroy(allocator, &value);

      forwardLink = unallocatedHead;
      unallocatedHead = &value;
    }
  }
}

#endif
