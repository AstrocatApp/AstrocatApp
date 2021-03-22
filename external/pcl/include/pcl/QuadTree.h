//     ____   ______ __
//    / __ \ / ____// /
//   / /_/ // /    / /
//  / ____// /___ / /___   PixInsight Class Library
// /_/     \____//_____/   PCL 2.4.7
// ----------------------------------------------------------------------------
// pcl/QuadTree.h - Released 2020-12-17T15:46:29Z
// ----------------------------------------------------------------------------
// This file is part of the PixInsight Class Library (PCL).
// PCL is a multiplatform C++ framework for development of PixInsight modules.
//
// Copyright (c) 2003-2020 Pleiades Astrophoto S.L. All Rights Reserved.
//
// Redistribution and use in both source and binary forms, with or without
// modification, is permitted provided that the following conditions are met:
//
// 1. All redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//
// 2. All redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
//
// 3. Neither the names "PixInsight" and "Pleiades Astrophoto", nor the names
//    of their contributors, may be used to endorse or promote products derived
//    from this software without specific prior written permission. For written
//    permission, please contact info@pixinsight.com.
//
// 4. All products derived from this software, in any form whatsoever, must
//    reproduce the following acknowledgment in the end-user documentation
//    and/or other materials provided with the product:
//
//    "This product is based on software from the PixInsight project, developed
//    by Pleiades Astrophoto and its contributors (https://pixinsight.com/)."
//
//    Alternatively, if that is where third-party acknowledgments normally
//    appear, this acknowledgment must be reproduced in the product itself.
//
// THIS SOFTWARE IS PROVIDED BY PLEIADES ASTROPHOTO AND ITS CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
// TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL PLEIADES ASTROPHOTO OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, BUSINESS
// INTERRUPTION; PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; AND LOSS OF USE,
// DATA OR PROFITS) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
// ----------------------------------------------------------------------------

#ifndef __PCL_QuadTree_h
#define __PCL_QuadTree_h

/// \file pcl/QuadTree.h

#include <pcl/Defs.h>

#include <pcl/Array.h>
#include <pcl/Rectangle.h>
#include <pcl/Vector.h>

namespace pcl
{

// ----------------------------------------------------------------------------

/*!
 * \class QuadTree
 * \brief Bucket PR quadtree for two-dimensional point data.
 *
 * A quadtree is a specialized binary search tree for partitioning of a set of
 * points in two dimensions. Quadtrees have important applications in
 * computational geometry problems requiring efficient rectangular range
 * searching and nearest neighbor queries.
 *
 * This class implements a <em>bucket point region quadtree</em> structure
 * (see Reference 2).
 *
 * The template type argument T represents the type of a \e point object stored
 * in a %QuadTree structure. The type T must have the following properties:
 *
 * \li The standard default and copy constructors are required:\n
 * \n
 * T::T() \n
 * T::T( const T& )
 *
 * \li The \c T::component subtype must be defined. It represents a component
 * of an object of type T. For example, if T is a vector type, T::component
 * must be the type of a vector component.
 *
 * \li The array subscript operator must be defined as follows:\n
 * \n
 * component T::operator []( int i ) const \n
 * \n
 * This operator must return the value of the first or second component of an
 * object being stored in the quadtree. The subindex i will be either 0 or 1
 * for the first or second point component, respectively.
 *
 * \b References
 *
 * 1. Mark de Berg et al, <em>Computational Geometry: Algorithms and
 * Applications Third Edition,</em> Springer, 2010, Chapter 14.
 *
 * 2. Hanan Samet, <em>Foundations of Multidimensional and Metric Data
 * Structures,</em> Morgan Kaufmann, 2006, Section 1.4.
 *
 * \sa KDTree
 */
template <class T>
class QuadTree
{
public:

   /*!
    * Represents a two-dimensional point stored in this quadtree.
    */
   typedef T                           point;

   /*!
    * Represents a point component.
    */
   typedef typename point::component   component;

   /*!
    * A list of points. Used for tree build and search operations.
    */
   typedef Array<point>                point_list;

   /*!
    * A rectangular region. Used for rectangular range search operations.
    */
   typedef DRect                       rectangle;

   /*!
    * The type of rectangular region coordinates.
    */
   typedef rectangle::component        coordinate;

   // -------------------------------------------------------------------------

   /*!
    * \class pcl::QuadTree::Node
    * \brief Quadtree node structure
    */
   struct Node
   {
      rectangle rect = 0.0;   //!< The rectangular region represented by this node.
      Node*     nw = nullptr; //!< North-West child node, representing the top-left subregion.
      Node*     ne = nullptr; //!< North-East child node, representing the top-right subregion.
      Node*     sw = nullptr; //!< South-West child node, representing the bottom-left subregion.
      Node*     se = nullptr; //!< South-East child node, representing the bottom-right subregion.

#ifdef __PCL_QUADTREE_STRUCTURAL_NODE_HAS_DATA
      void* data = nullptr;
#endif

      /*!
       * Default constructor. Constructs an uninitialized quadtree node.
       */
      Node() = default;

      /*!
       * Constructs a quadtree node for the specified rectangular region \a r.
       */
      Node( const rectangle& r )
         : rect( r )
      {
      }

      /*!
       * Returns true iff this is a leaf quadtree node. A leaf node does not
       * contain child nodes, that is, there is no further subdivision of the
       * domain space beyond a leaf quadtree node.
       *
       * In a healthy quadtree (as any QuadTree structure should be under
       * normal working conditions), you can expect any leaf node to be an
       * instance of QuadTree::LeafNode containing a nonempty list of points.
       */
      bool IsLeaf() const
      {
         return nw == nullptr && ne == nullptr && sw == nullptr && se == nullptr;
      }

      /*!
       * Returns true iff the rectangular region represented by this node
       * intersects the specified rectangle \a r.
       */
      bool Intersects( const rectangle& r ) const
      {
         return rect.x1 >= r.x0 && rect.x0 <= r.x1 &&
                rect.y1 >= r.y0 && rect.y0 <= r.y1;
      }

      /*!
       * Returns true iff the rectangular region represented by this node
       * includes a point in the plane specified by its coordinates \a x, \a y.
       */
      bool Includes( coordinate x, coordinate y ) const
      {
         return x >= rect.x0 && x <= rect.x1 &&
                y >= rect.y0 && y <= rect.y1;
      }

      /*!
       * Returns true iff the rectangular region represented by this node
       * includes the specified point \a p in the plane.
       */
      bool Includes( const rectangle::point& p ) const
      {
         return Includes( p.x, p.y );
      }

      /*!
       * Returns the Northwest (top left) splitting rectangle for this node.
       */
      rectangle NWRect() const
      {
         return rectangle( rect.TopLeft(), rect.Center() );
      }

      /*!
       * Returns the Northeast (top right) splitting rectangle for this node.
       */
      rectangle NERect() const
      {
         return rectangle( rect.CenterTop(), rect.CenterRight() );
      }

      /*!
       * Returns the Southwest (bottom left) splitting rectangle for this node.
       */
      rectangle SWRect() const
      {
         return rectangle( rect.CenterLeft(), rect.CenterBottom() );
      }

      /*!
       * Returns the Southeast (bottom right) splitting rectangle for this
       * node.
       */
      rectangle SERect() const
      {
         return rectangle( rect.Center(), rect.BottomRight() );
      }
   };

   // -------------------------------------------------------------------------

   /*!
    * \class pcl::QuadTree::LeafNode
    * \brief Quadtree leaf node structure
    */
   struct LeafNode : public Node
   {
      /*!
       * The list of points contained by this leaf node.
       *
       * In a healthy quadtree (as any QuadTree structure should be under
       * normal working conditions), every existing leaf node should contain a
       * nonempty point list.
       */
      point_list points;

#ifndef __PCL_QUADTREE_STRUCTURAL_NODE_HAS_DATA

      /*!
       * Pointer to an arbitrary data structure that can be associated with
       * this leaf node. Its default value is \c nullptr.
       *
       * The quadtree structure does not access this pointer in any way.
       * Destruction of the object pointed to by this member, if required, is
       * the sole responsibility of the external code that has defined it.
       */
      void* data = nullptr;

#endif

      /*!
       * Constructs a new leaf node representing the specified rectangular
       * region \a r and storing a nonempty point list \a p.
       */
      LeafNode( const rectangle& r, const point_list& p )
         : Node( r )
         , points( p )
      {
         PCL_CHECK( !points.IsEmpty() )
      }

      /*!
       * Constructs a new leaf node representing the specified rectangular
       * region \a r and storing the specified point \a p.
       */
      LeafNode( const rectangle& r, const point& p )
         : Node( r )
      {
         points << p;
      }

      /*!
       * Returns the number of points contained by this leaf node. Under normal
       * conditions, the returned value must be > 0.
       */
      int Length() const
      {
         return int( points.Length() );
      }
   };

   // -------------------------------------------------------------------------

   /*!
    * Constructs an empty quadtree.
    */
   QuadTree() = default;

   /*!
    * Constructs a quadtree and builds it for the specified list of \a points.
    *
    * \param points           A list of points that will be stored in this
    *                         quadtree.
    *
    * \param bucketCapacity   The maximum number of points in a leaf tree node.
    *                         Must be >= 1. The default value is 40.
    *
    * If the specified list of \a points is empty, this constructor yields an
    * empty quadtree.
    */
   QuadTree( const point_list& points, int bucketCapacity = 40 )
   {
      Build( points, bucketCapacity );
   }

   /*!
    * Constructs a quadtree and builds it for the specified list of \a points
    * and a prescribed rectangular search region.
    *
    * \param rect             The rectangular search region.
    *
    * \param points           A list of points that will be stored in this
    *                         quadtree.
    *
    * \param bucketCapacity   The maximum number of points in a leaf tree node.
    *                         Must be >= 1. The default value is 40.
    *
    * If the specified list of \a points is empty, or if no points lie within
    * the \a rect region, this constructor yields an empty quadtree.
    */
   QuadTree( const rectangle& rect, const point_list& points, int bucketCapacity = 40 )
   {
      Build( rect, points, bucketCapacity );
   }

   /*!
    * Copy constructor. Copy construction is disabled because this class uses
    * internal data structures that cannot be copy-constructed. However,
    * %QuadTree implements move construction and move assignment.
    */
   QuadTree( const QuadTree& ) = delete;

   /*!
    * Copy assignment operator. Copy assignment is disabled because this class
    * uses internal data structures that cannot be copy-assigned. However,
    * %QuadTree implements move assignment and move construction.
    */
   QuadTree& operator =( const QuadTree& ) = delete;

   /*!
    * Move constructor.
    */
   QuadTree( QuadTree&& x )
      : m_root( x.m_root )
      , m_bucketCapacity( x.m_bucketCapacity )
      , m_length( x.m_length )
   {
      x.m_root = nullptr;
      x.m_length = 0;
   }

   /*!
    * Move assignment operator. Returns a reference to this object.
    */
   QuadTree& operator =( QuadTree&& x )
   {
      if ( &x != this )
      {
         DestroyTree( m_root );
         m_root = x.m_root;
         m_bucketCapacity = x.m_bucketCapacity;
         m_length = x.m_length;
         x.m_root = nullptr;
         x.m_length = 0;
      }
      return *this;
   }

   /*!
    * Destroys a quadtree. All the stored point objects are deleted.
    */
   ~QuadTree()
   {
      Clear();
   }

   /*!
    * Removes all the stored point objects, yielding an empty quadtree.
    */
   void Clear()
   {
      DestroyTree( m_root );
      m_root = nullptr;
      m_length = 0;
   }

   /*!
    * Builds a new quadtree for the specified list of \a points.
    *
    * \param points           A list of points that will be stored in this
    *                         quadtree.
    *
    * \param bucketCapacity   The maximum number of points in a leaf tree node.
    *                         Must be >= 1. The default value is 40.
    *
    * If the tree stores point objects before calling this function, they are
    * destroyed and removed before building a new tree.
    *
    * If the specified list of \a points is empty, this member function yields
    * an empty quadtree.
    */
   void Build( const point_list& points, int bucketCapacity = 40 )
   {
      Clear();
      m_bucketCapacity = Max( 1, bucketCapacity );

      if ( !points.IsEmpty() )
      {
         component x = points[0][0];
         component y = points[0][1];
         rectangle rect( x, y, x, y );
         for ( const point& p : points )
         {
            x = p[0];
            y = p[1];
            if ( x < rect.x0 )
               rect.x0 = x;
            if ( y < rect.y0 )
               rect.y0 = y;
            if ( x > rect.x1 )
               rect.x1 = x;
            if ( y > rect.y1 )
               rect.y1 = y;
         }
         m_root = BuildTree( rect, points );
      }
   }

   /*!
    * Builds a new quadtree with the specified list of \a points and a
    * prescribed rectangular search region.
    *
    * \param rect             The rectangular search region.
    *
    * \param points           A list of points to be stored in this quadtree.
    *                         Only points included in the specified \a rect
    *                         search region will be inserted in the tree.
    *                         All points outside \a rect will be ignored.
    *
    * \param bucketCapacity   The maximum number of points in a leaf tree node.
    *                         Must be >= 1. The default value is 40.
    *
    * If the tree stores point objects before calling this function, they are
    * destroyed and removed before building a new tree.
    *
    * If the specified list of \a points is empty, or if no points lie within
    * the \a rect region, this member function yields an empty quadtree.
    */
   void Build( const rectangle& rect, const point_list& points, int bucketCapacity = 40 )
   {
      Clear();
      m_bucketCapacity = Max( 1, bucketCapacity );
      if ( !points.IsEmpty() )
         m_root = BuildTree( rect.Ordered(), points );
   }

   /*!
    * Performs a rectangular range search in this quadtree.
    *
    * \param rect    The rectangular search region.
    *
    * Returns a (possibly empty) list with all the points found in this tree
    * within the specified search range.
    */
   point_list Search( const rectangle& rect ) const
   {
      point_list found;
      SearchTree( found, rect.Ordered(), m_root );
      return found;
   }

   /*!
    * Performs a rectangular range search in this quadtree.
    *
    * \param rect       The rectangular search region.
    *
    * \param callback   Callback functional.
    *
    * \param data       Callback data.
    *
    * The callback function prototype should be:
    *
    * \code void callback( const point& pt, void* data ) \endcode
    *
    * The callback function will be called once for each point found in the
    * tree within the specified search range.
    */
   template <class F>
   void Search( const rectangle& rect, F callback, void* data ) const
   {
      SearchTree( rect.Ordered(), callback, data, m_root );
   }

   /*!
    * Inserts a point in this quadtree.
    */
   void Insert( const point& pt )
   {
      if ( m_root != nullptr )
         InsertTree( pt, m_root );
      else
      {
         component x = pt[0];
         component y = pt[1];
         m_root = new LeafNode( rectangle( x, y, x, y ), pt );
      }
   }

   /*!
    * Deletes all points in this quadtree equal to the specified point.
    */
   void Delete( const point& pt )
   {
      DeleteTree( pt, m_root );
   }

   /*!
    * Deletes all points in this quadtree included in the specified rectangular
    * region \a rect.
    */
   void Delete( const rectangle& rect )
   {
      DeleteTree( rect.Ordered(), m_root );
   }

   /*!
    * Returns the bucket capacity of this quadtree, or the maximum number of
    * points that can be stored in a leaf tree node. This parameter is
    * specified when a new tree is built.
    */
   int BucketCapacity() const
   {
      return m_bucketCapacity;
   }

   /*!
    * Returns the total number of points stored in this quadtree.
    */
   size_type Length() const
   {
      return m_length;
   }

   /*!
    * Returns true iff this quadtree is empty.
    */
   bool IsEmpty() const
   {
      return m_root == nullptr;
   }

   /*!
    * Returns a pointer to the root node of this quadtree, or nullptr if this
    * quadtree is empty.
    *
    * The returned pointer is const qualified to forbid uncontrolled
    * alterations that might invalidate the quadtree structure.
    */
   const Node* Root() const
   {
      return m_root;
   }

   /*!
    * Returns a pointer to the (mutable) root node of this quadtree, or nullptr
    * if this quadtree is empty.
    */
   Node* Root()
   {
      return m_root;
   }

   /*!
    * Returns a pointer to the (immutable) leaf node of this quadtree that
    * includes the specified point \a p in the plane, or nullptr if no such
    * leaf node exists in this quadtree.
    */
   const LeafNode* LeafNodeAt( rectangle::point p ) const
   {
      return SearchLeafNode( p, m_root );
   }

   /*!
    * Returns a pointer to the leaf node of this quadtree that includes the
    * specified point \a p in the plane, or nullptr if no such leaf node exists
    * in this quadtree.
    */
   LeafNode* LeafNodeAt( rectangle::point p )
   {
      return SearchLeafNode( p, m_root );
   }

   /*!
    * Returns a pointer to the (immutable) node of this quadtree that includes
    * the specified point \a p in the plane, or nullptr if no such node exists
    * in this quadtree.
    *
    * The returned node can be a leaf node or a structural node. This function
    * should only return nullptr if the specified point \a p is exterior to the
    * root rectangular region of this quadtree, or if this quadtree is empty.
    */
   const Node* NodeAt( rectangle::point p ) const
   {
      return SearchNode( p, m_root );
   }

   /*!
    * Returns a pointer to the node of this quadtree that includes the
    * specified point \a p in the plane, or nullptr if no such node exists in
    * this quadtree.
    *
    * The returned node can be a leaf node or a structural node. This function
    * should only return nullptr if the specified point \a p is exterior to the
    * root rectangular region of this quadtree, or if this quadtree is empty.
    */
   Node* NodeAt( rectangle::point p )
   {
      return SearchNode( p, m_root );
   }

   /*!
    * Forces a quadtree subdivision of the leaf node that includes the
    * specified point \a p in the plane.
    *
    * Returns the newly created structural node. This function should only
    * return nullptr if the specified point \a p is exterior to the root
    * rectangular region of this quadtree, or if this quadtree is empty. It
    * could also return nullptr in degenerate cases where no further
    * subdivision of the plane would be possible because of numerical limits.
    */
   Node* SplitAt( rectangle::point p )
   {
      Node* node = SearchDeepestStructuralNodeAt( p, m_root );
      if ( node != nullptr )
      {
         Node** leaf = nullptr;
         if ( node->nw != nullptr && node->nw->Includes( p ) )
            leaf = &node->nw;
         else if ( node->ne != nullptr && node->ne->Includes( p ) )
            leaf = &node->ne;
         else if ( node->sw != nullptr && node->sw->Includes( p ) )
            leaf = &node->sw;
         else if ( node->se != nullptr && node->se->Includes( p ) )
            leaf = &node->se;

         if ( leaf != nullptr ) // cannot be false!
         {
            Node* newNode = SplitLeafNode( *leaf );
            if ( newNode != nullptr ) // should be true
            {
               delete static_cast<LeafNode*>( *leaf );
               *leaf = newNode;
            }
            return newNode;
         }
      }

      return nullptr;
   }

   /*!
    * Performs a recursive left-to-right, depth-first traversal of the subtree
    * rooted at the specified \a node, invoking the specified function \a f
    * successively for each leaf node.
    *
    * The function \a f must be compatible with the form:
    *
    * \code void f( const rectangle& r, const point_list& p, void*& d ) \endcode
    *
    * where \a r is the plane region covered by the current leaf node, \a p is
    * the list of points in the current leaf node, and \a d is the optional
    * pointer to arbitrary data associated with the current leaf node.
    *
    * The sequence of calls for the subtrees in each non-leaf node is: NW, NE,
    * SW, SE. Only non-empty leaf nodes are included in the traversal, hence
    * the function \a f will be invoked exclusively for non-empty point lists.
    *
    * If the specified \a node is nullptr, this function takes no action.
    */
   template <class F>
   static void Traverse( const Node* node, F f )
   {
      if ( node != nullptr )
         if ( node->IsLeaf() )
            f( node->rect,
               static_cast<const LeafNode*>( node )->points,
               static_cast<const LeafNode*>( node )->data );
         else
         {
            Traverse( node->nw, f );
            Traverse( node->ne, f );
            Traverse( node->sw, f );
            Traverse( node->se, f );
         }
   }

   /*!
    * Performs a recursive left-to-right, depth-first traversal of the subtree
    * rooted at the specified (mutable) \a node, invoking the specified
    * function \a f successively for each leaf node.
    *
    * The function \a f must be compatible with the form:
    *
    * \code void f( const rectangle& r, point_list& p, void*& d ) \endcode
    *
    * where \a r is the plane region covered by the current leaf node, \a p is
    * the list of points in the current leaf node, and \a d is the optional
    * pointer to arbitrary data associated with the current leaf node.
    *
    * The sequence of calls for the subtrees in each non-leaf node is: NW, NE,
    * SW, SE. Only non-empty leaf nodes are included in the traversal, hence
    * the function \a f will be invoked exclusively for non-empty point lists.
    *
    * If the specified \a node is nullptr, this function takes no action.
    */
   template <class F>
   static void Traverse( Node* node, F f )
   {
      if ( node != nullptr )
         if ( node->IsLeaf() )
            f( node->rect, static_cast<LeafNode*>( node )->points,
                           static_cast<LeafNode*>( node )->data );
         else
         {
            Traverse( node->nw, f );
            Traverse( node->ne, f );
            Traverse( node->sw, f );
            Traverse( node->se, f );
         }
   }

   /*!
    * Performs a recursive left-to-right, depth-first traversal of the entire
    * quadtree, invoking the specified function \a f successively for each leaf
    * node. Calling this function is equivalent to:
    *
    * \code Traverse( Root(), f ) \endcode
    *
    * If this quadtree is empty, this function takes no action.
    */
   template <class F>
   void Traverse( F f ) const
   {
      Traverse( m_root, f );
   }

   /*!
    * Performs a recursive left-to-right, depth-first traversal of the entire
    * (mutable) quadtree, invoking the specified function \a f successively for
    * each leaf node. Calling this function is equivalent to:
    *
    * \code Traverse( Root(), f ) \endcode
    *
    * If this quadtree is empty, this function takes no action.
    */
   template <class F>
   void Traverse( F f )
   {
      Traverse( m_root, f );
   }

   /*!
    * Returns the total number of existing nodes in the subtree rooted at the
    * specified \a node, including structural and leaf nodes.
    */
   static size_type NumberOfNodes( const Node* node )
   {
      size_type N = 0;
      GetNumberOfNodes( N, node );
      return N;
   }

   /*!
    * Returns the total number of existing nodes in this quadtree, including
    * structural and leaf nodes.
    */
   size_type NumberOfNodes() const
   {
      return NumberOfNodes( m_root );
   }

   /*!
    * Returns the total number of existing leaf nodes in the subtree rooted at
    * the specified \a node.
    */
   static size_type NumberOfLeafNodes( const Node* node )
   {
      size_type N = 0;
      GetNumberOfLeafNodes( N, node );
      return N;
   }

   /*!
    * Returns the total number of existing leaf nodes in this quadtree.
    */
   size_type NumberOfLeafNodes() const
   {
      return NumberOfLeafNodes( m_root );
   }

   /*!
    * Returns the height of the subtree rooted at the specified \a node.
    */
   static int Height( const Node* node )
   {
      return TreeHeight( node, 0 );
   }

   /*!
    * Returns the height of this quadtree.
    */
   int Height() const
   {
      return Height( m_root );
   }

   /*!
    * Exchanges two %QuadTree objects \a x1 and \a x2.
    */
   friend void Swap( QuadTree& x1, QuadTree& x2 )
   {
      pcl::Swap( x1.m_root,           x2.m_root );
      pcl::Swap( x1.m_bucketCapacity, x2.m_bucketCapacity );
      pcl::Swap( x1.m_length,         x2.m_length );
   }

private:

   Node*     m_root = nullptr;
   int       m_bucketCapacity = 0;
   size_type m_length = 0;

   Node* NewLeafNode( const rectangle& rect, const point_list& points )
   {
      m_length += points.Length();
      return new LeafNode( rect, points );
   }

   LeafNode* SearchLeafNode( const rectangle::point& p, const Node* node ) const
   {
      if ( node != nullptr )
         if ( node->Includes( p ) )
         {
            if ( node->IsLeaf() )
               return const_cast<LeafNode*>( static_cast<const LeafNode*>( node ) );

            LeafNode* child = SearchLeafNode( p, node->nw );
            if ( child == nullptr )
            {
               child = SearchLeafNode( p, node->ne );
               if ( child == nullptr )
               {
                  child = SearchLeafNode( p, node->sw );
                  if ( child == nullptr )
                     child = SearchLeafNode( p, node->se );
               }
            }
            return child;
         }

      return nullptr;
   }

   Node* SearchNode( const rectangle::point& p, const Node* node ) const
   {
      if ( node != nullptr )
         if ( node->Includes( p ) )
         {
            if ( node->IsLeaf() )
               return const_cast<Node*>( node );

            Node* child = SearchNode( p, node->nw );
            if ( child == nullptr )
            {
               child = SearchNode( p, node->ne );
               if ( child == nullptr )
               {
                  child = SearchNode( p, node->sw );
                  if ( child == nullptr )
                  {
                     child = SearchNode( p, node->se );
                     if ( child == nullptr )
                        return const_cast<Node*>( node );
                  }
               }
            }
            return child;
         }

      return nullptr;
   }

   Node* SearchDeepestStructuralNodeAt( const rectangle::point& p, const Node* node ) const
   {
      if ( node != nullptr )
         if ( !node->IsLeaf() )
            if ( node->Includes( p ) )
            {
               Node* child = SearchDeepestStructuralNodeAt( p, node->nw );
               if ( child == nullptr )
               {
                  child = SearchDeepestStructuralNodeAt( p, node->ne );
                  if ( child == nullptr )
                  {
                     child = SearchDeepestStructuralNodeAt( p, node->sw );
                     if ( child == nullptr )
                     {
                        child = SearchDeepestStructuralNodeAt( p, node->se );
                        if ( child == nullptr )
                           return const_cast<Node*>( node );
                     }
                  }
               }
               return child;
            }

      return nullptr;
   }

   Node* BuildTree( const rectangle& rect, const point_list& points )
   {
      if ( points.IsEmpty() )
         return nullptr;

      if ( points.Length() <= size_type( m_bucketCapacity ) )
         return NewLeafNode( rect, points );

      double x2 = (rect.x0 + rect.x1)/2;
      double y2 = (rect.y0 + rect.y1)/2;
      // Prevent geometrically degenerate subtrees. For safety, we enforce
      // minimum region dimensions larger than twice the machine epsilon for
      // the rectangle coordinate type.
      if ( x2 - rect.x0 < 2*std::numeric_limits<coordinate>::epsilon() ||
           rect.x1 - x2 < 2*std::numeric_limits<coordinate>::epsilon() ||
           y2 - rect.y0 < 2*std::numeric_limits<coordinate>::epsilon() ||
           rect.y1 - y2 < 2*std::numeric_limits<coordinate>::epsilon() )
      {
         return NewLeafNode( rect, points );
      }

      point_list nw, ne, sw, se;
      for ( const point& p : points )
      {
         component x = p[0];
         component y = p[1];
         if ( x <= x2 )
         {
            if ( y <= y2 )
               nw << p;
            else
               sw << p;
         }
         else
         {
            if ( y <= y2 )
               ne << p;
            else
               se << p;
         }
      }

      Node* node = new Node( rect );
      node->nw = BuildTree( rectangle( rect.x0, rect.y0,      x2,      y2 ), nw );
      node->ne = BuildTree( rectangle(      x2, rect.y0, rect.x1,      y2 ), ne );
      node->sw = BuildTree( rectangle( rect.x0,      y2,      x2, rect.y1 ), sw );
      node->se = BuildTree( rectangle(      x2,      y2, rect.x1, rect.y1 ), se );

      // Further degeneracies may result, e.g. if the point class is not
      // behaving as expected. Do not allow them.
      if ( node->IsLeaf() )
      {
         delete node;
         return NewLeafNode( rect, points );
      }

      return node;
   }

   Node* SplitLeafNode( const Node* node )
   {
      if ( node == nullptr || !node->IsLeaf() )
         return nullptr;

      double x2 = (node->rect.x0 + node->rect.x1)/2;
      double y2 = (node->rect.y0 + node->rect.y1)/2;
      // Prevent geometrically degenerate subtrees. For safety, we enforce
      // minimum region dimensions larger than twice the machine epsilon for
      // the rectangle coordinate type.
      if ( x2 - node->rect.x0 < 2*std::numeric_limits<coordinate>::epsilon() ||
           node->rect.x1 - x2 < 2*std::numeric_limits<coordinate>::epsilon() ||
           y2 - node->rect.y0 < 2*std::numeric_limits<coordinate>::epsilon() ||
           node->rect.y1 - y2 < 2*std::numeric_limits<coordinate>::epsilon() )
      {
         return nullptr;
      }

      const LeafNode* leaf = static_cast<const LeafNode*>( node );
      point_list nw, ne, sw, se;
      for ( const point& p : leaf->points )
      {
         component x = p[0];
         component y = p[1];
         if ( x <= x2 )
         {
            if ( y <= y2 )
               nw << p;
            else
               sw << p;
         }
         else
         {
            if ( y <= y2 )
               ne << p;
            else
               se << p;
         }
      }

      size_type savedLength = m_length;
      Node* newNode = new Node( node->rect );
      newNode->nw = BuildTree( rectangle( node->rect.x0, node->rect.y0,      x2,      y2 ), nw );
      newNode->ne = BuildTree( rectangle(      x2, node->rect.y0, node->rect.x1,      y2 ), ne );
      newNode->sw = BuildTree( rectangle( node->rect.x0,      y2,      x2, node->rect.y1 ), sw );
      newNode->se = BuildTree( rectangle(      x2,      y2, node->rect.x1, node->rect.y1 ), se );
      m_length = savedLength;

      // Further degeneracies may result, e.g. if the point class is not
      // behaving as expected. Do not allow them.
      if ( newNode->IsLeaf() )
      {
         delete newNode;
         return nullptr;
      }

      return newNode;
   }

   void SearchTree( point_list& found, const rectangle& rect, const Node* node ) const
   {
      if ( node != nullptr )
         if ( node->Intersects( rect ) )
            if ( node->IsLeaf() )
            {
               const LeafNode* leaf = static_cast<const LeafNode*>( node );
               for ( const point& p : leaf->points )
               {
                  component x = p[0];
                  if ( x >= rect.x0 )
                     if ( x <= rect.x1 )
                     {
                        component y = p[1];
                        if ( y >= rect.y0 )
                           if ( y <= rect.y1 )
                              found << p;
                     }
               }
            }
            else
            {
               SearchTree( found, rect, node->nw );
               SearchTree( found, rect, node->ne );
               SearchTree( found, rect, node->sw );
               SearchTree( found, rect, node->se );
            }
   }

   template <class F>
   void SearchTree( const rectangle& rect, F callback, void* data, const Node* node ) const
   {
      if ( node != nullptr )
         if ( node->Intersects( rect ) )
            if ( node->IsLeaf() )
            {
               const LeafNode* leaf = static_cast<const LeafNode*>( node );
               for ( const point& p : leaf->points )
               {
                  component x = p[0];
                  if ( x >= rect.x0 )
                     if ( x <= rect.x1 )
                     {
                        component y = p[1];
                        if ( y >= rect.y0 )
                           if ( y <= rect.y1 )
                              callback( p, data );
                     }
               }
            }
            else
            {
               SearchTree( rect, callback, data, node->nw );
               SearchTree( rect, callback, data, node->ne );
               SearchTree( rect, callback, data, node->sw );
               SearchTree( rect, callback, data, node->se );
            }
   }

   void InsertTree( const point& pt, Node*& node )
   {
      PCL_CHECK( node != nullptr )

      component x = pt[0];
      component y = pt[1];

      if ( x < node->rect.x0 )
         node->rect.x0 = x;
      else if ( x > node->rect.x1 )
         node->rect.x1 = x;

      if ( y < node->rect.y0 )
         node->rect.y0 = y;
      else if ( y > node->rect.y1 )
         node->rect.y1 = y;

      if ( node->IsLeaf() )
      {
         LeafNode* leaf = static_cast<LeafNode*>( node );
         if ( leaf->Length() < m_bucketCapacity )
            leaf->points << pt;
         else
         {
            rectangle rect = leaf->rect;
            double x2 = (rect.x0 + rect.x1)/2;
            double y2 = (rect.y0 + rect.y1)/2;
            // Prevent geometrically degenerate subtrees. For safety, we
            // enforce minimum region dimensions larger than twice the
            // machine epsilon for the rectangle coordinate type.
            if ( x2 - rect.x0 < 2*std::numeric_limits<coordinate>::epsilon() ||
                 rect.x1 - x2 < 2*std::numeric_limits<coordinate>::epsilon() ||
                 y2 - rect.y0 < 2*std::numeric_limits<coordinate>::epsilon() ||
                 rect.y1 - y2 < 2*std::numeric_limits<coordinate>::epsilon() )
            {
               leaf->points << pt;
            }
            else
            {
               point_list nw, ne, sw, se;
               for ( const point& p : leaf->points )
               {
                  component x = p[0];
                  component y = p[1];
                  if ( x <= x2 )
                  {
                     if ( y <= y2 )
                        nw << p;
                     else
                        sw << p;
                  }
                  else
                  {
                     if ( y <= y2 )
                        ne << p;
                     else
                        se << p;
                  }
               }

               if ( x <= x2 )
               {
                  if ( y <= y2 )
                     nw << pt;
                  else
                     sw << pt;
               }
               else
               {
                  if ( y <= y2 )
                     ne << pt;
                  else
                     se << pt;
               }

               delete leaf;

               node = new Node( rect );

               if ( !nw.IsEmpty() )
                  node->nw = new LeafNode( rectangle( rect.x0, rect.y0, x2, y2 ), nw );
               if ( !ne.IsEmpty() )
                  node->ne = new LeafNode( rectangle( x2, rect.y0, rect.x1, y2 ), ne );
               if ( !sw.IsEmpty() )
                  node->sw = new LeafNode( rectangle( rect.x0, y2, x2, rect.y1 ), sw );
               if ( !se.IsEmpty() )
                  node->se = new LeafNode( rectangle( x2, y2, rect.x1, rect.y1 ), se );
            }
         }

         ++m_length;
      }
      else
      {
         rectangle rect = node->rect;
         double x2 = (rect.x0 + rect.x1)/2;
         double y2 = (rect.y0 + rect.y1)/2;
         if ( pt[0] <= x2 )
         {
            if ( pt[1] <= y2 )
            {
               if ( node->nw == nullptr )
                  node->nw = new LeafNode( rectangle( rect.x0, rect.y0, x2, y2 ), pt );
               else
                  InsertTree( pt, node->nw );
            }
            else
            {
               if ( node->sw == nullptr )
                  node->sw = new LeafNode( rectangle( rect.x0, y2, x2, rect.y1 ), pt );
               else
                  InsertTree( pt, node->sw );
            }
         }
         else
         {
            if ( pt[1] <= y2 )
            {
               if ( node->ne == nullptr )
                  node->ne = new LeafNode( rectangle( x2, rect.y0, rect.x1, y2 ), pt );
               else
                  InsertTree( pt, node->ne );
            }
            else
            {
               if ( node->se == nullptr )
                  node->se = new LeafNode( rectangle( x2, y2, rect.x1, rect.y1 ), pt );
               else
                  InsertTree( pt, node->se );
            }
         }
      }
   }

   void DeleteTree( const rectangle& rect, Node*& node )
   {
      if ( node != nullptr )
         if ( node->Intersects( rect ) )
         {
            if ( node->IsLeaf() )
            {
               LeafNode* leaf = static_cast<LeafNode*>( node );
               point_list points;
               for ( const point& p : leaf->points )
               {
                  component x = p[0];
                  if ( x >= rect.x0 )
                     if ( x <= rect.x1 )
                     {
                        component y = p[1];
                        if ( y >= rect.y0 )
                           if ( y <= rect.y1 )
                           {
                              --m_length;
                              continue;
                           }
                     }
                  points << p;
               }

               if ( points.IsEmpty() )
               {
                  delete leaf;
                  node = nullptr;
               }
               else
                  leaf->points = points;
            }
            else
            {
               DeleteTree( rect, node->nw );
               DeleteTree( rect, node->ne );
               DeleteTree( rect, node->sw );
               DeleteTree( rect, node->se );

               if ( node->IsLeaf() )
               {
                  delete node;
                  node = nullptr;
               }
            }
         }
   }

   void DeleteTree( const point& pt, Node*& node )
   {
      if ( node != nullptr )
      {
         component x = pt[0];
         component y = pt[1];
         if ( node->Includes( x, y ) )
            if ( node->IsLeaf() )
            {
               LeafNode* leaf = static_cast<LeafNode*>( node );
               point_list points;
               for ( const point& p : leaf->points )
               {
                  if ( p[0] == x )
                     if ( p[1] == y )
                     {
                        --m_length;
                        continue;
                     }
                  points << p;
               }

               if ( points.IsEmpty() )
               {
                  delete leaf;
                  node = nullptr;
               }
               else
                  leaf->points = points;
            }
            else
            {
               DeleteTree( pt, node->nw );
               DeleteTree( pt, node->ne );
               DeleteTree( pt, node->sw );
               DeleteTree( pt, node->se );

               if ( node->IsLeaf() )
               {
                  delete node;
                  node = nullptr;
               }
            }
      }
   }

   void DestroyTree( Node* node )
   {
      if ( node != nullptr )
         if ( node->IsLeaf() )
            delete static_cast<LeafNode*>( node );
         else
         {
            DestroyTree( node->nw );
            DestroyTree( node->ne );
            DestroyTree( node->sw );
            DestroyTree( node->se );
            delete node;
         }
   }

   static void GetNumberOfNodes( size_type& N, const Node* node )
   {
      if ( node != nullptr )
      {
         ++N;
         GetNumberOfNodes( N, node->nw );
         GetNumberOfNodes( N, node->ne );
         GetNumberOfNodes( N, node->sw );
         GetNumberOfNodes( N, node->se );
      }
   }

   static void GetNumberOfLeafNodes( size_type& N, const Node* node )
   {
      if ( node != nullptr )
      {
         if ( node->IsLeaf() )
            ++N;
         GetNumberOfLeafNodes( N, node->nw );
         GetNumberOfLeafNodes( N, node->ne );
         GetNumberOfLeafNodes( N, node->sw );
         GetNumberOfLeafNodes( N, node->se );
      }
   }

   static int TreeHeight( const Node* node, int h )
   {
      if ( node == nullptr )
         return h;
      return h + 1 + Max( Max( Max( TreeHeight( node->nw, h ),
                                    TreeHeight( node->ne, h ) ),
                                    TreeHeight( node->sw, h ) ),
                                    TreeHeight( node->se, h ) );
   }
};

// ----------------------------------------------------------------------------

} // pcl

#endif   // __PCL_QuadTree_h

// ----------------------------------------------------------------------------
// EOF pcl/QuadTree.h - Released 2020-12-17T15:46:29Z
