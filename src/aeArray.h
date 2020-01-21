//------------------------------------------------------------------------------
// aeArray.h
//------------------------------------------------------------------------------
// Copyright (c) 2020 John Hughes
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files( the "Software" ), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions :
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//------------------------------------------------------------------------------
#ifndef AEARRAY_H
#define AEARRAY_H

//------------------------------------------------------------------------------
// Headers
//------------------------------------------------------------------------------
#include "aeAlloc.h"
#include "aeLog.h"
#include "aePlatform.h"

//------------------------------------------------------------------------------
// aeArray class
//------------------------------------------------------------------------------
template < typename T >
class aeArray
{
public:
  aeArray();
  aeArray( uint32_t size ); // Reserve size
  aeArray( uint32_t length, const T& val ); // Init array with 'length' number of 'val's
  aeArray( const aeArray< T >& other );
  ~aeArray();
  void operator =( const aeArray< T >& other );
  
  T& Append( const T& value );
  void Append( const T* values, uint32_t count );

  void Remove( uint32_t index );
  template < typename U > uint32_t RemoveAll( const U& value );
  template < typename Fn, typename U > uint32_t RemoveAll( Fn testFn, const U& value );
  
  template < typename U > int32_t Find( const U& value ) const;
  template < typename Fn, typename U > int32_t Find( Fn testFn, const U& value ) const;

  void Reserve( uint32_t total );
  void Clear();
  
  const T& operator[]( int32_t index ) const;
  T& operator[]( int32_t index );

  uint32_t Length() const;
  uint32_t Size() const;

private:
  uint32_t m_length;
  uint32_t m_size;
  T* m_array;
};

template < typename T >
inline std::ostream& operator<<( std::ostream& os, const aeArray< T >& array )
{
  os << "<";
  for ( uint32_t i = 0; i < array.Length(); i++ )
  {
    os << array[ i ];
    if ( i != array.Length() - 1 )
    {
      os << ", ";
    }
  }
  return os << ">";
}

//------------------------------------------------------------------------------
// aeArray member functions
//------------------------------------------------------------------------------
template < typename T >
aeArray< T >::aeArray()
{
  m_length = 0;
  m_size = 0;
  m_array = nullptr;
}

template < typename T >
aeArray< T >::aeArray( uint32_t size )
{
  m_length = 0;
  m_size = 0;
  m_array = nullptr;

  Reserve( size );
}

template < typename T >
aeArray< T >::aeArray( uint32_t length, const T& value )
{
  m_length = 0;
  m_size = 0;
  m_array = nullptr;

  Reserve( length );

  for ( uint32_t i = 0; i < length; i++ )
  {
    m_array[ i ] = value;
  }
}

template < typename T >
aeArray< T >::aeArray( const aeArray< T >& other )
{
  m_length = 0;
  m_size = 0;
  m_array = nullptr;
  (*this) = other;
}

template < typename T >
aeArray< T >::~aeArray()
{
  aeAlloc::Release( m_array );
  m_length = 0;
  m_size = 0;
  m_array = nullptr;
}

template < typename T >
void aeArray< T >::operator =( const aeArray< T >& other )
{
  if ( m_size < other.m_length )
  {
    aeAlloc::Release( m_array );
    m_size = other.m_size;
    m_array = aeAlloc::AllocateArray< T >( m_size );
  }

  m_length = other.m_length;
  for ( uint32_t i = 0; i < m_length; i++ )
  {
    m_array[ i ] = other.m_array[ i ];
  }
}

template < typename T >
T& aeArray< T >::Append( const T& value )
{
  if ( m_length == m_size )
  {
    if ( m_size )
    {
      Reserve( m_size * 2 );
    }
    else
    {
      Reserve( 1 );
    }
  }

  m_array[ m_length ] = value;
  m_length++;

  return m_array[ m_length - 1 ];
}

template < typename T >
void aeArray< T >::Append( const T* values, uint32_t count )
{
  Reserve( m_length + count );

  AE_ASSERT( m_size >= m_length + count );
  for ( uint32_t i = 0; i < count; i++ )
  {
    m_array[ m_length ] = values[ i ];
    m_length++;
  }
}

template < typename T >
void aeArray< T >::Remove( uint32_t index )
{
  AE_ASSERT( index < m_length );
  m_length--;
  for ( uint32_t i = index; i < m_length; i++ )
  {
    m_array[ i ] = m_array[ i + 1 ];
  }
}

template < typename T >
template < typename U >
uint32_t aeArray< T >::RemoveAll( const U& value )
{
  uint32_t count = 0;
  int32_t index = 0;
  while ( ( index = Find( value ) ) >= 0 )
  {
    Remove( index );
    count++;
  }
  return count;
}

template < typename T >
template < typename Fn, typename U >
uint32_t aeArray< T >::RemoveAll( Fn testFn, const U& value )
{
  uint32_t count = 0;
  int32_t index = 0;
  while ( ( index = Find( testFn, value ) ) >= 0 )
  {
    Remove( index );
    count++;
  }
  return count;
}

template < typename T >
template < typename U >
int32_t aeArray< T >::Find( const U& value ) const
{
  for ( uint32_t i = 0; i < m_length; i++ )
  {
    if ( m_array[ i ] == value )
    {
      return i;
    }
  }
  return -1;
}

template < typename T >
template < typename Fn, typename U >
int32_t aeArray< T >::Find( Fn testFn, const U& value ) const
{
  for ( uint32_t i = 0; i < m_length; i++ )
  {
    if ( testFn( m_array[ i ], value ) )
    {
      return i;
    }
  }
  return -1;
}

template < typename T >
void aeArray< T >::Reserve( uint32_t size )
{
  // Next power of two
  size--;
  size |= size >> 1;
  size |= size >> 2;
  size |= size >> 4;
  size |= size >> 8;
  size |= size >> 16;
  size++;

  if ( size <= m_size )
  {
    return;
  }

  m_size = size;

  T* arr = aeAlloc::AllocateArray< T >( m_size );
  for ( uint32_t i = 0; i < m_length; i++ )
  {
    arr[ i ] = m_array[ i ];
  }

  aeAlloc::Release( m_array );
  m_array = arr;
}

template < typename T >
void aeArray< T >::Clear()
{
  m_length = 0;
}

template < typename T >
const T& aeArray< T >::operator[]( int32_t index ) const
{
  AE_ASSERT( index >= 0 );
  AE_ASSERT( index < (int32_t)m_length );
  return m_array[ index ];
}

template < typename T >
T& aeArray< T >::operator[]( int32_t index )
{
  AE_ASSERT( index >= 0 );
  AE_ASSERT( index < (int32_t)m_length );
  return m_array[ index ];
}

template < typename T >
uint32_t aeArray< T >::Length() const
{
  return m_length;
}

template < typename T >
uint32_t aeArray< T >::Size() const
{
  return m_size;
}

#endif
