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
#ifndef P1_ADT_CIRCULARLIST_HPP
#define P1_ADT_CIRCULARLIST_HPP

#include <cassert>
#include <iterator>

namespace p1 {

struct ListEntry
{
  ListEntry * prev, * next;

  void debugClear ()
  {
#ifndef NDEBUG
    this->prev = this->next = 0;
#endif
  }
};

// C++ trickery. In C++ we can't usually use container_of/offsetof to convert from a member field
// to the enclosing object. So, instead we must use inheritance and static_cast<>.
//
// There is an additional complication though: we get suboptimal code because static_cast<> from NULL
// always returns NULL even when the base offset is non-0. So we need a way to tell the compiler
// that the pointer is non-NULL. We can do that by using references.
//
// So, if we have a struct S which we want to use in a LinkedList, first we have it inherit from
// LinkedList through an intermediate struct, which we will use as a tag for for static_cast<>
// (for the cases when the same struct may need to participate in different lists).
//
// struct _Base1 : public ListEntry {};
// struct _Base2 : public ListEntry {};
//
// struct S : public _Base1, _Base2
// {
//   ....
// }
//
// Now "struct S" contains links for two independent lists. This is how we convert from a
// (ListEntry *) to a (struct S *):
//
// struct Accessor
// {
//   S * fromListEntry ( ListEntry * entry )
//   {
//     ListEntry & _entry = *entry; // convert it to a reference
//     return &static_cast<S&>(static_cast<_Base1&>(_entry));
//   }
//
//   // Note that exactly the same applies for conversion in the opposite direction:
//   ListEntry * toListEntry ( S * p )
//   {
//     S & _p = *p;
//     return &static_cast<_Base1&>(_p);
//   }
// }

// The default accessor for the simple case when there is only one ListEntry base
template <class T>
struct DefaultCircularListAccessor
{
  static T * fromListEntry ( ListEntry * entry )
  {
    // NOTE: we need to go through these contortions to avoid checking the pointer against NULL
    ListEntry & _entry = *entry;
    return &static_cast<T &>(_entry);
  }

  static ListEntry * toListEntry ( T * p )
  {
    // NOTE: we need to go through these contortions to avoid checking the pointer against NULL
    T & _p = *p;
    return &static_cast<ListEntry&>(_p);
  }
};

template <class T, class Accessor=DefaultCircularListAccessor<T> >
class CircularList
{
private:
  CircularList ( const CircularList & l );
  CircularList & operator = ( const CircularList & l );

public:
  CircularList ()
  {
    m_root.prev = m_root.next = &m_root;
  }

  explicit CircularList ( T * item )
  {
    m_root.prev = m_root.next = &m_root;
    push_back( item );
  }

  bool empty () const
  {
    return m_root.next == &m_root;
  }

  void push_back ( T * item )
  {
    insertAfterEntry( m_root.prev, Accessor::toListEntry(item) );
  }

  void insertAfter ( T * after, T * toInsert )
  {
    assert( toInsert != 0 );
    insertAfterEntry( after ? Accessor::toListEntry(after) : &m_root, Accessor::toListEntry(toInsert) );
  }

  void remove ( T * item )
  {
    removeEntry( Accessor::toListEntry(item) );
  }

  T * first ()
  {
    return m_root.next != &m_root ? Accessor::fromListEntry(m_root.next) : 0;
  }

  T * last ()
  {
    return m_root.prev != &m_root ? Accessor::fromListEntry(m_root.prev) : 0;
  }

  T * next ( T * item )
  {
    ListEntry * tmp = Accessor::toListEntry( item )->next;
    return tmp != &m_root ? Accessor::fromListEntry(tmp) : 0;
  }

  T * prev ( T * item )
  {
    ListEntry * tmp = Accessor::toListEntry( item )->prev;
    return tmp != &m_root ? Accessor::fromListEntry(tmp) : 0;
  }

  const T * first () const
  {
    return m_root.next != &m_root ? Accessor::fromListEntry(m_root.next) : 0;
  }

  const T * last () const
  {
    return m_root.prev != &m_root ? Accessor::fromListEntry(m_root.prev) : 0;
  }

  const T * next ( const T * item ) const
  {
    ListEntry * tmp = Accessor::toListEntry( const_cast<T *>(item) )->next; // TODO
    return tmp != &m_root ? Accessor::fromListEntry(tmp) : 0;
  }

  const T * prev ( const T * item ) const
  {
    ListEntry * tmp = Accessor::toListEntry( const_cast<T *>(item) )->prev; // TODO
    return tmp != &m_root ? Accessor::fromListEntry(tmp) : 0;
  }

  CircularList<T,Accessor> & operator += ( T * item )
  {
    push_back( item );
    return *this;
  }

  void destructiveAppend ( CircularList<T,Accessor> & lst )
  {
    if (!lst.empty())
    {
      // insertAfterEntry( last(), lst.first(), lst.last() );
      insertAfterEntry( m_root.prev, lst.m_root.next, lst.m_root.prev );
      lst.m_root.prev = lst.m_root.next = &lst.m_root; // mark it as empty
    }
  }

  class iterator_base : public std::iterator<std::bidirectional_iterator_tag,T>
  {
  public:
    bool operator == ( const iterator_base & it ) const { return m_p == it.m_p; }
    bool operator != ( const iterator_base & it ) const { return m_p != it.m_p; }

    const T * operator -> () const { return Accessor::fromListEntry(m_p); }
    const T & operator * () const { return *Accessor::fromListEntry(m_p); }

  protected:
    ListEntry * m_p;

    explicit iterator_base ( ListEntry * p ) { m_p = p; }
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

    T * operator -> () const { return Accessor::fromListEntry(iterator_base::m_p); }
    T & operator * () const { return *Accessor::fromListEntry(iterator_base::m_p); }

    iterator & operator ++ ()
    {
      iterator_base::m_p = iterator_base::m_p->next;
      return *this;
    }
    iterator & operator -- ()
    {
      iterator_base::m_p = iterator_base::m_p->prev;
      return *this;
    }
    iterator operator ++ ( int )
    {
      ListEntry * tmp = iterator_base::m_p;
      iterator_base::m_p = iterator_base::m_p->next;
      return iterator(tmp);
    }
    iterator operator -- ( int )
    {
      ListEntry * tmp = iterator_base::m_p;
      iterator_base::m_p = iterator_base::m_p->prev;
      return iterator(tmp);
    }
  protected:
    explicit iterator ( ListEntry * p ) : iterator_base( p ) {}

    friend class CircularList;
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
      iterator_base::m_p = iterator_base::m_p->next;
      return *this;
    }
    const_iterator & operator -- ()
    {
      iterator_base::m_p = iterator_base::m_p->prev;
      return *this;
    }
    const_iterator operator ++ ( int )
    {
      ListEntry * tmp = iterator_base::m_p;
      iterator_base::m_p = iterator_base::m_p->next;
      return const_iterator(tmp);
    }
    const_iterator operator -- ( int )
    {
      ListEntry * tmp = iterator_base::m_p;
      iterator_base::m_p = iterator_base::m_p->prev;
      return const_iterator(tmp);
    }
  protected:
    explicit const_iterator ( ListEntry * p ) : iterator_base( p ) {}

    friend class CircularList;
  };

  iterator begin () { return iterator(m_root.next); }
  iterator end () { return iterator(&m_root); }
  const_iterator begin () const { return const_iterator(m_root.next); }
  const_iterator end () const
  {
    return const_iterator(const_cast<ListEntry *>(&m_root)); // TODO
  }

private:
  ListEntry m_root;

  static void insertAfterEntry ( ListEntry * after, ListEntry * entry )
  {
    assert( entry->prev == 0 && entry->next == 0 );
    entry->next = after->next;
    entry->prev = after;
    after->next->prev = entry;
    after->next = entry;
  }

  static void insertAfterEntry ( ListEntry * after, ListEntry * entryFirst, ListEntry * entryLast )
  {
    entryLast->next = after->next;
    entryFirst->prev = after;
    after->next->prev = entryLast;
    after->next = entryFirst;
  }

  static void removeEntry ( ListEntry * entry )
  {
    entry->prev->next = entry->next;
    entry->next->prev = entry->prev;
#ifndef NDEBUG
    entry->prev = entry->next = 0;
#endif
  }
};

} // namespaces

#endif /* P1_ADT_CIRCULARLIST_HPP */
