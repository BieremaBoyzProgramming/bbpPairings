#ifndef MATCHINGDETAILTYPES_H
#define MATCHINGDETAILTYPES_H

#include <cstdint>
#include <iterator>
#include <vector>

#include <tournament/tournament.h>
#include <utility/dynamicuint.h>
#include <utility/uinttypes.h>

namespace matching
{
  namespace detail
  {
    typedef tournament::player_index vertex_index;

    /**
     * A RootBlossom has label LABEL_ZERO iff it is exposed and its base has
     * dual variable zero.
     */
    enum Label : utility::uinttypes::uint_least_for_value<4>
    {
      LABEL_OUTER, LABEL_ZERO, LABEL_INNER, LABEL_FREE
    };

    /**
     * A class acting as a vector of multiple DynamicUints that all have the
     * same size, but using only a single block of memory.
     */
    class DynamicUintVector : private std::vector<std::uintmax_t>
    {
    public:
      using std::vector<std::uintmax_t>::size_type;

      typedef utility::uinttypes::DynamicUintView<iterator> view;
      typedef utility::uinttypes::DynamicUintView<const_iterator> const_view;

      typedef view reference;

      DynamicUintVector() = default;
      /**
       * Create a DynamicUintVector containing size copies of value.
       */
      DynamicUintVector(size_type size, const const_view value)
      {
        while (size--)
        {
          push_back(value);
        }
      }

      view operator[](const size_type index) &
      {
        return
          view(
            begin() + index * elementSize,
            begin() + (index + 1u) * elementSize);
      }
      const_view operator[](const size_type index) const &
      {
        return
          const_view(
            begin() + index * elementSize,
            begin() + (index + 1u) * elementSize);
      }

      template <typename Iterator>
      void push_back(const utility::uinttypes::DynamicUintView<Iterator> view) &
      {
        for (
          auto iterator = view.begin();
          iterator != view.end();
          ++iterator)
        {
          std::vector<std::uintmax_t>::push_back(*iterator);
        }
        if (elementSize)
        {
          assert(!(size() % elementSize));
        }
        else
        {
          elementSize = size();
        }
      }
      void push_back(const utility::uinttypes::DynamicUint &value) &
      {
        push_back(utility::uinttypes::DynamicUint::const_view{ value });
      }

    private:
      std::vector<std::uintmax_t>::size_type elementSize{ };
    };

    namespace
    {
      /**
       * Behavior that can be customized for specific edge_weight types.
       */
      template <typename edge_weight>
      struct edge_weight_traits
      {
        /**
         * A vector of edge_weights.
         */
        typedef std::vector<edge_weight> vector;
        /**
         * Add addend1 to addend0, and subtract subtrahend.
         */
        static void addSubtract(
          edge_weight &addend0,
          const edge_weight addend1,
          const edge_weight subtrahend)
        {
          addend0 += addend1;
          addend0 -= subtrahend;
        }
      };
      template <>
      struct edge_weight_traits<utility::uinttypes::DynamicUint>
      {
        typedef DynamicUintVector vector;
        template <typename Iterator0, typename Iterator1, typename Iterator2>
        static void addSubtract(
          utility::uinttypes::DynamicUintView<Iterator0> addend0,
          utility::uinttypes::DynamicUintView<Iterator1> addend1,
          utility::uinttypes::DynamicUintView<Iterator2> subtrahend)
        {
          addend0.addSubtract(addend1, subtrahend);
        }
        template <typename Iterator0, typename Iterator1>
        static void addSubtract(
          utility::uinttypes::DynamicUint& addend0,
          utility::uinttypes::DynamicUintView<Iterator0> addend1,
          utility::uinttypes::DynamicUintView<Iterator1> subtrahend)
        {
          addSubtract(
            utility::uinttypes::DynamicUint::view(addend0),
            addend1,
            subtrahend);
        }
      };
    }
  }
}

#endif
