#ifndef ROOTBLOSSOMSIG_H
#define ROOTBLOSSOMSIG_H

#include <vector>

#include "types.h"

namespace matching
{
  namespace detail
  {
    template <typename>
    struct Blossom;
    template <typename>
    class Graph;
    template <typename>
    class ParentBlossom;
    template <typename>
    struct Vertex;

    /**
     * A class containing the extra info associated with a top-level blossom.
     */
    template <typename edge_weight>
    class RootBlossom
    {
    public:
      /**
       * If label is OUTER, this vector is used to determine the minimum edge
       * between a pair of OUTER RootBlossoms. For each other OUTER RootBlossom,
       * this vector holds a pointer to the Vertex in this RootBlossom that is
       * part of the minimum edge between them. The position in the vector is
       * indexed by the other RootBlossom's baseVertex's vertexIndex.
       *
       * Only valid during the augmentation step.
       */
      std::vector<Vertex<edge_weight> *> minOuterEdges;
      /**
       * If label is OUTER, this is the minimum resistance between this
       * RootBlossom and another OUTER RootBlossom, unless there are none, in
       * which case it is graph.maxEdgeWeight * 2u + 1u.
       *
       * Only valid during the augmentation step.
       */
      typename edge_weight_traits<edge_weight>::vector::reference
        minOuterEdgeResistance;
      Blossom<edge_weight> &rootChild;
      Vertex<edge_weight> *baseVertex;
      /**
       * The Vertex to which baseVertex is currently matched, or 0 if it is not.
       */
      Vertex<edge_weight> *baseVertexMatch{ };
      /**
       * Only valid during the augmentation step.
       */
      Label label;
      /**
       * If label is INNER, this is the vertex in another blossom that was used
       * to label this blossom.
       *
       * Only valid during the augmentation step.
       */
      Vertex<edge_weight> *labelingVertex;
      /**
       * If label is INNER, this is the vertex in this blossom that was used
       * to label this blossom.
       *
       * Only valid during the augmentation step.
       */
      Vertex<edge_weight> *labeledVertex;

      RootBlossom(RootBlossom<edge_weight> &) = delete;
      RootBlossom(RootBlossom<edge_weight> &&) = delete;
      RootBlossom(Vertex<edge_weight> &, vertex_index, Graph<edge_weight> &);
      template <class PathIterator>
      RootBlossom(PathIterator, PathIterator, Graph<edge_weight> &);
      RootBlossom(
        Blossom<edge_weight> &,
        Vertex<edge_weight> &,
        Vertex<edge_weight> *,
        Graph<edge_weight> &);
      RootBlossom(
        Blossom<edge_weight> &,
        Vertex<edge_weight> &,
        Vertex<edge_weight> *,
        Label,
        Vertex<edge_weight> *,
        Vertex<edge_weight> *,
        Graph<edge_weight> &);

      void putVerticesInMatchingOrder() const &;

      void freeAncestorOfBase(Blossom<edge_weight> &, Graph<edge_weight> &)
        &;
      void prepareVertexForWeightAdjustments(
        Vertex<edge_weight> &,
        Graph<edge_weight> &
      ) &;

    private:
      RootBlossom(
        ParentBlossom<edge_weight> &,
        const std::vector<RootBlossom<edge_weight> *> &,
        Graph<edge_weight> &,
        const RootBlossom<edge_weight> &);

      void updateRootBlossomInDescendants(RootBlossom<edge_weight> &) &;

      void initializeFromChildren(
        const std::vector<RootBlossom<edge_weight> *> &,
        Graph<edge_weight> &) &;
    };

    template <typename edge_weight>
    void augmentToSource(Vertex<edge_weight> *, Vertex<edge_weight> *);
  }
}

#endif
