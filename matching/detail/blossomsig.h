#ifndef BLOSSOMSIG_H
#define BLOSSOMSIG_H

namespace matching
{
  namespace detail
  {
    template <typename>
    class ParentBlossom;
    template <typename>
    class RootBlossom;
    template <typename>
    struct Vertex;

    /**
     * An abstract parent class for a blossom or subblossom.
     */
    template <typename edge_weight>
    struct Blossom
    {
      RootBlossom<edge_weight> *rootBlossom;
      ParentBlossom<edge_weight> *parentBlossom{ };
      /**
       * The head of a linked list of the vertices contained in this blossom.
       * The nextVertex field of Vertex forms the links.
       */
      Vertex<edge_weight> *vertexListHead;
      /**
       * The tail of the linked list of the vertices contained in this blossom.
       */
      Vertex<edge_weight> *vertexListTail;
      /**
       * The vertex in this subblossom which was used to link this subblossom to
       * the next child subblossom of their mutual parent (sub)blossom. If this
       * is not a subblossom, the value is unspecified.
       */
      Vertex<edge_weight> *vertexToPreviousSiblingBlossom;
      /**
       * The vertex in this subblossom which was used to link this subblossom to
       * the previous child subblossom of their mutual parent (sub)blossom. If
       * this is not a subblossom, the value is unspecified.
       */
      Vertex<edge_weight> *vertexToNextSiblingBlossom;
      /**
       * The next child of parentBlossom. If this is not a subblossom, the value
       * is unspecified.
       */
      Blossom<edge_weight> *nextBlossom;
      /**
       * The previous child of parentBlossom. If this is not a subblossom, the
       * value is unspecified.
       */
      Blossom<edge_weight> *previousBlossom;
      const bool isVertex;

      Blossom(Blossom<edge_weight> &&) noexcept;
      Blossom(
        RootBlossom<edge_weight> &,
        Vertex<edge_weight> &,
        Vertex<edge_weight> &,
        bool isVertex);

      virtual ~Blossom() = 0;
    };

    template <typename edge_weight>
    Blossom<edge_weight> &getAncestorOfVertex(
      Vertex<edge_weight> &,
      const ParentBlossom<edge_weight> *);
    template <typename edge_weight>
    void setPointersFromAncestor(
      Vertex<edge_weight> &,
      const Blossom<edge_weight> &,
      bool);
  }
}

#endif
