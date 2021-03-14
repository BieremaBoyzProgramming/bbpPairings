#ifndef DYNAMICUINT_H
#define DYNAMICUINT_H

#include <cassert>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

#include "typesizes.h"

namespace utility
{
  namespace uinttypes
  {
    class DynamicUint;

    /**
     * A class that can be used to perform arithmetic on two dynamically-sized
     * integers consisting of the same number of pieces of type std::uintmax_t,
     * accessible via iterators.
     *
     * All operations assume the operands have the same size. This includes the
     * assignment operator, which assigns the value represented by the argument
     * to the storage referenced by *this.
     */
    template <typename Iterator>
    class DynamicUintView
    {
    public:
      DynamicUintView(const Iterator beginIterator_, const Iterator endIterator_
      ) : beginIterator(beginIterator_), endIterator(endIterator_) { }

      DynamicUintView(const DynamicUintView<Iterator> &that) = default;

      template <typename ThatIterator>
      const DynamicUintView<Iterator>
        &operator=(const DynamicUintView<ThatIterator> that) const &
      {
        const auto savedEnd = end();
        Iterator iterator = begin();
        ThatIterator thatIterator = that.begin();
        while (iterator != savedEnd)
        {
          assert(thatIterator != that.end());
          *iterator = *thatIterator;
          ++iterator;
          ++thatIterator;
        }
        assert(thatIterator == that.end());
        return *this;
      }
      template <typename ThatIterator>
      DynamicUintView<Iterator>
        operator=(const DynamicUintView<ThatIterator> that) const &&
      {
        *this = that;
        return *this;
      }
      const DynamicUintView<Iterator>
        &operator=(const DynamicUintView<Iterator> that) const &
      {
        this->operator=<Iterator>(that);
        return *this;
      }
      const DynamicUintView<Iterator> &operator=(const DynamicUint &that)
        const &;
      DynamicUintView<Iterator> operator=(const DynamicUint &that) const &&
      {
        *this = that;
        return *this;
      }

      template <typename ThatIterator>
      bool operator==(const DynamicUintView<ThatIterator> that) const
      {
        Iterator currentIterator = beginIterator;
        ThatIterator currentThatIterator = that.beginIterator;
        while (currentIterator != endIterator)
        {
          assert(currentThatIterator != that.end());
          if (*currentIterator != *currentThatIterator)
          {
            return false;
          }
          ++currentIterator;
          ++currentThatIterator;
        }
        assert(currentThatIterator == that.end());
        return true;
      }
      template <typename ThatIterator>
      bool operator!=(const DynamicUintView<ThatIterator> that) const
      {
        return !(*this == that);
      }
      template <typename ThatIterator>
      bool operator<(const DynamicUintView<ThatIterator> that) const
      {
        Iterator currentIterator = endIterator;
        ThatIterator currentThatIterator = that.end();
        while (currentIterator != beginIterator)
        {
          assert(currentThatIterator != that.begin());
          --currentIterator;
          --currentThatIterator;
          if (*currentIterator != *currentThatIterator)
          {
            return *currentIterator < *currentThatIterator;
          }
        }
        assert(currentThatIterator == that.begin());
        return false;
      }
      template <typename ThatIterator>
      bool operator>(const DynamicUintView<ThatIterator> that) const
      {
        return that < *this;
      }
      template <typename ThatIterator>
      bool operator<=(const DynamicUintView<ThatIterator> that) const
      {
        return !(that < *this);
      }
      template <typename ThatIterator>
      bool operator>=(const DynamicUintView<ThatIterator> that) const
      {
        return !(*this < that);
      }

      const DynamicUintView<Iterator> &operator|=(const std::uintmax_t value)
        const &
      {
        if (value)
        {
          assert(beginIterator != endIterator);
          *beginIterator |= value;
        }
        return *this;
      }
      DynamicUintView<Iterator> operator|=(const std::uintmax_t value) const &&
      {
        *this |= value;
        return *this;
      }
      template <typename ThatIterator>
      const DynamicUintView<Iterator>
        &operator|=(const DynamicUintView<ThatIterator> that) const &
      {
        Iterator iterator = beginIterator;
        ThatIterator thatIterator = that.begin();
        while (iterator != endIterator)
        {
          *iterator |= *thatIterator;
          ++iterator;
          ++thatIterator;
        }
        return *this;
      }
      template <typename ThatIterator>
      DynamicUintView<Iterator>
        operator|=(const DynamicUintView<ThatIterator> that) const &&
      {
        *this |= that;
        return *this;
      }
      const DynamicUintView<Iterator> &operator&=(const std::uintmax_t value)
        const &
      {
        if (beginIterator != endIterator)
        {
          *beginIterator &= value;
          Iterator currentIterator = beginIterator;
          while (++currentIterator != endIterator)
          {
            *currentIterator = 0;
          }
        }
        return *this;
      }
      DynamicUintView<Iterator> operator&=(const std::uintmax_t value) const &&
      {
        *this &= value;
        return *this;
      }
      template <typename ThatIterator>
      const DynamicUintView<Iterator>
        &operator&=(const DynamicUintView<ThatIterator> that) const &
      {
        Iterator iterator = beginIterator;
        ThatIterator thatIterator = that.begin();
        while (iterator != endIterator)
        {
          *iterator &= *thatIterator;
          ++iterator;
          ++thatIterator;
        }
        return *this;
      }
      template <typename ThatIterator>
      DynamicUintView<Iterator>
        operator&=(const DynamicUintView<ThatIterator> that) const &&
      {
        *this &= that;
        return *this;
      }

      template <typename Shift>
      const DynamicUintView<Iterator> &operator<<=(Shift shift) const &
      {
        Iterator sourceIterator = endIterator;
        bool sourceIteratorIsValid = beginIterator != endIterator;
        if (sourceIteratorIsValid)
        {
          --sourceIterator;
          while (shift >= std::numeric_limits<std::uintmax_t>::digits)
          {
            if (sourceIterator == beginIterator)
            {
              sourceIteratorIsValid = false;
              break;
            }
            --sourceIterator;
            shift -= std::numeric_limits<std::uintmax_t>::digits;
          }
        }
        Iterator currentIterator = endIterator;
        while (currentIterator != beginIterator)
        {
          --currentIterator;
          if (sourceIteratorIsValid)
          {
            *currentIterator = *sourceIterator << shift;
            if (sourceIterator == beginIterator)
            {
              sourceIteratorIsValid = false;
            }
            else
            {
              --sourceIterator;
              if (shift)
              {
                *currentIterator |=
                  *sourceIterator
                    >> (std::numeric_limits<std::uintmax_t>::digits - shift);
              }
            }
          }
          else
          {
            *currentIterator = 0u;
          }
        }
        return *this;
      }
      template <typename Shift>
      DynamicUintView<Iterator> operator<<=(const Shift shift) const &&
      {
        *this <<= shift;
        return *this;
      }
      template <typename Shift>
      const DynamicUintView<Iterator> &operator>>=(Shift shift) const &
      {
        Iterator sourceIterator = beginIterator;
        while (
          shift >= std::numeric_limits<std::uintmax_t>::digits
            && sourceIterator != endIterator)
        {
          ++sourceIterator;
          shift -= std::numeric_limits<std::uintmax_t>::digits;
        }
        for (std::uintmax_t &currentValue : *this)
        {
          if (sourceIterator != endIterator)
          {
            currentValue = *sourceIterator >> shift;
            ++sourceIterator;
            if (sourceIterator != endIterator && shift)
            {
              currentValue |=
                *sourceIterator
                  << (std::numeric_limits<std::uintmax_t>::digits - shift);
            }
          }
          else
          {
            currentValue = 0u;
          }
        }
        return *this;
      }
      template <typename Shift>
      DynamicUintView<Iterator> operator>>=(const Shift shift) const &&
      {
        *this >>= shift;
        return *this;
      }

      const DynamicUintView<Iterator> &operator+=(std::uintmax_t value) const &
      {
        for (
          Iterator currentIterator = beginIterator;
          currentIterator != endIterator;
          ++currentIterator)
        {
          *currentIterator += value;
          if (*currentIterator >= value)
          {
            return *this;
          }
          value = 1;
        }
        assert(!value);
        return *this;
      }
      DynamicUintView<Iterator> operator+=(const std::uintmax_t value) const &&
      {
        *this += value;
        return *this;
      }
      template <typename ThatIterator>
      const DynamicUintView<Iterator>
        &operator+=(const DynamicUintView<ThatIterator> that) const &
      {
        const auto savedEndIterator = endIterator;
        bool carry{ };
        Iterator iterator = beginIterator;
        ThatIterator thatIterator = that.begin();
        while (iterator != savedEndIterator)
        {
          assert(thatIterator != that.end());
          const std::uintmax_t originalValue = *iterator;
          const std::uintmax_t intermediateValue = originalValue + carry;
          const std::uintmax_t result = intermediateValue + *thatIterator;
          carry =
            intermediateValue > ~*thatIterator || (carry && !~originalValue);
          *iterator = result;
          ++iterator;
          ++thatIterator;
        }
        assert(thatIterator == that.end());
        return *this;
      }
      template <typename ThatIterator>
      DynamicUintView<Iterator>
        operator+=(const DynamicUintView<ThatIterator> that) const &&
      {
        *this += that;
        return *this;
      }

      const DynamicUintView<Iterator> &operator-=(std::uintmax_t value) const &
      {
        for (
          Iterator currentIterator = beginIterator;
          currentIterator != endIterator;
          ++currentIterator)
        {
          const std::uintmax_t originalValue = *currentIterator;
          *currentIterator -= value;
          if (originalValue >= value)
          {
            break;
          }
          value = 1;
        }
        return *this;
      }
      DynamicUintView<Iterator> operator-=(const std::uintmax_t value) const &&
      {
        *this -= value;
        return *this;
      }
      template <typename ThatIterator>
      const DynamicUintView<Iterator>
        &operator-=(const DynamicUintView<ThatIterator> that) const &
      {
        const auto savedEndIterator = endIterator;
        bool carry{ };
        Iterator iterator = beginIterator;
        ThatIterator thatIterator = that.begin();
        while (iterator != savedEndIterator)
        {
          const std::uintmax_t originalValue = *iterator;
          const std::uintmax_t intermediateValue = originalValue - carry;
          const std::uintmax_t result = intermediateValue - *thatIterator;
          carry =
            (carry && !originalValue) ^ (intermediateValue < *thatIterator);
          *iterator = result;
          ++iterator;
          ++thatIterator;
        }
        return *this;
      }
      template <typename ThatIterator>
      DynamicUintView<Iterator>
        operator-=(const DynamicUintView<ThatIterator> that) const &&
      {
        *this -= that;
        return *this;
      }

      const DynamicUintView<Iterator> &operator++() const &
      {
        for (
          Iterator iterator = beginIterator;
          iterator != endIterator;
          ++iterator)
        {
          ++*iterator;
          if (*iterator)
          {
            break;
          }
        }
        return *this;
      }
      DynamicUintView<Iterator> operator++() const &&
      {
        ++*this;
        return *this;
      }

      explicit operator bool() const
      {
        for (
          Iterator iterator = beginIterator;
          iterator != endIterator;
          ++iterator)
        {
          if (*iterator)
          {
            return true;
          }
        }
        return false;
      }

      Iterator begin() const
      {
        return beginIterator;
      }
      Iterator end() const
      {
        return endIterator;
      }

      /**
       * Add addend to *this, and subtract subtrahend from it.
       */
      template <typename AddendIterator, typename SubtrahendIterator>
      const DynamicUintView<Iterator> &addSubtract(
        const DynamicUintView<AddendIterator> addend,
        const DynamicUintView<SubtrahendIterator> subtrahend
      ) const &
      {
        const auto savedEndIterator = endIterator;
        int carry{ };
        Iterator iterator = beginIterator;
        AddendIterator addendIterator = addend.begin();
        SubtrahendIterator subtrahendIterator = subtrahend.begin();
        while (iterator != savedEndIterator)
        {
          assert(addendIterator != addend.end());
          assert(subtrahendIterator != subtrahend.end());
          const std::uintmax_t originalValue = *iterator;
          const std::uintmax_t intermediateValue =
            originalValue + carry + *addendIterator;
          const std::uintmax_t subtrahendValue = *subtrahendIterator;
          carry =
            ((!~originalValue & (carry > 0))
              | (intermediateValue < *addendIterator)
            ) - (!originalValue & (carry < 0))
              - (subtrahendValue > intermediateValue);
          *iterator = intermediateValue - subtrahendValue;
          ++iterator;
          ++addendIterator;
          ++subtrahendIterator;
        }
        assert(addendIterator == addend.end());
        assert(subtrahendIterator == subtrahend.end());
        return *this;
      }
      /**
       * Add addend to *this, and subtract subtrahend from it.
       */
      template <typename AddendIterator, typename SubtrahendIterator>
      DynamicUintView<Iterator> addSubtract(
        const DynamicUintView<AddendIterator> addend,
        const DynamicUintView<SubtrahendIterator> subtrahend
      ) const &&
      {
        addSubtract(addend, subtrahend);
        return *this;
      }

    private:
      const Iterator beginIterator;
      const Iterator endIterator;
    };

    template <typename Iterator>
    inline bool operator==(
      const DynamicUintView<Iterator> value0,
      std::uintmax_t value1)
    {
      if (value0.begin() == value0.end())
      {
        return !value1;
      }
      for (
        Iterator iterator = value0.begin();
        iterator != value0.end();
        ++iterator)
      {
        if (*iterator != value1)
        {
          return false;
        }
        value1 = 0;
      }
      return true;
    }
    template <typename Iterator>
    inline bool operator==(
      std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      return value1 == value0;
    }
    template <typename Iterator>
    inline bool operator!=(
      const DynamicUintView<Iterator> value0,
      std::uintmax_t value1)
    {
      return !(value0 == value1);
    }
    template <typename Iterator>
    inline bool operator!=(
      std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      return value1 != value0;
    }
    template <typename Iterator>
    inline bool operator<(
      const DynamicUintView<Iterator> value0,
      std::uintmax_t value1)
    {
      if (value0.begin() == value0.end())
      {
        return value1;
      }
      for (
        Iterator iterator = value0.begin();
        iterator != value0.end();
        ++iterator)
      {
        if (*iterator >= value1)
        {
          return false;
        }
        value1 = 1;
      }
      return true;
    }
    template <typename Iterator>
    inline bool operator<(
      std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      for (
        Iterator iterator = value1.begin();
        iterator != value1.end();
        ++iterator)
      {
        if (*iterator > value0)
        {
          return true;
        }
        value0 = 0;
      }
      return value1.begin() != value1.end();
    }
    template <typename Iterator>
    inline bool operator>(
      const DynamicUintView<Iterator> value0,
      const std::uintmax_t value1)
    {
      return value1 < value0;
    }
    template <typename Iterator>
    inline bool operator>(
      const std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      return value1 < value0;
    }
    template <typename Iterator>
    inline bool operator<=(
      const DynamicUintView<Iterator> value0,
      const std::uintmax_t value1)
    {
      return !(value0 > value1);
    }
    template <typename Iterator>
    inline bool operator<=(
      const std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      return !(value0 > value1);
    }
    template <typename Iterator>
    inline bool operator>=(
      const DynamicUintView<Iterator> value0,
      const std::uintmax_t value1)
    {
      return !(value0 < value1);
    }
    template <typename Iterator>
    inline bool operator>=(
      const std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      return !(value0 < value1);
    }

    /**
     * A class used to store a single instance of a number compatible with the
     * DynamicUintView interface.
     */
    class DynamicUint : private std::vector<std::uintmax_t>
    {
    public:
      typedef DynamicUintView<iterator> view;
      typedef DynamicUintView<const_iterator> const_view;

      explicit DynamicUint(const std::uintmax_t value = 0)
        : std::vector<std::uintmax_t>{ value } { }
      template <typename Iterator>
      explicit DynamicUint(const DynamicUintView<Iterator> that)
        : std::vector<std::uintmax_t>(that.begin(), that.end()) { }
      DynamicUint(const DynamicUint &that) = default;
      DynamicUint(DynamicUint &&that) = default;

      template <typename Iterator>
      DynamicUint &operator=(const DynamicUintView<Iterator> that) &
      {
        view(*this) = that;
        return *this;
      }
      DynamicUint &operator=(const DynamicUint &that) &
      {
        view(*this) = const_view(that);
        return *this;
      }
      DynamicUint &operator=(DynamicUint &&that) & = default;

      DynamicUint operator-() const &
      {
        return -DynamicUint{ *this };
      }
      DynamicUint operator-() &&
      {
        bool carry{ };
        for (std::uintmax_t &value : *this)
        {
          if (carry)
          {
            value = ~value;
          }
          else
          {
            carry = value;
            value = -value;
          }
        }
        return std::move(*this);
      }

      bool operator==(const DynamicUint &that) const
      {
        return const_view(*this) == const_view(that);
      }
      bool operator!=(const DynamicUint &that) const
      {
        return const_view(*this) != const_view(that);
      }
      bool operator<(const DynamicUint &that) const
      {
        return const_view(*this) < const_view(that);
      }
      bool operator>(const DynamicUint &that) const
      {
        return const_view(*this) > const_view(that);
      }
      bool operator<=(const DynamicUint &that) const
      {
        return const_view(*this) <= const_view(that);
      }
      bool operator>=(const DynamicUint &that) const
      {
        return const_view(*this) >= const_view(that);
      }

      template <typename Shift>
      DynamicUint operator<<(const Shift shift) const &
      {
        DynamicUint result = *this;
        result <<= shift;
        return result;
      }
      template <typename Shift>
      DynamicUint operator<<(const Shift shift) &&
      {
        *this <<= shift;
        return std::move(*this);
      }
      template <typename Shift>
      DynamicUint operator>>(const Shift shift) const &
      {
        DynamicUint result = *this;
        result >>= shift;
        return result;
      }
      template <typename Shift>
      DynamicUint operator>>(const Shift shift) &&
      {
        *this >>= shift;
        return std::move(*this);
      }

      DynamicUint operator+(const DynamicUint &that) const &
      {
        DynamicUint result = *this;
        result += that;
        return result;
      }
      DynamicUint operator+(const DynamicUint &that) &&
      {
        *this += that;
        return std::move(*this);
      }
      DynamicUint operator+(DynamicUint &&that) const
      {
        that += *this;
        return std::move(that);
      }

      DynamicUint operator-(const DynamicUint &that) const &
      {
        DynamicUint result = *this;
        result -= that;
        return result;
      }
      DynamicUint operator-(const DynamicUint &that) &&
      {
        *this -= that;
        return std::move(*this);
      }
      DynamicUint operator-(DynamicUint &&that) const &
      {
        return *this + -std::move(that);
      }

      DynamicUint &operator|=(const std::uintmax_t value) &
      {
        view(*this) |= value;
        return *this;
      }
      DynamicUint &operator|=(const DynamicUint &value) &
      {
        view(*this) |= const_view(value);
        return *this;
      }
      DynamicUint &operator&=(const std::uintmax_t value) &
      {
        view(*this) &= value;
        return *this;
      }
      DynamicUint &operator&=(const DynamicUint &value) &
      {
        view(*this) &= const_view(value);
        return *this;
      }

      template <typename Shift>
      DynamicUint &operator<<=(const Shift shift) &
      {
        view(*this) <<= shift;
        return *this;
      }
      template <typename Shift>
      DynamicUint &operator>>=(const Shift shift) &
      {
        view(*this) >>= shift;
        return *this;
      }

      DynamicUint &operator+=(const std::uintmax_t value) &
      {
        view(*this) += value;
        return *this;
      }
      template <typename Iterator>
      DynamicUint &operator+=(const DynamicUintView<Iterator> that) &
      {
        view(*this) += that;
        return *this;
      }
      DynamicUint &operator+=(const DynamicUint &that) &
      {
        view(*this) += const_view(that);
        return *this;
      }

      DynamicUint &operator-=(const std::uintmax_t value) &
      {
        view(*this) -= value;
        return *this;
      }
      template <typename Iterator>
      DynamicUint &operator-=(const DynamicUintView<Iterator> that) &
      {
        view(*this) -= that;
        return *this;
      }
      DynamicUint &operator-=(const DynamicUint &that) &
      {
        view(*this) -= const_view(that);
        return *this;
      }

      DynamicUint &operator++() &
      {
        ++view(*this);
        return *this;
      }
      DynamicUint operator++(int) &
      {
        const DynamicUint result = *this;
        ++*this;
        return result;
      }

      explicit operator bool() const
      {
        return bool(const_view(*this));
      }
      operator view() &
      {
        return view(begin(), end());
      }
      operator const_view() const &
      {
        return const_view(begin(), end());
      }

      /**
       * Left-shift this number by the specified number of bits, but if the
       * result would be too large to fit, enlarge this so that it does.
       */
      template <typename Shift>
      DynamicUint &shiftGrow(const Shift shift) &
      {
        Shift extraShift = shift;
        for (
          iterator iterator = end();
          iterator != begin();
        )
        {
          --iterator;
          if (*iterator)
          {
            const unsigned int bufferBits =
              std::numeric_limits<std::uintmax_t>::digits
                - typesizes::bitsToRepresent<unsigned int>(*iterator);
            extraShift =
              bufferBits >= extraShift ? 0u : extraShift - bufferBits;
            break;
          }
          else
          {
            if (extraShift <= std::numeric_limits<std::uintmax_t>::digits)
            {
              extraShift = 0;
              break;
            }
            extraShift =
              extraShift - std::numeric_limits<std::uintmax_t>::digits;
          }
        }
        while (extraShift >= std::numeric_limits<std::uintmax_t>::digits)
        {
          push_back(0);
          extraShift -= std::numeric_limits<std::uintmax_t>::digits;
        }
        if (extraShift)
        {
          push_back(0);
        }
        *this <<= shift;
        return *this;
      }
    };

    template <typename Iterator>
    const DynamicUintView<Iterator>
      &DynamicUintView<Iterator>::operator=(const DynamicUint &value) const &
    {
      *this = DynamicUint::const_view(value);
      return *this;
    }

    template <typename Iterator>
    inline DynamicUint operator&(
      const DynamicUintView<Iterator> value0,
      const std::uintmax_t value1)
    {
      DynamicUint result{ value0 };
      result &= value1;
      return result;
    }
    template <typename Iterator>
    inline DynamicUint operator&(
      const std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      DynamicUint result{ value1 };
      result &= value0;
      return result;
    }
    template <typename Iterator>
    inline DynamicUint operator|(
      const DynamicUintView<Iterator> value0,
      const std::uintmax_t value1)
    {
      DynamicUint result{ value0 };
      result |= value1;
      return result;
    }
    template <typename Iterator>
    inline DynamicUint operator|(
      const std::uintmax_t value0,
      const DynamicUintView<Iterator> value1)
    {
      DynamicUint result{ value1 };
      result |= value0;
      return result;
    }

    inline bool
      operator==(const DynamicUint &value0, const std::uintmax_t value1)
    {
      return DynamicUint::const_view(value0) == value1;
    }
    inline bool
      operator==(const std::uintmax_t value0, const DynamicUint &value1)
    {
      return DynamicUint::const_view(value1) == value0;
    }
    inline bool
      operator!=(const DynamicUint &value0, const std::uintmax_t value1)
    {
      return DynamicUint::const_view(value0) != value1;
    }
    inline bool
      operator!=(const std::uintmax_t value0, const DynamicUint &value1)
    {
      return DynamicUint::const_view(value1) != value0;
    }
    inline bool
      operator<(const DynamicUint &value0, const std::uintmax_t value1)
    {
      return DynamicUint::const_view(value0) < value1;
    }
    inline bool
      operator<(const std::uintmax_t value0, const DynamicUint &value1)
    {
      return DynamicUint::const_view(value1) > value0;
    }
    inline bool
      operator>(const DynamicUint &value0, const std::uintmax_t value1)
    {
      return DynamicUint::const_view(value0) > value1;
    }
    inline bool
      operator>(const std::uintmax_t value0, const DynamicUint &value1)
    {
      return DynamicUint::const_view(value1) < value0;
    }
    inline bool
      operator<=(const DynamicUint &value0, const std::uintmax_t value1)
    {
      return DynamicUint::const_view(value0) <= value1;
    }
    inline bool
      operator<=(const std::uintmax_t value0, const DynamicUint &value1)
    {
      return DynamicUint::const_view(value1) >= value0;
    }
    inline bool
      operator>=(const DynamicUint &value0, const std::uintmax_t value1)
    {
      return DynamicUint::const_view(value0) >= value1;
    }
    inline bool
      operator>=(const std::uintmax_t value0, const DynamicUint &value1)
    {
      return DynamicUint::const_view(value1) <= value0;
    }

    template <typename Iterator>
    inline bool operator==(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      return DynamicUint::const_view(value0) == value1;
    }
    template <typename Iterator>
    inline bool operator==(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      return value0 == DynamicUint::const_view(value1);
    }
    template <typename Iterator>
    inline bool operator!=(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      return DynamicUint::const_view(value0) != value1;
    }
    template <typename Iterator>
    inline bool operator!=(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      return value0 != DynamicUint::const_view(value1);
    }
    template <typename Iterator>
    inline bool operator<(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      return DynamicUint::const_view(value0) < value1;
    }
    template <typename Iterator>
    inline bool operator<(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      return value0 < DynamicUint::const_view(value1);
    }
    template <typename Iterator>
    inline bool operator>(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      return DynamicUint::const_view(value0) > value1;
    }
    template <typename Iterator>
    inline bool operator>(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      return value0 > DynamicUint::const_view(value1);
    }
    template <typename Iterator>
    inline bool operator<=(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      return DynamicUint::const_view(value0) <= value1;
    }
    template <typename Iterator>
    inline bool operator<=(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      return value0 <= DynamicUint::const_view(value1);
    }
    template <typename Iterator>
    inline bool operator>=(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      return DynamicUint::const_view(value0) >= value1;
    }
    template <typename Iterator>
    inline bool operator>=(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      return value0 >= DynamicUint::const_view(value1);
    }

    inline DynamicUint
      operator|(const DynamicUint &value0, const std::uintmax_t value1)
    {
      DynamicUint result = value0;
      result |= value1;
      return result;
    }
    inline DynamicUint
      operator|(const std::uintmax_t value0, const DynamicUint &value1)
    {
      DynamicUint result = value1;
      result |= value0;
      return result;
    }
    inline DynamicUint
      operator|(DynamicUint &&value0, const std::uintmax_t value1)
    {
      value0 |= value1;
      return std::move(value0);
    }
    inline DynamicUint
      operator|(const std::uintmax_t value0, DynamicUint &&value1)
    {
      value1 |= value0;
      return std::move(value1);
    }
    inline DynamicUint
      operator&(const DynamicUint &value0, const std::uintmax_t value1)
    {
      DynamicUint result = value0;
      result &= value1;
      return result;
    }
    inline DynamicUint
      operator&(const std::uintmax_t value0, const DynamicUint &value1)
    {
      DynamicUint result = value1;
      result &= value0;
      return result;
    }
    inline DynamicUint
      operator&(DynamicUint &&value0, const std::uintmax_t value1)
    {
      value0 &= value1;
      return std::move(value0);
    }
    inline DynamicUint
      operator&(const std::uintmax_t value0, DynamicUint &&value1)
    {
      value1 &= value0;
      return std::move(value1);
    }

    inline DynamicUint
      operator+(const DynamicUint &value0, const std::uintmax_t value1)
    {
      DynamicUint result = value0;
      result += value1;
      return result;
    }
    inline DynamicUint
      operator+(const std::uintmax_t value0, const DynamicUint &value1)
    {
      DynamicUint result = value1;
      result += value0;
      return result;
    }
    inline DynamicUint
      operator+(DynamicUint &&value0, const std::uintmax_t value1)
    {
      value0 += value1;
      return std::move(value0);
    }
    inline DynamicUint
      operator+(const std::uintmax_t value0, DynamicUint &&value1)
    {
      value1 += value0;
      return std::move(value1);
    }
    inline DynamicUint
      operator-(const DynamicUint &value0, const std::uintmax_t value1)
    {
      DynamicUint result = value0;
      result -= value1;
      return result;
    }
    inline DynamicUint
      operator-(const std::uintmax_t value0, const DynamicUint &value1)
    {
      return value0 + -value1;
    }
    inline DynamicUint
      operator-(DynamicUint &&value0, const std::uintmax_t value1)
    {
      value0 -= value1;
      return std::move(value0);
    }
    inline DynamicUint
      operator-(const std::uintmax_t value0, DynamicUint &&value1)
    {
      return value0 + -std::move(value1);
    }

    template <typename Iterator>
    inline DynamicUint operator+(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      DynamicUint result = value0;
      result += value1;
      return result;
    }
    template <typename Iterator>
    inline DynamicUint operator+(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      DynamicUint result = value1;
      result += value0;
      return result;
    }
    template <typename Iterator>
    inline DynamicUint operator+(
      DynamicUint &&value0,
      const DynamicUintView<Iterator> value1)
    {
      value0 += value1;
      return std::move(value0);
    }
    template <typename Iterator>
    inline DynamicUint operator+(
      const DynamicUintView<Iterator> value0,
      DynamicUint &&value1)
    {
      value1 += value0;
      return std::move(value1);
    }

    template <typename Iterator>
    inline DynamicUint operator-(
      const DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      DynamicUint result = value0;
      result -= value1;
      return result;
    }
    template <typename Iterator>
    inline DynamicUint operator-(
      const DynamicUintView<Iterator> value0,
      const DynamicUint &value1)
    {
      DynamicUint result = value0;
      result -= value1;
      return result;
    }
    template <typename Iterator>
    inline DynamicUint operator-(
      DynamicUint &&value0,
      const DynamicUintView<Iterator> value1)
    {
      value0 -= value1;
      return std::move(value0);
    }
    template <typename Iterator>
    inline DynamicUint operator-(
      const DynamicUintView<Iterator> value0,
      DynamicUint &&value1)
    {
      return value0 + -std::move(value1);
    }

    template <typename Iterator>
    inline DynamicUint &operator+=(
      DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      DynamicUint::view(value0) += value1;
      return value0;
    }
    template <typename Iterator>
    inline DynamicUintView<Iterator> &operator+=(
      DynamicUintView<Iterator> &value0,
      const DynamicUint &value1)
    {
      value0 += DynamicUint::const_view(value1);
      return value0;
    }

    template <typename Iterator>
    inline DynamicUint &operator-=(
      DynamicUint &value0,
      const DynamicUintView<Iterator> value1)
    {
      DynamicUint::view(value0) -= value1;
      return value0;
    }
    template <typename Iterator>
    inline DynamicUintView<Iterator> &operator-=(
      DynamicUintView<Iterator> &value0,
      const DynamicUint &value1)
    {
      value0 -= DynamicUint::const_view(value1);
      return value0;
    }
  }
}

#endif
