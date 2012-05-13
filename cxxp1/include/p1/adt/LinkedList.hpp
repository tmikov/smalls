/*
   Copyright 2012 Tzvetan Mikov <tmikov@gmail.com>
   All rights reserved.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#ifndef P1_ADT_LINKEDLIST_HPP
#define P1_ADT_LINKEDLIST_HPP

#include <iterator>

namespace p1 {

template<class T>
struct SimpleLinkedListAccessor
{
  static T * & prev ( T * item ) { return item->prev; }
  static T * & next ( T * item ) { return item->next; }
  static T * prev ( const T * item ) { return item->prev; }
  static T * next ( const T * item ) { return item->next; }
};

template <class T, class Accessor = SimpleLinkedListAccessor<T> >
class LinkedList
{
public:
  LinkedList ()
  {
    m_first = m_last = 0;
  }

  LinkedList ( const LinkedList & l )
  {
    *this = l;
  }

  explicit LinkedList ( T * item )
  {
    assert(!Accessor::prev(item) && !Accessor::next(item));
    m_first = m_last = item;
    Accessor::next(item) = 0;
    Accessor::prev(item) = 0;
  }

  LinkedList & operator = ( const LinkedList & l )
  {
    m_first = l.m_first;
    m_last = l.m_last;
    return *this;
  }

  bool empty () const { return !m_first; }

  T * first () { return m_first; }
  T * last () { return m_last; }
  const T * first () const { return m_first; }
  const T * last () const { return m_last; }

  static T * next ( T * item ) { return Accessor::next(item); }
  static T * prev ( T * item ) { return Accessor::prev(item); }
  static const T * next ( const T * item ) { return Accessor::next(item); }
  static const T * prev ( const T * item ) { return Accessor::prev(item); }

  void push_back ( T * item );
  void push_front ( T * item );
  /**
   *
   * @param after if 0, the item is inserted in the beginning of the list
   * @param toInsert
   */
  void insertAfter ( T * after, T * toInsert );
  void destructiveAppend ( LinkedList<T,Accessor> & lst );

  LinkedList<T,Accessor> & operator += ( T * item )
  {
    push_back( item );
    return *this;
  }

  class iterator_base : public std::iterator<std::bidirectional_iterator_tag,T>
  {
  public:
    bool operator == ( const iterator_base & it ) const { return m_p == it.m_p; }
    bool operator != ( const iterator_base & it ) const { return m_p != it.m_p; }

    const T * operator -> () const { return m_p; }
    const T & operator * () const { return *m_p; }

  protected:
    T * m_p;

    explicit iterator_base ( T * p ) { m_p = p; }
  };

  class iterator : public iterator_base
  {
  public:
    iterator ( const iterator & it ) : iterator_base( it.m_p ) {}

    iterator & operator = ( const iterator & it )
    {
      iterator_base::m_p = it.m_p;
      return *this;
    }

    T * operator -> () const { return iterator_base::m_p; }
    T & operator * () const { return *iterator_base::m_p; }

    iterator & operator ++ ()
    {
      iterator_base::m_p = Accessor::next(iterator_base::m_p);
      return *this;
    }
    iterator & operator -- ()
    {
      iterator_base::m_p = Accessor::prev(iterator_base::m_p);
      return *this;
    }
    iterator operator ++ ( int )
    {
      T * tmp = iterator_base::m_p;
      iterator_base::m_p = Accessor::next(iterator_base::m_p);
      return iterator(tmp);
    }
    iterator operator -- ( int )
    {
      T * tmp = iterator_base::m_p;
      iterator_base::m_p = Accessor::prev(iterator_base::m_p);
      return iterator(tmp);
    }
  protected:
    explicit iterator ( T * p ) : iterator_base( p ) {}

    friend class LinkedList;
  };

  class const_iterator : public iterator_base
  {
  public:
    const_iterator ( const iterator_base & it ) : iterator_base( it.m_p ) {}

    const_iterator & operator = ( const iterator_base & it )
    {
      iterator_base::m_p = it.m_p;
      return *this;
    }

    const_iterator & operator ++ ()
    {
      iterator_base::m_p = Accessor::next(iterator_base::m_p);
      return *this;
    }
    const_iterator & operator -- ()
    {
      iterator_base::m_p = Accessor::prev(iterator_base::m_p);
      return *this;
    }
    const_iterator operator ++ ( int )
    {
      T * tmp = iterator_base::m_p;
      iterator_base::m_p = Accessor::next(iterator_base::m_p);
      return const_iterator(tmp);
    }
    const_iterator operator -- ( int )
    {
      T * tmp = iterator_base::m_p;
      iterator_base::m_p = Accessor::prev(iterator_base::m_p);
      return const_iterator(tmp);
    }
  protected:
    explicit const_iterator ( T * p ) : iterator_base( p ) {}

    friend class LinkedList;
  };

  iterator begin () { return iterator(m_first); }
  iterator end () { return iterator(0); }
  const_iterator begin () const { return const_iterator(m_first); }
  const_iterator end () const { return const_iterator(0); }

private:
  T * m_first, * m_last;

  void insertAfter ( T * after, T * toInsertFirst, T * toInsertLast );
};


template <class T, class Accessor>
void LinkedList<T,Accessor>::push_back ( T * item )
{
  Accessor::prev(item) = m_last;
  Accessor::next(item) = 0;
  if (m_last)
    Accessor::next(m_last) = item;
  else
    m_first = item;
  m_last = item;
}

template <class T, class Accessor>
void LinkedList<T,Accessor>::push_front ( T * item )
{
  Accessor::prev(item) = 0;
  Accessor::next(item) = m_first;
  if (m_first)
    Accessor::prev(m_first) = item;
  else
    m_last = item;
  m_first = item;
}

template <class T, class Accessor>
void LinkedList<T,Accessor>::insertAfter ( T * after, T * toInsertFirst, T * toInsertLast )
{
  T * nextItem;

  Accessor::prev(toInsertFirst) = after;

  if (after)
  {
    nextItem = Accessor::next(after);
    Accessor::next(after) = toInsertFirst;
  }
  else
  {
    nextItem = m_first;
    m_first = toInsertFirst;
  }

  Accessor::next(toInsertLast) = nextItem;

  if (nextItem)
    Accessor::prev(nextItem) = toInsertLast;
  else
    m_last = toInsertLast;
}


template <class T, class Accessor>
void LinkedList<T,Accessor>::insertAfter ( T * after, T * toInsert )
{
  T * nextItem;

  Accessor::prev(toInsert) = after;

  if (after)
  {
    nextItem = Accessor::next(after);
    Accessor::next(after) = toInsert;
  }
  else
  {
    nextItem = m_first;
    m_first = toInsert;
  }

  Accessor::next(toInsert) = nextItem;

  if (nextItem)
    Accessor::prev(nextItem) = toInsert;
  else
    m_last = toInsert;
}

template <class T, class Accessor>
void LinkedList<T,Accessor>::destructiveAppend ( LinkedList<T,Accessor> & lst )
{
  if (lst.empty())
    return;

  insertAfter( m_last, lst.m_first, lst.m_last );
  lst.m_first = lst.m_last = 0;
}

} // namespaces

#endif /* P1_ADT_LINKEDLIST_HPP */

