#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <iterator>
#include <memory>
#include <string>
#include <utility>

namespace utility
{
  /**
   * A iterator for tokenizing a string based on the space character (non-locale
   * specific).
   */
  template <class T>
  class Tokenizer
  {
  public:
    typedef void difference_type;
    typedef T value_type;
    typedef std::unique_ptr<T> pointer;
    typedef T reference;
    typedef std::forward_iterator_tag iterator_category;

    constexpr Tokenizer() noexcept : start(T::npos) { }
    Tokenizer(const T &string_, T &&delimiters_)
      : string(&string_),
        delimiters(std::move(delimiters_)),
        start(0),
        end(0)
    {
      ++*this;
    }

    bool operator==(const Tokenizer &that) const
    {
      return start == that.start;
    }
    bool operator!=(const Tokenizer &that) const
    {
      return start != that.start;
    }

    reference operator*() const
    {
      return string->substr(start, end - start);
    }
    pointer operator->() const
    {
      return new T(&(*string)[start], &(*string)[end]);
    }

    Tokenizer<T> &operator++() &
    {
      start = string->find_first_not_of(delimiters, end);
      end = string->find_first_of(delimiters, start);
      return *this;
    }
    Tokenizer<T> operator++() &&
    {
      ++*this;
      return std::move(*this);
    }
    Tokenizer<T> operator++(int) &
    {
      const Tokenizer<T> result = *this;
      ++*this;
      return result;
    }

  private:
    const T *string;
    const T delimiters;
    typename T::size_type start;
    typename T::size_type end;

    Tokenizer(const T &&, T &&);
  };
}

#endif
